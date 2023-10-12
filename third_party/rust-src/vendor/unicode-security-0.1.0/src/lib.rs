// Copyright 2012-2015 The Rust Project Developers. See the COPYRIGHT
// file at the top-level directory of this distribution and at
// http://rust-lang.org/COPYRIGHT.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.

//! Detect possible security problems with Unicode usage according to
//! [Unicode Technical Standard #39](http://www.unicode.org/reports/tr39/)
//! rules.
//!
//! ```rust
//! extern crate unicode_security;
//!
//! use unicode_security::GeneralSecurityProfile;
//!
//! fn main() {
//!     let ch = 'µ'; // U+00B5 MICRO SIGN
//!     let allowed = 'µ'.identifier_allowed();
//!     println!("{}", ch);
//!     println!("The above char is {} in unicode identifiers.",
//!              if allowed { "allowed" } else { "restricted" });
//! }
//! ```
//!
//! # features
//!
//! unicode-security supports a `no_std` feature. This eliminates dependence
//! on std, and instead uses equivalent functions from core.
//!
//! # crates.io
//!
//! You can use this package in your project by adding the following
//! to your `Cargo.toml`:
//!
//! ```toml
//! [dependencies]
//! unicode-security = "0.0.1"
//! ```

#![deny(missing_docs, unsafe_code)]
#![doc(
    html_logo_url = "https://unicode-rs.github.io/unicode-rs_sm.png",
    html_favicon_url = "https://unicode-rs.github.io/unicode-rs_sm.png"
)]
#![cfg_attr(feature = "bench", feature(test))]
#![no_std]

#[cfg(test)]
#[macro_use]
extern crate std;

#[cfg(feature = "bench")]
extern crate test;

pub use tables::UNICODE_VERSION;

pub mod confusable_detection;
pub mod general_security_profile;
pub mod mixed_script;
pub mod restriction_level;

pub use confusable_detection::skeleton;
pub use general_security_profile::GeneralSecurityProfile;
pub use mixed_script::is_potential_mixed_script_confusable_char;
pub use mixed_script::MixedScript;
pub use restriction_level::{RestrictionLevel, RestrictionLevelDetection};

#[rustfmt::skip]
pub(crate) mod tables;

#[cfg(test)]
mod tests;
