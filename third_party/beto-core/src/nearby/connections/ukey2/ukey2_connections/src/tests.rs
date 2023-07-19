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

use crypto_provider_rustcrypto::RustCryptoImpl;
use rand::SeedableRng;
use rand::{rngs::StdRng, CryptoRng, RngCore};
use rstest::rstest;

use crypto_provider::CryptoProvider;
use crypto_provider_openssl::Openssl;
use crypto_provider_rustcrypto::RustCrypto;
use ukey2_rs::HandshakeImplementation;

use crate::{
    crypto_utils::{decrypt, encrypt},
    java_utils, Aes256Key, D2DConnectionContextV1, D2DHandshakeContext, DeserializeError,
    InitiatorD2DHandshakeContext, ServerD2DHandshakeContext,
};

#[rstest]
fn crypto_test_encrypt_decrypt<C: CryptoProvider>(
    #[values(RustCrypto::new(), Openssl)] _crypto_provider: C,
) {
    let message = b"Hello World!";
    let key = b"42424242424242424242424242424242";
    let (ciphertext, iv) =
        encrypt::<_, C::AesCbcPkcs7Padded>(key, message, &mut rand::rngs::StdRng::from_entropy());
    let decrypt_result = decrypt::<C::AesCbcPkcs7Padded>(key, ciphertext.as_slice(), &iv);
    let ptext = decrypt_result.expect("Decrypt should be successful");
    assert_eq!(ptext, message.to_vec());
}

#[rstest]
fn crypto_test_encrypt_seeded<C: CryptoProvider>(
    #[values(RustCrypto::new(), Openssl)] _crypto_provider: C,
) {
    let message = b"Hello World!";
    let key = b"42424242424242424242424242424242";
    let mut rng = MockRng;
    let (ciphertext, iv) = encrypt::<_, C::AesCbcPkcs7Padded>(key, message, &mut rng);
    // Expected values extracted from the results of the current implementation.
    // This test makes sure that we don't accidentally change the encryption logic that
    // causes incompatibility between versions.
    assert_eq!(&iv, &[1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1]);
    assert_eq!(
        ciphertext,
        &[20, 59, 195, 101, 11, 208, 245, 128, 247, 196, 81, 80, 158, 77, 174, 61]
    );
}

#[rstest]
fn crypto_test_decrypt_seeded<C: CryptoProvider>(
    #[values(RustCrypto::new(), Openssl)] _crypto_provider: C,
) {
    let iv = [1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1];
    let ciphertext = [20, 59, 195, 101, 11, 208, 245, 128, 247, 196, 81, 80, 158, 77, 174, 61];
    let key = b"42424242424242424242424242424242";
    let plaintext = decrypt::<C::AesCbcPkcs7Padded>(key, &ciphertext, &iv).unwrap();
    assert_eq!(plaintext, b"Hello World!");
}

#[rstest]
fn decrypt_test_wrong_key<C: CryptoProvider>(
    #[values(RustCrypto::new(), Openssl)] _crypto_provider: C,
) {
    let message = b"Hello World!";
    let good_key = b"42424242424242424242424242424242";
    let (ciphertext, iv) = encrypt::<_, C::AesCbcPkcs7Padded>(
        good_key,
        message,
        &mut rand::rngs::StdRng::from_entropy(),
    );
    let bad_key = b"43434343434343434343434343434343";
    let decrypt_result = decrypt::<C::AesCbcPkcs7Padded>(bad_key, ciphertext.as_slice(), &iv);
    match decrypt_result {
        // The padding is valid, but the decrypted value should be bad since the keys don't match
        Ok(decrypted_bad) => assert_ne!(decrypted_bad, message),
        // The padding is bad, so it returns an error and is unable to decrypt
        Err(crypto_provider::aes::cbc::DecryptionError::BadPadding) => (),
    }
    let decrypt_result = decrypt::<C::AesCbcPkcs7Padded>(good_key, ciphertext.as_slice(), &iv);
    let ptext = decrypt_result.unwrap();
    assert_eq!(ptext, message.to_vec());
}

fn run_handshake<C: CryptoProvider>() -> (D2DConnectionContextV1, D2DConnectionContextV1) {
    run_handshake_with_rng::<C, _>(rand::rngs::StdRng::from_entropy())
}

