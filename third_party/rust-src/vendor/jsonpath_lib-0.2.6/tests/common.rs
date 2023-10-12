extern crate env_logger;
extern crate jsonpath_lib as jsonpath;
extern crate serde_json;

use std::io::Read;

use serde_json::Value;

use self::jsonpath::Selector;

#[allow(dead_code)]
pub fn setup() {
    let _ = env_logger::try_init();
}

#[allow(dead_code)]
pub fn read_json(path: &str) -> Value {
    let mut f = std::fs::File::open(path).unwrap();
    let mut contents = String::new();
    f.read_to_string(&mut contents).unwrap();
    serde_json::from_str(&contents).unwrap()
}

#[allow(dead_code)]
pub fn read_contents(path: &str) -> String {
    let mut f = std::fs::File::open(path).unwrap();
    let mut contents = String::new();
    f.read_to_string(&mut contents).unwrap();
    contents
}

#[allow(dead_code)]
pub fn select_and_then_compare(path: &str, json: Value, target: Value) {
    let mut selector = Selector::default();
    let result = selector
        .str_path(path)
        .unwrap()
        .value(&json)
        .select_as::<Value>()
        .unwrap();
    assert_eq!(
        result,
        match target {
            Value::Array(vec) => vec,
            _ => panic!("Give me the Array!"),
        },
        "{}",
        path
    );
}

#[allow(dead_code)]
pub fn compare_result(result: Vec<&Value>, target: Value) {
    let result = serde_json::to_value(result).unwrap();
    assert_eq!(result, target);
}
