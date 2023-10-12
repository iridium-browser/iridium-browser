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
use ldt_np_adv::*;

use array_view::ArrayView;
use rand::{Rng as _, SeedableRng as _};

use crypto_provider::CryptoProvider;
use crypto_provider_openssl::Openssl;
use crypto_provider_rustcrypto::RustCrypto;

use np_hkdf::NpKeySeedHkdf;

fn ldt_adv_scan<C: CryptoProvider>(c: &mut Criterion) {
    let mut seed: <rand_pcg::Pcg64 as rand::SeedableRng>::Seed = Default::default();
    rand::thread_rng().fill(&mut seed);
    let mut rng = rand_pcg::Pcg64::from_seed(seed);

    for &len in &[1_usize, 10, 1000] {
        c.bench_function(&format!("Scan adv with fresh ciphers/{len}"), |b| {
            let configs = random_configs::<C, _>(&mut rng, len);
            let payload_len = rng.gen_range(crypto_provider::aes::BLOCK_SIZE..=LDT_XTS_AES_MAX_LEN);
            let payload = random_vec(&mut rng, payload_len);

            let salt = LegacySalt::from(rng.gen::<[u8; 2]>());
            #[allow(clippy::unit_arg)]
            b.iter(|| {
                let ciphers = build_ciphers(&configs);
                black_box(find_matching_item::<C>(&ciphers, salt, &payload))
            });
        });
        c.bench_function(&format!("Scan adv with existing ciphers/{len}"), |b| {
            let configs = random_configs::<C, _>(&mut rng, len);
            let payload_len = rng.gen_range(crypto_provider::aes::BLOCK_SIZE..=LDT_XTS_AES_MAX_LEN);
            let payload = random_vec(&mut rng, payload_len);

            let salt = LegacySalt::from(rng.gen::<[u8; 2]>());
            let ciphers = build_ciphers(&configs);
            #[allow(clippy::unit_arg)]
            b.iter(|| black_box(find_matching_item::<C>(&ciphers, salt, &payload)));
        });
    }
}

criterion_group!(benches, ldt_adv_scan::<RustCrypto>, ldt_adv_scan::<Openssl>);
criterion_main!(benches);

fn find_matching_item<C: CryptoProvider>(
    ciphers: &[LdtNpAdvDecrypterXtsAes128<C>],
    salt: LegacySalt,
    payload: &[u8],
) {
    let padder = salt_padder::<16, C>(salt);
    ciphers
        .iter()
        .enumerate()
        .filter_map(|(index, item)| {
            item.decrypt_and_verify(payload, &padder)
                .map(|buffer| (index, buffer))
                // any error = move to the next item
                .ok()
        })
        .next()
        .map(|(index, buffer)| MatchResult { matching_index: index, buffer });
}

fn build_ciphers<C: CryptoProvider>(
    configs: &[CipherConfig<C>],
) -> Vec<LdtNpAdvDecrypterXtsAes128<C>> {
    configs
        .iter()
        .map(|config| {
            build_np_adv_decrypter_from_key_seed(&config.key_seed, config.metadata_key_hmac)
        })
        .collect::<Vec<_>>()
}

struct CipherConfig<C: CryptoProvider> {
    key_seed: NpKeySeedHkdf<C>,
    metadata_key_hmac: [u8; 32],
}

/// `O` is the buffer size of the LDT config that produced this
#[derive(PartialEq, Eq, Debug)]
pub struct MatchResult<const O: usize> {
    /// The index of the batch item that matched
    matching_index: usize,
    /// The buffer holding the plaintext
    buffer: ArrayView<u8, O>,
}

fn random_configs<C: CryptoProvider, R: rand::Rng>(
    rng: &mut R,
    len: usize,
) -> Vec<CipherConfig<C>> {
    (0..len)
        // ok to use random hmac since we want to try all configs always
        .map(|_| CipherConfig {
            key_seed: NpKeySeedHkdf::new(&rng.gen()),
            metadata_key_hmac: rng.gen(),
        })
        .collect()
}

fn random_vec<R: rand::Rng>(rng: &mut R, len: usize) -> Vec<u8> {
    let mut bytes = Vec::<u8>::new();
    bytes.extend((0..len).map(|_| rng.gen::<u8>()));
    bytes
}
