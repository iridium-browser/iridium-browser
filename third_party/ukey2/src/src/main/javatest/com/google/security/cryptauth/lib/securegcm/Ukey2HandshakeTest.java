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

import com.google.protobuf.ByteString;
import com.google.security.cryptauth.lib.securegcm.Ukey2Handshake.AlertException;
import com.google.security.cryptauth.lib.securegcm.Ukey2Handshake.HandshakeCipher;
import com.google.security.cryptauth.lib.securegcm.Ukey2Handshake.State;
import com.google.security.cryptauth.lib.securegcm.UkeyProto.Ukey2ClientFinished;
import com.google.security.cryptauth.lib.securegcm.UkeyProto.Ukey2ClientInit;
import com.google.security.cryptauth.lib.securegcm.UkeyProto.Ukey2ClientInit.CipherCommitment;
import com.google.security.cryptauth.lib.securegcm.UkeyProto.Ukey2Message;
import com.google.security.cryptauth.lib.securegcm.UkeyProto.Ukey2ServerInit;
import java.util.Arrays;
import java.util.HashSet;
import java.util.Set;
import junit.framework.TestCase;
import org.junit.Assert;

/**
 * Android compatible tests for the {@link Ukey2Handshake} class.
 */
public class Ukey2HandshakeTest extends TestCase {

  private static final int MAX_AUTH_STRING_LENGTH = 32;

  @Override
  protected void setUp() throws Exception {
    KeyEncodingTest.installSunEcSecurityProviderIfNecessary();
    super.setUp();
  }

  /**
   * Tests correct use
   */
  public void testHandshake() throws Exception {
    if (KeyEncoding.isLegacyCryptoRequired()) {
      // this means we're running on an old SDK, which doesn't support the
      // necessary crypto. Let's not test anything in this case.
      return;
    }

    Ukey2Handshake client = Ukey2Handshake.forInitiator(HandshakeCipher.P256_SHA512);
    Ukey2Handshake server = Ukey2Handshake.forResponder(HandshakeCipher.P256_SHA512);
    byte[] handshakeMessage;

    assertEquals(State.IN_PROGRESS, client.getHandshakeState());
    assertEquals(State.IN_PROGRESS, server.getHandshakeState());

    // Message 1 (Client Init)
    handshakeMessage = client.getNextHandshakeMessage();
    server.parseHandshakeMessage(handshakeMessage);
    assertEquals(State.IN_PROGRESS, client.getHandshakeState());
    assertEquals(State.IN_PROGRESS, server.getHandshakeState());

    // Message 2 (Server Init)
    handshakeMessage = server.getNextHandshakeMessage();
    client.parseHandshakeMessage(handshakeMessage);
    assertEquals(State.IN_PROGRESS, client.getHandshakeState());
    assertEquals(State.IN_PROGRESS, server.getHandshakeState());

    // Message 3 (Client Finish)
    handshakeMessage = client.getNextHandshakeMessage();
    server.parseHandshakeMessage(handshakeMessage);
    assertEquals(State.VERIFICATION_NEEDED, client.getHandshakeState());
    assertEquals(State.VERIFICATION_NEEDED, server.getHandshakeState());

    // Get the auth string
    byte[] clientAuthString = client.getVerificationString(MAX_AUTH_STRING_LENGTH);
    byte[] serverAuthString = server.getVerificationString(MAX_AUTH_STRING_LENGTH);
    Assert.assertArrayEquals(clientAuthString, serverAuthString);
    assertEquals(State.VERIFICATION_IN_PROGRESS, client.getHandshakeState());
    assertEquals(State.VERIFICATION_IN_PROGRESS, server.getHandshakeState());

    // Verify the auth string
    client.verifyHandshake();
    server.verifyHandshake();
    assertEquals(State.FINISHED, client.getHandshakeState());
    assertEquals(State.FINISHED, server.getHandshakeState());

    // Make a context
    D2DConnectionContext clientContext = client.toConnectionContext();
    D2DConnectionContext serverContext = server.toConnectionContext();
    assertContextsCompatible(clientContext, serverContext);
    assertEquals(State.ALREADY_USED, client.getHandshakeState());
    assertEquals(State.ALREADY_USED, server.getHandshakeState());
  }

  /**
   * Verify enums for ciphers match the proto values
   */
  public void testCipherEnumValuesCorrect() {
    assertEquals(
        "You added a cipher, but forgot to change the test", 1, HandshakeCipher.values().length);

    assertEquals(UkeyProto.Ukey2HandshakeCipher.P256_SHA512,
        HandshakeCipher.P256_SHA512.getValue());
  }

