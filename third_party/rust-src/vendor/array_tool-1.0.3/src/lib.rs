#![deny(missing_docs,trivial_casts,trivial_numeric_casts,
        missing_debug_implementations, missing_copy_implementations,
        unsafe_code,unused_import_braces,unused_qualifications)
]
// Copyright 2015-2017 Daniel P. Clark & array_tool Developers
// 
// Licensed under the Apache License, Version 2.0, <LICENSE-APACHE or
// http://apache.org/licenses/LICENSE-2.0> or the MIT license <LICENSE-MIT or
// http://opensource.org/licenses/MIT>, at your option. This file may not be
// copied, modified, or distributed except according to those terms.

//! # Array Tool
//!
//! is a collection of powerful methods for working with collections.
//! Some of the most common methods you would use on Arrays made available
//! on Vectors. Polymorphic implementations for handling most of your use cases.
//!
//! In your rust files where you plan to use it put this at the top
//!
//! ```
//! extern crate array_tool;
//! ```
//!
//! And if you plan to use all of the Vector helper methods available:
//!
//! ```
//! use array_tool::vec::*;
//! ```
//!
//! This crate is not limited to just Vector methods and has some helpful
//! string methods as well.


/// Array Tool provides useful methods for iterators 
pub mod iter;
/// Array Tool provides many useful methods for vectors
pub mod vec;
/// A string is a collection so we should have more methods for handling strings.
pub mod string;

/// Get `uniques` from two vectors
///
/// # Example
/// ```
/// use array_tool::uniques;
///
/// uniques(vec![1,2,3,4,5], vec![2,5,6,7,8]);
/// ```
///
/// # Output
/// ```text
/// vec![vec![1,3,4], vec![6,7,8]]
/// ```
pub fn uniques<T: PartialEq + Clone>(a: Vec<T>, b: Vec<T>) -> Vec<Vec<T>> {
  use self::vec::Uniq;
  vec![a.uniq(b.clone()), b.uniq(a)]
}

