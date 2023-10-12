#![allow(clippy::expect_used)]
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

#[cfg(test)]
/// A mock implementation of a CryptoRng that returns the given bytes in `values` in
/// sequence.
pub(crate) struct MockCryptoRng<'a, I: Iterator<Item = &'a u8>> {
    pub(crate) values: I,
}

impl<'a, I: Iterator<Item = &'a u8>> rand::CryptoRng for MockCryptoRng<'a, I> {}

impl<'a, I: Iterator<Item = &'a u8>> rand::RngCore for MockCryptoRng<'a, I> {
    fn fill_bytes(&mut self, dest: &mut [u8]) {
        for i in dest {
            *i = *self.values.next().expect("Expecting more data in MockCryptoRng input");
        }
    }

    fn next_u32(&mut self) -> u32 {
        let mut bytes = [0; 4];
        self.fill_bytes(&mut bytes);
        u32::from_le_bytes(bytes)
    }

    fn next_u64(&mut self) -> u64 {
        let mut bytes = [0; 8];
        self.fill_bytes(&mut bytes);
        u64::from_le_bytes(bytes)
    }

    fn try_fill_bytes(&mut self, dest: &mut [u8]) -> Result<(), rand::Error> {
        self.fill_bytes(dest);
        Ok(())
    }
}
