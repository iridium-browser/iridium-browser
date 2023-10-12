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
use crypto_provider_rustcrypto::RustCrypto;
use ctr::cipher::{KeyIvInit as _, StreamCipher as _, StreamCipherSeek as _};
use ldt::{
    DefaultPadder, LdtDecryptCipher, LdtEncryptCipher, LdtKey, Mix, Padder, Swap, XorPadder,
};
use ldt_tbc::TweakableBlockCipher;
use sha2::Digest as _;
use std::marker;
use subtle::ConstantTimeEq as _;
use xts_aes::{XtsAes128, XtsAes256};

pub fn ldt_scan(c: &mut Criterion) {
    for &num_keys in &[1_usize, 10, 1000] {
        c.bench_function(&format!("LDT-XTS-AES-128/SHA-256/{num_keys} keys"), |b| {
            let mut state = build_bench_state::<_, sha2::Sha256>(
                ldt_factory::<16, XtsAes128<RustCrypto>, Swap, DefaultPadder>(),
                num_keys,
                24,
            );
            b.iter(|| black_box(state.scan()));
        });
        c.bench_function(&format!("LDT-XTS-AES-128/SHA-256/XOR pad/{num_keys} keys"), |b| {
            let mut state = build_bench_state::<_, sha2::Sha256>(
                ldt_factory::<
                    16,
                    XtsAes128<RustCrypto>,
                    Swap,
                    XorPadder<{ crypto_provider::aes::BLOCK_SIZE }>,
                >(),
                num_keys,
                24,
            );
            b.iter(|| black_box(state.scan()));
        });
        c.bench_function(&format!("LDT-XTS-AES-256/SHA-256/{num_keys} keys",), |b| {
            let mut state = build_bench_state::<_, sha2::Sha256>(
                ldt_factory::<16, XtsAes256<RustCrypto>, Swap, DefaultPadder>(),
                num_keys,
                24,
            );
            b.iter(|| black_box(state.scan()));
        });
        c.bench_function(&format!("AES-CTR-128/SHA-256/{num_keys} keys",), |b| {
            let mut state = build_bench_state::<_, sha2::Sha256>(AesCtrFactory {}, num_keys, 24);
            b.iter(|| black_box(state.scan()));
        });
        c.bench_function(&format!("LDT-XTS-AES-128/BLAKE2b-512/{num_keys} keys",), |b| {
            let mut state = build_bench_state::<_, blake2::Blake2b512>(
                ldt_factory::<16, XtsAes128<RustCrypto>, Swap, DefaultPadder>(),
                num_keys,
                24,
            );
            b.iter(|| black_box(state.scan()));
        });
        c.bench_function(&format!("LDT-XTS-AES-128/BLAKE2s-256/{num_keys} keys",), |b| {
            let mut state = build_bench_state::<_, blake2::Blake2s256>(
                ldt_factory::<16, XtsAes128<RustCrypto>, Swap, DefaultPadder>(),
                num_keys,
                24,
            );
            b.iter(|| black_box(state.scan()));
        });
    }
}

criterion_group!(benches, ldt_scan);
criterion_main!(benches);

struct LdtBenchState<C: ScanCipher, D: ScanDigest> {
    scenarios: Vec<ScanScenario<C, D>>,
    unfindable_ciphertext: Vec<u8>,
    decrypt_buf: Vec<u8>,
}

/// How much of the plaintext should be hashed for subsequent matching
const MATCH_LEN: usize = 16;

impl<C: ScanCipher, D: ScanDigest> LdtBenchState<C, D> {
    fn scan(&mut self) -> bool {
        let ciphertext = &self.unfindable_ciphertext;

        let mut hasher = D::new();
        let mut hash_output = D::new_output();

        self.scenarios.iter_mut().any(|scenario| {
            hasher.reset();
            self.decrypt_buf.clear();
            self.decrypt_buf.extend_from_slice(ciphertext);
            scenario.cipher.decrypt(&mut self.decrypt_buf[..]);
            // see if we decrypted to plaintext associated with this ldt / key
            hasher.update(&self.decrypt_buf[..MATCH_LEN]);
            hasher.finalize_and_reset(&mut hash_output);

            D::constant_time_compare(&scenario.plaintext_prefix_hash, &hash_output)
        })
    }
}

