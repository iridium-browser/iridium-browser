use std::path::{Path, PathBuf};
use std::str::FromStr;

use crate::common::{
    parse_codepoint_association, CodepointIter, Codepoints, UcdFile,
    UcdFileByCodepoint,
};
use crate::error::Error;

/// A single row in the `emoji-data.txt` file.
///
/// The `emoji-data.txt` file is the source of truth on several Emoji-related
/// Unicode properties.
///
/// Note that `emoji-data.txt` is not formally part of the Unicode Character
/// Database. You can download the Emoji data files separately here:
/// https://unicode.org/Public/emoji/
#[derive(Clone, Debug, Default, Eq, PartialEq)]
pub struct EmojiProperty {
    /// The codepoint or codepoint range for this entry.
    pub codepoints: Codepoints,
    /// The property name assigned to the codepoints in this entry.
    pub property: String,
}

impl UcdFile for EmojiProperty {
    fn relative_file_path() -> &'static Path {
        Path::new("emoji/emoji-data.txt")
    }

    fn file_path<P: AsRef<Path>>(ucd_dir: P) -> PathBuf {
        let ucd_dir = ucd_dir.as_ref();
        // The standard location, but only on UCDs from 13.0.0 and up.
        let std = ucd_dir.join(Self::relative_file_path());
        if std.exists() {
            std
        } else {
            // If the old location does exist, use it.
            let legacy = ucd_dir.join("emoji-data.txt");
            if legacy.exists() {
                legacy
            } else {
                // This might end up in an error message, so use the standard
                // one if forced to choose. Arguably we could do something like
                // peek
                std
            }
        }
    }
}

impl UcdFileByCodepoint for EmojiProperty {
    fn codepoints(&self) -> CodepointIter {
        self.codepoints.into_iter()
    }
}

impl FromStr for EmojiProperty {
    type Err = Error;

    fn from_str(line: &str) -> Result<EmojiProperty, Error> {
        let (codepoints, property) = parse_codepoint_association(line)?;
        Ok(EmojiProperty { codepoints, property: property.to_string() })
    }
}

#[cfg(test)]
mod tests {
    use super::EmojiProperty;

    #[test]
    fn parse_single() {
        let line = "24C2          ; Emoji                #  1.1  [1] (Ⓜ️)       circled M\n";
        let row: EmojiProperty = line.parse().unwrap();
        assert_eq!(row.codepoints, 0x24C2);
        assert_eq!(row.property, "Emoji");
    }

    #[test]
    fn parse_range() {
        let line = "1FA6E..1FFFD  ; Extended_Pictographic#   NA[1424] (🩮️..🿽️)   <reserved-1FA6E>..<reserved-1FFFD>\n";
        let row: EmojiProperty = line.parse().unwrap();
        assert_eq!(row.codepoints, (0x1FA6E, 0x1FFFD));
        assert_eq!(row.property, "Extended_Pictographic");
    }
}