  /**
   * Tests incorrect use by callers (client and servers accidentally sending the wrong message at
   * the wrong time)
   */
  public void testHandshakeClientAndServerSendRepeatedOutOfOrderMessages() throws Exception {
    if (KeyEncoding.isLegacyCryptoRequired()) {
      // this means we're running on an old SDK, which doesn't support the
      // necessary crypto. Let's not test anything in this case.
      return;
    }

    // Client sends ClientInit (again) instead of ClientFinished
    Ukey2Handshake client = Ukey2Handshake.forInitiator(HandshakeCipher.P256_SHA512);
    Ukey2Handshake server = Ukey2Handshake.forResponder(HandshakeCipher.P256_SHA512);
    byte[] handshakeMessage = client.getNextHandshakeMessage();
    server.parseHandshakeMessage(handshakeMessage);
    server.getNextHandshakeMessage(); // do this to avoid illegal state
    try {
      server.parseHandshakeMessage(handshakeMessage);
      fail("Expected Alert for client sending ClientInit twice");
    } catch (HandshakeException e) {
      // success
    }
    assertEquals(State.ERROR, server.getHandshakeState());

    // Server sends ClientInit back to client instead of ServerInit
    client = Ukey2Handshake.forInitiator(HandshakeCipher.P256_SHA512);
    server = Ukey2Handshake.forResponder(HandshakeCipher.P256_SHA512);
    handshakeMessage = client.getNextHandshakeMessage();
    try {
      client.parseHandshakeMessage(handshakeMessage);
      fail("Expected Alert for server sending ClientInit back to client");
    } catch (AlertException e) {
      // success
    }
    assertEquals(State.ERROR, client.getHandshakeState());

    // Clients sends ServerInit back to client instead of ClientFinished
    client = Ukey2Handshake.forInitiator(HandshakeCipher.P256_SHA512);
    server = Ukey2Handshake.forResponder(HandshakeCipher.P256_SHA512);
    handshakeMessage = client.getNextHandshakeMessage();
    server.parseHandshakeMessage(handshakeMessage);
    handshakeMessage = server.getNextHandshakeMessage();
    try {
      server.parseHandshakeMessage(handshakeMessage);
      fail("Expected Alert for client sending ServerInit back to server");
    } catch (HandshakeException e) {
      // success
    }
    assertEquals(State.ERROR, server.getHandshakeState());
  }

  /**
   * Tests that verification codes are different for different handshake runs. Also tests a full
   * man-in-the-middle attack.
   */
  public void testVerificationCodeUniqueToSession() throws Exception {
    if (KeyEncoding.isLegacyCryptoRequired()) {
      // this means we're running on an old SDK, which doesn't support the
      // necessary crypto. Let's not test anything in this case.
      return;
    }

    // Client 1 and Server 1
    Ukey2Handshake client1 = Ukey2Handshake.forInitiator(HandshakeCipher.P256_SHA512);
    Ukey2Handshake server1 = Ukey2Handshake.forResponder(HandshakeCipher.P256_SHA512);
    byte[] handshakeMessage = client1.getNextHandshakeMessage();
    server1.parseHandshakeMessage(handshakeMessage);
    handshakeMessage = server1.getNextHandshakeMessage();
    client1.parseHandshakeMessage(handshakeMessage);
    handshakeMessage = client1.getNextHandshakeMessage();
    server1.parseHandshakeMessage(handshakeMessage);
    byte[] client1AuthString = client1.getVerificationString(MAX_AUTH_STRING_LENGTH);
    byte[] server1AuthString = server1.getVerificationString(MAX_AUTH_STRING_LENGTH);
    Assert.assertArrayEquals(client1AuthString, server1AuthString);

    // Client 2 and Server 2
    Ukey2Handshake client2 = Ukey2Handshake.forInitiator(HandshakeCipher.P256_SHA512);
    Ukey2Handshake server2 = Ukey2Handshake.forResponder(HandshakeCipher.P256_SHA512);
    handshakeMessage = client2.getNextHandshakeMessage();
    server2.parseHandshakeMessage(handshakeMessage);
    handshakeMessage = server2.getNextHandshakeMessage();
    client2.parseHandshakeMessage(handshakeMessage);
    handshakeMessage = client2.getNextHandshakeMessage();
    server2.parseHandshakeMessage(handshakeMessage);
    byte[] client2AuthString = client2.getVerificationString(MAX_AUTH_STRING_LENGTH);
    byte[] server2AuthString = server2.getVerificationString(MAX_AUTH_STRING_LENGTH);
    Assert.assertArrayEquals(client2AuthString, server2AuthString);

    // Make sure the verification strings differ
    assertFalse(Arrays.equals(client1AuthString, client2AuthString));
  }

