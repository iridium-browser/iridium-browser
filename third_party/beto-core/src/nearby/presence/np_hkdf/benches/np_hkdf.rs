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

use criterion::{black_box, criterion_group, criterion_main, Criterion};
use crypto_provider::{CryptoProvider, CryptoRng};
use crypto_provider_default::CryptoProviderImpl;
use rand_ext::random_bytes;

pub fn build_np_hkdf(c: &mut Criterion) {
    let mut rng = <CryptoProviderImpl as CryptoProvider>::CryptoRng::new();
    for &num_keys in &[1_usize, 10, 100] {
        c.bench_function(&format!("build {num_keys} np_hkdf from key_seed"), |b| {
            let keys = (0..num_keys)
                .map(|_| random_bytes::<32, CryptoProviderImpl>(&mut rng))
                .collect::<Vec<_>>();
            b.iter(|| {
                for key_seed in keys.iter() {
                    black_box(np_hkdf::NpKeySeedHkdf::<CryptoProviderImpl>::new(key_seed));
                }
            });
        });
        c.bench_function(&format!("hkdf generate {num_keys} hmac keys"), |b| {
            let keys = (0..num_keys)
                .map(|_| {
                    np_hkdf::NpKeySeedHkdf::<CryptoProviderImpl>::new(&random_bytes::<
                        32,
                        CryptoProviderImpl,
                    >(
                        &mut rng
                    ))
                })
                .collect::<Vec<_>>();
            b.iter(|| {
                for hkdf in keys.iter() {
                    black_box(hkdf.extended_unsigned_metadata_key_hmac_key());
                }
            });
        });
        c.bench_function(&format!("hkdf generate {num_keys} AES keys"), |b| {
            let keys = (0..num_keys)
                .map(|_| {
                    np_hkdf::NpKeySeedHkdf::<CryptoProviderImpl>::new(&random_bytes::<
                        32,
                        CryptoProviderImpl,
                    >(
                        &mut rng
                    ))
                })
                .collect::<Vec<_>>();
            b.iter(|| {
                for hkdf in keys.iter() {
                    black_box(np_hkdf::UnsignedSectionKeys::aes_key(hkdf));
                }
            });
        });
        c.bench_function(&format!("hkdf generate {num_keys} LDT keys"), |b| {
            let keys = (0..num_keys)
                .map(|_| {
                    np_hkdf::NpKeySeedHkdf::<CryptoProviderImpl>::new(&random_bytes::<
                        32,
                        CryptoProviderImpl,
                    >(
                        &mut rng
                    ))
                })
                .collect::<Vec<_>>();
            b.iter(|| {
                for hkdf in keys.iter() {
                    black_box(hkdf.legacy_ldt_key());
                }
            });
        });
    }
}

criterion_group!(benches, build_np_hkdf);
criterion_main!(benches);
