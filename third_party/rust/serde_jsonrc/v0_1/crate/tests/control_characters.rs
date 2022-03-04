extern crate serde;

#[macro_use]
extern crate serde_jsonrc;

use serde_jsonrc::{from_str, Value};

#[test]
fn test_control_character() {
    let s = "{ \"key\": \"a\0b\nc\" }";
    let value: Value = from_str(s).unwrap();
    assert_eq!(value, json!({"key": "a\0b\nc"}));
}
