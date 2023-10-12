// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.

use crates::serde_yaml::Value;

use merge_keys::MergeKeyError;
use serde::merge_keys_serde;

fn assert_yaml_idempotent(doc: Value) {
    assert_eq!(merge_keys_serde(doc.clone()).unwrap(), doc);
}

fn merge_key() -> Value {
    Value::String("<<".into())
}

macro_rules! yaml_hash {
    // XXX(rust-1.37.0): Use `$(,)?` when available.
    [ $( $pair:expr ),* $(,)* ] => {
        Value::Mapping([$( $pair, )*].iter().cloned().collect())
    };
}

#[test]
fn test_ignore_non_containers() {
    let null = Value::Null;
    let bool_true = Value::Bool(true);
    let bool_false = Value::Bool(false);
    let string = Value::String("".into());
    let integer = Value::Number(1234.into());
    let real = Value::Number(0.02.into());

    assert_yaml_idempotent(null);
    assert_yaml_idempotent(bool_true);
    assert_yaml_idempotent(bool_false);
    assert_yaml_idempotent(string);
    assert_yaml_idempotent(integer);
    assert_yaml_idempotent(real);
}

#[test]
fn test_ignore_container_no_merge_keys() {
    let arr = Value::Sequence(vec![Value::Number(10.into()), Value::Number(100.into())]);
    let hash = yaml_hash![
        (Value::Number(10.into()), Value::Null),
        (Value::Number(100.into()), Value::String("string".into())),
    ];

    assert_yaml_idempotent(arr);
    assert_yaml_idempotent(hash);
}

#[test]
fn test_remove_merge_keys() {
    let hash = yaml_hash![
        (merge_key(), yaml_hash![]),
        (Value::Number(10.into()), Value::Null),
        (Value::Number(100.into()), Value::String("string".into())),
    ];
    let expected = yaml_hash![
        (Value::Number(10.into()), Value::Null),
        (Value::Number(100.into()), Value::String("string".into())),
    ];

    assert_eq!(merge_keys_serde(hash).unwrap(), expected);
}

#[test]
fn test_handle_merge_keys() {
    let hash = yaml_hash![
        (
            merge_key(),
            yaml_hash![(Value::Number(15.into()), Value::Null)],
        ),
        (Value::Number(10.into()), Value::Null),
        (Value::Number(100.into()), Value::String("string".into())),
    ];
    let expected = yaml_hash![
        (Value::Number(10.into()), Value::Null),
        (Value::Number(100.into()), Value::String("string".into())),
        (Value::Number(15.into()), Value::Null),
    ];

    assert_eq!(merge_keys_serde(hash).unwrap(), expected);
}

#[test]
fn test_merge_key_precedence() {
    let hash = yaml_hash![
        (
            merge_key(),
            yaml_hash![(Value::Number(10.into()), Value::Number(10.into()))],
        ),
        (Value::Number(10.into()), Value::Null),
        (Value::Number(100.into()), Value::String("string".into())),
    ];
    let expected = yaml_hash![
        (Value::Number(10.into()), Value::Null),
        (Value::Number(100.into()), Value::String("string".into())),
    ];

    assert_eq!(merge_keys_serde(hash).unwrap(), expected);
}

#[test]
fn test_merge_key_array() {
    let hash = yaml_hash![
        (
            merge_key(),
            Value::Sequence(vec![
                yaml_hash![(Value::Number(15.into()), Value::Number(10.into()))],
                yaml_hash![(Value::Number(20.into()), Value::Number(10.into()))],
            ]),
        ),
        (Value::Number(10.into()), Value::Null),
        (Value::Number(100.into()), Value::String("string".into())),
    ];
    let expected = yaml_hash![
        (Value::Number(10.into()), Value::Null),
        (Value::Number(100.into()), Value::String("string".into())),
        (Value::Number(15.into()), Value::Number(10.into())),
        (Value::Number(20.into()), Value::Number(10.into())),
    ];

    assert_eq!(merge_keys_serde(hash).unwrap(), expected);
}

