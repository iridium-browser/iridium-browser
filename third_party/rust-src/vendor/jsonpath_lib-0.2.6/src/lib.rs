//! JsonPath implementation written in Rust.
//!
//! # Example
//! ```
//! extern crate jsonpath_lib as jsonpath;
//! #[macro_use] extern crate serde_json;
//! let json_obj = json!({
//!     "store": {
//!         "book": [
//!             {
//!                 "category": "reference",
//!                 "author": "Nigel Rees",
//!                 "title": "Sayings of the Century",
//!                 "price": 8.95
//!             },
//!             {
//!                 "category": "fiction",
//!                 "author": "Evelyn Waugh",
//!                 "title": "Sword of Honour",
//!                 "price": 12.99
//!             },
//!             {
//!                 "category": "fiction",
//!                 "author": "Herman Melville",
//!                 "title": "Moby Dick",
//!                 "isbn": "0-553-21311-3",
//!                 "price": 8.99
//!             },
//!             {
//!                 "category": "fiction",
//!                 "author": "J. R. R. Tolkien",
//!                 "title": "The Lord of the Rings",
//!                 "isbn": "0-395-19395-8",
//!                 "price": 22.99
//!             }
//!         ],
//!         "bicycle": {
//!             "color": "red",
//!             "price": 19.95
//!         }
//!     },
//!     "expensive": 10
//! });
//!
//! let mut selector = jsonpath::selector(&json_obj);
//!
//! assert_eq!(selector("$.store.book[*].author").unwrap(),
//!             vec![
//!                 "Nigel Rees", "Evelyn Waugh", "Herman Melville", "J. R. R. Tolkien"
//!             ]);
//!
//! assert_eq!(selector("$..author").unwrap(),
//!             vec![
//!                 "Nigel Rees", "Evelyn Waugh", "Herman Melville", "J. R. R. Tolkien"
//!             ]);
//!
//! assert_eq!(selector("$.store.*").unwrap(),
//!             vec![
//!                 &json!([
//!                     { "category": "reference", "author": "Nigel Rees", "title": "Sayings of the Century", "price": 8.95 },
//!                     { "category": "fiction", "author": "Evelyn Waugh", "title": "Sword of Honour", "price": 12.99 },
//!                     { "category": "fiction", "author": "Herman Melville", "title": "Moby Dick", "isbn": "0-553-21311-3", "price": 8.99 },
//!                     { "category": "fiction", "author": "J. R. R. Tolkien", "title": "The Lord of the Rings", "isbn": "0-395-19395-8", "price": 22.99 }
//!                 ]),
//!                 &json!({ "color": "red", "price": 19.95 })
//!             ]);
//!
//! assert_eq!(selector("$.store..price").unwrap(),
//!             vec![
//!                 8.95, 12.99, 8.99, 22.99, 19.95
//!             ]);
//!
//! assert_eq!(selector("$..book[2]").unwrap(),
//!             vec![
//!                 &json!({
//!                     "category" : "fiction",
//!                     "author" : "Herman Melville",
//!                     "title" : "Moby Dick",
//!                     "isbn" : "0-553-21311-3",
//!                     "price" : 8.99
//!                 })
//!             ]);
//!
//! assert_eq!(selector("$..book[-2]").unwrap(),
//!             vec![
//!                 &json!({
//!                     "category" : "fiction",
//!                     "author" : "Herman Melville",
//!                     "title" : "Moby Dick",
//!                     "isbn" : "0-553-21311-3",
//!                     "price" : 8.99
//!                 })
//!             ]);
//!
//! assert_eq!(selector("$..book[0,1]").unwrap(),
//!             vec![
//!                 &json!({"category" : "reference","author" : "Nigel Rees","title" : "Sayings of the Century","price" : 8.95}),
//!                 &json!({"category" : "fiction","author" : "Evelyn Waugh","title" : "Sword of Honour","price" : 12.99})
//!             ]);
//!
//! assert_eq!(selector("$..book[:2]").unwrap(),
//!             vec![
//!                 &json!({"category" : "reference","author" : "Nigel Rees","title" : "Sayings of the Century","price" : 8.95}),
//!                 &json!({"category" : "fiction","author" : "Evelyn Waugh","title" : "Sword of Honour","price" : 12.99})
//!             ]);
//!
//! assert_eq!(selector("$..book[:2]").unwrap(),
//!             vec![
//!                 &json!({"category" : "reference","author" : "Nigel Rees","title" : "Sayings of the Century","price" : 8.95}),
//!                 &json!({"category" : "fiction","author" : "Evelyn Waugh","title" : "Sword of Honour","price" : 12.99})
//!             ]);
//!
//! assert_eq!(selector("$..book[?(@.isbn)]").unwrap(),
//!             vec![
//!                 &json!({"category" : "fiction","author" : "Herman Melville","title" : "Moby Dick","isbn" : "0-553-21311-3","price" : 8.99}),
//!                 &json!({"category" : "fiction","author" : "J. R. R. Tolkien","title" : "The Lord of the Rings","isbn" : "0-395-19395-8","price" : 22.99})
//!             ]);
//!
//! assert_eq!(selector("$.store.book[?(@.price < 10)]").unwrap(),
//!             vec![
//!                 &json!({"category" : "reference","author" : "Nigel Rees","title" : "Sayings of the Century","price" : 8.95}),
//!                 &json!({"category" : "fiction","author" : "Herman Melville","title" : "Moby Dick","isbn" : "0-553-21311-3","price" : 8.99})
//!             ]);
//! ```
extern crate array_tool;
extern crate core;
extern crate env_logger;
#[macro_use]
extern crate log;
extern crate serde;
extern crate serde_json;

