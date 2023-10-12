// Copyright 2023 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "ukey2_bindings.h"
#include <string>

struct D2DRestoreConnectionContextV1Result;

// The Connection object that can handle encryption/decryption of messages over the wire.
// This object should only be constructed via FromSavedSession() or Ukey2Handshake::ToConnectionContext().
class D2DConnectionContextV1 {
    public:
        // Encodes a message to the connection peer using the derived key from the handshake
        // If associated_data is not empty, it will be used to compute the signature and the same
        // associated_data string must be passed into DecodeMessageFromPeer() in order for the
        // message to be validated.
        std::string EncodeMessageToPeer(std::string message, std::string associated_data);
        // Decodes a message from the connection peer. If associated_data was passed into
        // EncodeMessageToPeer(), that same associated_data must be passed here in order for
        // this function to succeed.
        std::string DecodeMessageFromPeer(std::string message, std::string associated_data);
        // Gets a session-specific unique identifier.
        std::string GetSessionUnique();
        // Gets the encoding sequence number.
        int GetSequenceNumberForEncoding();
        // Gets the decoding sequence number.
        int GetSequenceNumberForDecoding();
        // Returns byte data suitable for use with FromSavedSession().
        std::string SaveSession();
        // Recreates the state of a previous D2DConnectionContextV1 using the data from SaveSession().
        // This function will return an error if the byte pattern is not as expected.
        // Expected format:
        // -------------------------------------------------------------------------------------------
        // |     1 byte      |       4 bytes       |         4 bytes        |  32 bytes |  32 bytes  |
        // -------------------------------------------------------------------------------------------
        //  Protocol version | Encode sequence number | Decode sequence number | Encode key | Decode key
        //    (always 1)
        static D2DRestoreConnectionContextV1Result FromSavedSession(std::string data);

    private:
        friend class Ukey2Handshake;
        D2DConnectionContextV1(Ukey2ConnectionContextHandle handle) : handle_(handle) {}
        const Ukey2ConnectionContextHandle handle_;
};

struct D2DRestoreConnectionContextV1Result {
    D2DConnectionContextV1 handle;
    CD2DRestoreConnectionContextV1Status status;
};

struct ParseResult {
    bool success;
    std::string alert_to_send;
};

// Base handshake. This should be used to start a secure channel represented by a D2DConnectionContextV1.
class Ukey2Handshake {
    public:
        // Creates a Ukey2Handshake instance for the responder.
        static Ukey2Handshake ForResponder();
        // Creates a Ukey2Handshake instance for the initiator.
        static Ukey2Handshake ForInitiator();
        // Returns true if the handshake is complete, false otherwise.
        bool IsHandshakeComplete();
        // Returns raw byte data with the message to send over the wire.
        std::string GetNextHandshakeMessage();
        // Parses the raw handshake message received over the wire.
        ParseResult ParseHandshakeMessage(std::string message);
        // Returns the authentication string of length output_length to be confirmed on both devices.
        std::string GetVerificationString(size_t output_length);
        // Turns this Ukey2Handshake instance into a D2DConnectionContextV1. This method once called,
        // renders the Ukey2Handshake object unusable.
        D2DConnectionContextV1 ToConnectionContext();

    private:
        Ukey2Handshake(Ukey2HandshakeContextHandle handle) : handle_(handle) {}
        const Ukey2HandshakeContextHandle handle_;
};
