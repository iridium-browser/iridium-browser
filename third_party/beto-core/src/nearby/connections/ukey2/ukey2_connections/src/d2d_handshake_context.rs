#![allow(missing_docs)]
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

use crate::d2d_connection_context_v1::D2DConnectionContextV1;
use crypto_provider::CryptoProvider;
use rand::{rngs::StdRng, SeedableRng as _};
use std::{collections::HashSet, mem};
use ukey2_rs::{
    CompletedHandshake, HandshakeImplementation, StateMachine, Ukey2Client, Ukey2ClientStage1,
    Ukey2Server, Ukey2ServerStage1, Ukey2ServerStage2,
};

#[derive(Debug)]
pub enum HandshakeError {
    HandshakeNotComplete,
}

#[derive(Debug)]
pub enum HandleMessageError {
    /// The supplied message was not applicable for the current state
    InvalidState,
    /// Handling the message produced an error that should be sent to the other party
    ErrorMessage(Vec<u8>),
    /// Bad message
    BadMessage,
}

/// Implements UKEY2 and produces a [`D2DConnectionContextV1`].
/// This class should be kept compatible with the Java and C++ implementations in
/// <https://github.com/google/ukey2>.
///
/// For usage examples, see `ukey2_shell`. This file contains a shell exercising
/// both the initiator and responder handshake roles.
pub trait D2DHandshakeContext<R = rand::rngs::StdRng>: Send
where
    R: rand::RngCore + rand::CryptoRng + rand::SeedableRng + Send,
{
    /// Tells the caller whether the handshake has completed or not. If the handshake is complete,
    /// the caller may call [`to_connection_context`][Self::to_connection_context] to obtain a
    /// connection context.
    ///
    /// Returns true if the handshake is complete, false otherwise.
    fn is_handshake_complete(&self) -> bool;

    /// Constructs the next message that should be sent in the handshake.
    ///
    /// Returns the next message or `None` if the handshake is over.
    fn get_next_handshake_message(&self) -> Option<Vec<u8>>;

    /// Parses a handshake message and advances the internal state of the context.
    ///
    /// * `handshakeMessage` - message received from the remote end in the handshake
    fn handle_handshake_message(&mut self, message: &[u8]) -> Result<(), HandleMessageError>;

    /// Creates a [`D2DConnectionContextV1`] using the results of the handshake. May only be called
    /// if [`is_handshake_complete`][Self::is_handshake_complete] returns true. Before trusting the
    /// connection, callers should check that `to_completed_handshake().auth_string()` matches on
    /// the client and server sides first. See the documentation for
    /// [`to_completed_handshake`][Self::to_completed_handshake].
    fn to_connection_context(&mut self) -> Result<D2DConnectionContextV1<R>, HandshakeError>;

    /// Returns the [`CompletedHandshake`] using the results from this handshake context. May only
    /// be called if [`is_handshake_complete`][Self::is_handshake_complete] returns true.
    /// Callers should verify that the authentication strings from
    /// `to_completed_handshake().auth_string()` matches on the server and client sides before
    /// trying to create a connection context. This authentication string verification needs to be
    /// done out-of-band, either by displaying the string to the user, or verified by some other
    /// secure means.
    fn to_completed_handshake(&self) -> Result<&CompletedHandshake, HandshakeError>;
}

enum InitiatorState<C: CryptoProvider> {
    Stage1(Ukey2ClientStage1<C>),
    Complete(Ukey2Client),
    /// If the initiator enters into an invalid state, e.g. by receiving invalid input.
    /// Also a momentary placeholder while swapping out states.
    Invalid,
}

/// Implementation of [`D2DHandshakeContext`] for the initiator (a.k.a the client).
pub struct InitiatorD2DHandshakeContext<C: CryptoProvider, R = rand::rngs::StdRng>
where
    R: rand::RngCore + rand::CryptoRng + rand::SeedableRng + Send,
{
    state: InitiatorState<C>,
    rng: R,
}

