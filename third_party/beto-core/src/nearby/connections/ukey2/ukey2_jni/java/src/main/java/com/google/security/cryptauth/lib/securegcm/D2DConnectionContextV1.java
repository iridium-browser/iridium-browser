/*
 * Copyright 2023 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.google.security.cryptauth.lib.securegcm;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

public class D2DConnectionContextV1 {

    static {
        System.loadLibrary("ukey2_jni");
    }

    private static native byte[] encode_message_to_peer(long contextPtr, byte[] payload, byte[] associatedData) throws BadHandleException;

    private static native byte[] decode_message_from_peer(long contextPtr, byte[] message, byte[] associatedData) throws CryptoException;

    private static native byte[] get_session_unique(long contextPtr) throws BadHandleException;

    private static native int get_sequence_number_for_encoding(long contextPtr) throws BadHandleException;

    private static native int get_sequence_number_for_decoding(long contextPtr) throws BadHandleException;

    private static native byte[] save_session(long contextPtr) throws BadHandleException;

    private static native long from_saved_session(byte[] savedSessionInfo);

    private final long contextPtr;

    /**
     * Java wrapper for D2DConnectionContextV1 to interact with the underlying Rust implementation
     *
     * @param contextPtr the handle to the Rust implementation.
     */
    D2DConnectionContextV1(@Nonnull long contextPtr) {
        this.contextPtr = contextPtr;
    }

    /**
     * Encode a message to the connection peer using session keys derived from the handshake.
     *
     * @param payload The message to be encrypted.
     * @return The encrypted/encoded message.
     */
     @Nonnull
    public byte[] encodeMessageToPeer(@Nonnull byte[] payload, @Nullable byte[] associatedData) throws BadHandleException {
        return encode_message_to_peer(contextPtr, payload, associatedData);
    }

    /**
     * Decodes/decrypts a message from the connection peer.
     *
     * @param message The message received over the connection.
     * @return The decoded message from the connection peer.
     */
     @Nonnull
    public byte[] decodeMessageFromPeer(@Nonnull byte[] message, @Nullable byte[] associatedData) throws CryptoException {
        return decode_message_from_peer(contextPtr, message, associatedData);
    }

    /**
     * A unique session identifier derived from session-specific information
     *
     * @return The session unique identifier
     */
     @Nonnull
    public  byte[] getSessionUnique() throws BadHandleException {
        return get_session_unique(contextPtr);
    }

    /**
     * Returns the encoding sequence number.
     *
     * @return the encoding sequence number.
     */
    public int getSequenceNumberForEncoding() throws BadHandleException {
        return get_sequence_number_for_encoding(contextPtr);
    }

    /**
     * Returns the decoding sequence number.
     *
     * @return the decoding sequence number.
     */
    public int getSequenceNumberForDecoding() throws BadHandleException {
        return get_sequence_number_for_decoding(contextPtr);
    }

    /**
     * Serializes the current session in a form usable by {@link D2DConnectionContextV1#fromSavedSession}
     *
     * @return a byte array representing the current session.
     */
     @Nonnull
    public byte[] saveSession() throws BadHandleException {
        return save_session(contextPtr);
    }

    /**
     * Reconstructs and returns the session originally serialized by {@link D2DConnectionContextV1#saveSession}
     *
     * @param savedSessionInfo the byte array from saveSession()
     * @return a D2DConnectionContextV1 session with the same properties as the context saved.
     */
    public static D2DConnectionContextV1 fromSavedSession(@Nonnull byte[] savedSessionInfo) {
        return new D2DConnectionContextV1(from_saved_session(savedSessionInfo));
    }

}
