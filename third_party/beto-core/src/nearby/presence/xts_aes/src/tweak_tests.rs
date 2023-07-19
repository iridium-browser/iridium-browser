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
#![allow(clippy::indexing_slicing, clippy::unwrap_used, clippy::panic, clippy::expect_used)]

extern crate std;

use std::{
    fs,
    io::{self, BufRead as _},
    vec::Vec,
};

use crate::TweakState;

#[test]
fn advance_from_all_ones() {
    let mut tweak_state = TweakState::new([1; 16]);

    tweak_state.advance_to_block(1);

    assert_eq!(1, tweak_state.block_num);
    assert_eq!([2; 16], tweak_state.tweak);
}

#[test]
fn advance_with_carry() {
    let mut tweak_state = TweakState::new([0xF0; 16]);
    tweak_state.advance_to_block(1);

    assert_eq!(1, tweak_state.block_num);
    let mut expected_state = [225; 16];
    expected_state[0] = 103;
    assert_eq!(expected_state, tweak_state.tweak);
}

#[test]
fn tweak_test_vectors() {
    let file_path = test_helper::get_data_file(
        "presence/xts_aes/resources/test/tweak-state-advance-test-vectors.txt",
    );
    let file = fs::File::open(file_path).expect("Should be able to open file");

    let count = io::BufReader::new(file)
        .lines()
        .map(|r| r.unwrap())
        .filter(|l| !l.starts_with('#'))
        .inspect(|l| {
            let chunks = l.split(' ').collect::<Vec<_>>();
            let start_bytes = hex::decode(chunks[0]).unwrap();
            let end_bytes = hex::decode(chunks[2]).unwrap();
            let block_num: u32 = chunks[1].parse().unwrap();

            let mut state = TweakState::new(start_bytes.try_into().unwrap());
            state.advance_to_block(block_num);
            assert_eq!(end_bytes, state.tweak);
        })
        .count();

    assert_eq!(1000, count);
}
