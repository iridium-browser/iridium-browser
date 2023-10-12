#[macro_use]
extern crate serde_json;
extern crate jsonpath_lib;

use common::{setup};
use jsonpath_lib::Compiled;
use serde_json::Value;

mod common;

#[test]
fn precompile_test() {
    setup();

    let json = json!({
        "foo": {"bar": "baz"}
    });

    // compile once

    let compiled = Compiled::compile("$.foo.bar");

    assert!(compiled.is_ok());

    let compiled = compiled.unwrap();

    // re-use

    //let result = compiled(&json).unwrap();
    assert_eq!(compiled.select(&json).unwrap().clone(), vec![&Value::String("baz".into())]);
    assert_eq!(compiled.select(&json).unwrap().clone(), vec![&Value::String("baz".into())]);
}

#[test]
fn precompile_failure() {
    setup();

    let compiled = Compiled::compile("");

    assert!(compiled.is_err());
}