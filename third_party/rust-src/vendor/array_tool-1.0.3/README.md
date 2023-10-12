# array_tool
[![Build Status](https://travis-ci.org/danielpclark/array_tool.svg?branch=master)](https://travis-ci.org/danielpclark/array_tool)
[![Build Status](https://ci.appveyor.com/api/projects/status/dffq3dwb8w220q4f/branch/master?svg=true)](https://ci.appveyor.com/project/danielpclark/array-tool/branch/master)
[![Documentation](https://img.shields.io/badge/docs-100%25-brightgreen.svg)](http://danielpclark.github.io/array_tool/index.html)
[![crates.io version](https://img.shields.io/crates/v/array_tool.svg)](https://crates.io/crates/array_tool)
[![License](https://img.shields.io/badge/license-MIT%20OR%20Apache--2.0-blue.svg)]()

Array helpers for Rust.  Some of the most common methods you would
use on Arrays made available on Vectors.  Polymorphic implementations
for handling most of your use cases.


### Installation

Add the following to your Cargo.toml file
```toml
[dependencies]
array_tool = "1.0.0"
```

And in your rust files where you plan to use it put this at the top
```rust
extern crate array_tool;
```

And if you plan to use all of the Vector helper methods available you may do
```rust
use array_tool::vec::*;
```

This crate has helpful methods for strings as well.

## Iterator Usage

```rust
use array_tool::iter::ZipOpt;
fn zip_option<U: Iterator>(self, other: U) -> ZipOption<Self, U>
  where Self: Sized, U: IntoIterator;
  //  let a = vec![1];
  //  let b = vec![];
  //  a.zip_option(b).next()      // input
  //  Some((Some(1), None))       // return value
```

## Vector Usage

```rust
pub fn uniques<T: PartialEq + Clone>(a: Vec<T>, b: Vec<T>) -> Vec<Vec<T>>
  //  array_tool::uniques(vec![1,2,3,4,5], vec![2,5,6,7,8]) // input
  //  vec![vec![1,3,4], vec![6,7,8]]                        // return value

use array_tool::vec::Uniq;
fn uniq(&self, other: Vec<T>) -> Vec<T>;
  //  vec![1,2,3,4,5,6].uniq( vec![1,2,5,7,9] ) // input
  //  vec![3,4,6]                               // return value
fn uniq_via<F: Fn(&T, &T) -> bool>(&self, other: Self, f: F) -> Self;
  //  vec![1,2,3,4,5,6].uniq_via( vec![1,2,5,7,9], |&l, r| l == r + 2 ) // input 
  //  vec![1,2,4,6]                                                     // return value
fn unique(&self) -> Vec<T>;
  //  vec![1,2,1,3,2,3,4,5,6].unique()          // input
  //  vec![1,2,3,4,5,6]                         // return value
fn unique_via<F: Fn(&T, &T) -> bool>(&self, f: F) -> Self;
  //  vec![1.0,2.0,1.4,3.3,2.1,3.5,4.6,5.2,6.2].
  //  unique_via( |l: &f64, r: &f64| l.floor() == r.floor() ) // input
  //  vec![1.0,2.0,3.3,4.6,5.2,6.2]                           // return value
fn is_unique(&self) -> bool;
  //  vec![1,2,1,3,4,3,4,5,6].is_unique()       // input
  //  false                                     // return value
  //  vec![1,2,3,4,5,6].is_unique()             // input
  //  true                                      // return value

use array_tool::vec::Shift;
fn unshift(&mut self, other: T);    // no return value, modifies &mut self directly
  //  let mut x = vec![1,2,3];
  //  x.unshift(0);
  //  assert_eq!(x, vec![0,1,2,3]);
fn shift(&mut self) -> Option<T>;
  //  let mut x = vec![0,1,2,3];
  //  assert_eq!(x.shift(), Some(0));
  //  assert_eq!(x, vec![1,2,3]);

use array_tool::vec::Intersect;
fn intersect(&self, other: Vec<T>) -> Vec<T>;
  //  vec![1,1,3,5].intersect(vec![1,2,3]) // input
  //  vec![1,3]                            // return value
fn intersect_if<F: Fn(&T, &T) -> bool>(&self, other: Vec<T>, validator: F) -> Vec<T>;
  //  vec!['a','a','c','e'].intersect_if(vec!['A','B','C'], |l, r| l.eq_ignore_ascii_case(r)) // input
  //  vec!['a','c']                                                                           // return value

use array_tool::vec::Join;
fn join(&self, joiner: &'static str) -> String;
  //  vec![1,2,3].join(",")                // input
  //  "1,2,3"                              // return value

use array_tool::vec::Times;
fn times(&self, qty: i32) -> Vec<T>;
  //  vec![1,2,3].times(3)                 // input
  //  vec![1,2,3,1,2,3,1,2,3]              // return value

use array_tool::vec::Union;
fn union(&self, other: Vec<T>) -> Vec<T>;
  //  vec!["a","b","c"].union(vec!["c","d","a"])   // input
  //  vec![ "a", "b", "c", "d" ]                   // return value
```

## String Usage

```rust
use array_tool::string::ToGraphemeBytesIter;
fn grapheme_bytes_iter(&'a self) -> GraphemeBytesIter<'a>;
  //  let string = "a s—d féZ";
  //  let mut graphemes = string.grapheme_bytes_iter()
  //  graphemes.skip(3).next();            // input
  //  [226, 128, 148]                      // return value for emdash `—`

use array_tool::string::Squeeze;
fn squeeze(&self, targets: &'static str) -> String;
  //  "yellow moon".squeeze("")            // input
  //  "yelow mon"                          // return value
  //  "  now   is  the".squeeze(" ")       // input
  //  " now is the"                        // return value

use array_tool::string::Justify;
fn justify_line(&self, width: usize) -> String;
  //  "asd as df asd".justify_line(16)     // input
  //  "asd  as  df  asd"                   // return value
  //  "asd as df asd".justify_line(18)     // input
  //  "asd   as   df  asd"                 // return value

use array_tool::string::SubstMarks;
fn subst_marks(&self, marks: Vec<usize>, chr: &'static str) -> String;
  //  "asdf asdf asdf".subst_marks(vec![0,5,8], "Z") // input
  //  "Zsdf ZsdZ asdf"                               // return value

use array_tool::string::WordWrap;
fn word_wrap(&self, width: usize) -> String;
  //  "01234 67 9 BC EFG IJ".word_wrap(6)  // input
  //  "01234\n67 9\nBC\nEFG IJ"            // return value

use array_tool::string::AfterWhitespace;
fn seek_end_of_whitespace(&self, offset: usize) -> Option<usize>;
  //  "asdf           asdf asdf".seek_end_of_whitespace(6) // input
  //  Some(9)                                              // return value
  //  "asdf".seek_end_of_whitespace(3)                     // input
  //  Some(0)                                              // return value
  //  "asdf           ".seek_end_of_whitespace(6)          // input
  //  None                                                 // return_value

```

## Future plans

Expect methods to become more polymorphic over time (same method implemented
for similar & compatible types).  I plan to implement many of the methods
available for Arrays in higher languages; such as Ruby. Expect regular updates.

## License

Licensed under either of

 * Apache License, Version 2.0, ([LICENSE-APACHE](LICENSE-APACHE) or http://www.apache.org/licenses/LICENSE-2.0)
 * MIT license ([MIT-LICENSE](MIT-LICENSE) or http://opensource.org/licenses/MIT)

at your option.

### Contribution

Unless you explicitly state otherwise, any contribution intentionally submitted
for inclusion in the work by you, as defined in the Apache-2.0 license, shall be dual licensed as above, without any
additional terms or conditions.