use serde_json::Value;

pub use parser::Parser; // TODO private
pub use select::JsonPathError;
pub use select::{Selector, SelectorMut};
use parser::Node;

#[doc(hidden)]
mod ffi;
#[doc(hidden)]
mod parser;
#[doc(hidden)]
mod select;

/// It is a high-order function. it compile a jsonpath and then returns a closure that has JSON as argument. if you need to reuse a jsonpath, it is good for performance.
///
/// ```rust
/// extern crate jsonpath_lib as jsonpath;
/// #[macro_use] extern crate serde_json;
///
/// let mut first_firend = jsonpath::compile("$..friends[0]");
///
/// let json_obj = json!({
///     "school": {
///         "friends": [
///             {"name": "친구1", "age": 20},
///             {"name": "친구2", "age": 20}
///         ]
///     },
///     "friends": [
///         {"name": "친구3", "age": 30},
///         {"name": "친구4"}
/// ]});
///
/// let json = first_firend(&json_obj).unwrap();
///
/// assert_eq!(json, vec![
///     &json!({"name": "친구3", "age": 30}),
///     &json!({"name": "친구1", "age": 20})
/// ]);
/// ```
#[deprecated(
    since = "0.2.5",
    note = "Please use the Compiled::compile function instead"
)]
pub fn compile(path: &str) -> impl FnMut(&Value) -> Result<Vec<&Value>, JsonPathError> {
    let node = parser::Parser::compile(path);
    move |json| match &node {
        Ok(node) => {
            let mut selector = Selector::default();
            selector.compiled_path(node).value(json).select()
        }
        Err(e) => Err(JsonPathError::Path(e.to_string())),
    }
}

/// It is a high-order function. it returns a closure that has a jsonpath string as argument. you can use diffenent jsonpath for one JSON object.
///
/// ```rust
/// extern crate jsonpath_lib as jsonpath;
/// #[macro_use] extern crate serde_json;
///
/// let json_obj = json!({
///     "school": {
///         "friends": [
///             {"name": "친구1", "age": 20},
///             {"name": "친구2", "age": 20}
///         ]
///     },
///     "friends": [
///         {"name": "친구3", "age": 30},
///         {"name": "친구4"}
/// ]});
///
/// let mut selector = jsonpath::selector(&json_obj);
///
/// let json = selector("$..friends[0]").unwrap();
///
/// assert_eq!(json, vec![
///     &json!({"name": "친구3", "age": 30}),
///     &json!({"name": "친구1", "age": 20})
/// ]);
///
/// let json = selector("$..friends[1]").unwrap();
///
/// assert_eq!(json, vec![
///     &json!({"name": "친구4"}),
///     &json!({"name": "친구2", "age": 20})
/// ]);
/// ```
#[allow(clippy::needless_lifetimes)]
pub fn selector<'a>(json: &'a Value) -> impl FnMut(&str) -> Result<Vec<&'a Value>, JsonPathError> {
    let mut selector = Selector::default();
    let _ = selector.value(json);
    move |path: &str| selector.str_path(path)?.reset_value().select()
}

