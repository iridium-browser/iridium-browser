use std::fmt;
use std::path::Path;
use std::str::FromStr;

use lazy_static::lazy_static;
use regex::Regex;

use crate::common::{Codepoint, CodepointIter, UcdFile, UcdFileByCodepoint};
use crate::error::Error;

/// Represents a single row in the `BidiMirroring.txt` file.
///
/// The field names were taken from the header of BidiMirroring.txt.
#[derive(Clone, Debug, Default, Eq, PartialEq)]
pub struct BidiMirroring {
    /// The codepoint corresponding to this row.
    pub codepoint: Codepoint,
    /// The codepoint that has typically has a glyph that is the mirror image
    /// of `codepoint`.
    pub bidi_mirroring_glyph: Codepoint,
}

impl UcdFile for BidiMirroring {
    fn relative_file_path() -> &'static Path {
        Path::new("BidiMirroring.txt")
    }
}

impl UcdFileByCodepoint for BidiMirroring {
    fn codepoints(&self) -> CodepointIter {
        self.codepoint.into_iter()
    }
}

impl FromStr for BidiMirroring {
    type Err = Error;

    fn from_str(line: &str) -> Result<BidiMirroring, Error> {
        lazy_static! {
            static ref PARTS: Regex = Regex::new(
                r"(?x)
                ^
                \s*(?P<codepoint>[A-F0-9]+)\s*;
                \s*(?P<substitute_codepoint>[A-F0-9]+)
                \s+
                \#(?:.+)
                $
                "
            )
            .unwrap();
        };
        let caps = match PARTS.captures(line.trim()) {
            Some(caps) => caps,
            None => return err!("invalid BidiMirroring line"),
        };

        Ok(BidiMirroring {
            codepoint: caps["codepoint"].parse()?,
            bidi_mirroring_glyph: caps["substitute_codepoint"].parse()?,
        })
    }
}

impl fmt::Display for BidiMirroring {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{};", self.codepoint)?;
        write!(f, "{};", self.bidi_mirroring_glyph)?;
        Ok(())
    }
}

#[cfg(test)]
mod tests {
    use crate::common::Codepoint;

    use super::BidiMirroring;

    fn codepoint(n: u32) -> Codepoint {
        Codepoint::from_u32(n).unwrap()
    }

    #[test]
    fn parse() {
        let line = "0028; 0029 # LEFT PARENTHESIS\n";
        let data: BidiMirroring = line.parse().unwrap();
        assert_eq!(
            data,
            BidiMirroring {
                codepoint: codepoint(0x0028),
                bidi_mirroring_glyph: codepoint(0x0029),
            }
        );
    }

    #[test]
    fn parse_best_fit() {
        let line = "228A; 228B # [BEST FIT] SUBSET OF WITH NOT EQUAL TO\n";
        let data: BidiMirroring = line.parse().unwrap();
        assert_eq!(
            data,
            BidiMirroring {
                codepoint: codepoint(0x228A),
                bidi_mirroring_glyph: codepoint(0x228B),
            }
        );
    }
}
