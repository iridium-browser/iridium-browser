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
use crate::extended::deserialize;
use core::cmp;
use rand_ext::rand;
use rand_ext::rand::{distributions, Rng as _};

#[test]
fn de_header_1_byte_0s() {
    let hdr = DeHeader { de_type: 0_u32.into(), len: 0_u8.try_into().unwrap() };

    assert_eq!(&[0x00], hdr.serialize().as_slice());
}

#[test]
fn de_header_1_byte_max() {
    let hdr = DeHeader { de_type: 15_u32.into(), len: 7_u8.try_into().unwrap() };

    assert_eq!(&[0x7F], hdr.serialize().as_slice());
}

#[test]
fn de_header_2_byte_len_too_big() {
    let hdr = DeHeader { de_type: 15_u32.into(), len: 8_u8.try_into().unwrap() };

    assert_eq!(&[0x88, 0x0F], hdr.serialize().as_slice());
}

#[test]
fn de_header_2_byte_type_too_big() {
    let hdr = DeHeader { de_type: 16_u32.into(), len: 7_u8.try_into().unwrap() };

    assert_eq!(&[0x87, 0x10], hdr.serialize().as_slice());
}

#[test]
fn de_header_max() {
    let hdr = DeHeader { de_type: u32::MAX.into(), len: 127_u8.try_into().unwrap() };
    assert_eq!(
        // first type byte has 3x 0 bits because there are 35 total bits in 5 chunks of 7 bits, but
        // only 32 bits to start with
        &[0xFF, 0x8F, 0xFF, 0xFF, 0xFF, 0x7F],
        hdr.serialize().as_slice()
    );
}

#[test]
fn de_header_special_values() {
    for de_type in [0_u32, 1, 15, 16, u32::MAX - 1, u32::MAX].iter().map(|t| DeType::from(*t)) {
        for len in [0_u8, 1, 7, 8, 126, 127].iter().map(|l| DeLength::try_from(*l).unwrap()) {
            let hdr = DeHeader { de_type, len };
            let buf = hdr.serialize();
            let header_len = expected_header_len(hdr);

            assert_eq!(header_len as usize, buf.len());

            let (_, deser) = deserialize::DeHeader::parse(buf.as_slice()).unwrap();

            assert_eq!(
                deserialize::DeHeader { de_type, contents_len: len, header_bytes: buf },
                deser
            )
        }
    }
}

#[test]
fn de_header_random_roundtrip() {
    let mut rng = rand_ext::seeded_rng();

    for _ in 0..100_000 {
        let hdr = DeHeader { de_type: rng.gen(), len: rng.gen() };
        let buf = hdr.serialize();
        let header_len = expected_header_len(hdr);

        assert_eq!(header_len as usize, buf.len());
        let (_, deser) = deserialize::DeHeader::parse(buf.as_slice()).unwrap();

        assert_eq!(
            deserialize::DeHeader {
                de_type: hdr.de_type,
                contents_len: hdr.len,
                header_bytes: buf
            },
            deser
        )
    }
}

fn expected_header_len(hdr: DeHeader) -> u8 {
    if hdr.de_type.as_u32() <= 15 && hdr.len.len <= 7 {
        1
    } else {
        // at least one type byte to handle the type = 0 case
        1_u8 + cmp::max(1, (32 - hdr.de_type.as_u32().leading_zeros() as u8 + 6) / 7)
    }
}

impl distributions::Distribution<DeLength> for distributions::Standard {
    fn sample<R: rand::Rng + ?Sized>(&self, rng: &mut R) -> DeLength {
        DeLength::try_from(rng.gen_range(0_u8..128)).unwrap()
    }
}

impl distributions::Distribution<DeType> for distributions::Standard {
    fn sample<R: rand::Rng + ?Sized>(&self, rng: &mut R) -> DeType {
        rng.gen::<u32>().into()
    }
}
