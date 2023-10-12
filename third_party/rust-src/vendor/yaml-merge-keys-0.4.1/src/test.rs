// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.

use crates::yaml_rust::Yaml;

use merge_keys::{merge_keys, MergeKeyError};

fn assert_yaml_idempotent(doc: Yaml) {
    assert_eq!(merge_keys(doc.clone()).unwrap(), doc);
}

fn merge_key() -> Yaml {
    Yaml::String("<<".into())
}

macro_rules! yaml_hash {
    // XXX(rust-1.37.0): Use `$(,)?` when available.
    [ $( $pair:expr ),* $(,)* ] => {
        Yaml::Hash([$( $pair, )*].iter().cloned().collect())
    };
}

#[test]
fn test_ignore_non_containers() {
    let null = Yaml::Null;
    let bool_true = Yaml::Boolean(true);
    let bool_false = Yaml::Boolean(false);
    let string = Yaml::String("".into());
    let integer = Yaml::Integer(1234);
    let real = Yaml::Real("0.02".into());

    assert_yaml_idempotent(null);
    assert_yaml_idempotent(bool_true);
    assert_yaml_idempotent(bool_false);
    assert_yaml_idempotent(string);
    assert_yaml_idempotent(integer);
    assert_yaml_idempotent(real);
}

#[test]
fn test_ignore_container_no_merge_keys() {
    let arr = Yaml::Array(vec![Yaml::Integer(10), Yaml::Integer(100)]);
    let hash = yaml_hash![
        (Yaml::Integer(10), Yaml::Null),
        (Yaml::Integer(100), Yaml::String("string".into())),
    ];

    assert_yaml_idempotent(arr);
    assert_yaml_idempotent(hash);
}

#[test]
fn test_remove_merge_keys() {
    let hash = yaml_hash![
        (merge_key(), yaml_hash![]),
        (Yaml::Integer(10), Yaml::Null),
        (Yaml::Integer(100), Yaml::String("string".into())),
    ];
    let expected = yaml_hash![
        (Yaml::Integer(10), Yaml::Null),
        (Yaml::Integer(100), Yaml::String("string".into())),
    ];

    assert_eq!(merge_keys(hash).unwrap(), expected);
}

#[test]
fn test_handle_merge_keys() {
    let hash = yaml_hash![
        (merge_key(), yaml_hash![(Yaml::Integer(15), Yaml::Null)]),
        (Yaml::Integer(10), Yaml::Null),
        (Yaml::Integer(100), Yaml::String("string".into())),
    ];
    let expected = yaml_hash![
        (Yaml::Integer(10), Yaml::Null),
        (Yaml::Integer(100), Yaml::String("string".into())),
        (Yaml::Integer(15), Yaml::Null),
    ];

    assert_eq!(merge_keys(hash).unwrap(), expected);
}

#[test]
fn test_merge_key_precedence() {
    let hash = yaml_hash![
        (
            merge_key(),
            yaml_hash![(Yaml::Integer(10), Yaml::Integer(10))],
        ),
        (Yaml::Integer(10), Yaml::Null),
        (Yaml::Integer(100), Yaml::String("string".into())),
    ];
    let expected = yaml_hash![
        (Yaml::Integer(10), Yaml::Null),
        (Yaml::Integer(100), Yaml::String("string".into())),
    ];

    assert_eq!(merge_keys(hash).unwrap(), expected);
}

#[test]
fn test_merge_key_array() {
    let hash = yaml_hash![
        (
            merge_key(),
            Yaml::Array(vec![
                yaml_hash![(Yaml::Integer(15), Yaml::Integer(10))],
                yaml_hash![(Yaml::Integer(20), Yaml::Integer(10))],
            ]),
        ),
        (Yaml::Integer(10), Yaml::Null),
        (Yaml::Integer(100), Yaml::String("string".into())),
    ];
    let expected = yaml_hash![
        (Yaml::Integer(10), Yaml::Null),
        (Yaml::Integer(100), Yaml::String("string".into())),
        (Yaml::Integer(15), Yaml::Integer(10)),
        (Yaml::Integer(20), Yaml::Integer(10)),
    ];

    assert_eq!(merge_keys(hash).unwrap(), expected);
}

