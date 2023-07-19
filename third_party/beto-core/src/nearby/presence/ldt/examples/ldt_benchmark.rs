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

//! A manual benchmark for more interactive parameter-twiddling.

use clap::Parser as _;
use crypto_provider_rustcrypto::RustCrypto;
use ldt::{LdtDecryptCipher, LdtEncryptCipher, LdtKey, Mix, Swap, XorPadder};

use crypto_provider::{CryptoProvider, CryptoRng};
use ldt_tbc::TweakableBlockCipher;
use rand::{distributions, seq::SliceRandom, Rng as _, SeedableRng as _};
use sha2::digest::{generic_array, Digest as _};
use std::time;
use subtle::ConstantTimeEq as _;

use rand_ext::*;
use xts_aes::XtsAes128;

fn main() {
    let args = Args::parse();
    let mut rng = <RustCrypto as CryptoProvider>::CryptoRng::new();

    // generate a suitable number of random keys
    let scenarios = (0..args.keys)
        .map(|_| {
            random_ldt_scenario::<16, XtsAes128<RustCrypto>, Swap, RustCrypto>(&mut rng, args.len)
        })
        .collect::<Vec<_>>();

    let padder = XorPadder::from([0x42; crypto_provider::aes::BLOCK_SIZE]);

    let ciphertexts = scenarios
        .iter()
        .map(|s| {
            let mut ciphertext = s.plaintext.clone();
            s.ldt_enc.encrypt(&mut ciphertext[..], &padder).unwrap();
            ciphertext
        })
        .collect::<Vec<_>>();

    let not_found_distrib = distributions::Uniform::from(0_f64..=100_f64);
    let unfindable_ciphertext = random_vec::<RustCrypto>(&mut rng, args.len);

    let mut histogram = hdrhistogram::Histogram::<u64>::new(3).unwrap();
    let mut buf = Vec::new();

    let mut hasher = sha2::Sha256::new();
    let mut hash_output = generic_array::GenericArray::default();

    let mut rc_rng = rand::rngs::StdRng::from_entropy();
    let found = (0..args.trials)
        .map(|_| {
            let ciphertext = if rc_rng.sample(not_found_distrib) <= args.not_found_pct as f64 {
                &unfindable_ciphertext
            } else {
                ciphertexts.choose(&mut rc_rng).unwrap()
            };

            let start = time::Instant::now();

            let found = scenarios.iter().any(|scenario| {
                hasher.reset();

                buf.clear();
                buf.extend_from_slice(ciphertext.as_slice());
                scenario.ldt_dec.decrypt(&mut buf, &padder).unwrap();

                hasher.update(&buf[..MATCH_LEN]);
                hasher.finalize_into_reset(&mut hash_output);

                let arr_ref: &[u8; 32] = hash_output.as_ref();
                arr_ref.ct_eq(&scenario.plaintext_prefix_hash).into()
            });

            histogram.record((start.elapsed().as_micros()) as u64).unwrap();

            found
        })
        .filter(|&found| found)
        .count();

    println!(
        "Found {} of {} ({}%)",
        found,
        args.trials,
        (found as f64) / (args.trials as f64) * 100_f64
    );

    println!(
        "90%ile:    {}μs\n95%ile:    {}μs\n99%ile:    {}μs\n99.9%ile:  {}μs\n99.99%ile: {}μs\nMax:       {}μs",
        histogram.value_at_quantile(0.90),
        histogram.value_at_quantile(0.95),
        histogram.value_at_quantile(0.99),
        histogram.value_at_quantile(0.999),
        histogram.value_at_quantile(0.9999),
        histogram.max(),
    );
}

#[derive(clap::Parser, Debug)]
struct Args {
    /// How many keys/plaintexts/ciphertexts to generate
    #[clap(long, default_value_t = 1000)]
    keys: u64,
    /// How many trials to run
    #[clap(long, default_value_t = 100_000)]
    trials: u64,
    /// Plaintext len
    #[clap(long, default_value_t = 24)]
    len: usize,
    /// What percentage of decryptions should fail to find a match
    #[clap(long, default_value_t = 50)]
    not_found_pct: u8,
}

/// How much of the plaintext should be hashed for subsequent matching
const MATCH_LEN: usize = 16;

struct LdtScenario<const B: usize, T: TweakableBlockCipher<B>, M: Mix> {
    ldt_enc: LdtEncryptCipher<B, T, M>,
    ldt_dec: LdtDecryptCipher<B, T, M>,
    plaintext: Vec<u8>,
    plaintext_prefix_hash: [u8; 32],
}

fn random_ldt_scenario<const B: usize, T: TweakableBlockCipher<B>, M: Mix, C: CryptoProvider>(
    rng: &mut C::CryptoRng,
    plaintext_len: usize,
) -> LdtScenario<B, T, M> {
    let ldt_key: LdtKey<T::Key> = LdtKey::from_random::<C>(rng);
    let ldt_enc = LdtEncryptCipher::new(&ldt_key);
    let ldt_dec = LdtDecryptCipher::new(&ldt_key);
    let plaintext = random_vec::<C>(rng, plaintext_len);

    let mut hasher = sha2::Sha256::new();
    let mut plaintext_prefix_hash = generic_array::GenericArray::default();
    hasher.update(&plaintext[..MATCH_LEN]);
    hasher.finalize_into_reset(&mut plaintext_prefix_hash);

    LdtScenario { ldt_enc, ldt_dec, plaintext, plaintext_prefix_hash: plaintext_prefix_hash.into() }
}
