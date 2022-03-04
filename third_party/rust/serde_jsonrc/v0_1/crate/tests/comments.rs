#[macro_use]
extern crate serde_jsonrc;

use serde_jsonrc::{from_str, Value};

#[test]
fn test_parse_line_comments() {
    let s = r#"
    {
        // Here is a comment.
        "key": "value"
    }// Here is a another comment at the end."#;
    let value: Value = from_str(s).unwrap();
    assert_eq!(value, json!({"key": "value"}));
}

#[test]
fn test_parse_block_comments() {
    let s = r#"
    /*
     Here is a comment up here.
     */
    {
        /* And one in here. */
        "key": "value"
    }/* Some at the end... *//* ...back to *** back! */ /* Two trailing asterisk **/"#;
    let value: Value = from_str(s).unwrap();
    assert_eq!(value, json!({"key": "value"}));
}

#[test]
fn test_parse_block_comments_before_value() {
    let s = r#"
    /* foo **/{"key": "value"}"#;
    let value: Value = from_str(s).unwrap();
    assert_eq!(value, json!({"key": "value"}));
}
