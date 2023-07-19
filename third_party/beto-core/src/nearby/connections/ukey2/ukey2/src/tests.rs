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

use crate::{
    proto_adapter::{IntoAdapter as _, MessageType, ToWrappedMessage as _},
    ukey2_handshake::HandshakeCipher,
    HandshakeImplementation, StateMachine, Ukey2ClientStage1, Ukey2ServerStage1,
};
use crypto_provider::elliptic_curve::{EcdhProvider, EphemeralSecret, PublicKey};
use crypto_provider::p256::P256;
use crypto_provider::x25519::X25519;
use crypto_provider::{CryptoProvider, CryptoRng};
use crypto_provider_openssl::Openssl;
use crypto_provider_rustcrypto::RustCrypto;
use rand::rngs::StdRng;
use rand::{Rng, SeedableRng};
use rstest::rstest;
use sha2::Digest;
use std::collections::hash_set;
use ukey2_proto::protobuf::Message;
use ukey2_proto::ukey2_all_proto::ukey;

#[rstest]
fn advance_from_init_to_finish_client_test<C: CryptoProvider>(
    #[values(RustCrypto::new(), Openssl)] _crypto_provider: C,
) {
    let mut rng = StdRng::from_entropy();
    let client1 = Ukey2ClientStage1::<C>::from(
        &mut rng,
        "next protocol".to_string(),
        HandshakeImplementation::Spec,
    );

    let secret =
        <C::X25519 as EcdhProvider<X25519>>::EphemeralSecret::generate_random(
            &mut <<C::X25519 as EcdhProvider<X25519>>::EphemeralSecret as EphemeralSecret<
                X25519,
            >>::Rng::new(),
        );
    let public_key =
        <C::X25519 as EcdhProvider<X25519>>::PublicKey::from_bytes(&secret.public_key_bytes())
            .unwrap();
    let random: [u8; 32] = rng.gen();
    let message_data: ukey::Ukey2ServerInit = ukey::Ukey2ServerInit {
        version: Some(1),
        random: Some(random.to_vec()),
        handshake_cipher: Some(ukey::Ukey2HandshakeCipher::CURVE25519_SHA512.into()),
        public_key: Some(public_key.to_bytes()),
        ..Default::default()
    };

    let _client = client1
        .advance_state(&mut rng, &message_data.to_wrapped_msg().write_to_bytes().unwrap())
        .unwrap();
    // TODO assertions on client state
}

#[rstest]
fn advance_from_init_to_complete_server_x25519_test<C: CryptoProvider>(
    #[values(RustCrypto::new(), Openssl)] _crypto_provider: C,
) {
    let mut rng = StdRng::from_entropy();
    let mut next_protocols = hash_set::HashSet::new();
    let _ = next_protocols.insert("AES_256_CBC-HMAC_SHA256".to_string());
    let server1 = Ukey2ServerStage1::<C>::from(next_protocols, HandshakeImplementation::Spec);
    // We construct a ClientInit message for the server to get it into the state to handle
    // ClientFinish messages.
    let secret =
        <C::X25519 as EcdhProvider<X25519>>::EphemeralSecret::generate_random(
            &mut <<C::X25519 as EcdhProvider<X25519>>::EphemeralSecret as EphemeralSecret<
                X25519,
            >>::Rng::new(),
        );
    let client_finished_msg = {
        let mut msg = ukey::Ukey2ClientFinished::default();
        msg.set_public_key(secret.public_key_bytes());
        msg.to_wrapped_msg()
    };
    let client_finished_bytes = client_finished_msg.write_to_bytes().unwrap();
    let mut hasher = sha2::Sha512::new();
    hasher.update(&client_finished_bytes);
    let client_finished_hash = hasher.finalize().to_vec();
    let cipher = HandshakeCipher::Curve25519Sha512;
    let client_random = rng.gen::<[u8; 32]>();
    let client_init_framed = {
        let commitment = ukey::ukey2client_init::CipherCommitment {
            handshake_cipher: Some(cipher.as_proto().into()),
            commitment: Some(client_finished_hash),
            ..Default::default()
        };
        let client_init = ukey::Ukey2ClientInit {
            version: Some(1),
            random: Some(client_random.to_vec()),
            cipher_commitments: vec![commitment],
            next_protocol: Some("AES_256_CBC-HMAC_SHA256".to_string()),
            ..Default::default()
        };
        client_init.to_wrapped_msg()
    };
    let server2 =
        server1.advance_state(&mut rng, &client_init_framed.write_to_bytes().unwrap()).unwrap();
    assert!(
        !server2.server_init_msg().windows(client_random.len()).any(|w| w == client_random),
        "Server init msg should not contain the client's random"
    );
    // TODO assertions on server2 state
    // We now move the server to the post-ClientFinished state
    let _server = server2.advance_state(&mut rng, &client_finished_bytes).unwrap();
    // TODO assertions on server state
}