#[test]
fn test_merge_key_array_precedence() {
    let hash = yaml_hash![
        (
            merge_key(),
            Value::Sequence(vec![
                yaml_hash![(Value::Number(15.into()), Value::Number(10.into()))],
                yaml_hash![(Value::Number(15.into()), Value::Number(20.into()))],
            ]),
        ),
        (Value::Number(10.into()), Value::Null),
        (Value::Number(100.into()), Value::String("string".into())),
    ];
    let expected = yaml_hash![
        (Value::Number(10.into()), Value::Null),
        (Value::Number(100.into()), Value::String("string".into())),
        (Value::Number(15.into()), Value::Number(10.into())),
    ];

    assert_eq!(merge_keys_serde(hash).unwrap(), expected);
}

#[test]
fn test_merge_key_nested_array() {
    let hash = Value::Sequence(vec![yaml_hash![
        (
            merge_key(),
            Value::Sequence(vec![
                yaml_hash![(Value::Number(15.into()), Value::Number(10.into()))],
                yaml_hash![(Value::Number(15.into()), Value::Number(20.into()))],
            ]),
        ),
        (Value::Number(10.into()), Value::Null),
        (Value::Number(100.into()), Value::String("string".into())),
    ]]);
    let expected = Value::Sequence(vec![yaml_hash![
        (Value::Number(10.into()), Value::Null),
        (Value::Number(100.into()), Value::String("string".into())),
        (Value::Number(15.into()), Value::Number(10.into())),
    ]]);

    assert_eq!(merge_keys_serde(hash).unwrap(), expected);
}

#[test]
fn test_merge_key_nested_hash_value() {
    let hash = yaml_hash![(
        Value::Null,
        yaml_hash![
            (
                merge_key(),
                Value::Sequence(vec![
                    yaml_hash![(Value::Number(15.into()), Value::Number(10.into()))],
                    yaml_hash![(Value::Number(15.into()), Value::Number(20.into()))],
                ]),
            ),
            (Value::Number(10.into()), Value::Null),
            (Value::Number(100.into()), Value::String("string".into())),
        ],
    )];
    let expected = yaml_hash![(
        Value::Null,
        yaml_hash![
            (Value::Number(10.into()), Value::Null),
            (Value::Number(100.into()), Value::String("string".into())),
            (Value::Number(15.into()), Value::Number(10.into())),
        ],
    )];

    assert_eq!(merge_keys_serde(hash).unwrap(), expected);
}

#[test]
fn test_merge_key_nested_hash_key() {
    let hash = yaml_hash![(
        yaml_hash![
            (
                merge_key(),
                Value::Sequence(vec![
                    yaml_hash![(Value::Number(15.into()), Value::Number(10.into()))],
                    yaml_hash![(Value::Number(15.into()), Value::Number(20.into()))],
                ]),
            ),
            (Value::Number(10.into()), Value::Null),
            (Value::Number(100.into()), Value::String("string".into())),
        ],
        Value::Null,
    )];
    let expected = yaml_hash![(
        yaml_hash![
            (Value::Number(10.into()), Value::Null),
            (Value::Number(100.into()), Value::String("string".into())),
            (Value::Number(15.into()), Value::Number(10.into())),
        ],
        Value::Null,
    )];

    assert_eq!(merge_keys_serde(hash).unwrap(), expected);
}

