// Copyright 2023 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

use crypto_provider::hmac::MacError;
use crypto_provider_default::CryptoProviderImpl;
use np_hkdf::*;

#[test]
fn verify_hmac_correct_mac() {
    let data = &[1_u8; 32];
    let hmac_key = [2; 32];

    let hmac = NpHmacSha256Key::<CryptoProviderImpl>::from(hmac_key);

    let mac = hmac.calculate_hmac(data);

    assert_eq!(Ok(()), hmac.verify_hmac(data, mac));
}

#[test]
fn verify_hmac_incorrect_mac() {
    let data = &[1_u8; 32];
    let hmac_key = [2; 32];

    let hmac = NpHmacSha256Key::<CryptoProviderImpl>::from(hmac_key);

    let _mac = hmac.calculate_hmac(data);

    // wrong mac
    assert_eq!(Err(MacError), hmac.verify_hmac(data, [0xFF; 32]));
}
