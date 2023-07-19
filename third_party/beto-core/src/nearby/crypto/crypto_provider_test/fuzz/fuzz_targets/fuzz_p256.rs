#![no_main]
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

use arbitrary::Arbitrary;
use crypto_provider::{
    elliptic_curve::EcdhProvider,
    p256::{P256PublicKey, P256},
    CryptoProvider,
};
use crypto_provider_default::CryptoProviderImpl;
use libfuzzer_sys::fuzz_target;

#[derive(Debug, Arbitrary)]
struct FuzzInput {
    bytes: Vec<u8>,
}

type P256PublicKeyAlias<P> = <<P as CryptoProvider>::P256 as EcdhProvider<P256>>::PublicKey;

fuzz_target!(|input: FuzzInput| {
    let pubkey = P256PublicKeyAlias::<CryptoProviderImpl>::from_sec1_bytes(&input.bytes);
    if let Ok(key) = pubkey {
        let (x, y) = key
            .to_affine_coordinates()
            .expect("Valid keys should always be able to output affine coordinates");
        let recreated_pubkey =
            P256PublicKeyAlias::<CryptoProviderImpl>::from_affine_coordinates(&x, &y)
                .expect("Creating public key from affine coordinates should succeed");
        assert_eq!(key, recreated_pubkey);
    }
});