  /**
   * Test an attack where the adversary swaps out the public key in the final message (i.e.,
   * commitment doesn't match public key)
   */
  public void testPublicKeyDoesntMatchCommitment() throws Exception {
    if (KeyEncoding.isLegacyCryptoRequired()) {
      // this means we're running on an old SDK, which doesn't support the
      // necessary crypto. Let's not test anything in this case.
      return;
    }

    // Run handshake as usual, but stop before sending client finished
    Ukey2Handshake client1 = Ukey2Handshake.forInitiator(HandshakeCipher.P256_SHA512);
    Ukey2Handshake server1 = Ukey2Handshake.forResponder(HandshakeCipher.P256_SHA512);
    byte[] handshakeMessage = client1.getNextHandshakeMessage();
    server1.parseHandshakeMessage(handshakeMessage);
    handshakeMessage = server1.getNextHandshakeMessage();

    // Run another handshake and get the final client finished
    Ukey2Handshake client2 = Ukey2Handshake.forInitiator(HandshakeCipher.P256_SHA512);
    Ukey2Handshake server2 = Ukey2Handshake.forResponder(HandshakeCipher.P256_SHA512);
    handshakeMessage = client2.getNextHandshakeMessage();
    server2.parseHandshakeMessage(handshakeMessage);
    handshakeMessage = server2.getNextHandshakeMessage();
    client2.parseHandshakeMessage(handshakeMessage);
    handshakeMessage = client2.getNextHandshakeMessage();

    // Now use the client finished from second handshake in first handshake (simulates where an
    // attacker switches out the last message).
    try {
      server1.parseHandshakeMessage(handshakeMessage);
      fail("Expected server to catch mismatched ClientFinished");
    } catch (HandshakeException e) {
      // success
    }
    assertEquals(State.ERROR, server1.getHandshakeState());

    // Make sure caller can't actually do anything with the server now that an error has occurred
    try {
      server1.getVerificationString(MAX_AUTH_STRING_LENGTH);
      fail("Server allows operations post error");
    } catch (IllegalStateException e) {
      // success
    }
    try {
      server1.verifyHandshake();
      fail("Server allows operations post error");
    } catch (IllegalStateException e) {
      // success
    }
    try {
      server1.toConnectionContext();
      fail("Server allows operations post error");
    } catch (IllegalStateException e) {
      // success
    }
  }

  /**
   * Test commitment having unsupported version
   */
  public void testClientInitUnsupportedVersion() throws Exception {
    if (KeyEncoding.isLegacyCryptoRequired()) {
      // this means we're running on an old SDK, which doesn't support the
      // necessary crypto. Let's not test anything in this case.
      return;
    }

    // Get ClientInit and modify the version to be too big
    Ukey2Handshake client = Ukey2Handshake.forInitiator(HandshakeCipher.P256_SHA512);
    Ukey2Handshake server = Ukey2Handshake.forResponder(HandshakeCipher.P256_SHA512);
    byte[] handshakeMessage = client.getNextHandshakeMessage();

    Ukey2Message.Builder message = Ukey2Message.newBuilder(
        Ukey2Message.parseFrom(handshakeMessage));
    Ukey2ClientInit.Builder clientInit =
        Ukey2ClientInit.newBuilder(Ukey2ClientInit.parseFrom(message.getMessageData()));
    clientInit.setVersion(Ukey2Handshake.VERSION + 1);
    message.setMessageData(ByteString.copyFrom(clientInit.build().toByteArray()));
    handshakeMessage = message.build().toByteArray();

    try {
      server.parseHandshakeMessage(handshakeMessage);
      fail("Server did not catch unsupported version (too big) in ClientInit");
    } catch (AlertException e) {
      // success
    }
    assertEquals(State.ERROR, server.getHandshakeState());

    // Get ClientInit and modify the version to be too big
    client = Ukey2Handshake.forInitiator(HandshakeCipher.P256_SHA512);
    server = Ukey2Handshake.forResponder(HandshakeCipher.P256_SHA512);
    handshakeMessage = client.getNextHandshakeMessage();

    message = Ukey2Message.newBuilder(
        Ukey2Message.parseFrom(handshakeMessage));
    clientInit = Ukey2ClientInit.newBuilder(Ukey2ClientInit.parseFrom(message.getMessageData()));
    clientInit.setVersion(0 /* minimum version is 1 */);
    message.setMessageData(ByteString.copyFrom(clientInit.build().toByteArray()));
    handshakeMessage = message.build().toByteArray();

    try {
      server.parseHandshakeMessage(handshakeMessage);
      fail("Server did not catch unsupported version (too small) in ClientInit");
    } catch (AlertException e) {
      // success
    }
    assertEquals(State.ERROR, server.getHandshakeState());
  }

