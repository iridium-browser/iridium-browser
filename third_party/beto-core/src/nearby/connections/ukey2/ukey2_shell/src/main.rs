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

use std::io::{Read, Write};
use std::process::exit;

use clap::Parser;

use crypto_provider_rustcrypto::RustCrypto;
use ukey2_connections::{
    D2DConnectionContextV1, D2DHandshakeContext, InitiatorD2DHandshakeContext,
    ServerD2DHandshakeContext,
};
use ukey2_rs::HandshakeImplementation;

const MODE_INITIATOR: &str = "initiator";
const MODE_RESPONDER: &str = "responder";

#[derive(Parser, Debug)]
struct Ukey2Cli {
    /// initiator or responder mode
    #[arg(short, long)]
    mode: String,
    /// length of auth string/next proto secret
    #[arg(short, long, default_value_t = 32)]
    verification_string_length: i32,
}

/// Framing functions
/*
// Writes |message| to stdout in the frame format.
void WriteFrame(const string& message) {
  // Write length of |message| in little-endian.
  const uint32_t length = message.length();
  fputc((length >> (3 * 8)) & 0xFF, stdout);
  fputc((length >> (2 * 8)) & 0xFF, stdout);
  fputc((length >> (1 * 8)) & 0xFF, stdout);
  fputc((length >> (0 * 8)) & 0xFF, stdout);

  // Write message to stdout.
  CHECK_EQ(message.length(),
           fwrite(message.c_str(), 1, message.length(), stdout));
  CHECK_EQ(0, fflush(stdout));
}
*/
fn write_frame(message: Vec<u8>) {
    let length: u32 = message.len() as u32;
    let length_bytes = length.to_be_bytes();
    std::io::stdout().write_all(&length_bytes).unwrap();
    std::io::stdout().write_all(message.as_slice()).expect("failed to write message");
    let _ = std::io::stdout().flush();
}

/*
// Returns a message read from stdin after parsing it from the frame format.
string ReadFrame() {
  // Read length of the frame from the stream.
  uint8_t length_data[sizeof(uint32_t)];
  CHECK_EQ(sizeof(uint32_t), fread(&length_data, 1, sizeof(uint32_t), stdin));

  uint32_t length = 0;
  length |= static_cast<uint32_t>(length_data[0]) << (3 * 8);
  length |= static_cast<uint32_t>(length_data[1]) << (2 * 8);
  length |= static_cast<uint32_t>(length_data[2]) << (1 * 8);
  length |= static_cast<uint32_t>(length_data[3]) << (0 * 8);

  // Read |length| bytes from the stream.
  absl::FixedArray<char> buffer(length);
  CHECK_EQ(length, fread(buffer.data(), 1, length, stdin));

  return string(buffer.data(), length);
}

 */
const LENGTH: usize = std::mem::size_of::<u32>();

fn read_frame() -> Vec<u8> {
    let mut length_buf = [0u8; LENGTH];
    assert_eq!(LENGTH, std::io::stdin().read(&mut length_buf).unwrap());
    let length_usize = u32::from_be_bytes(length_buf);
    let mut buffer = vec![0u8; length_usize as usize];
    std::io::stdin().read_exact(buffer.as_mut_slice()).expect("failed to read frame");
    buffer
}

struct Ukey2Shell {
    verification_string_length: usize,
}

impl Ukey2Shell {
    fn new(verification_string_length: i32) -> Self {
        Self { verification_string_length: verification_string_length as usize }
    }

    fn run_secure_connection_loop(connection_ctx: &mut D2DConnectionContextV1) -> bool {
        loop {
            let input = read_frame();
            let idx = input.iter().enumerate().find(|(_index, &byte)| byte == 0x20).unwrap().0;
            let (cmd, payload) = (&input[0..idx], &input[idx + 1..]);
            if cmd == b"encrypt" {
                let result =
                    connection_ctx.encode_message_to_peer::<RustCrypto, &[u8]>(payload, None);
                write_frame(result);
            } else if cmd == b"decrypt" {
                let result =
                    connection_ctx.decode_message_from_peer::<RustCrypto, &[u8]>(payload, None);
                if result.is_err() {
                    println!("failed to decode payload");
                    return false;
                }
                write_frame(result.unwrap());
            } else if cmd == b"session_unique" {
                let result = connection_ctx.get_session_unique::<RustCrypto>();
                write_frame(result);
            } else {
                println!("unknown command");
                return false;
            }
        }
    }

    fn run_as_initiator(&self) -> bool {
        let mut initiator_ctx = InitiatorD2DHandshakeContext::<RustCrypto, _>::new(
            HandshakeImplementation::PublicKeyInProtobuf,
        );
        write_frame(initiator_ctx.get_next_handshake_message().unwrap());
        let server_init_msg = read_frame();
        initiator_ctx
            .handle_handshake_message(server_init_msg.as_slice())
            .expect("Failed to handle message");
        write_frame(initiator_ctx.get_next_handshake_message().unwrap_or_default());
        // confirm auth str
        let auth_str = initiator_ctx
            .to_completed_handshake()
            .ok()
            .and_then(|h| h.auth_string::<RustCrypto>().derive_vec(self.verification_string_length))
            .unwrap_or_else(|| vec![0; self.verification_string_length]);
        write_frame(auth_str);
        let ack = read_frame();
        if ack != "ok".to_string().into_bytes() {
            println!("handshake failed");
            return false;
        }
        // upgrade to connection context
        let mut initiator_conn_ctx = initiator_ctx.to_connection_context().unwrap();
        Self::run_secure_connection_loop(&mut initiator_conn_ctx)
    }

    fn run_as_responder(&self) -> bool {
        let mut server_ctx = ServerD2DHandshakeContext::<RustCrypto, _>::new(
            HandshakeImplementation::PublicKeyInProtobuf,
        );
        let initiator_init_msg = read_frame();
        server_ctx.handle_handshake_message(initiator_init_msg.as_slice()).unwrap();
        let server_next_msg = server_ctx.get_next_handshake_message().unwrap();
        write_frame(server_next_msg);
        let initiator_finish_msg = read_frame();
        server_ctx
            .handle_handshake_message(initiator_finish_msg.as_slice())
            .expect("Failed to handle message");
        // confirm auth str
        let auth_str = server_ctx
            .to_completed_handshake()
            .ok()
            .and_then(|h| h.auth_string::<RustCrypto>().derive_vec(self.verification_string_length))
            .unwrap_or_else(|| vec![0; self.verification_string_length]);
        write_frame(auth_str);
        let ack = read_frame();
        if ack != "ok".to_string().into_bytes() {
            println!("handshake failed");
            return false;
        }
        // upgrade to connection context
        let mut server_conn_ctx = server_ctx.to_connection_context().unwrap();
        Self::run_secure_connection_loop(&mut server_conn_ctx)
    }
}

fn main() {
    let args = Ukey2Cli::parse();
    let shell = Ukey2Shell::new(args.verification_string_length);
    if args.mode == MODE_INITIATOR {
        shell.run_as_initiator();
    } else if args.mode == MODE_RESPONDER {
        shell.run_as_responder();
    } else {
        exit(1);
    }
    exit(0)
}
