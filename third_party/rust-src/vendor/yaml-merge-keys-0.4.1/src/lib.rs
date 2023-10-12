// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.

//! YAML Merge Keys
//!
//! The YAML Merge Key extension is not supported by the core YAML crate, but can be implemented
//! after parsing. This crate transforms a parsed YAML document and merges dictionaries together.
//!
//! # Usage
//!
//! This crate provides a function which implements the [YAML Merge Key extension]. Given a YAML
//! document from `yaml-rust` (or `serde_yaml` with the `serde_yaml` feature), it will return
//! a YAML document with the merge keys removed and merged into their owning dictionaries.
//!
//! ```rust
//! # extern crate yaml_rust;
//! # extern crate yaml_merge_keys;
//! # #[cfg(feature = "serde_yaml")]
//! # extern crate serde_yaml;
//! use yaml_rust::YamlLoader;
//! use yaml_merge_keys::merge_keys;
//!
//! // YAML document contents.
//! let raw = "\
//! ref: &ref
//!     merged_key: merged
//!     added_key: merged
//! dict:
//!     <<: *ref
//!     top_key: given
//!     merged_key: given
//! ";
//! let merged = "\
//! ref:
//!     merged_key: merged
//!     added_key: merged
//! dict:
//!     top_key: given
//!     merged_key: given
//!     added_key: merged
//! ";
//!
//! // Parse the YAML documents.
//! let raw_yaml = YamlLoader::load_from_str(raw).unwrap().remove(0);
//! let merged_yaml = YamlLoader::load_from_str(merged).unwrap().remove(0);
//!
//! // Merge the keys.
//! let merged_keys = merge_keys(raw_yaml).unwrap();
//!
//! // The keys have been merged.
//! assert_eq!(merged_keys, merged_yaml);
//!
//! // Using `serde_yaml` is also supported with the feature.
//! #[cfg(feature = "serde_yaml")]
//! {
//!     use yaml_merge_keys::merge_keys_serde;
//!
//!     let raw_yaml = serde_yaml::from_str(raw).unwrap();
//!     let merged_yaml: serde_yaml::Value = serde_yaml::from_str(merged).unwrap();
//!
//!     let merged_keys = merge_keys_serde(raw_yaml).unwrap();
//!
//!     assert_eq!(merged_keys, merged_yaml);
//! }
//! ```
//!
//! [YAML Merge Key extension]: http://yaml.org/type/merge.html
//!
//! # Example
//!
//! ```yaml
//! ---
//! - &CENTER { x: 1, y: 2 }
//! - &LEFT { x: 0, y: 2 }
//! - &BIG { r: 10 }
//! - &SMALL { r: 1 }
//!
//! # All the following maps are equal:
//!
//! - # Explicit keys
//!   x: 1
//!   y: 2
//!   r: 10
//!   label: center/big
//!
//! - # Merge one map
//!   << : *CENTER
//!   r: 10
//!   label: center/big
//!
//! - # Merge multiple maps
//!   << : [ *CENTER, *BIG ]
//!   label: center/big
//!
//! - # Override
//!   << : [ *BIG, *LEFT, *SMALL ]
//!   x: 1
//!   label: center/big
//! ```

#![deny(missing_docs)]

#[macro_use]
extern crate lazy_static;

mod crates {
    // public
    #[cfg(feature = "serde_yaml")]
    pub extern crate serde_yaml;
    pub extern crate yaml_rust;

    // private
    pub extern crate thiserror;
}

mod merge_keys;
#[cfg(feature = "serde_yaml")]
mod serde;

pub use merge_keys::merge_keys;
pub use merge_keys::MergeKeyError;
#[cfg(feature = "serde_yaml")]
pub use serde::merge_keys_serde;

#[cfg(test)]
mod test;
#[cfg(all(test, feature = "serde_yaml"))]
mod test_serde;