fn build_bench_state<F: ScanCipherFactory, D: ScanDigest>(
    factory: F,
    keys: usize,
    plaintext_len: usize,
) -> LdtBenchState<F::Cipher, D> {
    let mut rng = <RustCrypto as CryptoProvider>::CryptoRng::new();

    let scenarios = (0..keys)
        .map(|_| random_ldt_scenario::<RustCrypto, _, D>(&factory, &mut rng, plaintext_len))
        .collect::<Vec<_>>();

    LdtBenchState {
        scenarios,
        unfindable_ciphertext: random_vec::<RustCrypto>(&mut rng, plaintext_len),
        decrypt_buf: Vec::with_capacity(plaintext_len),
    }
}

struct ScanScenario<C: ScanCipher, D: ScanDigest> {
    cipher: C,
    plaintext_prefix_hash: D::Output,
}

fn random_ldt_scenario<C: CryptoProvider, F: ScanCipherFactory, D: ScanDigest>(
    factory: &F,
    rng: &mut C::CryptoRng,
    plaintext_len: usize,
) -> ScanScenario<F::Cipher, D> {
    let cipher = factory.build_cipher::<C>(rng);
    let plaintext = random_vec::<C>(rng, plaintext_len);
    let mut hasher = D::new();
    let mut plaintext_prefix_hash = D::new_output();
    hasher.update(&plaintext[..MATCH_LEN]);
    hasher.finalize_and_reset(&mut plaintext_prefix_hash);

    ScanScenario { cipher, plaintext_prefix_hash }
}

fn random_vec<C: CryptoProvider>(rng: &mut C::CryptoRng, len: usize) -> Vec<u8> {
    let mut bytes = Vec::<u8>::new();
    bytes.extend((0..len).map(|_| rng.gen::<u8>()));
    bytes
}

trait ScanCipher {
    fn encrypt(&mut self, buf: &mut [u8]);
    fn decrypt(&mut self, buf: &mut [u8]);
}

trait ScanCipherFactory {
    type Cipher: ScanCipher;

    fn build_cipher<C: CryptoProvider>(&self, key_rng: &mut C::CryptoRng) -> Self::Cipher;
}

/// A wrapper that lets us avoid percolating the need to specify a bogus and type-confused padder
/// for ciphers that don't use one.
struct LdtScanCipher<const B: usize, T: TweakableBlockCipher<B>, M: Mix, P: Padder<B, T>> {
    ldt_enc: LdtEncryptCipher<B, T, M>,
    ldt_dec: LdtDecryptCipher<B, T, M>,
    padder: P,
}

impl<const B: usize, T: TweakableBlockCipher<B>, M: Mix, P: Padder<B, T>> ScanCipher
    for LdtScanCipher<B, T, M, P>
{
    fn encrypt(&mut self, buf: &mut [u8]) {
        self.ldt_enc.encrypt(buf, &self.padder).unwrap();
    }

    fn decrypt(&mut self, buf: &mut [u8]) {
        self.ldt_dec.decrypt(buf, &self.padder).unwrap();
    }
}

fn ldt_factory<
    const B: usize,
    T: TweakableBlockCipher<B>,
    M: Mix,
    P: Padder<B, T> + RandomPadder,
>() -> LdtXtsAesFactory<B, T, M, P> {
    LdtXtsAesFactory {
        padder_phantom: marker::PhantomData,
        key_phantom: marker::PhantomData,
        mix_phantom: marker::PhantomData,
    }
}

struct LdtXtsAesFactory<
    const B: usize,
    T: TweakableBlockCipher<B>,
    M: Mix,
    P: Padder<B, T> + RandomPadder,
> {
    padder_phantom: marker::PhantomData<P>,
    key_phantom: marker::PhantomData<T>,
    mix_phantom: marker::PhantomData<M>,
}

impl<const B: usize, T, P, M> ScanCipherFactory for LdtXtsAesFactory<B, T, M, P>
where
    T: TweakableBlockCipher<B>,
    P: Padder<B, T> + RandomPadder,
    M: Mix,
{
    type Cipher = LdtScanCipher<B, T, M, P>;

    fn build_cipher<C: CryptoProvider>(&self, key_rng: &mut C::CryptoRng) -> Self::Cipher {
        let key: LdtKey<T::Key> = LdtKey::from_random::<C>(key_rng);
        LdtScanCipher {
            ldt_enc: LdtEncryptCipher::new(&key),
            ldt_dec: LdtDecryptCipher::new(&key),
            padder: P::generate::<C>(key_rng),
        }
    }
}

