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

//! This crate is an implementation of the UKEY2 handshake protocol.
//!
//! UKEY2 is a Diffie-Hellman based authentication exchange protocol that allows two devices to
//! establish a secure channel.
//!
//! For a full description of the protocol, see <https://github.com/google/ukey2>.
#![forbid(unsafe_code)]
#![deny(
    missing_docs,
    trivial_casts,
    trivial_numeric_casts,
    unused_extern_crates,
    unused_import_braces,
    unused_results
)]

mod proto_adapter;
mod state_machine;
#[cfg(test)]
mod tests;
mod ukey2_handshake;

pub use state_machine::{SendAlert, StateMachine};
pub use ukey2_handshake::{
    CompletedHandshake, HandshakeImplementation, Ukey2Client, Ukey2ClientStage1, Ukey2Server,
    Ukey2ServerStage1, Ukey2ServerStage2, WireCompatibilityLayer,
};