impl<C: CryptoProvider> InitiatorD2DHandshakeContext<C, rand::rngs::StdRng> {
    pub fn new(handshake_impl: HandshakeImplementation) -> Self {
        Self::new_impl(handshake_impl, rand::rngs::StdRng::from_entropy())
    }
}

impl<C: CryptoProvider, R> InitiatorD2DHandshakeContext<C, R>
where
    R: rand::RngCore + rand::CryptoRng + rand::SeedableRng + Send,
{
    // Used for testing / fuzzing only.
    #[doc(hidden)]
    pub fn new_impl(handshake_impl: HandshakeImplementation, mut rng: R) -> Self {
        let client = Ukey2ClientStage1::from(
            &mut rng,
            D2DConnectionContextV1::<StdRng>::NEXT_PROTOCOL_IDENTIFIER.to_owned(),
            handshake_impl,
        );
        Self { state: InitiatorState::Stage1(client), rng }
    }
}

impl<C: CryptoProvider, R> D2DHandshakeContext<R> for InitiatorD2DHandshakeContext<C, R>
where
    R: rand::RngCore + rand::CryptoRng + rand::SeedableRng + Send,
{
    fn is_handshake_complete(&self) -> bool {
        match self.state {
            InitiatorState::Stage1(_) => false,
            InitiatorState::Complete(_) => true,
            InitiatorState::Invalid => false,
        }
    }

    fn get_next_handshake_message(&self) -> Option<Vec<u8>> {
        let next_msg = match &self.state {
            InitiatorState::Stage1(c) => Some(c.client_init_msg().to_vec()),
            InitiatorState::Complete(c) => Some(c.client_finished_msg().to_vec()),
            InitiatorState::Invalid => None,
        }?;
        Some(next_msg)
    }

    fn handle_handshake_message(&mut self, message: &[u8]) -> Result<(), HandleMessageError> {
        match mem::replace(&mut self.state, InitiatorState::Invalid) {
            InitiatorState::Stage1(c) => {
                let client = c
                    .advance_state(&mut self.rng, message)
                    .map_err(|a| HandleMessageError::ErrorMessage(a.into_wrapped_alert_msg()))?;
                self.state = InitiatorState::Complete(client);
                Ok(())
            }
            InitiatorState::Complete(_) | InitiatorState::Invalid => {
                // already in invalid state, so leave it as is
                Err(HandleMessageError::InvalidState)
            }
        }
    }

    fn to_completed_handshake(&self) -> Result<&CompletedHandshake, HandshakeError> {
        match &self.state {
            InitiatorState::Stage1(_) | InitiatorState::Invalid => {
                Err(HandshakeError::HandshakeNotComplete)
            }
            InitiatorState::Complete(c) => Ok(c.completed_handshake()),
        }
    }

    fn to_connection_context(&mut self) -> Result<D2DConnectionContextV1<R>, HandshakeError> {
        // Since self.rng is expected to be a seeded PRNG, not an OsRng directly, from_rng
        // should never fail. https://rust-random.github.io/book/guide-err.html
        let rng = R::from_rng(&mut self.rng).unwrap();
        self.to_completed_handshake().and_then(|h| match h.next_protocol.as_ref() {
            D2DConnectionContextV1::<R>::NEXT_PROTOCOL_IDENTIFIER => {
                Ok(D2DConnectionContextV1::from_initiator_handshake::<C>(h, rng))
            }
            _ => Err(HandshakeError::HandshakeNotComplete),
        })
    }
}

enum ServerState<C: CryptoProvider> {
    Stage1(Ukey2ServerStage1<C>),
    Stage2(Ukey2ServerStage2<C>),
    Complete(Ukey2Server),
    /// If the initiator enters into an invalid state, e.g. by receiving invalid input.
    /// Also a momentary placeholder while swapping out states.
    Invalid,
}

/// Implementation of [`D2DHandshakeContext`] for the server.
pub struct ServerD2DHandshakeContext<C: CryptoProvider, R = rand::rngs::StdRng>
where
    R: rand::Rng + rand::SeedableRng + rand::CryptoRng + Send,
{
    state: ServerState<C>,
    rng: R,
}

