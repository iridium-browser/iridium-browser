/*!
A library for parsing the Unicode character database.
*/

#![deny(missing_docs)]

pub use crate::common::{
    parse, parse_by_codepoint, parse_many_by_codepoint, ucd_directory_version,
    Codepoint, CodepointIter, CodepointRange, Codepoints, UcdFile,
    UcdFileByCodepoint, UcdLineParser,
};
pub use crate::error::{Error, ErrorKind};

pub use crate::age::Age;
pub use crate::arabic_shaping::ArabicShaping;
pub use crate::bidi_mirroring_glyph::BidiMirroring;
pub use crate::case_folding::{CaseFold, CaseStatus};
pub use crate::core_properties::CoreProperty;
pub use crate::emoji_properties::EmojiProperty;
pub use crate::grapheme_cluster_break::{
    GraphemeClusterBreak, GraphemeClusterBreakTest,
};
pub use crate::jamo_short_name::JamoShortName;
pub use crate::line_break::LineBreakTest;
pub use crate::name_aliases::{NameAlias, NameAliasLabel};
pub use crate::prop_list::Property;
pub use crate::property_aliases::PropertyAlias;
pub use crate::property_value_aliases::PropertyValueAlias;
pub use crate::script_extensions::ScriptExtension;
pub use crate::scripts::Script;
pub use crate::sentence_break::{SentenceBreak, SentenceBreakTest};
pub use crate::special_casing::SpecialCaseMapping;
pub use crate::unicode_data::{
    UnicodeData, UnicodeDataDecomposition, UnicodeDataDecompositionTag,
    UnicodeDataExpander, UnicodeDataNumeric,
};
pub use crate::word_break::{WordBreak, WordBreakTest};

macro_rules! err {
    ($($tt:tt)*) => {
        Err(crate::error::Error::parse(format!($($tt)*)))
    }
}

mod common;
mod error;

mod age;
mod arabic_shaping;
mod bidi_mirroring_glyph;
mod case_folding;
mod core_properties;
mod emoji_properties;
mod grapheme_cluster_break;
mod jamo_short_name;
mod line_break;
mod name_aliases;
mod prop_list;
mod property_aliases;
mod property_value_aliases;
mod script_extensions;
mod scripts;
mod sentence_break;
mod special_casing;
mod unicode_data;
mod word_break;
