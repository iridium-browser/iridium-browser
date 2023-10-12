//! [Mixed-script detection](https://www.unicode.org/reports/tr39/#Mixed_Script_Detection)

use core::fmt::{self, Debug};
use unicode_script::{Script, ScriptExtension};

/// An Augmented script set, as defined by UTS 39
///
/// https://www.unicode.org/reports/tr39/#def-augmented-script-set
#[derive(Copy, Clone, PartialEq, Hash, Eq)]
pub struct AugmentedScriptSet {
    /// The base ScriptExtension value
    pub base: ScriptExtension,
    /// Han With Bopomofo
    pub hanb: bool,
    /// Japanese
    pub jpan: bool,
    /// Korean
    pub kore: bool,
}

impl From<ScriptExtension> for AugmentedScriptSet {
    fn from(ext: ScriptExtension) -> Self {
        let mut hanb = false;
        let mut jpan = false;
        let mut kore = false;

        if ext.is_common() || ext.is_inherited() || ext.contains_script(Script::Han) {
            hanb = true;
            jpan = true;
            kore = true;
        } else {
            if ext.contains_script(Script::Hiragana) || ext.contains_script(Script::Katakana) {
                jpan = true;
            }

            if ext.contains_script(Script::Hangul) {
                kore = true;
            }

            if ext.contains_script(Script::Bopomofo) {
                hanb = true;
            }
        }
        Self {
            base: ext,
            hanb,
            jpan,
            kore,
        }
    }
}

impl From<char> for AugmentedScriptSet {
    fn from(c: char) -> Self {
        AugmentedScriptSet::for_char(c)
    }
}

impl From<&'_ str> for AugmentedScriptSet {
    fn from(s: &'_ str) -> Self {
        AugmentedScriptSet::for_str(s)
    }
}

impl Default for AugmentedScriptSet {
    fn default() -> Self {
        AugmentedScriptSet {
            base: Script::Common.into(),
            hanb: true,
            jpan: true,
            kore: true,
        }
    }
}

impl Debug for AugmentedScriptSet {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        if self.is_empty() {
            write!(f, "AugmentedScriptSet {{∅}}")?;
        } else if self.is_all() {
            write!(f, "AugmentedScriptSet {{ALL}}")?;
        } else {
            write!(f, "AugmentedScriptSet {{")?;
            let mut first_entry = true;
            let hanb = if self.hanb { Some("Hanb") } else { None };
            let jpan = if self.jpan { Some("Jpan") } else { None };
            let kore = if self.kore { Some("Kore") } else { None };
            for writing_system in None
                .into_iter()
                .chain(hanb)
                .chain(jpan)
                .chain(kore)
                .chain(self.base.iter().map(Script::short_name))
            {
                if !first_entry {
                    write!(f, ", ")?;
                } else {
                    first_entry = false;
                }
                write!(f, "{}", writing_system)?;
            }
            write!(f, "}}")?;
        }
        Ok(())
    }
}

impl fmt::Display for AugmentedScriptSet {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        if self.is_empty() {
            write!(f, "Empty")?;
        } else if self.is_all() {
            write!(f, "All")?;
        } else {
            let mut first_entry = true;
            let hanb = if self.hanb {
                Some("Han with Bopomofo")
            } else {
                None
            };
            let jpan = if self.jpan { Some("Japanese") } else { None };
            let kore = if self.kore { Some("Korean") } else { None };
            for writing_system in None
                .into_iter()
                .chain(hanb)
                .chain(jpan)
                .chain(kore)
                .chain(self.base.iter().map(Script::full_name))
            {
                if !first_entry {
                    write!(f, ", ")?;
                } else {
                    first_entry = false;
                }
                write!(f, "{}", writing_system)?;
            }
        }
        Ok(())
    }
}

impl AugmentedScriptSet {
    /// Intersect this set with another
    pub fn intersect_with(&mut self, other: Self) {
        self.base.intersect_with(other.base);
        self.hanb = self.hanb && other.hanb;
        self.jpan = self.jpan && other.jpan;
        self.kore = self.kore && other.kore;
    }

    /// Check if the set is empty
    pub fn is_empty(&self) -> bool {
        self.base.is_empty() && !self.hanb && !self.jpan && !self.kore
    }

    /// Check if the set is "All" (Common or Inherited)
    pub fn is_all(&self) -> bool {
        self.base.is_common() || self.base.is_inherited()
    }

    /// Construct an AugmentedScriptSet for a given character
    pub fn for_char(c: char) -> Self {
        ScriptExtension::from(c).into()
    }

    /// Find the [resolved script set](https://www.unicode.org/reports/tr39/#def-resolved-script-set) of a given string
    pub fn for_str(s: &str) -> Self {
        let mut set = AugmentedScriptSet::default();
        for ch in s.chars() {
            set.intersect_with(ch.into())
        }
        set
    }
}

/// Extension trait for [mixed-script detection](https://www.unicode.org/reports/tr39/#Mixed_Script_Detection)
pub trait MixedScript {
    /// Check if a string is [single-script](https://www.unicode.org/reports/tr39/#def-single-script)
    ///
    /// Note that a single-script string may still contain multiple Script properties!
    fn is_single_script(self) -> bool;

    /// Find the [resolved script set](https://www.unicode.org/reports/tr39/#def-resolved-script-set) of a given string
    fn resolve_script_set(self) -> AugmentedScriptSet;
}

impl MixedScript for &'_ str {
    fn is_single_script(self) -> bool {
        !AugmentedScriptSet::for_str(self).is_empty()
    }

    fn resolve_script_set(self) -> AugmentedScriptSet {
        self.into()
    }
}

/// Check if a character is considered potential mixed script confusable.
///
/// If the specified character is not restricted from use for identifiers,
/// this function returns whether it is considered mixed script confusable
/// with another character that is not restricted from use for identifiers.
///
/// If the specified character is restricted from use for identifiers,
/// the return value is unspecified.
pub fn is_potential_mixed_script_confusable_char(c: char) -> bool {
    use crate::tables::potential_mixed_script_confusable::potential_mixed_script_confusable;

    potential_mixed_script_confusable(c)
}