impl<C: CryptoProvider> ServerD2DHandshakeContext<C, rand::rngs::StdRng> {
    pub fn new(handshake_impl: HandshakeImplementation) -> Self {
        Self::new_impl(handshake_impl, rand::rngs::StdRng::from_entropy())
    }
}

impl<C: CryptoProvider, R> ServerD2DHandshakeContext<C, R>
where
    R: rand::Rng + rand::SeedableRng + rand::CryptoRng + Send,
{
    // Used for testing / fuzzing only.
    #[doc(hidden)]
    pub fn new_impl(handshake_impl: HandshakeImplementation, rng: R) -> Self {
        Self {
            state: ServerState::Stage1(Ukey2ServerStage1::from(
                HashSet::from([
                    D2DConnectionContextV1::<rand::rngs::StdRng>::NEXT_PROTOCOL_IDENTIFIER
                        .to_owned(),
                ]),
                handshake_impl,
            )),
            rng,
        }
    }
}

impl<C, R> D2DHandshakeContext<R> for ServerD2DHandshakeContext<C, R>
where
    C: CryptoProvider,
    R: rand::Rng + rand::SeedableRng + rand::CryptoRng + Send,
{
    fn is_handshake_complete(&self) -> bool {
        match &self.state {
            ServerState::Complete(_) => true,
            ServerState::Stage1(_) | ServerState::Stage2(_) | ServerState::Invalid => false,
        }
    }

    fn get_next_handshake_message(&self) -> Option<Vec<u8>> {
        let next_msg = match &self.state {
            ServerState::Stage1(_) => None,
            ServerState::Stage2(s) => Some(s.server_init_msg().to_vec()),
            ServerState::Complete(_) => None,
            ServerState::Invalid => None,
        }?;
        Some(next_msg)
    }

    fn handle_handshake_message(&mut self, message: &[u8]) -> Result<(), HandleMessageError> {
        match mem::replace(&mut self.state, ServerState::Invalid) {
            ServerState::Stage1(s) => {
                let server2 = s
                    .advance_state(&mut self.rng, message)
                    .map_err(|a| HandleMessageError::ErrorMessage(a.into_wrapped_alert_msg()))?;
                self.state = ServerState::Stage2(server2);
                Ok(())
            }
            ServerState::Stage2(s) => {
                let server = s
                    .advance_state(&mut self.rng, message)
                    .map_err(|a| HandleMessageError::ErrorMessage(a.into_wrapped_alert_msg()))?;
                self.state = ServerState::Complete(server);
                Ok(())
            }
            ServerState::Complete(_) | ServerState::Invalid => {
                Err(HandleMessageError::InvalidState)
            }
        }
    }

    fn to_completed_handshake(&self) -> Result<&CompletedHandshake, HandshakeError> {
        match &self.state {
            ServerState::Stage1(_) | ServerState::Stage2(_) | ServerState::Invalid => {
                Err(HandshakeError::HandshakeNotComplete)
            }
            ServerState::Complete(s) => Ok(s.completed_handshake()),
        }
    }

    fn to_connection_context(&mut self) -> Result<D2DConnectionContextV1<R>, HandshakeError> {
        // Since self.rng is expected to be a seeded PRNG, not an OsRng directly, from_rng
        // should never fail. https://rust-random.github.io/book/guide-err.html
        let rng = R::from_rng(&mut self.rng).unwrap();
        self.to_completed_handshake().map(|h| match h.next_protocol.as_ref() {
            D2DConnectionContextV1::<R>::NEXT_PROTOCOL_IDENTIFIER => {
                D2DConnectionContextV1::from_responder_handshake::<C>(h, rng)
            }
            _ => {
                // This should never happen because ukey2_handshake should set next_protocol to
                // one of the values we passed in Ukey2ServerStage1::from, which doesn't contain
                // any other value.
                panic!("Unknown next protocol: {}", h.next_protocol);
            }
        })
    }
}
