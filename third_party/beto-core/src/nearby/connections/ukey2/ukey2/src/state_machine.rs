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

use crate::proto_adapter::{IntoAdapter, MessageType, ToWrappedMessage as _};
use crate::ukey2_handshake::ClientFinishedError;
use crate::ukey2_handshake::{
    ClientInit, ClientInitError, Ukey2Client, Ukey2ClientStage1, Ukey2Server, Ukey2ServerStage1,
    Ukey2ServerStage2,
};
use crypto_provider::CryptoProvider;
use log::error;
use std::fmt::Debug;
use ukey2_proto::protobuf::Message;
use ukey2_proto::ukey2_all_proto::ukey;

/// An alert type and message to be sent to the other party.
#[derive(Debug, PartialEq, Eq)]
pub struct SendAlert {
    alert_type: ukey::ukey2alert::AlertType,
    msg: Option<String>,
}

impl SendAlert {
    pub(crate) fn from(alert_type: ukey::ukey2alert::AlertType, msg: Option<String>) -> Self {
        Self { alert_type, msg }
    }

    /// Convert this `SendAlert` into serialized bytes of the `Ukey2Alert` protobuf message.
    pub fn into_wrapped_alert_msg(self) -> Vec<u8> {
        let alert_message = ukey::Ukey2Alert {
            type_: Some(self.alert_type.into()),
            error_message: self.msg,
            ..Default::default()
        };
        alert_message.to_wrapped_msg().write_to_bytes().unwrap()
    }
}

/// Generic trait for implementation of a state machine. Each state in this machine has two possible
/// transitions – Success and Failure.
///
/// On Success, the machine will transition to the next state, represented by the associated type
/// `Success`.
///
/// On Failure, a [`SendAlert`] message is returned indicating the failure, and there no further
/// transitions will be possible on this state machine.
///
/// ### State transitions
///
/// Here are the states both parties of the handshake goes through, with the Failure transitions
/// omitted to keep the documentation simple.
///
/// ```text
///          Ukey2ClientStage1               Ukey2ServerStage1
///                 |
///                 | -------[msg: ClientInit]-----> |
///                                                  |
///                                           Ukey2ServerStage2
///                                                  |
///                 | <------[msg: ServerInit]------ |
///                 |
///              Ukey2Client
///                 |
///                 | -----[msg: ClientFinished]---> |
///                                                  |
///                                              Ukey2Server
/// ```
///
pub trait StateMachine {
    /// The type produced by each successful state transition
    type Success;

    /// Advance to the next state in the relevant half (client/server) of the protocol.
    fn advance_state<R: rand::Rng + rand::CryptoRng>(
        self,
        rng: &mut R,
        message_bytes: &[u8],
    ) -> Result<Self::Success, SendAlert>;
}

impl<C: CryptoProvider> StateMachine for Ukey2ClientStage1<C> {
    type Success = Ukey2Client;

    fn advance_state<R: rand::Rng + rand::CryptoRng>(
        self,
        _rng: &mut R,
        message_bytes: &[u8],
    ) -> Result<Self::Success, SendAlert> {
        let (message_data, message_type) = decode_wrapper_msg_and_type(message_bytes)?;

        match message_type {
            // Client should not be receiving ClientInit/ClientFinish
            MessageType::ClientInit | MessageType::ClientFinish => Err(SendAlert::from(
                ukey::ukey2alert::AlertType::INCORRECT_MESSAGE,
                Some("wrong message".to_string()),
            )),
            MessageType::ServerInit => {
                let message = decode_msg_contents::<_, ukey::Ukey2ServerInit>(message_data)?;
                self.handle_server_init(message, message_bytes.to_vec()).map_err(|_| {
                    SendAlert::from(
                        ukey::ukey2alert::AlertType::BAD_MESSAGE_DATA,
                        Some("bad message_data".to_string()),
                    )
                })
            }
        }
    }
}

impl<C: CryptoProvider> StateMachine for Ukey2ServerStage1<C> {
    type Success = Ukey2ServerStage2<C>;

