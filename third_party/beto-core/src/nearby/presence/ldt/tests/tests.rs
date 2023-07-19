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

extern crate alloc;

use alloc::vec;
use crypto_provider::aes::BLOCK_SIZE;
use crypto_provider_default::CryptoProviderImpl;
use ldt::{
    DefaultPadder, LdtDecryptCipher, LdtEncryptCipher, LdtError, LdtKey, Padder, Swap, XorPadder,
};
use xts_aes::{XtsAes128, XtsAes128Key};

#[test]
fn normal_pad_empty() {
    let padder = DefaultPadder;
    let tweak: xts_aes::Tweak =
        <DefaultPadder as Padder<16, XtsAes128<CryptoProviderImpl>>>::pad_tweak(&padder, &[]);
    let bytes = tweak.le_bytes();

    // leading 1 bit
    let mut expected = [0; 16];
    expected[0] = 0x80;
    assert_eq!(expected, bytes);
}

#[test]
fn normal_pad_one_byte() {
    let padder = DefaultPadder;
    let tweak: xts_aes::Tweak =
        <DefaultPadder as Padder<16, XtsAes128<CryptoProviderImpl>>>::pad_tweak(&padder, &[0x81]);

    let bytes = tweak.le_bytes();

    // 1 bit after original byte
    let mut expected = [0; 16];
    expected[0] = 0x81;
    expected[1] = 0x80;
    assert_eq!(expected, bytes);
}

#[test]
fn normal_pad_max_len() {
    let padder = DefaultPadder;
    let input = [0x99; 15];
    let tweak: xts_aes::Tweak =
        <DefaultPadder as Padder<16, XtsAes128<CryptoProviderImpl>>>::pad_tweak(&padder, &input);

    let bytes = tweak.le_bytes();

    // 1 bit after original bytes
    let mut expected = [0x99; 16];
    expected[15] = 0x80;
    assert_eq!(expected, bytes);
}

#[test]
#[should_panic]
fn normal_pad_too_big_panics() {
    let padder = DefaultPadder;
    let input = [0x99; 16];
    <DefaultPadder as Padder<16, XtsAes128<CryptoProviderImpl>>>::pad_tweak(&padder, &input);
}

#[test]
fn xor_pad_empty() {
    let padder = [0x24; BLOCK_SIZE].into();
    let tweak: xts_aes::Tweak = <XorPadder<BLOCK_SIZE> as Padder<
        BLOCK_SIZE,
        XtsAes128<CryptoProviderImpl>,
    >>::pad_tweak(&padder, &[]);

    let bytes = tweak.le_bytes();

    // leading 1 bit
    let mut expected = [0x24; BLOCK_SIZE];
    expected[0] = 0x24 ^ 0x80;
    assert_eq!(expected, bytes);
}

#[test]
fn xor_pad_one_byte() {
    let padder = [0x24; BLOCK_SIZE].into();
    let tweak: xts_aes::Tweak = <XorPadder<BLOCK_SIZE> as Padder<
        BLOCK_SIZE,
        XtsAes128<CryptoProviderImpl>,
    >>::pad_tweak(&padder, &[0x81]);

    let bytes = tweak.le_bytes();

    // 1 bit after original byte
    let mut expected = [0x24; BLOCK_SIZE];
    expected[0] = 0x81 ^ 0x24;
    expected[1] = 0x80 ^ 0x24;
    assert_eq!(expected, bytes);
}

#[test]
fn xor_pad_max_len() {
    let padder = [0x24; BLOCK_SIZE].into();
    let input = [0x99; 15];
    let tweak: xts_aes::Tweak = <XorPadder<BLOCK_SIZE> as Padder<
        BLOCK_SIZE,
        XtsAes128<CryptoProviderImpl>,
    >>::pad_tweak(&padder, &input);

    let bytes = tweak.le_bytes();

    // 1 bit after original bytes
    let mut expected = [0x99 ^ 0x24; BLOCK_SIZE];
    expected[15] = 0x80 ^ 0x24;
    assert_eq!(expected, bytes);
}

#[test]
#[should_panic]
fn xor_pad_too_big_panics() {
    let padder = [0x24; BLOCK_SIZE].into();
    // need 1 byte for padding, and 2 more for salt
    let input = [0x99; 16];
    <XorPadder<BLOCK_SIZE> as Padder<BLOCK_SIZE, XtsAes128<CryptoProviderImpl>>>::pad_tweak(
        &padder, &input,
    );
}

#[test]
fn encrypt_too_short_err() {
    do_length_check_enc(7)
}

#[test]
fn encrypt_too_long_err() {
    do_length_check_enc(40)
}
#[test]
fn decrypt_too_short_err() {
    do_length_check_dec(7)
}
#[test]
fn decrypt_too_long_err() {
    do_length_check_dec(40)
}

fn do_length_check_dec(len: usize) {
    let ldt_dec = LdtDecryptCipher::<{ BLOCK_SIZE }, XtsAes128<CryptoProviderImpl>, Swap>::new(
        &LdtKey::<XtsAes128Key>::from_concatenated(&[0u8; 64]),
    );

    let mut payload = vec![0; len];
    assert_eq!(Err(LdtError::InvalidLength(len)), ldt_dec.decrypt(&mut payload, &DefaultPadder));
}

fn do_length_check_enc(len: usize) {
    let ldt_enc = LdtEncryptCipher::<{ BLOCK_SIZE }, XtsAes128<CryptoProviderImpl>, Swap>::new(
        &LdtKey::<XtsAes128Key>::from_concatenated(&[0u8; 64]),
    );

    let mut payload = vec![0; len];
    assert_eq!(Err(LdtError::InvalidLength(len)), ldt_enc.encrypt(&mut payload, &DefaultPadder));
}