#[test]
fn test_merge_key_array_precedence() {
    let hash = yaml_hash![
        (
            merge_key(),
            Yaml::Array(vec![
                yaml_hash![(Yaml::Integer(15), Yaml::Integer(10))],
                yaml_hash![(Yaml::Integer(15), Yaml::Integer(20))],
            ]),
        ),
        (Yaml::Integer(10), Yaml::Null),
        (Yaml::Integer(100), Yaml::String("string".into())),
    ];
    let expected = yaml_hash![
        (Yaml::Integer(10), Yaml::Null),
        (Yaml::Integer(100), Yaml::String("string".into())),
        (Yaml::Integer(15), Yaml::Integer(10)),
    ];

    assert_eq!(merge_keys(hash).unwrap(), expected);
}

#[test]
fn test_merge_key_nested_array() {
    let hash = Yaml::Array(vec![yaml_hash![
        (
            merge_key(),
            Yaml::Array(vec![
                yaml_hash![(Yaml::Integer(15), Yaml::Integer(10))],
                yaml_hash![(Yaml::Integer(15), Yaml::Integer(20))],
            ]),
        ),
        (Yaml::Integer(10), Yaml::Null),
        (Yaml::Integer(100), Yaml::String("string".into())),
    ]]);
    let expected = Yaml::Array(vec![yaml_hash![
        (Yaml::Integer(10), Yaml::Null),
        (Yaml::Integer(100), Yaml::String("string".into())),
        (Yaml::Integer(15), Yaml::Integer(10)),
    ]]);

    assert_eq!(merge_keys(hash).unwrap(), expected);
}

#[test]
fn test_merge_key_nested_hash_value() {
    let hash = yaml_hash![(
        Yaml::Null,
        yaml_hash![
            (
                merge_key(),
                Yaml::Array(vec![
                    yaml_hash![(Yaml::Integer(15), Yaml::Integer(10))],
                    yaml_hash![(Yaml::Integer(15), Yaml::Integer(20))],
                ]),
            ),
            (Yaml::Integer(10), Yaml::Null),
            (Yaml::Integer(100), Yaml::String("string".into())),
        ],
    )];
    let expected = yaml_hash![(
        Yaml::Null,
        yaml_hash![
            (Yaml::Integer(10), Yaml::Null),
            (Yaml::Integer(100), Yaml::String("string".into())),
            (Yaml::Integer(15), Yaml::Integer(10)),
        ],
    )];

    assert_eq!(merge_keys(hash).unwrap(), expected);
}

#[test]
fn test_merge_key_nested_hash_key() {
    let hash = yaml_hash![(
        yaml_hash![
            (
                merge_key(),
                Yaml::Array(vec![
                    yaml_hash![(Yaml::Integer(15), Yaml::Integer(10))],
                    yaml_hash![(Yaml::Integer(15), Yaml::Integer(20))],
                ]),
            ),
            (Yaml::Integer(10), Yaml::Null),
            (Yaml::Integer(100), Yaml::String("string".into())),
        ],
        Yaml::Null,
    )];
    let expected = yaml_hash![(
        yaml_hash![
            (Yaml::Integer(10), Yaml::Null),
            (Yaml::Integer(100), Yaml::String("string".into())),
            (Yaml::Integer(15), Yaml::Integer(10)),
        ],
        Yaml::Null,
    )];

    assert_eq!(merge_keys(hash).unwrap(), expected);
}