    fn advance_state<R: rand::Rng + rand::CryptoRng>(
        self,
        rng: &mut R,
        message_bytes: &[u8],
    ) -> Result<Self::Success, SendAlert> {
        let (message_data, message_type) = decode_wrapper_msg_and_type(message_bytes)?;
        match message_type {
            MessageType::ClientInit => {
                let message: ClientInit =
                    decode_msg_contents::<_, ukey::Ukey2ClientInit>(message_data)?;
                self.handle_client_init(rng, message, message_bytes.to_vec()).map_err(|e| {
                    SendAlert::from(
                        match e {
                            ClientInitError::BadVersion => ukey::ukey2alert::AlertType::BAD_VERSION,
                            ClientInitError::BadHandshakeCipher => {
                                ukey::ukey2alert::AlertType::BAD_HANDSHAKE_CIPHER
                            }
                            ClientInitError::BadNextProtocol => {
                                ukey::ukey2alert::AlertType::BAD_NEXT_PROTOCOL
                            }
                        },
                        None,
                    )
                })
            }
            MessageType::ClientFinish | MessageType::ServerInit => Err(SendAlert::from(
                ukey::ukey2alert::AlertType::INCORRECT_MESSAGE,
                Some("wrong message".to_string()),
            )),
        }
    }
}

impl<C: CryptoProvider> StateMachine for Ukey2ServerStage2<C> {
    type Success = Ukey2Server;

    fn advance_state<R: rand::Rng + rand::CryptoRng>(
        self,
        _rng: &mut R,
        message_bytes: &[u8],
    ) -> Result<Self::Success, SendAlert> {
        let (message_data, message_type) = decode_wrapper_msg_and_type(message_bytes)?;
        match message_type {
            MessageType::ClientFinish => {
                let message = decode_msg_contents::<_, ukey::Ukey2ClientFinished>(message_data)?;
                self.handle_client_finished_msg(message, message_bytes).map_err(|e| match e {
                    ClientFinishedError::BadEd25519Key => SendAlert::from(
                        ukey::ukey2alert::AlertType::BAD_PUBLIC_KEY,
                        "Bad ED25519 Key".to_string().into(),
                    ),
                    ClientFinishedError::BadP256Key => SendAlert::from(
                        ukey::ukey2alert::AlertType::BAD_PUBLIC_KEY,
                        "Bad P256 Key".to_string().into(),
                    ),
                    ClientFinishedError::UnknownCommitment => SendAlert::from(
                        ukey::ukey2alert::AlertType::BAD_MESSAGE_DATA,
                        "Unknown commitment".to_string().into(),
                    ),
                    ClientFinishedError::BadKeyExchange => SendAlert::from(
                        ukey::ukey2alert::AlertType::INTERNAL_ERROR,
                        "Key exchange error".to_string().into(),
                    ),
                })
            }
            MessageType::ClientInit | MessageType::ServerInit => Err(SendAlert::from(
                ukey::ukey2alert::AlertType::INCORRECT_MESSAGE,
                "wrong message".to_string().into(),
            )),
        }
    }
}

/// Extract the message field and message type from a Ukey2Message
fn decode_wrapper_msg_and_type(bytes: &[u8]) -> Result<(Vec<u8>, MessageType), SendAlert> {
    ukey::Ukey2Message::parse_from_bytes(bytes)
        .map_err(|_| {
            error!("Unable to marshal into Ukey2Message");
            SendAlert::from(
                ukey::ukey2alert::AlertType::BAD_MESSAGE,
                Some("Bad message data".to_string()),
            )
        })
        .and_then(|message| {
            let message_data = message.message_data();
            if message_data.is_empty() {
                return Err(SendAlert::from(ukey::ukey2alert::AlertType::BAD_MESSAGE_DATA, None));
            }
            let message_type = message.message_type();
            if message_type == ukey::ukey2message::Type::UNKNOWN_DO_NOT_USE {
                return Err(SendAlert::from(ukey::ukey2alert::AlertType::BAD_MESSAGE_TYPE, None));
            }
            message_type
                .into_adapter()
                .map_err(|e| {
                    error!("Unknown UKEY2 Message Type");
                    SendAlert::from(e, Some("bad message type".to_string()))
                })
                .map(|message_type| (message_data.to_vec(), message_type))
        })
}

/// Extract a specific message type from message data in a Ukey2Messaage
///
/// See [decode_wrapper_msg_and_type] for getting the message data.
fn decode_msg_contents<A, M: Message + Default + IntoAdapter<A>>(
    message_data: Vec<u8>,
) -> Result<A, SendAlert> {
    M::parse_from_bytes(message_data.as_slice())
        .map_err(|_| {
            error!(
                "Unable to unmarshal message, check frame of the message you were trying to send"
            );
            SendAlert::from(
                ukey::ukey2alert::AlertType::BAD_MESSAGE_DATA,
                Some("frame error".to_string()),
            )
        })?
        .into_adapter()
        .map_err(|t| SendAlert::from(t, Some("failed to translate proto".to_string())))
}
