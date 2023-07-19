#![no_main]
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

use arbitrary::Arbitrary;
use crypto_provider_rustcrypto::RustCrypto;
use libfuzzer_sys::fuzz_target;
use rand_chacha::rand_core::SeedableRng;
use ukey2_connections::HandshakeImplementation;
use ukey2_connections::{
    D2DHandshakeContext, InitiatorD2DHandshakeContext, ServerD2DHandshakeContext,
};

#[derive(Debug, Arbitrary)]
struct FuzzInput<'a> {
    client_rng_seed: [u8; 32],
    server_rng_seed: [u8; 32],
    override_client_init: Option<&'a [u8]>,
    override_server_init: Option<&'a [u8]>,
    override_client_finish: Option<&'a [u8]>,
}

fuzz_target!(|input: FuzzInput| {
    let mut initiator_ctx = InitiatorD2DHandshakeContext::<RustCrypto, _>::new_impl(
        HandshakeImplementation::Spec,
        rand_chacha::ChaChaRng::from_seed(input.client_rng_seed),
    );
    let mut server_ctx = ServerD2DHandshakeContext::<RustCrypto, _>::new_impl(
        HandshakeImplementation::Spec,
        rand_chacha::ChaChaRng::from_seed(input.server_rng_seed),
    );
    let client_init = initiator_ctx
        .get_next_handshake_message()
        .expect("Initial get_next_handshake_message should succeed");
    let client_init_with_override = input.override_client_init.unwrap_or(&client_init);
    let _result = server_ctx
        .handle_handshake_message(client_init_with_override)
        .and_then(|_| {
            let server_init = server_ctx.get_next_handshake_message().expect(concat!(
                "get_next_handshake_message should succeed when previous ",
                "handle_handshake_message succeeded"
            ));
            let server_init_with_override = input.override_server_init.unwrap_or(&server_init);
            initiator_ctx.handle_handshake_message(server_init_with_override)
        })
        .and_then(|_| {
            let client_finish = initiator_ctx.get_next_handshake_message().expect(concat!(
                "get_next_handshake_message should succeed when previous ",
                "handle_handshake_message succeeded"
            ));
            let client_finish_with_override =
                input.override_client_finish.unwrap_or(&client_finish);
            server_ctx.handle_handshake_message(client_finish_with_override)
        })
        .map(|_| {
            assert!(server_ctx.is_handshake_complete());
            assert!(initiator_ctx.is_handshake_complete());
            if let Some(msg) = server_ctx.get_next_handshake_message() {
                panic!("No more server messages expected: {msg:?}");
            }
            // Note: initiator keeps returning client_finish at the Complete state
        });
});