  /**
   * Tests that server catches wrong number of random bytes in ClientInit
   */
  public void testWrongNonceLengthInClientInit() throws Exception {
    if (KeyEncoding.isLegacyCryptoRequired()) {
      // this means we're running on an old SDK, which doesn't support the
      // necessary crypto. Let's not test anything in this case.
      return;
    }

    // Get ClientInit and modify the nonce
    Ukey2Handshake client = Ukey2Handshake.forInitiator(HandshakeCipher.P256_SHA512);
    Ukey2Handshake server = Ukey2Handshake.forResponder(HandshakeCipher.P256_SHA512);
    byte[] handshakeMessage = client.getNextHandshakeMessage();

    Ukey2Message.Builder message = Ukey2Message.newBuilder(
        Ukey2Message.parseFrom(handshakeMessage));
    Ukey2ClientInit.Builder clientInit =
        Ukey2ClientInit.newBuilder(Ukey2ClientInit.parseFrom(message.getMessageData()));
    clientInit.setRandom(
        ByteString.copyFrom(
            Arrays.copyOf(
                clientInit.getRandom().toByteArray(),
                31 /* as per go/ukey2, nonces must be 32 bytes long */)));
    message.setMessageData(ByteString.copyFrom(clientInit.build().toByteArray()));
    handshakeMessage = message.build().toByteArray();

    try {
      server.parseHandshakeMessage(handshakeMessage);
      fail("Server did not catch nonce being too short in ClientInit");
    } catch (AlertException e) {
      // success
    }
    assertEquals(State.ERROR, server.getHandshakeState());
  }

  /**
   * Test that server catches missing commitment in ClientInit message
   */
  public void testServerCatchesMissingCommitmentInClientInit() throws Exception {
    if (KeyEncoding.isLegacyCryptoRequired()) {
      // this means we're running on an old SDK, which doesn't support the
      // necessary crypto. Let's not test anything in this case.
      return;
    }

    // Get ClientInit and modify the commitment
    Ukey2Handshake client = Ukey2Handshake.forInitiator(HandshakeCipher.P256_SHA512);
    Ukey2Handshake server = Ukey2Handshake.forResponder(HandshakeCipher.P256_SHA512);
    byte[] handshakeMessage = client.getNextHandshakeMessage();

    Ukey2Message.Builder message = Ukey2Message.newBuilder(
        Ukey2Message.parseFrom(handshakeMessage));
    Ukey2ClientInit clientInit =
        Ukey2ClientInit.newBuilder(Ukey2ClientInit.parseFrom(message.getMessageData()))
        .build();
    Ukey2ClientInit.Builder badClientInit = Ukey2ClientInit.newBuilder()
        .setVersion(clientInit.getVersion())
        .setRandom(clientInit.getRandom());
    for (CipherCommitment commitment : clientInit.getCipherCommitmentsList()) {
      badClientInit.addCipherCommitments(commitment);
    }

    message.setMessageData(ByteString.copyFrom(badClientInit.build().toByteArray()));
    handshakeMessage = message.build().toByteArray();

    try {
      server.parseHandshakeMessage(handshakeMessage);
      fail("Server did not catch missing commitment in ClientInit");
    } catch (AlertException e) {
      // success
    }
  }