fn run_handshake_with_rng<C, R>(
    mut rng: R,
) -> (D2DConnectionContextV1<R>, D2DConnectionContextV1<R>)
where
    C: CryptoProvider,
    R: rand::RngCore + rand::CryptoRng + rand::SeedableRng + Send,
{
    let mut initiator_ctx = InitiatorD2DHandshakeContext::<C, R>::new_impl(
        HandshakeImplementation::Spec,
        R::from_rng(&mut rng).unwrap(),
    );
    let mut server_ctx = ServerD2DHandshakeContext::<C, R>::new_impl(
        HandshakeImplementation::Spec,
        R::from_rng(&mut rng).unwrap(),
    );
    server_ctx
        .handle_handshake_message(
            initiator_ctx.get_next_handshake_message().expect("No message").as_slice(),
        )
        .expect("Failed to handle message");
    initiator_ctx
        .handle_handshake_message(
            server_ctx.get_next_handshake_message().expect("No message").as_slice(),
        )
        .expect("Failed to handle message");
    server_ctx
        .handle_handshake_message(
            initiator_ctx.get_next_handshake_message().expect("No message").as_slice(),
        )
        .expect("Failed to handle message");
    assert!(initiator_ctx.is_handshake_complete());
    assert!(server_ctx.is_handshake_complete());
    (initiator_ctx.to_connection_context().unwrap(), server_ctx.to_connection_context().unwrap())
}

#[rstest]
fn send_receive_message_seeded<C: CryptoProvider>(
    // TODO: Find a way to inject RNG / generated ephemeral secrets in openSSL and test them here
    #[values(RustCryptoImpl::< MockRng >::new())] _crypto_provider: C,
) {
    let rng = MockRng;
    let message = b"Hello World!";
    let (mut init_conn_ctx, mut server_conn_ctx) = run_handshake_with_rng::<C, _>(rng);
    let encoded = init_conn_ctx.encode_message_to_peer::<C, &[u8]>(message, None);
    // Expected values extracted from the results of the current implementation.
    // This test makes sure that we don't accidentally change the encryption logic that
    // causes incompatibility between versions.
    assert_eq!(
        encoded,
        &[
            10, 64, 10, 28, 8, 1, 16, 2, 42, 16, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
            50, 4, 8, 13, 16, 1, 18, 32, 58, 224, 12, 10, 216, 38, 219, 232, 231, 222, 226, 63, 37,
            20, 92, 208, 40, 8, 29, 98, 226, 132, 30, 61, 229, 78, 20, 182, 217, 26, 176, 77, 18,
            32, 212, 221, 67, 39, 137, 138, 163, 222, 119, 216, 28, 176, 130, 152, 211, 63, 182,
            45, 239, 234, 248, 148, 9, 150, 204, 117, 32, 216, 5, 126, 224, 39
        ]
    );
    let decoded = server_conn_ctx.decode_message_from_peer::<C, &[u8]>(&encoded, None).unwrap();
    assert_eq!(message, &decoded[..]);
}

#[rstest]
fn send_receive_message<C: CryptoProvider>(
    #[values(RustCrypto::new(), Openssl)] _crypto_provider: C,
) {
    let message = b"Hello World!";
    let (mut init_conn_ctx, mut server_conn_ctx) = run_handshake::<C>();
    let encoded = init_conn_ctx.encode_message_to_peer::<C, &[u8]>(message, None);
    let decoded = server_conn_ctx.decode_message_from_peer::<C, &[u8]>(encoded.as_slice(), None);
    assert_eq!(message.to_vec(), decoded.expect("Decode should be successful"));
}

#[rstest]
fn send_receive_message_associated_data<C: CryptoProvider>(
    #[values(RustCrypto::new(), Openssl)] _crypto_provider: C,
) {
    let message = b"Hello World!";
    let (mut init_conn_ctx, mut server_conn_ctx) = run_handshake::<C>();
    let encoded = init_conn_ctx.encode_message_to_peer::<C, _>(message, Some(b"associated data"));
    let decoded = server_conn_ctx
        .decode_message_from_peer::<C, _>(encoded.as_slice(), Some(b"associated data"));
    assert_eq!(message.to_vec(), decoded.expect("Decode should be successful"));
    // Make sure decode fails with missing associated data.
    let decoded = server_conn_ctx.decode_message_from_peer::<C, &[u8]>(encoded.as_slice(), None);
    assert!(decoded.is_err());
    // Make sure decode fails with different associated data.
    let decoded = server_conn_ctx
        .decode_message_from_peer::<C, _>(encoded.as_slice(), Some(b"assoc1ated data"));
    assert!(decoded.is_err());
}

#[rstest]
fn test_save_restore_session<C: CryptoProvider>(
    #[values(RustCrypto::new(), Openssl)] _crypto_provider: C,
) {
    let (init_conn_ctx, server_conn_ctx) = run_handshake::<C>();
    let init_session = init_conn_ctx.save_session();
    let server_session = server_conn_ctx.save_session();
    let mut init_restored_ctx =
        D2DConnectionContextV1::from_saved_session::<C>(init_session.as_slice())
            .expect("failed to restore client session");
    let mut server_restored_ctx =
        D2DConnectionContextV1::from_saved_session::<C>(server_session.as_slice())
            .expect("failed to restore server session");
    let message = b"Hello World!";
    let encoded = init_restored_ctx.encode_message_to_peer::<C, &[u8]>(message, None);
    let decoded =
        server_restored_ctx.decode_message_from_peer::<C, &[u8]>(encoded.as_slice(), None);
    assert_eq!(message.to_vec(), decoded.expect("Decode should be successful"));
}