#[test]
fn test_yaml_spec_examples() {
    let center = yaml_hash![
        (Value::String("x".into()), Value::Number(1.into())),
        (Value::String("y".into()), Value::Number(2.into())),
    ];
    let left = yaml_hash![
        (Value::String("x".into()), Value::Number(0.into())),
        (Value::String("y".into()), Value::Number(2.into())),
    ];
    let big = yaml_hash![(Value::String("r".into()), Value::Number(10.into()))];
    let small = yaml_hash![(Value::String("r".into()), Value::Number(1.into()))];

    let explicit = yaml_hash![
        (Value::String("x".into()), Value::Number(1.into())),
        (Value::String("y".into()), Value::Number(2.into())),
        (Value::String("r".into()), Value::Number(10.into())),
        (
            Value::String("label".into()),
            Value::String("center/big".into()),
        ),
    ];
    let explicit_ordered = yaml_hash![
        (Value::String("r".into()), Value::Number(10.into())),
        (
            Value::String("label".into()),
            Value::String("center/big".into()),
        ),
        (Value::String("x".into()), Value::Number(1.into())),
        (Value::String("y".into()), Value::Number(2.into())),
    ];
    let explicit_ordered_overrides = yaml_hash![
        (Value::String("x".into()), Value::Number(1.into())),
        (
            Value::String("label".into()),
            Value::String("center/big".into()),
        ),
        (Value::String("r".into()), Value::Number(10.into())),
        (Value::String("y".into()), Value::Number(2.into())),
    ];
    let merge_one_map = yaml_hash![
        (merge_key(), center.clone()),
        (Value::String("r".into()), Value::Number(10.into())),
        (
            Value::String("label".into()),
            Value::String("center/big".into()),
        ),
    ];
    let merge_multiple_maps = yaml_hash![
        (merge_key(), Value::Sequence(vec![center, big.clone()])),
        (Value::String("r".into()), Value::Number(10.into())),
        (
            Value::String("label".into()),
            Value::String("center/big".into()),
        ),
    ];
    let overrides = yaml_hash![
        (merge_key(), Value::Sequence(vec![big, left, small]),),
        (Value::String("x".into()), Value::Number(1.into())),
        (
            Value::String("label".into()),
            Value::String("center/big".into()),
        ),
    ];

    assert_eq!(merge_keys_serde(explicit.clone()).unwrap(), explicit);
    assert_eq!(merge_keys_serde(merge_one_map).unwrap(), explicit_ordered);
    assert_eq!(
        merge_keys_serde(merge_multiple_maps).unwrap(),
        explicit_ordered,
    );
    assert_eq!(
        merge_keys_serde(overrides).unwrap(),
        explicit_ordered_overrides,
    );
}

macro_rules! assert_is_error {
    ( $doc:expr, $kind:path ) => {
        let err = merge_keys_serde($doc).unwrap_err();

        if let $kind = err {
            // Expected error.
        } else {
            panic!("unexpected error: {:?}", err);
        }
    };
}

#[test]
fn test_invalid_merge_key_values() {
    let merge_null = yaml_hash![(merge_key(), Value::Null)];
    let merge_bool = yaml_hash![(merge_key(), Value::Bool(false))];
    let merge_string = yaml_hash![(merge_key(), Value::String("".into()))];
    let merge_integer = yaml_hash![(merge_key(), Value::Number(0.into()))];
    let merge_real = yaml_hash![(merge_key(), Value::Number(0.02.into()))];

    assert_is_error!(merge_null, MergeKeyError::InvalidMergeValue);
    assert_is_error!(merge_bool, MergeKeyError::InvalidMergeValue);
    assert_is_error!(merge_string, MergeKeyError::InvalidMergeValue);
    assert_is_error!(merge_integer, MergeKeyError::InvalidMergeValue);
    assert_is_error!(merge_real, MergeKeyError::InvalidMergeValue);
}

#[test]
fn test_invalid_merge_key_array_values() {
    let merge_null = yaml_hash![(merge_key(), Value::Sequence(vec![Value::Null]))];
    let merge_bool = yaml_hash![(merge_key(), Value::Sequence(vec![Value::Bool(false)]))];
    let merge_string = yaml_hash![(merge_key(), Value::Sequence(vec![Value::String("".into())]))];
    let merge_integer = yaml_hash![(merge_key(), Value::Sequence(vec![Value::Number(0.into())]))];
    let merge_real = yaml_hash![(
        merge_key(),
        Value::Sequence(vec![Value::Number(0.02.into())]),
    )];

    assert_is_error!(merge_null, MergeKeyError::InvalidMergeValue);
    assert_is_error!(merge_bool, MergeKeyError::InvalidMergeValue);
    assert_is_error!(merge_string, MergeKeyError::InvalidMergeValue);
    assert_is_error!(merge_integer, MergeKeyError::InvalidMergeValue);
    assert_is_error!(merge_real, MergeKeyError::InvalidMergeValue);
}
