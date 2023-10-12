//! For detecting the [restriction level](https://www.unicode.org/reports/tr39/#Restriction_Level_Detection)
//! a string conforms to

use crate::mixed_script::AugmentedScriptSet;
use crate::GeneralSecurityProfile;
use unicode_script::Script;

#[derive(Copy, Clone, PartialEq, PartialOrd, Eq, Ord, Debug, Hash)]
/// The [Restriction level](https://www.unicode.org/reports/tr39/#Restriction_Level_Detection)
/// a string conforms to
pub enum RestrictionLevel {
    /// https://www.unicode.org/reports/tr39/#ascii_only
    ASCIIOnly,
    /// https://www.unicode.org/reports/tr39/#single_script
    SingleScript,
    /// https://www.unicode.org/reports/tr39/#highly_restrictive
    HighlyRestrictive,
    /// https://www.unicode.org/reports/tr39/#moderately_restrictive
    ModeratelyRestrictive,
    /// https://www.unicode.org/reports/tr39/#minimally_restrictive
    MinimallyRestrictive,
    /// https://www.unicode.org/reports/tr39/#unrestricted
    Unrestricted,
}

/// Utilities for determining which [restriction level](https://www.unicode.org/reports/tr39/#Restriction_Level_Detection)
/// a string satisfies
pub trait RestrictionLevelDetection: Sized {
    /// Detect the [restriction level](https://www.unicode.org/reports/tr39/#Restriction_Level_Detection)
    ///
    /// This will _not_ check identifier well-formedness, as different applications may have different notions of well-formedness
    fn detect_restriction_level(self) -> RestrictionLevel;

    /// Check if a string satisfies the supplied [restriction level](https://www.unicode.org/reports/tr39/#Restriction_Level_Detection)
    ///
    /// This will _not_ check identifier well-formedness, as different applications may have different notions of well-formedness
    fn check_restriction_level(self, level: RestrictionLevel) -> bool {
        self.detect_restriction_level() <= level
    }
}

impl RestrictionLevelDetection for &'_ str {
    fn detect_restriction_level(self) -> RestrictionLevel {
        let mut ascii_only = true;
        let mut set = AugmentedScriptSet::default();
        let mut exclude_latin_set = AugmentedScriptSet::default();
        for ch in self.chars() {
            if !GeneralSecurityProfile::identifier_allowed(ch) {
                return RestrictionLevel::Unrestricted;
            }
            if !ch.is_ascii() {
                ascii_only = false;
            }
            let ch_set = ch.into();
            set.intersect_with(ch_set);
            if !ch_set.base.contains_script(Script::Latin) {
                exclude_latin_set.intersect_with(ch_set);
            }
        }

        if ascii_only {
            return RestrictionLevel::ASCIIOnly;
        } else if !set.is_empty() {
            return RestrictionLevel::SingleScript;
        } else if exclude_latin_set.kore || exclude_latin_set.hanb || exclude_latin_set.jpan {
            return RestrictionLevel::HighlyRestrictive;
        } else if exclude_latin_set.base.len() == 1 {
            let script = exclude_latin_set.base.iter().next().unwrap();
            if script.is_recommended() && script != Script::Cyrillic && script != Script::Greek {
                return RestrictionLevel::ModeratelyRestrictive;
            }
        }
        return RestrictionLevel::MinimallyRestrictive;
    }
}
