extern crate serde;

#[macro_use]
extern crate serde_jsonrc;

use serde_jsonrc::{Value, Deserializer, Error};
use serde_jsonrc::de::SliceRead;
use serde::de::Deserialize;

fn from_slice_with_unicode_substitution(s: &[u8]) -> Result<Value, Error> {
    let mut de = Deserializer::new(SliceRead::new(s, true, false, false, false));
    Deserialize::deserialize(&mut de)
}

#[test]
fn test_invalid_characters() {
    let prefix = r#" { "key": "value"#;
    let suffix = r#"" }"#;
    let mut s: Vec<u8> = vec![];
    s.extend(prefix.as_bytes());
    s.extend(&[0xed, 0xa0, 0x80]);
    s.extend(suffix.as_bytes());
    let value = from_slice_with_unicode_substitution(&s).unwrap();
    assert_eq!(value, json!({"key": "value\u{fffd}\u{fffd}\u{fffd}"}));
}

#[test]
fn test_invalid_utf16_escape_sequence() {
    let s = r#"
    {
        "key": "value\ud800"
    }"#.as_bytes();
    let value: Value = from_slice_with_unicode_substitution(&s).unwrap();
    assert_eq!(value, json!({"key": "value\u{fffd}"}));
}
