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

use criterion::{criterion_group, criterion_main, Criterion};
use hex_literal::hex;

use crypto_provider::hkdf::Hkdf;
use crypto_provider::CryptoProvider;
use crypto_provider_openssl::Openssl;
use crypto_provider_rustcrypto::RustCrypto;

// simple benchmark, which creates a new hmac, updates once, then finalizes
fn hkdf_sha256_operations<C: CryptoProvider>(c: &mut Criterion) {
    let ikm = hex!("0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b");
    let salt = hex!("000102030405060708090a0b0c");
    let info = hex!("f0f1f2f3f4f5f6f7f8f9");

    c.bench_function(&format!("bench hkdf with salt {}", std::any::type_name::<C>()), |b| {
        b.iter(|| {
            let hk = C::HkdfSha256::new(Some(&salt[..]), &ikm);
            let mut okm = [0u8; 42];
            hk.expand(&info, &mut okm).expect("42 is a valid length for Sha256 to output");
        });
    });

    c.bench_function(&format!("bench hkdf no salt {}", std::any::type_name::<C>()), |b| {
        b.iter(|| {
            let hk = C::HkdfSha256::new(None, &ikm);
            let mut okm = [0u8; 42];
            hk.expand(&info, &mut okm).expect("42 is a valid length for Sha256 to output");
        });
    });
}

criterion_group!(benches, hkdf_sha256_operations::<RustCrypto>, hkdf_sha256_operations::<Openssl>,);

criterion_main!(benches);
