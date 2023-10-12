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

use bssl_crypto::aes::{AesDecryptKey, AesEncryptKey};
use crypto_provider::aes::{
    Aes, Aes128Key, Aes256Key, AesBlock, AesCipher, AesDecryptCipher, AesEncryptCipher, AesKey,
};

/// BoringSSL AES-128 operations
pub struct Aes128;
impl Aes for Aes128 {
    type Key = Aes128Key;
    type EncryptCipher = Aes128EncryptCipher;
    type DecryptCipher = Aes128DecryptCipher;
}

/// BoringSSL AES-256 operations
pub struct Aes256;
impl Aes for Aes256 {
    type Key = Aes256Key;
    type EncryptCipher = Aes256EncryptCipher;
    type DecryptCipher = Aes256DecryptCipher;
}

/// A BoringSSL backed AES-128 Encryption Cipher
pub struct Aes128EncryptCipher(AesEncryptKey);

/// A BoringSSL backed AES-128 Decryption Cipher
pub struct Aes128DecryptCipher(AesDecryptKey);

/// A BoringSSL backed AES-256 Encryption Cipher
pub struct Aes256EncryptCipher(AesEncryptKey);

/// A BoringSSL backed AES-256 Decryption Cipher
pub struct Aes256DecryptCipher(AesDecryptKey);

impl AesCipher for Aes128EncryptCipher {
    type Key = Aes128Key;

    fn new(key: &Self::Key) -> Self {
        Self(bssl_crypto::aes::AesEncryptKey::new_aes_128(*key.as_array()))
    }
}

impl AesEncryptCipher for Aes128EncryptCipher {
    fn encrypt(&self, block: &mut AesBlock) {
        bssl_crypto::aes::Aes::encrypt(&self.0, block)
    }
}

impl AesCipher for Aes128DecryptCipher {
    type Key = Aes128Key;

    fn new(key: &Self::Key) -> Self {
        Self(bssl_crypto::aes::AesDecryptKey::new_aes_128(*key.as_array()))
    }
}

impl AesDecryptCipher for Aes128DecryptCipher {
    fn decrypt(&self, block: &mut AesBlock) {
        bssl_crypto::aes::Aes::decrypt(&self.0, block)
    }
}

impl AesCipher for Aes256EncryptCipher {
    type Key = Aes256Key;

    fn new(key: &Self::Key) -> Self {
        Self(bssl_crypto::aes::AesEncryptKey::new_aes_256(*key.as_array()))
    }
}

impl AesEncryptCipher for Aes256EncryptCipher {
    fn encrypt(&self, block: &mut AesBlock) {
        bssl_crypto::aes::Aes::encrypt(&self.0, block)
    }
}

impl AesCipher for Aes256DecryptCipher {
    type Key = Aes256Key;

    fn new(key: &Self::Key) -> Self {
        Self(bssl_crypto::aes::AesDecryptKey::new_aes_256(*key.as_array()))
    }
}

impl AesDecryptCipher for Aes256DecryptCipher {
    fn decrypt(&self, block: &mut AesBlock) {
        bssl_crypto::aes::Aes::decrypt(&self.0, block)
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use core::marker::PhantomData;
    use crypto_provider_test::aes::*;

    #[apply(aes_128_encrypt_test_cases)]
    fn aes_128_encrypt_test(testcase: CryptoProviderTestCase<Aes128EncryptCipher>) {
        testcase(PhantomData);
    }

    #[apply(aes_128_decrypt_test_cases)]
    fn aes_128_decrypt_test(testcase: CryptoProviderTestCase<Aes128DecryptCipher>) {
        testcase(PhantomData);
    }

    #[apply(aes_256_encrypt_test_cases)]
    fn aes_256_encrypt_test(testcase: CryptoProviderTestCase<Aes256EncryptCipher>) {
        testcase(PhantomData);
    }

    #[apply(aes_256_decrypt_test_cases)]
    fn aes_256_decrypt_test(testcase: CryptoProviderTestCase<Aes256DecryptCipher>) {
        testcase(PhantomData);
    }
}