/// It is the same to `selector` function. but it deserialize the result as given type `T`.
///
/// ```rust
/// extern crate jsonpath_lib as jsonpath;
/// extern crate serde;
/// #[macro_use] extern crate serde_json;
///
/// use serde::{Deserialize, Serialize};
///
/// let json_obj = json!({
///     "school": {
///         "friends": [
///             {"name": "친구1", "age": 20},
///             {"name": "친구2", "age": 20}
///         ]
///     },
///     "friends": [
///         {"name": "친구3", "age": 30},
///         {"name": "친구4"}
/// ]});
///
/// #[derive(Deserialize, PartialEq, Debug)]
/// struct Friend {
///     name: String,
///     age: Option<u8>,
/// }
///
/// let mut selector = jsonpath::selector_as::<Friend>(&json_obj);
///
/// let json = selector("$..friends[0]").unwrap();
///
/// let ret = vec!(
///     Friend { name: "친구3".to_string(), age: Some(30) },
///     Friend { name: "친구1".to_string(), age: Some(20) }
/// );
/// assert_eq!(json, ret);
///
/// let json = selector("$..friends[1]").unwrap();
///
/// let ret = vec!(
///     Friend { name: "친구4".to_string(), age: None },
///     Friend { name: "친구2".to_string(), age: Some(20) }
/// );
///
/// assert_eq!(json, ret);
/// ```
pub fn selector_as<T: serde::de::DeserializeOwned>(
    json: &Value,
) -> impl FnMut(&str) -> Result<Vec<T>, JsonPathError> + '_ {
    let mut selector = Selector::default();
    let _ = selector.value(json);
    move |path: &str| selector.str_path(path)?.reset_value().select_as()
}

/// It is a simple select function. but it compile the jsonpath argument every time.
///
/// ```rust
/// extern crate jsonpath_lib as jsonpath;
/// #[macro_use] extern crate serde_json;
///
/// let json_obj = json!({
///     "school": {
///         "friends": [
///             {"name": "친구1", "age": 20},
///             {"name": "친구2", "age": 20}
///         ]
///     },
///     "friends": [
///         {"name": "친구3", "age": 30},
///         {"name": "친구4"}
/// ]});
///
/// let json = jsonpath::select(&json_obj, "$..friends[0]").unwrap();
///
/// assert_eq!(json, vec![
///     &json!({"name": "친구3", "age": 30}),
///     &json!({"name": "친구1", "age": 20})
/// ]);
/// ```
pub fn select<'a>(json: &'a Value, path: &str) -> Result<Vec<&'a Value>, JsonPathError> {
    Selector::default().str_path(path)?.value(json).select()
}

/// It is the same to `select` function but it return the result as string.
///
/// ```rust
/// extern crate jsonpath_lib as jsonpath;
/// #[macro_use] extern crate serde_json;
///
/// let ret = jsonpath::select_as_str(r#"
/// {
///     "school": {
///         "friends": [
///                 {"name": "친구1", "age": 20},
///                 {"name": "친구2", "age": 20}
///             ]
///     },
///     "friends": [
///         {"name": "친구3", "age": 30},
///         {"name": "친구4"}
///     ]
/// }
/// "#, "$..friends[0]").unwrap();
///
/// assert_eq!(ret, r#"[{"name":"친구3","age":30},{"name":"친구1","age":20}]"#);
/// ```
pub fn select_as_str(json_str: &str, path: &str) -> Result<String, JsonPathError> {
    let json = serde_json::from_str(json_str).map_err(|e| JsonPathError::Serde(e.to_string()))?;
    let ret = Selector::default().str_path(path)?.value(&json).select()?;
    serde_json::to_string(&ret).map_err(|e| JsonPathError::Serde(e.to_string()))
}

/// It is the same to `select` function but it deserialize the the result as given type `T`.
///
/// ```rust
/// extern crate jsonpath_lib as jsonpath;
/// extern crate serde;
/// #[macro_use] extern crate serde_json;
///
/// use serde::{Deserialize, Serialize};
///
/// #[derive(Deserialize, PartialEq, Debug)]
/// struct Person {
///     name: String,
///     age: u8,
///     phones: Vec<String>,
/// }
///
/// let ret: Vec<Person> = jsonpath::select_as(r#"
/// {
///     "person":
///         {
///             "name": "Doe John",
///             "age": 44,
///             "phones": [
///                 "+44 1234567",
///                 "+44 2345678"
///             ]
///         }
/// }
/// "#, "$.person").unwrap();
///
/// let person = Person {
///     name: "Doe John".to_string(),
///     age: 44,
///     phones: vec!["+44 1234567".to_string(), "+44 2345678".to_string()],
/// };
///
/// assert_eq!(ret[0], person);
/// ```
pub fn select_as<T: serde::de::DeserializeOwned>(
    json_str: &str,
    path: &str,
) -> Result<Vec<T>, JsonPathError> {
    let json = serde_json::from_str(json_str).map_err(|e| JsonPathError::Serde(e.to_string()))?;
    Selector::default().str_path(path)?.value(&json).select_as()
}

