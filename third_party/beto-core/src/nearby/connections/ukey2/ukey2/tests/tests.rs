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

use crypto_provider_rustcrypto::RustCrypto;
use rand::{rngs::StdRng, SeedableRng};
use std::collections::hash_set;
use ukey2_rs::*;

#[test]
fn full_integration_state_machine() {
    let mut next_protocols = hash_set::HashSet::new();
    let next_protocol = "AES_256_CBC-HMAC_SHA256".to_string();
    let _ = next_protocols.insert(next_protocol.clone());
    let server1 =
        Ukey2ServerStage1::<RustCrypto>::from(next_protocols, HandshakeImplementation::Spec);
    let mut rng = StdRng::from_entropy();
    let client1 = Ukey2ClientStage1::<RustCrypto>::from(
        &mut rng,
        next_protocol,
        HandshakeImplementation::Spec,
    );
    let server2 = server1.advance_state(&mut rng, client1.client_init_msg()).unwrap();

    let client2 = client1.advance_state(&mut rng, server2.server_init_msg()).unwrap();

    let server3 = server2.advance_state(&mut rng, client2.client_finished_msg()).unwrap();

    assert_eq!(
        server3.completed_handshake().auth_string::<RustCrypto>().derive_array::<32>(),
        client2.completed_handshake().auth_string::<RustCrypto>().derive_array::<32>()
    );
    assert_eq!(
        server3.completed_handshake().next_protocol_secret::<RustCrypto>().derive_array::<32>(),
        client2.completed_handshake().next_protocol_secret::<RustCrypto>().derive_array::<32>()
    );
}

#[test]
fn full_integration_state_machine_public_key_in_protobuf() {
    let mut next_protocols = hash_set::HashSet::new();
    let next_protocol = "AES_256_CBC-HMAC_SHA256".to_string();
    let _ = next_protocols.insert(next_protocol.clone());
    let server1 = Ukey2ServerStage1::<RustCrypto>::from(
        next_protocols,
        HandshakeImplementation::PublicKeyInProtobuf,
    );
    let mut rng = StdRng::from_entropy();
    let client1 = Ukey2ClientStage1::<RustCrypto>::from(
        &mut rng,
        next_protocol,
        HandshakeImplementation::PublicKeyInProtobuf,
    );
    let server2 = server1.advance_state(&mut rng, client1.client_init_msg()).unwrap();

    let client2 = client1.advance_state(&mut rng, server2.server_init_msg()).unwrap();

    let server3 = server2.advance_state(&mut rng, client2.client_finished_msg()).unwrap();

    assert_eq!(
        server3.completed_handshake().auth_string::<RustCrypto>().derive_array::<32>(),
        client2.completed_handshake().auth_string::<RustCrypto>().derive_array::<32>()
    );
    assert_eq!(
        server3.completed_handshake().next_protocol_secret::<RustCrypto>().derive_array::<32>(),
        client2.completed_handshake().next_protocol_secret::<RustCrypto>().derive_array::<32>()
    );
}
