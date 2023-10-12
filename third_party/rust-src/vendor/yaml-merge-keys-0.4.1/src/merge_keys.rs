// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.

use crates::thiserror::Error;
use crates::yaml_rust::yaml::{Array, Hash};
use crates::yaml_rust::Yaml;

/// Errors which may occur when performing the YAML merge key process.
///
/// This enum is `non_exhaustive`, but cannot be marked as such until it is stable. In the
/// meantime, there is a hidden variant.
#[derive(Debug, Error)]
// TODO: #[non_exhaustive]
pub enum MergeKeyError {
    /// A non-hash value was given as a value to merge into a hash.
    ///
    /// This happens with a document such as:
    ///
    /// ```yaml
    /// -
    ///   <<: 4
    ///   x: 1
    /// ```
    #[error("only mappings and arrays of mappings may be merged")]
    InvalidMergeValue,
    /// This is here to force `_` matching right now.
    ///
    /// **DO NOT USE**
    #[doc(hidden)]
    #[error("unreachable...")]
    _NonExhaustive,
}

lazy_static! {
    /// The name of the key to use for merge data.
    static ref MERGE_KEY: Yaml = Yaml::String("<<".into());
}

/// Merge two hashes together.
fn merge_hashes(mut hash: Hash, rhs: Hash) -> Hash {
    rhs.into_iter().for_each(|(key, value)| {
        hash.entry(key).or_insert(value);
    });
    hash
}

/// Merge values together.
fn merge_values(hash: Hash, value: Yaml) -> Result<Hash, MergeKeyError> {
    let merge_values = match value {
        Yaml::Array(arr) => {
            let init: Result<Hash, _> = Ok(Hash::new());

            arr.into_iter().fold(init, |res_hash, item| {
                // Merge in the next item.
                res_hash.and_then(move |res_hash| {
                    if let Yaml::Hash(next_hash) = item {
                        Ok(merge_hashes(res_hash, next_hash))
                    } else {
                        // Non-hash values at this level are not allowed.
                        Err(MergeKeyError::InvalidMergeValue)
                    }
                })
            })?
        },
        Yaml::Hash(merge_hash) => merge_hash,
        _ => return Err(MergeKeyError::InvalidMergeValue),
    };

    Ok(merge_hashes(hash, merge_values))
}

/// Recurse into a hash and handle items with merge keys in them.
fn merge_hash(hash: Hash) -> Result<Yaml, MergeKeyError> {
    let mut hash = hash
        .into_iter()
        // First handle any merge keys in the key or value...
        .map(|(key, value)| {
            merge_keys(key).and_then(|key| merge_keys(value).map(|value| (key, value)))
        })
        .collect::<Result<Hash, _>>()?;

    if let Some(merge_value) = hash.remove(&MERGE_KEY) {
        merge_values(hash, merge_value).map(Yaml::Hash)
    } else {
        Ok(Yaml::Hash(hash))
    }
}

/// Recurse into an array and handle items with merge keys in them.
fn merge_array(arr: Array) -> Result<Yaml, MergeKeyError> {
    arr.into_iter()
        .map(merge_keys)
        .collect::<Result<Array, _>>()
        .map(Yaml::Array)
}

/// Handle merge keys in a YAML document.
pub fn merge_keys(doc: Yaml) -> Result<Yaml, MergeKeyError> {
    match doc {
        Yaml::Hash(hash) => merge_hash(hash),
        Yaml::Array(arr) => merge_array(arr),
        _ => Ok(doc),
    }
}