/// Delete(= replace with null) the JSON property using the jsonpath.
///
/// ```rust
/// extern crate jsonpath_lib as jsonpath;
/// #[macro_use] extern crate serde_json;
///
/// let json_obj = json!({
///     "school": {
///         "friends": [
///             {"name": "친구1", "age": 20},
///             {"name": "친구2", "age": 20}
///         ]
///     },
///     "friends": [
///         {"name": "친구3", "age": 30},
///         {"name": "친구4"}
/// ]});
///
/// let ret = jsonpath::delete(json_obj, "$..[?(20 == @.age)]").unwrap();
///
/// assert_eq!(ret, json!({
///     "school": {
///         "friends": [
///             null,
///             null
///         ]
///     },
///     "friends": [
///         {"name": "친구3", "age": 30},
///         {"name": "친구4"}
/// ]}));
/// ```
pub fn delete(value: Value, path: &str) -> Result<Value, JsonPathError> {
    let mut selector = SelectorMut::default();
    let value = selector.str_path(path)?.value(value).delete()?;
    Ok(value.take().unwrap_or(Value::Null))
}

/// Select JSON properties using a jsonpath and transform the result and then replace it. via closure that implements `FnMut` you can transform the selected results.
///
/// ```rust
/// extern crate jsonpath_lib as jsonpath;
/// #[macro_use] extern crate serde_json;
///
/// use serde_json::Value;
///
/// let json_obj = json!({
///     "school": {
///         "friends": [
///             {"name": "친구1", "age": 20},
///             {"name": "친구2", "age": 20}
///         ]
///     },
///     "friends": [
///         {"name": "친구3", "age": 30},
///         {"name": "친구4"}
/// ]});
///
/// let ret = jsonpath::replace_with(json_obj, "$..[?(@.age == 20)].age", &mut |v| {
///     let age = if let Value::Number(n) = v {
///         n.as_u64().unwrap() * 2
///     } else {
///         0
///     };
///
///     Some(json!(age))
/// }).unwrap();
///
/// assert_eq!(ret, json!({
///     "school": {
///         "friends": [
///             {"name": "친구1", "age": 40},
///             {"name": "친구2", "age": 40}
///         ]
///     },
///     "friends": [
///         {"name": "친구3", "age": 30},
///         {"name": "친구4"}
/// ]}));
/// ```
pub fn replace_with<F>(value: Value, path: &str, fun: &mut F) -> Result<Value, JsonPathError>
where
    F: FnMut(Value) -> Option<Value>,
{
    let mut selector = SelectorMut::default();
    let value = selector.str_path(path)?.value(value).replace_with(fun)?;
    Ok(value.take().unwrap_or(Value::Null))
}

/// A pre-compiled expression.
///
/// Calling the select function of this struct will re-use the existing, compiled expression.
///
/// ## Example
///
/// ```rust
/// extern crate jsonpath_lib as jsonpath;
/// #[macro_use] extern crate serde_json;
///
/// let mut first_friend = jsonpath::Compiled::compile("$..friends[0]").unwrap();
///
/// let json_obj = json!({
///     "school": {
///         "friends": [
///             {"name": "친구1", "age": 20},
///             {"name": "친구2", "age": 20}
///         ]
///     },
///     "friends": [
///         {"name": "친구3", "age": 30},
///         {"name": "친구4"}
/// ]});
///
/// // call a first time
///
/// let json = first_friend.select(&json_obj).unwrap();
///
/// assert_eq!(json, vec![
///     &json!({"name": "친구3", "age": 30}),
///     &json!({"name": "친구1", "age": 20})
/// ]);
///
/// // call a second time
///
/// let json = first_friend.select(&json_obj).unwrap();
///
/// assert_eq!(json, vec![
///     &json!({"name": "친구3", "age": 30}),
///     &json!({"name": "친구1", "age": 20})
/// ]);
/// ```
#[derive(Clone, Debug)]
pub struct Compiled {
    node: Node,
}

impl Compiled {
    /// Compile a path expression and return a compiled instance.
    ///
    /// If parsing the path fails, it will return an error.
    pub fn compile(path: &str) -> Result<Compiled, String> {
        let node = parser::Parser::compile(path)?;
        Ok(Compiled{
            node
        })
    }

    /// Execute the select operation on the pre-compiled path.
    pub fn select<'a>(&self, value: &'a Value) -> Result<Vec<&'a Value>, JsonPathError> {
        let mut selector = Selector::default();
        selector.compiled_path(&self.node).value(value).select()
    }
}