  /**
   * Test that client catches invalid version in ServerInit
   */
  public void testServerInitUnsupportedVersion() throws Exception {
    if (KeyEncoding.isLegacyCryptoRequired()) {
      // this means we're running on an old SDK, which doesn't support the
      // necessary crypto. Let's not test anything in this case.
      return;
    }

    // Get ServerInit and modify the version to be too big
    Ukey2Handshake client = Ukey2Handshake.forInitiator(HandshakeCipher.P256_SHA512);
    Ukey2Handshake server = Ukey2Handshake.forResponder(HandshakeCipher.P256_SHA512);
    byte[] handshakeMessage = client.getNextHandshakeMessage();
    server.parseHandshakeMessage(handshakeMessage);
    handshakeMessage = server.getNextHandshakeMessage();

    Ukey2Message.Builder message = Ukey2Message.newBuilder(
        Ukey2Message.parseFrom(handshakeMessage));
    Ukey2ServerInit serverInit =
        Ukey2ServerInit.newBuilder(Ukey2ServerInit.parseFrom(message.getMessageData()))
            .setVersion(Ukey2Handshake.VERSION + 1)
            .build();
    message.setMessageData(ByteString.copyFrom(serverInit.toByteArray()));
    handshakeMessage = message.build().toByteArray();

    try {
      client.parseHandshakeMessage(handshakeMessage);
      fail("Client did not catch unsupported version (too big) in ServerInit");
    } catch (AlertException e) {
      // success
    }
    assertEquals(State.ERROR, client.getHandshakeState());

    // Get ServerInit and modify the version to be too big
    client = Ukey2Handshake.forInitiator(HandshakeCipher.P256_SHA512);
    server = Ukey2Handshake.forResponder(HandshakeCipher.P256_SHA512);
    handshakeMessage = client.getNextHandshakeMessage();
    server.parseHandshakeMessage(handshakeMessage);
    handshakeMessage = server.getNextHandshakeMessage();

    message = Ukey2Message.newBuilder(Ukey2Message.parseFrom(handshakeMessage));
    serverInit =
        Ukey2ServerInit.newBuilder(Ukey2ServerInit.parseFrom(message.getMessageData()))
            .setVersion(0 /* minimum version is 1 */)
            .build();
    message.setMessageData(ByteString.copyFrom(serverInit.toByteArray()));
    handshakeMessage = message.build().toByteArray();

    try {
      client.parseHandshakeMessage(handshakeMessage);
      fail("Client did not catch unsupported version (too small) in ServerInit");
    } catch (AlertException e) {
      // success
    }
    assertEquals(State.ERROR, client.getHandshakeState());
  }

  /**
   * Tests that client catches wrong number of random bytes in ServerInit
   */
  public void testWrongNonceLengthInServerInit() throws Exception {
    if (KeyEncoding.isLegacyCryptoRequired()) {
      // this means we're running on an old SDK, which doesn't support the
      // necessary crypto. Let's not test anything in this case.
      return;
    }

    // Get ServerInit and modify the nonce
    Ukey2Handshake client = Ukey2Handshake.forInitiator(HandshakeCipher.P256_SHA512);
    Ukey2Handshake server = Ukey2Handshake.forResponder(HandshakeCipher.P256_SHA512);
    byte[] handshakeMessage = client.getNextHandshakeMessage();
    server.parseHandshakeMessage(handshakeMessage);
    handshakeMessage = server.getNextHandshakeMessage();

    Ukey2Message.Builder message = Ukey2Message.newBuilder(
        Ukey2Message.parseFrom(handshakeMessage));
    Ukey2ServerInit.Builder serverInitBuilder =
        Ukey2ServerInit.newBuilder(Ukey2ServerInit.parseFrom(message.getMessageData()));
    Ukey2ServerInit serverInit = serverInitBuilder.setRandom(ByteString.copyFrom(Arrays.copyOf(
            serverInitBuilder.getRandom().toByteArray(),
            31 /* as per go/ukey2, nonces must be 32 bytes long */)))
        .build();
    message.setMessageData(ByteString.copyFrom(serverInit.toByteArray()));
    handshakeMessage = message.build().toByteArray();

    try {
      client.parseHandshakeMessage(handshakeMessage);
      fail("Client did not catch nonce being too short in ServerInit");
    } catch (AlertException e) {
      // success
    }
    assertEquals(State.ERROR, client.getHandshakeState());
  }