/// A helper trait for making padders from an RNG
trait RandomPadder {
    fn generate<C: CryptoProvider>(rng: &mut C::CryptoRng) -> Self;
}

impl RandomPadder for DefaultPadder {
    fn generate<C: CryptoProvider>(_rng: &mut C::CryptoRng) -> Self {
        Self
    }
}

impl<const T: usize> RandomPadder for XorPadder<T> {
    fn generate<C: CryptoProvider>(rng: &mut C::CryptoRng) -> Self {
        let mut salt = [0_u8; T];
        rng.fill(&mut salt[..]);
        salt.into()
    }
}

type Aes128Ctr64LE = ctr::Ctr64LE<aes::Aes128>;

impl ScanCipher for Aes128Ctr64LE {
    fn encrypt(&mut self, buf: &mut [u8]) {
        self.seek(0);
        self.apply_keystream(buf)
    }

    fn decrypt(&mut self, buf: &mut [u8]) {
        self.seek(0);
        self.apply_keystream(buf)
    }
}

struct AesCtrFactory {}

impl ScanCipherFactory for AesCtrFactory {
    type Cipher = Aes128Ctr64LE;

    fn build_cipher<C: CryptoProvider>(&self, key_rng: &mut C::CryptoRng) -> Self::Cipher {
        let mut key = [0_u8; 16];
        key_rng.fill(&mut key);

        let iv = [0_u8; 16];

        Aes128Ctr64LE::new(&key.into(), &iv.into())
    }
}

trait ScanDigest {
    type Output;

    fn new() -> Self;

    fn reset(&mut self);

    fn new_output() -> Self::Output;

    fn update(&mut self, data: &[u8]);

    fn finalize_and_reset(&mut self, out: &mut Self::Output);

    fn constant_time_compare(a: &Self::Output, b: &Self::Output) -> bool;
}

impl ScanDigest for sha2::Sha256 {
    type Output = sha2::digest::generic_array::GenericArray<u8, sha2::digest::consts::U32>;

    fn new() -> Self {
        <Self as sha2::digest::Digest>::new()
    }

    fn reset(&mut self) {
        <Self as sha2::digest::Digest>::reset(self)
    }

    fn new_output() -> Self::Output {
        Self::Output::default()
    }

    fn update(&mut self, data: &[u8]) {
        <Self as sha2::digest::Digest>::update(self, data);
    }

    fn finalize_and_reset(&mut self, out: &mut Self::Output) {
        self.finalize_into_reset(out);
    }

    fn constant_time_compare(a: &Self::Output, b: &Self::Output) -> bool {
        a.ct_eq(b).into()
    }
}

impl ScanDigest for blake2::Blake2b512 {
    type Output = blake2::digest::generic_array::GenericArray<u8, blake2::digest::consts::U64>;

    fn new() -> Self {
        <Self as blake2::digest::Digest>::new()
    }

    fn reset(&mut self) {
        <Self as blake2::digest::Digest>::reset(self)
    }

    fn new_output() -> Self::Output {
        Self::Output::default()
    }

    fn update(&mut self, data: &[u8]) {
        <Self as blake2::digest::Digest>::update(self, data)
    }

    fn finalize_and_reset(&mut self, out: &mut Self::Output) {
        self.finalize_into_reset(out)
    }

    fn constant_time_compare(a: &Self::Output, b: &Self::Output) -> bool {
        a.ct_eq(b).into()
    }
}

impl ScanDigest for blake2::Blake2s256 {
    type Output = blake2::digest::generic_array::GenericArray<u8, blake2::digest::consts::U32>;

    fn new() -> Self {
        <Self as blake2::digest::Digest>::new()
    }

    fn reset(&mut self) {
        <Self as blake2::digest::Digest>::reset(self)
    }

    fn new_output() -> Self::Output {
        Self::Output::default()
    }

    fn update(&mut self, data: &[u8]) {
        <Self as blake2::digest::Digest>::update(self, data)
    }

    fn finalize_and_reset(&mut self, out: &mut Self::Output) {
        self.finalize_into_reset(out)
    }

    fn constant_time_compare(a: &Self::Output, b: &Self::Output) -> bool {
        a.ct_eq(b).into()
    }
}
