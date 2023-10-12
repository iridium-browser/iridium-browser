// Take a look at the license at the top of the repository in the LICENSE file.

use crate::json::{
    json_minifier::{keep_element, JsonMinifier},
    read::json_read::JsonRead,
    string::JsonMultiFilter,
};
use std::fmt;
use std::io::{self, Read};

mod read {
    mod byte_to_char;
    mod internal_buffer;
    mod internal_reader;
    pub mod json_read;
}

mod json_minifier;
mod string;

type JsonMethod = fn(&mut JsonMinifier, &char, Option<&char>) -> bool;

/// Minifies a given String by JSON minification rules
///
/// # Example
///
/// ```rust
/// use minifier::json::minify;
///
/// let json = r#"
///        {
///            "test": "test",
///            "test2": 2
///        }
///    "#.into();
/// let json_minified = minify(json);
/// assert_eq!(&json_minified.to_string(), r#"{"test":"test","test2":2}"#);
/// ```
#[inline]
pub fn minify(json: &str) -> Minified<'_> {
    Minified(JsonMultiFilter::new(json.chars(), keep_element))
}

#[derive(Debug)]
pub struct Minified<'a>(JsonMultiFilter<'a, JsonMethod>);

impl<'a> Minified<'a> {
    pub fn write<W: io::Write>(self, w: W) -> io::Result<()> {
        self.0.write(w)
    }
}

impl<'a> fmt::Display for Minified<'a> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        self.0.fmt(f)
    }
}

/// Minifies a given Read by JSON minification rules
///
/// # Example
///
/// ```rust
/// extern crate minifier;
/// use std::fs::File;
/// use std::io::Read;
/// use minifier::json::minify_from_read;
///
/// fn main() {
///     let mut html_minified = String::new();
///     let mut file = File::open("tests/files/test.json").expect("file not found");
///     minify_from_read(file).read_to_string(&mut html_minified);
/// }
/// ```
#[inline]
pub fn minify_from_read<R: Read>(json: R) -> JsonRead<JsonMethod, R> {
    JsonRead::new(json, keep_element)
}

#[test]
fn removal_from_read() {
    use std::fs::File;

    let input = File::open("tests/files/test.json").expect("file not found");
    let expected: String = "{\"test\":\"\\\" test2\",\"test2\":\"\",\"test3\":\" \"}".into();
    let mut actual = String::new();
    minify_from_read(input)
        .read_to_string(&mut actual)
        .expect("error at read");
    assert_eq!(actual, expected);
}

#[test]
fn removal_of_control_characters() {
    let input = "\n".into();
    let expected: String = "".into();
    let actual = minify(input);
    assert_eq!(actual.to_string(), expected);
}

#[test]
fn removal_of_whitespace_outside_of_tags() {
    let input = r#"
            {
              "test": "\" test2",
              "test2": "",
              "test3": " "
            }
        "#
    .into();
    let expected: String = "{\"test\":\"\\\" test2\",\"test2\":\"\",\"test3\":\" \"}".into();
    let actual = minify(input);
    assert_eq!(actual.to_string(), expected);
}
