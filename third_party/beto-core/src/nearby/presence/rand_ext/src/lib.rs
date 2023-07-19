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

//! Helper functions around `rand`'s offerings for convenient test usage.
#![no_std]
#![forbid(unsafe_code)]
#![deny(missing_docs)]

extern crate alloc;

use alloc::vec::Vec;
use crypto_provider::{CryptoProvider, CryptoRng};
use log::info;
pub use rand;
use rand::{Rng as _, SeedableRng};

/// Returns a random Vec with the provided length.
pub fn random_vec<C: CryptoProvider>(rng: &mut C::CryptoRng, len: usize) -> Vec<u8> {
    let mut bytes = Vec::<u8>::new();
    bytes.extend((0..len).map(|_| rng.gen::<u8>()));
    bytes
}

/// Returns a random array with the provided length.
pub fn random_bytes<const B: usize, C: CryptoProvider>(rng: &mut C::CryptoRng) -> [u8; B] {
    let mut bytes = [0; B];
    rng.fill(bytes.as_mut_slice());
    bytes
}

/// Uses a RustCrypto Rng to return a random Vec with the provided length
pub fn random_vec_rc<R: rand::Rng>(rng: &mut R, len: usize) -> Vec<u8> {
    let mut bytes = Vec::<u8>::new();
    bytes.extend((0..len).map(|_| rng.gen::<u8>()));
    bytes
}

/// Uses a RustCrypto Rng to return random bytes with the provided length
pub fn random_bytes_rc<const B: usize, R: rand::Rng>(rng: &mut R) -> [u8; B] {
    let mut bytes = [0; B];
    rng.fill(bytes.as_mut_slice());
    bytes
}

/// Returns a fast rng seeded with the thread rng (which is itself seeded from the OS).
pub fn seeded_rng() -> impl rand::Rng {
    let mut seed: <rand_pcg::Pcg64 as rand::SeedableRng>::Seed = Default::default();
    rand::thread_rng().fill(&mut seed);
    // print it out so if a test fails, the seed will be visible for further investigation
    info!("seed: {:?}", seed);
    rand_pcg::Pcg64::from_seed(seed)
}