#[rstest]
fn test_save_restore_bad_session<C: CryptoProvider>(
    #[values(RustCrypto::new(), Openssl)] _crypto_provider: C,
) {
    let (init_conn_ctx, server_conn_ctx) = run_handshake::<C>();
    let init_session = init_conn_ctx.save_session();
    let server_session = server_conn_ctx.save_session();
    let _ = D2DConnectionContextV1::from_saved_session::<C>(init_session.as_slice())
        .expect("failed to restore client session");
    let server_restored_ctx =
        D2DConnectionContextV1::from_saved_session::<C>(&server_session[0..60]);
    assert_eq!(server_restored_ctx.unwrap_err(), DeserializeError::BadDataLength);
}

#[rstest]
fn test_save_restore_bad_protocol_version<C: CryptoProvider>(
    #[values(RustCrypto::new(), Openssl)] _crypto_provider: C,
) {
    let (init_conn_ctx, server_conn_ctx) = run_handshake::<C>();
    let init_session = init_conn_ctx.save_session();
    let mut server_session = server_conn_ctx.save_session();
    let _ = D2DConnectionContextV1::from_saved_session::<C>(init_session.as_slice())
        .expect("failed to restore client session");
    server_session[0] = 0; // Change the protocol version to an invalid one (0)
    let server_restored_ctx = D2DConnectionContextV1::from_saved_session::<C>(&server_session);
    assert_eq!(server_restored_ctx.unwrap_err(), DeserializeError::BadProtocolVersion);
}

#[rstest]
fn test_unique_session<C: CryptoProvider>(
    #[values(RustCrypto::new(), Openssl)] _crypto_provider: C,
) {
    let (mut init_conn_ctx, mut server_conn_ctx) = run_handshake::<C>();
    let init_session = init_conn_ctx.get_session_unique::<C>();
    let server_session = server_conn_ctx.get_session_unique::<C>();
    let message = b"Hello World!";
    let encoded = init_conn_ctx.encode_message_to_peer::<C, &[u8]>(message, None);
    let decoded = server_conn_ctx.decode_message_from_peer::<C, &[u8]>(encoded.as_slice(), None);
    assert_eq!(message.to_vec(), decoded.expect("Decode should be successful"));
    let init_session_after = init_conn_ctx.get_session_unique::<C>();
    let server_session_after = server_conn_ctx.get_session_unique::<C>();
    let bad_server_ctx = D2DConnectionContextV1::new::<C>(
        server_conn_ctx.get_sequence_number_for_decoding(),
        server_conn_ctx.get_sequence_number_for_encoding(),
        Aes256Key::default(),
        Aes256Key::default(),
        StdRng::from_entropy(),
    );
    assert_eq!(init_session, init_session_after);
    assert_eq!(server_session, server_session_after);
    assert_eq!(init_session, server_session);
    assert_ne!(server_session, bad_server_ctx.get_session_unique::<C>());
}

#[test]
fn test_java_hashcode() {
    assert_eq!(java_utils::hash_code("4".as_bytes()), 83i32);
    assert_eq!(java_utils::hash_code(&[0x65, 0x47]), 4163i32);
    assert_eq!(java_utils::hash_code(&[0x12, 0x23, 0x34, 0x45, 0x56, 0x67, 0x78]), 1590192324i32);
    assert_eq!(
        java_utils::hash_code(&[0x12, 0x23, 0x34, 0x45, 0x56, 0x67, 0x78, 0xFF]),
        2051321787
    );
}

/// A mock RNG that always returns 1 at each byte. The output from this RNG is
/// not changed from call to call to avoid ordering changes in code from
/// changing the expected output. The downside is that code that keeps looping
/// and generating a new random number until it fits certain criteria will hang
/// indefinitely.
#[derive(Eq, PartialEq, Clone, Debug)]
struct MockRng;

impl SeedableRng for MockRng {
    type Seed = [u8; 0];

    fn from_seed(_seed: Self::Seed) -> Self {
        Self
    }
}

impl CryptoRng for MockRng {}

impl RngCore for MockRng {
    fn next_u32(&mut self) -> u32 {
        let mut buf = [0_u8; 4];
        self.fill_bytes(&mut buf);
        u32::from_le_bytes(buf)
    }

    fn next_u64(&mut self) -> u64 {
        let mut buf = [0_u8; 8];
        self.fill_bytes(&mut buf);
        u64::from_le_bytes(buf)
    }

    fn fill_bytes(&mut self, dest: &mut [u8]) {
        dest.fill(1);
    }

    fn try_fill_bytes(&mut self, dest: &mut [u8]) -> Result<(), rand::Error> {
        self.fill_bytes(dest);
        Ok(())
    }
}
