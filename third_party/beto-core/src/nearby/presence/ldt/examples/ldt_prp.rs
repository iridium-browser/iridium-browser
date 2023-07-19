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

//! A demonstration of LDT's PRP behavior.
//!
//! For each trial, between 16 and 31 bytes of random data are generated and encrypted. One random
//! bit of the ciphertext is flipped, then the ciphertext is decrypted. The percentage of flipped
//! bits vs the original plaintext is recorded, and the first n bytes are compared.
//!
//! The output shows how many times a change to the first n bytes wasn't detected, as well as a
//! histogram of how many bits were flipped in the entire plaintext.
use clap::{self, Parser as _};
use crypto_provider::aes::BLOCK_SIZE;
use crypto_provider::{CryptoProvider, CryptoRng};
use crypto_provider_rustcrypto::RustCrypto;
use ldt::*;
use ldt_tbc::TweakableBlockCipher;
use rand::{distributions, Rng as _};

use rand_ext::*;
use xts_aes::{XtsAes128, XtsAes256};

fn main() {
    let args = Args::parse();

    run_trials(args)
}

fn run_trials(args: Args) {
    let mut rng = seeded_rng();
    let mut histo = (0..=100).map(|_| 0_u64).collect::<Vec<_>>();
    let mut undetected_changes = 0_u64;
    let mut cp_rng = <RustCrypto as CryptoProvider>::CryptoRng::new();
    for _ in 0..args.trials {
        let (percent, ok) =
            if rng.gen() {
                do_trial(
                    LdtEncryptCipher::<16, XtsAes128<RustCrypto>, Swap>::new(
                        &LdtKey::from_random::<RustCrypto>(&mut cp_rng),
                    ),
                    LdtDecryptCipher::<16, XtsAes128<RustCrypto>, Swap>::new(
                        &LdtKey::from_random::<RustCrypto>(&mut cp_rng),
                    ),
                    &mut rng,
                    DefaultPadder,
                    &args,
                )
            } else {
                do_trial(
                    LdtEncryptCipher::<16, XtsAes256<RustCrypto>, Swap>::new(
                        &LdtKey::from_random::<RustCrypto>(&mut cp_rng),
                    ),
                    LdtDecryptCipher::<16, XtsAes256<RustCrypto>, Swap>::new(
                        &LdtKey::from_random::<RustCrypto>(&mut cp_rng),
                    ),
                    &mut rng,
                    DefaultPadder,
                    &args,
                )
            };

        histo[percent] += 1;
        if !ok {
            undetected_changes += 1;
        }
    }

    let sum: u64 = histo.iter().sum();
    println!("Histogram of altered plaintext bits");
    for (pct, count) in histo.iter().enumerate() {
        // primitive horizontal bar graph
        println!(
            "{:3}%: {:8} {}",
            pct,
            count,
            (0..((*count as f64) / (sum as f64) * 500_f64).round() as u32)
                .map(|_| "*")
                .collect::<String>()
        );
    }

    println!(
        "{} of {} trials ({:.4}%) failed detect changes to the first {} decrypted bytes",
        undetected_changes,
        args.trials,
        undetected_changes as f64 / (args.trials as f64) * 100_f64,
        args.check_leading_bytes
    );
}

fn do_trial<const B: usize, T: TweakableBlockCipher<B>, P: Padder<B, T>, M: Mix, R: rand::Rng>(
    ldt_enc: LdtEncryptCipher<B, T, M>,
    ldt_dec: LdtDecryptCipher<B, T, M>,
    rng: &mut R,
    padder: P,
    args: &Args,
) -> (usize, bool) {
    let plaintext_len_range = distributions::Uniform::new_inclusive(BLOCK_SIZE, BLOCK_SIZE * 2 - 1);
    let len = rng.sample(plaintext_len_range);
    let plaintext = random_vec_rc(rng, len);

    let mut ciphertext = plaintext.clone();
    ldt_enc.encrypt(&mut ciphertext, &padder).unwrap();

    // flip a random bit
    ciphertext[rng.gen_range(0..len)] ^= 1 << rng.gen_range(0..8);

    ldt_dec.decrypt(&mut ciphertext, &padder).unwrap();
    assert_ne!(plaintext, ciphertext);

    let differing_bits: u32 = plaintext
        .iter()
        .zip(ciphertext.iter())
        .map(|(plain_byte, mangled_byte)| (plain_byte ^ mangled_byte).count_ones())
        .sum();

    let percent = (differing_bits as f64) / (8_f64 * len as f64) * 100_f64;
    let ok = plaintext[0..args.check_leading_bytes] != ciphertext[0..args.check_leading_bytes];

    (percent.round() as usize, ok)
}

#[derive(clap::Parser, Debug)]
struct Args {
    /// How many trials to run
    #[clap(long, default_value_t = 1_000_000)]
    trials: u64,
    /// How many leading bytes to confirm are unchanged
    #[clap(long, default_value_t = 16)]
    check_leading_bytes: usize,
}