#[test]
fn test_yaml_spec_examples() {
    let center = yaml_hash![
        (Yaml::String("x".into()), Yaml::Integer(1)),
        (Yaml::String("y".into()), Yaml::Integer(2)),
    ];
    let left = yaml_hash![
        (Yaml::String("x".into()), Yaml::Integer(0)),
        (Yaml::String("y".into()), Yaml::Integer(2)),
    ];
    let big = yaml_hash![(Yaml::String("r".into()), Yaml::Integer(10))];
    let small = yaml_hash![(Yaml::String("r".into()), Yaml::Integer(1))];

    let explicit = yaml_hash![
        (Yaml::String("x".into()), Yaml::Integer(1)),
        (Yaml::String("y".into()), Yaml::Integer(2)),
        (Yaml::String("r".into()), Yaml::Integer(10)),
        (
            Yaml::String("label".into()),
            Yaml::String("center/big".into()),
        ),
    ];
    let explicit_ordered = yaml_hash![
        (Yaml::String("r".into()), Yaml::Integer(10)),
        (
            Yaml::String("label".into()),
            Yaml::String("center/big".into()),
        ),
        (Yaml::String("x".into()), Yaml::Integer(1)),
        (Yaml::String("y".into()), Yaml::Integer(2)),
    ];
    let explicit_ordered_overrides = yaml_hash![
        (Yaml::String("x".into()), Yaml::Integer(1)),
        (
            Yaml::String("label".into()),
            Yaml::String("center/big".into()),
        ),
        (Yaml::String("r".into()), Yaml::Integer(10)),
        (Yaml::String("y".into()), Yaml::Integer(2)),
    ];
    let merge_one_map = yaml_hash![
        (merge_key(), center.clone()),
        (Yaml::String("r".into()), Yaml::Integer(10)),
        (
            Yaml::String("label".into()),
            Yaml::String("center/big".into()),
        ),
    ];
    let merge_multiple_maps = yaml_hash![
        (merge_key(), Yaml::Array(vec![center, big.clone()])),
        (Yaml::String("r".into()), Yaml::Integer(10)),
        (
            Yaml::String("label".into()),
            Yaml::String("center/big".into()),
        ),
    ];
    let overrides = yaml_hash![
        (merge_key(), Yaml::Array(vec![big, left, small]),),
        (Yaml::String("x".into()), Yaml::Integer(1)),
        (
            Yaml::String("label".into()),
            Yaml::String("center/big".into()),
        ),
    ];

    assert_eq!(merge_keys(explicit.clone()).unwrap(), explicit);
    assert_eq!(merge_keys(merge_one_map).unwrap(), explicit_ordered);
    assert_eq!(merge_keys(merge_multiple_maps).unwrap(), explicit_ordered);
    assert_eq!(merge_keys(overrides).unwrap(), explicit_ordered_overrides);
}

macro_rules! assert_is_error {
    ( $doc:expr, $kind:path ) => {
        let err = merge_keys($doc).unwrap_err();

        if let $kind = err {
            // Expected error.
        } else {
            panic!("unexpected error: {:?}", err);
        }
    };
}

#[test]
fn test_invalid_merge_key_values() {
    let merge_null = yaml_hash![(merge_key(), Yaml::Null)];
    let merge_bool = yaml_hash![(merge_key(), Yaml::Boolean(false))];
    let merge_string = yaml_hash![(merge_key(), Yaml::String("".into()))];
    let merge_integer = yaml_hash![(merge_key(), Yaml::Integer(0))];
    let merge_real = yaml_hash![(merge_key(), Yaml::Real("0.02".into()))];

    assert_is_error!(merge_null, MergeKeyError::InvalidMergeValue);
    assert_is_error!(merge_bool, MergeKeyError::InvalidMergeValue);
    assert_is_error!(merge_string, MergeKeyError::InvalidMergeValue);
    assert_is_error!(merge_integer, MergeKeyError::InvalidMergeValue);
    assert_is_error!(merge_real, MergeKeyError::InvalidMergeValue);
}

#[test]
fn test_invalid_merge_key_array_values() {
    let merge_null = yaml_hash![(merge_key(), Yaml::Array(vec![Yaml::Null]))];
    let merge_bool = yaml_hash![(merge_key(), Yaml::Array(vec![Yaml::Boolean(false)]))];
    let merge_string = yaml_hash![(merge_key(), Yaml::Array(vec![Yaml::String("".into())]))];
    let merge_integer = yaml_hash![(merge_key(), Yaml::Array(vec![Yaml::Integer(0)]))];
    let merge_real = yaml_hash![(merge_key(), Yaml::Array(vec![Yaml::Real("0.02".into())]))];

    assert_is_error!(merge_null, MergeKeyError::InvalidMergeValue);
    assert_is_error!(merge_bool, MergeKeyError::InvalidMergeValue);
    assert_is_error!(merge_string, MergeKeyError::InvalidMergeValue);
    assert_is_error!(merge_integer, MergeKeyError::InvalidMergeValue);
    assert_is_error!(merge_real, MergeKeyError::InvalidMergeValue);
}