  /**
   * Test that client catches missing or incorrect handshake cipher in serverInit
   */
  public void testMissingOrIncorrectHandshakeCipherInServerInit() throws Exception {
    if (KeyEncoding.isLegacyCryptoRequired()) {
      // this means we're running on an old SDK, which doesn't support the
      // necessary crypto. Let's not test anything in this case.
      return;
    }

    // Get ServerInit
    Ukey2Handshake client = Ukey2Handshake.forInitiator(HandshakeCipher.P256_SHA512);
    Ukey2Handshake server = Ukey2Handshake.forResponder(HandshakeCipher.P256_SHA512);
    byte[] handshakeMessage = client.getNextHandshakeMessage();
    server.parseHandshakeMessage(handshakeMessage);
    handshakeMessage = server.getNextHandshakeMessage();
    Ukey2Message.Builder message = Ukey2Message.newBuilder(
        Ukey2Message.parseFrom(handshakeMessage));
    Ukey2ServerInit serverInit = Ukey2ServerInit.parseFrom(message.getMessageData());

    // remove handshake cipher
    Ukey2ServerInit badServerInit = Ukey2ServerInit.newBuilder()
        .setPublicKey(serverInit.getPublicKey())
        .setRandom(serverInit.getRandom())
        .setVersion(serverInit.getVersion())
        .build();

    message.setMessageData(ByteString.copyFrom(badServerInit.toByteArray()));
    handshakeMessage = message.build().toByteArray();

    try {
      client.parseHandshakeMessage(handshakeMessage);
      fail("Client did not catch missing handshake cipher in ServerInit");
    } catch (AlertException e) {
      // success
    }
    assertEquals(State.ERROR, client.getHandshakeState());

    // Get ServerInit
    client = Ukey2Handshake.forInitiator(HandshakeCipher.P256_SHA512);
    server = Ukey2Handshake.forResponder(HandshakeCipher.P256_SHA512);
    handshakeMessage = client.getNextHandshakeMessage();
    server.parseHandshakeMessage(handshakeMessage);
    handshakeMessage = server.getNextHandshakeMessage();
    message = Ukey2Message.newBuilder(Ukey2Message.parseFrom(handshakeMessage));
    serverInit = Ukey2ServerInit.parseFrom(message.getMessageData());

    // put in a bad handshake cipher
    badServerInit = Ukey2ServerInit.newBuilder()
        .setPublicKey(serverInit.getPublicKey())
        .setRandom(serverInit.getRandom())
        .setVersion(serverInit.getVersion())
        .setHandshakeCipher(UkeyProto.Ukey2HandshakeCipher.RESERVED)
        .build();

    message.setMessageData(ByteString.copyFrom(badServerInit.toByteArray()));
    handshakeMessage = message.build().toByteArray();

    try {
      client.parseHandshakeMessage(handshakeMessage);
      fail("Client did not catch bad handshake cipher in ServerInit");
    } catch (AlertException e) {
      // success
    }
    assertEquals(State.ERROR, client.getHandshakeState());
  }

  /**
   * Test that client catches missing or incorrect public key in serverInit
   */
  public void testMissingOrIncorrectPublicKeyInServerInit() throws Exception {
    if (KeyEncoding.isLegacyCryptoRequired()) {
      // this means we're running on an old SDK, which doesn't support the
      // necessary crypto. Let's not test anything in this case.
      return;
    }

    // Get ServerInit
    Ukey2Handshake client = Ukey2Handshake.forInitiator(HandshakeCipher.P256_SHA512);
    Ukey2Handshake server = Ukey2Handshake.forResponder(HandshakeCipher.P256_SHA512);
    byte[] handshakeMessage = client.getNextHandshakeMessage();
    server.parseHandshakeMessage(handshakeMessage);
    handshakeMessage = server.getNextHandshakeMessage();
    Ukey2Message.Builder message = Ukey2Message.newBuilder(
        Ukey2Message.parseFrom(handshakeMessage));
    Ukey2ServerInit serverInit = Ukey2ServerInit.parseFrom(message.getMessageData());

    // remove public key
    Ukey2ServerInit badServerInit = Ukey2ServerInit.newBuilder()
        .setRandom(serverInit.getRandom())
        .setVersion(serverInit.getVersion())
        .setHandshakeCipher(serverInit.getHandshakeCipher())
        .build();

    message.setMessageData(ByteString.copyFrom(badServerInit.toByteArray()));
    handshakeMessage = message.build().toByteArray();

    try {
      client.parseHandshakeMessage(handshakeMessage);
      fail("Client did not catch missing public key in ServerInit");
    } catch (AlertException e) {
      // success
    }
    assertEquals(State.ERROR, client.getHandshakeState());

    // Get ServerInit
    client = Ukey2Handshake.forInitiator(HandshakeCipher.P256_SHA512);
    server = Ukey2Handshake.forResponder(HandshakeCipher.P256_SHA512);
    handshakeMessage = client.getNextHandshakeMessage();
    server.parseHandshakeMessage(handshakeMessage);
    handshakeMessage = server.getNextHandshakeMessage();
    message = Ukey2Message.newBuilder(
        Ukey2Message.parseFrom(handshakeMessage));
    serverInit = Ukey2ServerInit.parseFrom(message.getMessageData());

    // put in a bad public key
    badServerInit = Ukey2ServerInit.newBuilder()
        .setPublicKey(ByteString.copyFrom(new byte[] {42, 12, 1}))
        .setRandom(serverInit.getRandom())
        .setVersion(serverInit.getVersion())
        .setHandshakeCipher(serverInit.getHandshakeCipher())
        .build();

    message.setMessageData(ByteString.copyFrom(badServerInit.toByteArray()));
    handshakeMessage = message.build().toByteArray();

    try {
      client.parseHandshakeMessage(handshakeMessage);
      fail("Client did not catch bad public key in ServerInit");
    } catch (AlertException e) {
      // success
    }
    assertEquals(State.ERROR, client.getHandshakeState());
  }

