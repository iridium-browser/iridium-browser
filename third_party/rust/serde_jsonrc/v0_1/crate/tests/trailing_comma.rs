#[macro_use]
extern crate serde_derive;

extern crate serde;

#[macro_use]
extern crate serde_jsonrc;

use serde::Deserialize;
use serde_jsonrc::{from_str, Deserializer, Value};

#[test]
fn test_trailing_comma_object() {
    let s = r#"
    {
        "key": "value",
    }"#;
    let value: Value = from_str(s).unwrap();
    assert_eq!(value, json!({"key": "value"}));
}

#[test]
fn test_double_comma_object() {
    let s = r#"
    {
        "key1": "value1",,
        "key2": "value2"
    }"#;
    let actual = from_str::<Value>(s).unwrap_err().to_string();
    assert_eq!(actual, "key must be a string at line 3 column 26");
}

#[test]
fn test_double_trailing_comma_object() {
    let s = r#"
    {
        "key1": "value1",
        "key2": "value2",,
    }"#;
    let actual = from_str::<Value>(s).unwrap_err().to_string();
    assert_eq!(actual, "key must be a string at line 4 column 26");
}

#[test]
fn test_trailing_comma_array() {
    let s = r#"
    {
        "key": [
            "one",
            "two",
            "three",
        ]
    }"#;
    let value: Value = from_str(s).unwrap();
    assert_eq!(value, json!({"key": ["one", "two", "three"]}));
}

#[test]
fn test_double_comma_array() {
    let s = r#"
    {
        "key": [
            "one",,
            "two",
        ]
    }"#;
    let actual = from_str::<Value>(s).unwrap_err().to_string();
    assert_eq!(actual, "expected value at line 4 column 19");
}

#[test]
fn test_double_trailing_comma_array() {
    let s = r#"
    {
        "key": [
            "one",
            "two",,
        ]
    }"#;
    let actual = from_str::<Value>(s).unwrap_err().to_string();
    assert_eq!(actual, "expected value at line 5 column 19");
}

#[derive(Clone, Debug, PartialEq, Serialize, Deserialize)]
#[serde(deny_unknown_fields)]
enum Animal {
    Dog,
    Frog(String, Vec<isize>),
    Cat { age: usize, name: String },
    AntHive(Vec<String>),
}

#[test]
fn test_parse_enum_as_array_with_deny_unknown_fields() {
    let animal: Animal = from_str("{\"Cat\":[0, \"Kate\",]}").unwrap();
    assert_eq!(
        animal,
        Animal::Cat {
            age: 0,
            name: "Kate".to_string()
        }
    );
}

#[test]
fn test_parse_enum_as_object_with_deny_unknown_fields() {
    let animal: Animal = from_str("{\"Cat\":{\"age\": 2, \"name\": \"Kate\",}}").unwrap();
    assert_eq!(
        animal,
        Animal::Cat {
            age: 2,
            name: "Kate".to_string()
        }
    );
}

#[test]
fn test_commas_switchable() {
    let s = r#"
    {
        "key": "value",
    }"#;
    let mut deserializer = Deserializer::from_str(&s);
    deserializer.set_ignore_trailing_commas(false);
    assert!(Value::deserialize(&mut deserializer)
        .unwrap_err()
        .to_string()
        .contains("comma"));
    let mut deserializer = Deserializer::from_str(&s);
    let value = Value::deserialize(&mut deserializer).unwrap();
    assert_eq!(value, json!({ "key": "value" }));
}
