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

use crypto_provider::hmac::Hmac;
use crypto_provider::{CryptoProvider, CryptoRng};
use crypto_provider_openssl::Openssl;
use crypto_provider_rustcrypto::RustCrypto;
use rand_ext::random_bytes;

// simple benchmark, which creates a new hmac, updates once, then finalizes
fn hmac_sha256_operations<C: CryptoProvider>(c: &mut Criterion) {
    let mut rng = C::CryptoRng::new();
    let key: [u8; 32] = rand_ext::random_bytes::<32, C>(&mut rng);
    let update_data: [u8; 16] = rand_ext::random_bytes::<16, C>(&mut rng);

    c.bench_function("bench for hmac sha256 single update", |b| {
        b.iter(|| {
            let mut hmac = C::HmacSha256::new_from_key(key);
            hmac.update(&update_data);
            let _result = hmac.finalize();
        });
    });
}

fn hmac_sha512_operations<C: CryptoProvider>(c: &mut Criterion) {
    let mut rng = C::CryptoRng::new();
    let key: [u8; 64] = rand_ext::random_bytes::<64, C>(&mut rng);
    let update_data: [u8; 16] = random_bytes::<16, C>(&mut rng);

    c.bench_function("bench for hmac sha512 single update", |b| {
        b.iter(|| {
            let mut hmac = C::HmacSha512::new_from_key(key);
            hmac.update(&update_data);
            let _result = hmac.finalize();
        });
    });
}

criterion_group!(
    benches,
    hmac_sha256_operations::<RustCrypto>,
    hmac_sha256_operations::<Openssl>,
    hmac_sha512_operations::<RustCrypto>,
    hmac_sha512_operations::<Openssl>
);

criterion_main!(benches);
