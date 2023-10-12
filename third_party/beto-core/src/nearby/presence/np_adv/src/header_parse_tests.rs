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

use super::*;

extern crate std;

use nom::error;

#[test]
fn parse_header_v0() {
    let (_, header) = parse_adv_header(&[0x00]).unwrap();
    assert_eq!(AdvHeader::V0, header);
}

#[test]
fn parse_header_v0_nonzero_reserved() {
    let input = &[0x01];
    assert_eq!(
        nom::Err::Error(error::Error { input: input.as_slice(), code: error::ErrorKind::Verify }),
        parse_adv_header(input).unwrap_err()
    );
}

#[test]
fn parse_header_v1_nonzero_reserved() {
    let input = &[0x30];
    assert_eq!(
        nom::Err::Error(error::Error { input: input.as_slice(), code: error::ErrorKind::Verify }),
        parse_adv_header(input).unwrap_err()
    );
}

#[test]
fn parse_header_bad_version() {
    let input = &[0x80];
    assert_eq!(
        nom::Err::Error(error::Error { input: input.as_slice(), code: error::ErrorKind::Verify }),
        parse_adv_header(input).unwrap_err()
    );
}

#[test]
fn parse_header_v1() {
    let (_, header) = parse_adv_header(&[0x20]).unwrap();
    assert_eq!(AdvHeader::V1(V1Header { header_byte: 0x20 }), header);
}
