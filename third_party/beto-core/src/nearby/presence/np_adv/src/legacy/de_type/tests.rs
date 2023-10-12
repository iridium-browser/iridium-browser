// Copyright 2022 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

extern crate std;

use super::*;
use crate::legacy::serialize::id_de_type_as_generic_de_type;
use std::{collections, vec::Vec};
use strum::IntoEnumIterator as _;

#[test]
fn no_plain_vs_identity_type_overlap() {
    let plain_types =
        PlainDataElementType::iter().map(|t| t.as_generic_de_type()).collect::<Vec<_>>();
    let identity_types =
        IdentityDataElementType::iter().map(id_de_type_as_generic_de_type).collect::<Vec<_>>();

    for plain_de_type in plain_types.iter() {
        assert!(!identity_types.iter().any(|i| i == plain_de_type));
        assert_eq!(None, plain_de_type.try_as_identity_de_type());
    }

    for id_de_type in identity_types.iter() {
        assert!(!plain_types.iter().any(|p| p == id_de_type));
        assert_eq!(None, id_de_type.try_as_plain_de_type());
    }
}

#[test]
fn generic_type_is_either_plain_or_identity() {
    let generic_types = DataElementType::iter().collect::<Vec<_>>();

    for g in generic_types.iter() {
        let total = g.try_as_identity_de_type().map(|_| 1).unwrap_or(0)
            + g.try_as_plain_de_type().map(|_| 1).unwrap_or(0);

        assert_eq!(1, total);
    }
}

#[test]
fn generic_de_type_codes_are_consistent() {
    for det in DataElementType::iter() {
        let actual = DataElementType::from_type_code(det.type_code());
        assert_eq!(Some(det), actual)
    }
}

#[test]
fn generic_de_distinct_type_codes() {
    let codes =
        DataElementType::iter().map(|det| det.type_code()).collect::<collections::HashSet<_>>();
    assert_eq!(codes.len(), DataElementType::iter().count());
}

#[test]
fn generic_de_no_accidentally_mapped_type_codes() {
    let codes =
        DataElementType::iter().map(|det| det.type_code()).collect::<collections::HashSet<_>>();
    for possible_code in 0..=15 {
        if codes.contains(&possible_code.try_into().unwrap()) {
            continue;
        }

        assert_eq!(None, DataElementType::from_type_code(possible_code.try_into().unwrap()));
    }
}

#[test]
fn actions_de_length_zero_rejected() {
    let encoded = DeEncodedLength::try_from(0).unwrap();
    let maybe_actual_len = DataElementType::Actions.actual_len_for_encoded_len(encoded);
    assert_eq!(Err(DeLengthOutOfRange), maybe_actual_len);
}

#[test]
fn de_length_actual_encoded_round_trip() {
    for de_type in DataElementType::iter() {
        // for all possible lengths, calculate actual -> encoded and the inverse
        let actual_to_encoded = (0_u8..=255)
            .filter_map(|num| num.try_into().ok())
            .filter_map(|actual: DeActualLength| {
                de_type.encoded_len_for_actual_len(actual).ok().map(|encoded| (actual, encoded))
            })
            .collect::<collections::HashMap<_, _>>();

        let encoded_to_actual = (0_u8..=255)
            .filter_map(|num| num.try_into().ok())
            .filter_map(|encoded: DeEncodedLength| {
                de_type.actual_len_for_encoded_len(encoded).ok().map(|actual| (encoded, actual))
            })
            .collect::<collections::HashMap<_, _>>();

        // ensure the two maps are inverses of each other
        assert_eq!(
            actual_to_encoded,
            encoded_to_actual.into_iter().map(|(encoded, actual)| (actual, encoded)).collect(),
            "de type: {de_type:?}"
        );
    }
}
