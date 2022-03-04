// Copyright 2020 Google LLC
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

package com.google.security.cryptauth.lib.securegcm;

import com.google.security.cryptauth.lib.securegcm.Ukey2Handshake.HandshakeCipher;
import com.google.security.cryptauth.lib.securegcm.Ukey2ShellCppWrapper.Mode;
import java.util.Arrays;
import junit.framework.TestCase;

/**
 * Tests the compatibility between the Java and C++ implementations of the UKEY2 protocol. This
 * integration test executes and talks to a compiled binary exposing the C++ implementation (wrapped
 * by {@link Ukey2ShellCppWrapper}).
 *
 * <p>The C++ implementation is located in //security/cryptauth/lib/securegcm.
 */
public class Ukey2CppCompatibilityTest extends TestCase {
  private static final int VERIFICATION_STRING_LENGTH = 32;

  private static final byte[] sPayload1 = "payload to encrypt1".getBytes();
  private static final byte[] sPayload2 = "payload to encrypt2".getBytes();

  /** Tests full handshake with C++ client and Java server. */
  public void testCppClientJavaServer() throws Exception {
    Ukey2ShellCppWrapper cppUkey2Shell =
        new Ukey2ShellCppWrapper(Mode.INITIATOR, VERIFICATION_STRING_LENGTH);
    cppUkey2Shell.startShell();
    Ukey2Handshake javaUkey2Handshake = Ukey2Handshake.forResponder(HandshakeCipher.P256_SHA512);

    // ClientInit:
    byte[] clientInit = cppUkey2Shell.readHandshakeMessage();
    javaUkey2Handshake.parseHandshakeMessage(clientInit);

    // ServerInit:
    byte[] serverInit = javaUkey2Handshake.getNextHandshakeMessage();
    cppUkey2Shell.writeHandshakeMessage(serverInit);

    // ClientFinished:
    byte[] clientFinished = cppUkey2Shell.readHandshakeMessage();
    javaUkey2Handshake.parseHandshakeMessage(clientFinished);

    // Verification String:
    cppUkey2Shell.confirmAuthString(
        javaUkey2Handshake.getVerificationString(VERIFICATION_STRING_LENGTH));
    javaUkey2Handshake.verifyHandshake();

    // Secure channel:
    D2DConnectionContext javaSecureContext = javaUkey2Handshake.toConnectionContext();

    // ukey2_shell encodes data:
    byte[] encodedData = cppUkey2Shell.sendEncryptCommand(sPayload1);
    byte[] decodedData = javaSecureContext.decodeMessageFromPeer(encodedData);
    assertTrue(Arrays.equals(sPayload1, decodedData));

    // ukey2_shell decodes data:
    encodedData = javaSecureContext.encodeMessageToPeer(sPayload2);
    decodedData = cppUkey2Shell.sendDecryptCommand(encodedData);
    assertTrue(Arrays.equals(sPayload2, decodedData));

    // ukey2_shell session unique:
    byte[] localSessionUnique = javaSecureContext.getSessionUnique();
    byte[] remoteSessionUnique = cppUkey2Shell.sendSessionUniqueCommand();
    assertTrue(Arrays.equals(localSessionUnique, remoteSessionUnique));

    cppUkey2Shell.stopShell();
  }

  /** Tests full handshake with C++ server and Java client. */
  public void testCppServerJavaClient() throws Exception {
    Ukey2ShellCppWrapper cppUkey2Shell =
        new Ukey2ShellCppWrapper(Mode.RESPONDER, VERIFICATION_STRING_LENGTH);
    cppUkey2Shell.startShell();
    Ukey2Handshake javaUkey2Handshake = Ukey2Handshake.forInitiator(HandshakeCipher.P256_SHA512);

    // ClientInit:
    byte[] clientInit = javaUkey2Handshake.getNextHandshakeMessage();
    cppUkey2Shell.writeHandshakeMessage(clientInit);

    // ServerInit:
    byte[] serverInit = cppUkey2Shell.readHandshakeMessage();
    javaUkey2Handshake.parseHandshakeMessage(serverInit);

    // ClientFinished:
    byte[] clientFinished = javaUkey2Handshake.getNextHandshakeMessage();
    cppUkey2Shell.writeHandshakeMessage(clientFinished);

    // Verification String:
    cppUkey2Shell.confirmAuthString(
        javaUkey2Handshake.getVerificationString(VERIFICATION_STRING_LENGTH));
    javaUkey2Handshake.verifyHandshake();

    // Secure channel:
    D2DConnectionContext javaSecureContext = javaUkey2Handshake.toConnectionContext();

    // ukey2_shell encodes data:
    byte[] encodedData = cppUkey2Shell.sendEncryptCommand(sPayload1);
    byte[] decodedData = javaSecureContext.decodeMessageFromPeer(encodedData);
    assertTrue(Arrays.equals(sPayload1, decodedData));

    // ukey2_shell decodes data:
    encodedData = javaSecureContext.encodeMessageToPeer(sPayload2);
    decodedData = cppUkey2Shell.sendDecryptCommand(encodedData);
    assertTrue(Arrays.equals(sPayload2, decodedData));

    // ukey2_shell session unique:
    byte[] localSessionUnique = javaSecureContext.getSessionUnique();
    byte[] remoteSessionUnique = cppUkey2Shell.sendSessionUniqueCommand();
    assertTrue(Arrays.equals(localSessionUnique, remoteSessionUnique));

    cppUkey2Shell.stopShell();
  }
}
