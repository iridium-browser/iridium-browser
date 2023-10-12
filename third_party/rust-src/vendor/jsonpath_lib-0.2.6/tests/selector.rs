extern crate jsonpath_lib as jsonpath;
#[macro_use]
extern crate serde_json;

use common::{read_json, setup};
use jsonpath::{Parser, Selector, SelectorMut};
use serde_json::Value;

mod common;

#[test]
fn selector_mut() {
    setup();

    let mut selector_mut = SelectorMut::default();

    let mut nums = Vec::new();
    let result = selector_mut
        .str_path(r#"$.store..price"#)
        .unwrap()
        .value(read_json("./benchmark/example.json"))
        .replace_with(&mut |v| {
            if let Value::Number(n) = v {
                nums.push(n.as_f64().unwrap());
            }
            Some(Value::String("a".to_string()))
        })
        .unwrap()
        .take()
        .unwrap();

    assert_eq!(
        nums,
        vec![8.95_f64, 12.99_f64, 8.99_f64, 22.99_f64, 19.95_f64]
    );

    let mut selector = Selector::default();
    let result = selector
        .str_path(r#"$.store..price"#)
        .unwrap()
        .value(&result)
        .select()
        .unwrap();

    assert_eq!(
        vec![
            &json!("a"),
            &json!("a"),
            &json!("a"),
            &json!("a"),
            &json!("a")
        ],
        result
    );
}

#[test]
fn selector_node_ref() {
    let node = Parser::compile("$.*").unwrap();
    let mut selector = Selector::default();
    selector.compiled_path(&node);
    assert!(std::ptr::eq(selector.node_ref().unwrap(), &node));
}

#[test]
fn selector_delete() {
    setup();

    let mut selector_mut = SelectorMut::default();

    let result = selector_mut
        .str_path(r#"$.store..price[?(@>13)]"#)
        .unwrap()
        .value(read_json("./benchmark/example.json"))
        .delete()
        .unwrap()
        .take()
        .unwrap();

    let mut selector = Selector::default();
    let result = selector
        .str_path(r#"$.store..price"#)
        .unwrap()
        .value(&result)
        .select()
        .unwrap();

    assert_eq!(
        result,
        vec![
            &json!(8.95),
            &json!(12.99),
            &json!(8.99),
            &Value::Null,
            &Value::Null
        ]
    );
}

#[test]
fn selector_remove() {
    setup();

    let mut selector_mut = SelectorMut::default();

    let result = selector_mut
        .str_path(r#"$.store..price[?(@>13)]"#)
        .unwrap()
        .value(read_json("./benchmark/example.json"))
        .remove()
        .unwrap()
        .take()
        .unwrap();

    let mut selector = Selector::default();
    let result = selector
        .str_path(r#"$.store..price"#)
        .unwrap()
        .value(&result)
        .select()
        .unwrap();

    assert_eq!(
        result,
        vec![
            &json!(8.95),
            &json!(12.99),
            &json!(8.99)
        ]
    );
}