#[rstest]
fn advance_from_init_to_complete_server_p256_test<C: CryptoProvider>(
    #[values(RustCrypto::new(), Openssl)] _crypto_provider: C,
) {
    let mut rng = StdRng::from_entropy();
    let mut next_protocols = hash_set::HashSet::new();
    let _ = next_protocols.insert("AES_256_CBC-HMAC_SHA256".to_string());
    let server1 = Ukey2ServerStage1::<C>::from(next_protocols, HandshakeImplementation::Spec);
    // We construct a ClientInit message for the server to get it into the state to handle
    // ClientFinish messages.
    let secret = <C::P256 as EcdhProvider<P256>>::EphemeralSecret::generate_random(
        &mut <<C::P256 as EcdhProvider<P256>>::EphemeralSecret as EphemeralSecret<P256>>::Rng::new(
        ),
    );
    let client_finished_msg = {
        let mut msg = ukey::Ukey2ClientFinished::default();
        msg.set_public_key(secret.public_key_bytes());
        msg.to_wrapped_msg()
    };
    let client_finished_bytes = client_finished_msg.write_to_bytes().unwrap();
    let mut hasher = sha2::Sha512::new();
    hasher.update(&client_finished_bytes);
    let client_finished_hash = hasher.finalize().to_vec();
    let cipher = HandshakeCipher::P256Sha512;
    let client_init_framed = {
        let commitment = ukey::ukey2client_init::CipherCommitment {
            handshake_cipher: Some(cipher.as_proto().into()),
            commitment: Some(client_finished_hash),
            ..Default::default()
        };
        ukey::Ukey2ClientInit {
            version: Some(1),
            random: Some(rng.gen::<[u8; 32]>().to_vec()),
            cipher_commitments: vec![commitment],
            next_protocol: Some("AES_256_CBC-HMAC_SHA256".to_string()),
            ..Default::default()
        }
        .to_wrapped_msg()
    };
    let server2 =
        server1.advance_state(&mut rng, &client_init_framed.write_to_bytes().unwrap()).unwrap();
    // TODO assertions on server2 state
    let _server =
        server2.advance_state(&mut rng, &client_finished_msg.write_to_bytes().unwrap()).unwrap();
    // TODO assertions on server state
}

#[test]
fn message_type_discriminant() {
    assert_eq!(1, ukey::ukey2message::Type::ALERT as i32);
    assert_eq!(2, ukey::ukey2message::Type::CLIENT_INIT as i32);
    assert_eq!(3, ukey::ukey2message::Type::SERVER_INIT as i32);
    assert_eq!(4, ukey::ukey2message::Type::CLIENT_FINISH as i32);
}

#[test]
fn cipher_type_discriminant() {
    assert_eq!(100, ukey::Ukey2HandshakeCipher::P256_SHA512 as i32);
    assert_eq!(200, ukey::Ukey2HandshakeCipher::CURVE25519_SHA512 as i32);
}

#[test]
fn convert_to_message_type() {
    assert_eq!(
        MessageType::ClientInit,
        ukey::ukey2message::Type::CLIENT_INIT.into_adapter().unwrap()
    );
    assert_eq!(
        MessageType::ServerInit,
        ukey::ukey2message::Type::SERVER_INIT.into_adapter().unwrap()
    );
    assert_eq!(
        MessageType::ClientFinish,
        ukey::ukey2message::Type::CLIENT_FINISH.into_adapter().unwrap()
    );
}

#[test]
fn convert_to_cipher_type() {
    assert_eq!(HandshakeCipher::P256Sha512, 100.into_adapter().unwrap());
    assert_eq!(HandshakeCipher::Curve25519Sha512, 200.into_adapter().unwrap());
}