  /**
   * Test that client catches missing or incorrect public key in clientFinished
   */
  public void testMissingOrIncorrectPublicKeyInClientFinished() throws Exception {
    if (KeyEncoding.isLegacyCryptoRequired()) {
      // this means we're running on an old SDK, which doesn't support the
      // necessary crypto. Let's not test anything in this case.
      return;
    }

    // Get ClientFinished
    Ukey2Handshake client = Ukey2Handshake.forInitiator(HandshakeCipher.P256_SHA512);
    Ukey2Handshake server = Ukey2Handshake.forResponder(HandshakeCipher.P256_SHA512);
    byte[] handshakeMessage = client.getNextHandshakeMessage();
    server.parseHandshakeMessage(handshakeMessage);
    handshakeMessage = server.getNextHandshakeMessage();
    client.parseHandshakeMessage(handshakeMessage);
    handshakeMessage = client.getNextHandshakeMessage();
    Ukey2Message.Builder message = Ukey2Message.newBuilder(
        Ukey2Message.parseFrom(handshakeMessage));

    // remove public key
    Ukey2ClientFinished.Builder badClientFinished = Ukey2ClientFinished.newBuilder();

    message.setMessageData(ByteString.copyFrom(badClientFinished.build().toByteArray()));
    handshakeMessage = message.build().toByteArray();

    try {
      server.parseHandshakeMessage(handshakeMessage);
      fail("Server did not catch missing public key in ClientFinished");
    } catch (HandshakeException e) {
      // success
    }
    assertEquals(State.ERROR, server.getHandshakeState());

    // Get ClientFinished
    client = Ukey2Handshake.forInitiator(HandshakeCipher.P256_SHA512);
    server = Ukey2Handshake.forResponder(HandshakeCipher.P256_SHA512);
    handshakeMessage = client.getNextHandshakeMessage();
    server.parseHandshakeMessage(handshakeMessage);
    handshakeMessage = server.getNextHandshakeMessage();
    client.parseHandshakeMessage(handshakeMessage);
    handshakeMessage = client.getNextHandshakeMessage();
    message = Ukey2Message.newBuilder(Ukey2Message.parseFrom(handshakeMessage));

    // remove public key
    badClientFinished = Ukey2ClientFinished.newBuilder()
        .setPublicKey(ByteString.copyFrom(new byte[] {42, 12, 1}));

    message.setMessageData(ByteString.copyFrom(badClientFinished.build().toByteArray()));
    handshakeMessage = message.build().toByteArray();

    try {
      server.parseHandshakeMessage(handshakeMessage);
      fail("Server did not catch bad public key in ClientFinished");
    } catch (HandshakeException e) {
      // success
    }
    assertEquals(State.ERROR, server.getHandshakeState());
  }

