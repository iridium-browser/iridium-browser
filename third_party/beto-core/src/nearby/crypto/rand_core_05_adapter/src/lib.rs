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

//! Adapter for using rand_core 0.5 RNGs with code that expects rand_core 0.5 RNGs.
#![no_std]
#![forbid(unsafe_code)]
#![deny(
    missing_docs,
    clippy::indexing_slicing,
    clippy::unwrap_used,
    clippy::panic,
    clippy::expect_used
)]

/// A trivial adapter to expose rand 1.0/rand_core 0.6 rngs to ed25519-dalek's rand_core 0.5 types,
/// which we import under a separate name so there's no clash.
pub struct RandWrapper<'r, R: rand::RngCore + rand::CryptoRng> {
    rng: &'r mut R,
}

impl<'r, R: rand::RngCore + rand::CryptoRng> RandWrapper<'r, R> {
    /// Build a rand_core 0.5 compatible wrapper around the provided rng.
    pub fn from(rng: &'r mut R) -> RandWrapper<'r, R> {
        RandWrapper { rng }
    }
}

impl<'r, R: rand::RngCore + rand::CryptoRng> rand_core05::RngCore for RandWrapper<'r, R> {
    fn next_u32(&mut self) -> u32 {
        self.rng.next_u32()
    }

    fn next_u64(&mut self) -> u64 {
        self.rng.next_u64()
    }

    fn fill_bytes(&mut self, dest: &mut [u8]) {
        self.rng.fill_bytes(dest)
    }

    #[cfg(feature = "std")]
    fn try_fill_bytes(&mut self, dest: &mut [u8]) -> Result<(), rand_core05::Error> {
        self.rng.try_fill_bytes(dest).map_err(|e| rand_core05::Error::new(e.take_inner()))
    }

    #[cfg(not(feature = "std"))]
    #[allow(clippy::expect_used)]
    fn try_fill_bytes(&mut self, dest: &mut [u8]) -> Result<(), rand_core05::Error> {
        self.rng
            .try_fill_bytes(dest)
            .map_err(|e| rand_core05::Error::from(e.code().expect("for no_std this is never none")))
    }
}

impl<'r, R: rand::RngCore + rand::CryptoRng> rand_core05::CryptoRng for RandWrapper<'r, R> {
    // marker trait
}
