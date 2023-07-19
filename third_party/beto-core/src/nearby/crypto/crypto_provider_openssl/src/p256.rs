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

use crypto_provider::elliptic_curve::{EcdhProvider, EphemeralSecret};
use crypto_provider::p256::P256;
use openssl::bn::{BigNum, BigNumContext};
use openssl::derive::Deriver;
use openssl::ec::{EcGroup, EcKey, EcPoint, PointConversionForm};
use openssl::error::ErrorStack;
use openssl::nid::Nid;
use openssl::pkey::{PKey, Private, Public};

/// Public key type for P256 using OpenSSL's implementation.
#[derive(Debug)]
pub struct P256PublicKey(PKey<Public>);

impl PartialEq for P256PublicKey {
    fn eq(&self, other: &Self) -> bool {
        self.0.public_eq(&other.0)
    }
}

/// Custom error type for OpenSSL operations.
#[derive(Debug)]
#[allow(clippy::enum_variant_names)]
pub enum Error {
    /// Error from the openssl crate.
    OpenSslError(ErrorStack),
    /// Unexpected size for the given input.
    WrongSize,
    /// Invalid input given when creating keys from their byte representations.
    InvalidInput,
}

impl From<ErrorStack> for Error {
    fn from(value: ErrorStack) -> Self {
        Self::OpenSslError(value)
    }
}

/// The OpenSSL implementation of P256 public key.
impl crypto_provider::p256::P256PublicKey for P256PublicKey {
    type Error = Error;

    fn from_sec1_bytes(bytes: &[u8]) -> Result<Self, Self::Error> {
        if bytes == [0] {
            // Single 0 byte means infinity point.
            return Err(Error::InvalidInput);
        }
        let ecgroup = EcGroup::from_curve_name(Nid::X9_62_PRIME256V1)?;
        let mut bncontext = BigNumContext::new()?;
        let ecpoint = EcPoint::from_bytes(&ecgroup, bytes, &mut bncontext)?;
        let eckey = EcKey::from_public_key(&ecgroup, &ecpoint)?;
        Ok(Self(eckey.try_into()?))
    }

    fn to_sec1_bytes(&self) -> Vec<u8> {
        let ecgroup = EcGroup::from_curve_name(Nid::X9_62_PRIME256V1).unwrap();
        let mut bncontext = BigNumContext::new().unwrap();
        self.0
            .ec_key()
            .unwrap()
            .public_key()
            .to_bytes(&ecgroup, PointConversionForm::COMPRESSED, &mut bncontext)
            .unwrap()
    }

    fn from_affine_coordinates(x: &[u8; 32], y: &[u8; 32]) -> Result<Self, Self::Error> {
        let bn_x = BigNum::from_slice(x)?;
        let bn_y = BigNum::from_slice(y)?;
        let ecgroup = EcGroup::from_curve_name(Nid::X9_62_PRIME256V1)?;
        let eckey = EcKey::from_public_key_affine_coordinates(&ecgroup, &bn_x, &bn_y)?;
        Ok(Self(eckey.try_into()?))
    }

    fn to_affine_coordinates(&self) -> Result<([u8; 32], [u8; 32]), Self::Error> {
        let mut bnctx = openssl::bn::BigNumContext::new()?;
        let ecgroup = EcGroup::from_curve_name(Nid::X9_62_PRIME256V1)?;
        let p256_key = self.0.ec_key()?;
        let mut p256x = BigNum::new()?;
        let mut p256y = BigNum::new()?;
        p256_key
            .public_key()
            .affine_coordinates_gfp(&ecgroup, &mut p256x, &mut p256y, &mut bnctx)?;
        Ok((
            p256x
                .to_vec_padded(32)
                .map_err(|_| Error::WrongSize)?
                .try_into()
                .expect("to_vec_padded(32) should always return vec of length 32"),
            p256y
                .to_vec_padded(32)
                .map_err(|_| Error::WrongSize)?
                .try_into()
                .expect("to_vec_padded(32) should always return vec of length 32"),
        ))
    }
}

/// Private key type for P256 using OpenSSL's implementation.
pub struct P256EphemeralSecret(PKey<Private>);

impl EphemeralSecret<P256> for P256EphemeralSecret {
    type Impl = P256Ecdh;
    type Error = Error;
    type Rng = ();

    fn generate_random(_rng: &mut Self::Rng) -> Self {
        let ecgroup = EcGroup::from_curve_name(Nid::X9_62_PRIME256V1).unwrap();
        let eckey = EcKey::generate(&ecgroup).unwrap();
        Self(eckey.try_into().unwrap())
    }

    fn public_key_bytes(&self) -> Vec<u8> {
        let ecgroup = EcGroup::from_curve_name(Nid::X9_62_PRIME256V1).unwrap();
        let mut bncontext = BigNumContext::new().unwrap();
        self.0
            .ec_key()
            .unwrap()
            .public_key()
            .to_bytes(&ecgroup, PointConversionForm::COMPRESSED, &mut bncontext)
            .unwrap()
    }

    fn diffie_hellman(
        self,
        other_pub: &<Self::Impl as EcdhProvider<P256>>::PublicKey,
    ) -> Result<<Self::Impl as EcdhProvider<P256>>::SharedSecret, Self::Error> {
        let mut deriver = Deriver::new(&self.0)?;
        deriver.set_peer(&other_pub.0)?;
        let mut buf = [0_u8; 32];
        let bytes_written = deriver.derive(&mut buf)?;
        debug_assert_eq!(bytes_written, 32);
        Ok(buf)
    }
}

#[cfg(test)]
impl crypto_provider_test::elliptic_curve::EphemeralSecretForTesting<P256> for P256EphemeralSecret {
    fn from_private_components(
        private_bytes: &[u8; 32],
        public_key: &P256PublicKey,
    ) -> Result<Self, Self::Error> {
        let ecgroup = EcGroup::from_curve_name(Nid::X9_62_PRIME256V1).unwrap();
        let private_key_bn = BigNum::from_slice(private_bytes)?;
        let eckey = EcKey::from_private_components(
            &ecgroup,
            &private_key_bn,
            public_key.0.ec_key()?.public_key(),
        )?;
        Ok(Self(eckey.try_into()?))
    }
}

/// The OpenSSL implementation of P256 ECDH.
pub enum P256Ecdh {}
impl EcdhProvider<P256> for P256Ecdh {
    type PublicKey = P256PublicKey;
    type EphemeralSecret = P256EphemeralSecret;
    type SharedSecret = [u8; 32];
}

#[cfg(test)]
mod tests {
    use super::P256Ecdh;
    use core::marker::PhantomData;
    use crypto_provider_test::p256::*;

    #[apply(p256_test_cases)]
    fn p256_tests(testcase: CryptoProviderTestCase<P256Ecdh>) {
        testcase(PhantomData::<P256Ecdh>)
    }
}