  /**
   * Tests that items (nonces, commitments, public keys) that should be random are at least
   * different on every run.
   */
  public void testRandomItemsDifferentOnEveryRun() throws Exception {
    if (KeyEncoding.isLegacyCryptoRequired()) {
      // this means we're running on an old SDK, which doesn't support the
      // necessary crypto. Let's not test anything in this case.
      return;
    }

    int numberOfRuns = 50;

    // Search for collisions
    Set<Integer> commitments = new HashSet<>(numberOfRuns);
    Set<Integer> clientNonces = new HashSet<>(numberOfRuns);
    Set<Integer> serverNonces = new HashSet<>(numberOfRuns);
    Set<Integer> serverPublicKeys = new HashSet<>(numberOfRuns);
    Set<Integer> clientPublicKeys = new HashSet<>(numberOfRuns);

    for (int i = 0; i < numberOfRuns; i++) {
      Ukey2Handshake client = Ukey2Handshake.forInitiator(HandshakeCipher.P256_SHA512);
      Ukey2Handshake server = Ukey2Handshake.forResponder(HandshakeCipher.P256_SHA512);
      byte[] handshakeMessage = client.getNextHandshakeMessage();
      Ukey2Message message = Ukey2Message.parseFrom(handshakeMessage);
      Ukey2ClientInit clientInit = Ukey2ClientInit.parseFrom(message.getMessageData());

      server.parseHandshakeMessage(handshakeMessage);
      handshakeMessage = server.getNextHandshakeMessage();
      message = Ukey2Message.parseFrom(handshakeMessage);
      Ukey2ServerInit serverInit = Ukey2ServerInit.parseFrom(message.getMessageData());

      client.parseHandshakeMessage(handshakeMessage);
      handshakeMessage = client.getNextHandshakeMessage();
      message = Ukey2Message.parseFrom(handshakeMessage);
      Ukey2ClientFinished clientFinished = Ukey2ClientFinished.parseFrom(message.getMessageData());

      // Clean up to save some memory (b/32054837)
      client = null;
      server = null;
      handshakeMessage = null;
      message = null;
      System.gc();

      // ClientInit randomness
      Integer nonceHash = Integer.valueOf(Arrays.hashCode(clientInit.getRandom().toByteArray()));
      if (clientNonces.contains(nonceHash) || serverNonces.contains(nonceHash)) {
        fail("Nonce in ClientINit has repeated!");
      }
      clientNonces.add(nonceHash);

      Integer commitmentHash = 0;
      for (CipherCommitment commitement : clientInit.getCipherCommitmentsList()) {
        commitmentHash += Arrays.hashCode(commitement.toByteArray());
      }
      if (commitments.contains(nonceHash)) {
        fail("Commitment has repeated!");
      }
      commitments.add(commitmentHash);

      // ServerInit randomness
      nonceHash = Integer.valueOf(Arrays.hashCode(serverInit.getRandom().toByteArray()));
      if (serverNonces.contains(nonceHash) || clientNonces.contains(nonceHash)) {
        fail("Nonce in ServerInit repeated!");
      }
      serverNonces.add(nonceHash);

      Integer publicKeyHash =
          Integer.valueOf(Arrays.hashCode(serverInit.getPublicKey().toByteArray()));
      if (serverPublicKeys.contains(publicKeyHash) || clientPublicKeys.contains(publicKeyHash)) {
        fail("Public Key in ServerInit repeated!");
      }
      serverPublicKeys.add(publicKeyHash);

      // Client Finished randomness
      publicKeyHash = Integer.valueOf(Arrays.hashCode(clientFinished.getPublicKey().toByteArray()));
      if (serverPublicKeys.contains(publicKeyHash) || clientPublicKeys.contains(publicKeyHash)) {
        fail("Public Key in ClientFinished repeated!");
      }
      clientPublicKeys.add(publicKeyHash);
    }
  }

  /**
   * Tests that {@link Ukey2Handshake#getVerificationString(int)} enforces sane verification string
   * lengths.
   */
  public void testGetVerificationEnforcesSaneLengths() throws Exception {
    if (KeyEncoding.isLegacyCryptoRequired()) {
      // this means we're running on an old SDK, which doesn't support the
      // necessary crypto. Let's not test anything in this case.
      return;
    }

    // Run the protocol
    Ukey2Handshake client = Ukey2Handshake.forInitiator(HandshakeCipher.P256_SHA512);
    Ukey2Handshake server = Ukey2Handshake.forResponder(HandshakeCipher.P256_SHA512);
    byte[] handshakeMessage = client.getNextHandshakeMessage();
    server.parseHandshakeMessage(handshakeMessage);
    handshakeMessage = server.getNextHandshakeMessage();
    client.parseHandshakeMessage(handshakeMessage);
    handshakeMessage = client.getNextHandshakeMessage();
    server.parseHandshakeMessage(handshakeMessage);

    // Try to get too short verification string
    try {
      client.getVerificationString(0);
      fail("Too short verification string allowed");
    } catch (IllegalArgumentException e) {
      // success
    }

    // Try to get too long verification string
    try {
      server.getVerificationString(MAX_AUTH_STRING_LENGTH + 1);
      fail("Too long verification string allowed");
    } catch (IllegalArgumentException e) {
      // success
    }
  }

  /**
   * Asserts that the given client and server contexts are compatible
   */
  private void assertContextsCompatible(
      D2DConnectionContext clientContext, D2DConnectionContext serverContext) {
    assertNotNull(clientContext);
    assertNotNull(serverContext);
    assertEquals(D2DConnectionContextV1.PROTOCOL_VERSION, clientContext.getProtocolVersion());
    assertEquals(D2DConnectionContextV1.PROTOCOL_VERSION, serverContext.getProtocolVersion());
    assertEquals(clientContext.getEncodeKey(), serverContext.getDecodeKey());
    assertEquals(clientContext.getDecodeKey(), serverContext.getEncodeKey());
    assertFalse(clientContext.getEncodeKey().equals(clientContext.getDecodeKey()));
    assertEquals(0, clientContext.getSequenceNumberForEncoding());
    assertEquals(0, clientContext.getSequenceNumberForDecoding());
    assertEquals(0, serverContext.getSequenceNumberForEncoding());
    assertEquals(0, serverContext.getSequenceNumberForDecoding());
  }
}
