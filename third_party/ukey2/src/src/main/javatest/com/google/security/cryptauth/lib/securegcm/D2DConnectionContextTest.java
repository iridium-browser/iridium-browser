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

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import java.security.SignatureException;
import java.util.Arrays;
import javax.crypto.SecretKey;
import javax.crypto.spec.SecretKeySpec;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/**
 * Base class for Android compatible tests for {@link D2DConnectionContext} subclasses.
 * Note: We would use a Parameterized test runner to test different versions, but this
 * functionality is not supported by Android tests.
 */
@RunWith(JUnit4.class)
public class D2DConnectionContextTest {
  private static final String PING = "ping";
  private static final String PONG = "pong";

  // Key is: "initiator_encode_key_for_aes_256"
  private static final SecretKey INITIATOR_ENCODE_KEY = new SecretKeySpec(
      new byte[] {
          (byte) 0x69, (byte) 0x6e, (byte) 0x69, (byte) 0x74, (byte) 0x69, (byte) 0x61, (byte) 0x74,
          (byte) 0x6f, (byte) 0x72, (byte) 0x5f, (byte) 0x65, (byte) 0x6e, (byte) 0x63, (byte) 0x6f,
          (byte) 0x64, (byte) 0x65, (byte) 0x5f, (byte) 0x6b, (byte) 0x65, (byte) 0x79, (byte) 0x5f,
          (byte) 0x66, (byte) 0x6f, (byte) 0x72, (byte) 0x5f, (byte) 0x61, (byte) 0x65, (byte) 0x73,
          (byte) 0x5f, (byte) 0x32, (byte) 0x35, (byte) 0x36
      },
      "AES");

  // Key is: "initiator_decode_key_for_aes_256"
  private static final SecretKey INITIATOR_DECODE_KEY = new SecretKeySpec(
      new byte[] {
          (byte) 0x69, (byte) 0x6e, (byte) 0x69, (byte) 0x74, (byte) 0x69, (byte) 0x61, (byte) 0x74,
          (byte) 0x6f, (byte) 0x72, (byte) 0x5f, (byte) 0x64, (byte) 0x65, (byte) 0x63, (byte) 0x6f,
          (byte) 0x64, (byte) 0x65, (byte) 0x5f, (byte) 0x6b, (byte) 0x65, (byte) 0x79, (byte) 0x5f,
          (byte) 0x66, (byte) 0x6f, (byte) 0x72, (byte) 0x5f, (byte) 0x61, (byte) 0x65, (byte) 0x73,
          (byte) 0x5f, (byte) 0x32, (byte) 0x35, (byte) 0x36
      },
      "AES");

  private D2DConnectionContext initiatorCtx;
  private D2DConnectionContext responderCtx;

  @Before
  public void setUp() throws Exception {
    KeyEncodingTest.installSunEcSecurityProviderIfNecessary();
  }

  protected void testPeerToPeerProtocol(int protocolVersion) throws Exception {

    if (KeyEncoding.isLegacyCryptoRequired()) {
      // this means we're running on an old SDK, which doesn't support the
      // necessary crypto. Let's not test anything in this case.
      return;
    }

    initiatorCtx = createConnectionContext(protocolVersion, true /** isInitiator */);
    responderCtx = createConnectionContext(protocolVersion, false /** isInitiator */);

    byte[] pingMessage = initiatorCtx.encodeMessageToPeer(PING);
    // (send message to responder)

    // responder
    String messageStr = responderCtx.decodeMessageFromPeerAsString(pingMessage);
    assertEquals(PING, messageStr);

    byte[] pongMessage = responderCtx.encodeMessageToPeer(PONG);
    // (send message to initiator)

    // initiator
    messageStr = initiatorCtx.decodeMessageFromPeerAsString(pongMessage);
    assertEquals(PONG, messageStr);

    // let's make sure there is actually some crypto involved.
    pingMessage = initiatorCtx.encodeMessageToPeer("can you see this?");
    pingMessage[2] = (byte) (pingMessage[2] + 1); // twiddle with the message
    try {
      responderCtx.decodeMessageFromPeerAsString(pingMessage);
      fail("expected exception, but didn't get it");
    } catch (SignatureException expected) {
      assertTrue(expected.getMessage().contains("failed verification"));
    }

    // Try and replay the previous encoded message to the initiator (replays should not work).
    try {
      initiatorCtx.decodeMessageFromPeerAsString(pongMessage);
      fail("expected exception, but didn't get it");
    } catch (SignatureException expected) {
      assertTrue(expected.getMessage().contains("sequence"));
    }

    assertEquals(protocolVersion, initiatorCtx.getProtocolVersion());
    assertEquals(protocolVersion, responderCtx.getProtocolVersion());
  }

  @Test
  public void testPeerToPeerProtocol_V0() throws Exception {
    testPeerToPeerProtocol(D2DConnectionContextV0.PROTOCOL_VERSION);
  }

  @Test
  public void testPeerToPeerProtocol_V1() throws Exception {
    testPeerToPeerProtocol(D2DConnectionContextV1.PROTOCOL_VERSION);
  }

  protected void testResponderSendsFirst(int protocolVersion) throws Exception {

    if (KeyEncoding.isLegacyCryptoRequired()) {
      // this means we're running on an old SDK, which doesn't support the
      // necessary crypto. Let's not test anything in this case.
      return;
    }

    initiatorCtx = createConnectionContext(protocolVersion, true /** isInitiator */);
    responderCtx = createConnectionContext(protocolVersion, false /** isInitiator */);

    byte[] pongMessage = responderCtx.encodeMessageToPeer(PONG);
    assertEquals(PONG, initiatorCtx.decodeMessageFromPeerAsString(pongMessage));

    pongMessage = responderCtx.encodeMessageToPeer(PONG);
    assertEquals(PONG, initiatorCtx.decodeMessageFromPeerAsString(pongMessage));

    // for good measure, if the initiator now responds, it should also work:
    byte[] pingMessage = initiatorCtx.encodeMessageToPeer(PING);
    assertEquals(PING, responderCtx.decodeMessageFromPeerAsString(pingMessage));

    pingMessage = initiatorCtx.encodeMessageToPeer(PING);
    assertEquals(PING, responderCtx.decodeMessageFromPeerAsString(pingMessage));

    pingMessage = initiatorCtx.encodeMessageToPeer(PING);
    assertEquals(PING, responderCtx.decodeMessageFromPeerAsString(pingMessage));
  }

  @Test
  public void testResponderSendsFirst_V0() throws Exception {
    testResponderSendsFirst(D2DConnectionContextV0.PROTOCOL_VERSION);
  }

  @Test
  public void testResponderSendsFirst_V1() throws Exception {
    testResponderSendsFirst(D2DConnectionContextV1.PROTOCOL_VERSION);
  }

  protected void testAssymmetricFlows(int protocolVersion) throws Exception {

    if (KeyEncoding.isLegacyCryptoRequired()) {
      // this means we're running on an old SDK, which doesn't support the
      // necessary crypto. Let's not test anything in this case.
      return;
    }

    initiatorCtx = createConnectionContext(protocolVersion, true /** isInitiator */);
    responderCtx = createConnectionContext(protocolVersion, false /** isInitiator */);

    // Let's test that this still works if one side sends a few messages in a row.
    byte[] pingMessage = initiatorCtx.encodeMessageToPeer(PING);
    assertEquals(PING, responderCtx.decodeMessageFromPeerAsString(pingMessage));

    pingMessage = initiatorCtx.encodeMessageToPeer(PING);
    assertEquals(PING, responderCtx.decodeMessageFromPeerAsString(pingMessage));

    pingMessage = initiatorCtx.encodeMessageToPeer(PING);
    assertEquals(PING, responderCtx.decodeMessageFromPeerAsString(pingMessage));


    byte[] pongMessage = responderCtx.encodeMessageToPeer(PONG);
    assertEquals(PONG, initiatorCtx.decodeMessageFromPeerAsString(pongMessage));

    pongMessage = responderCtx.encodeMessageToPeer(PONG);
    assertEquals(PONG, initiatorCtx.decodeMessageFromPeerAsString(pongMessage));
  }

  @Test
  public void testAssymmetricFlows_V0() throws Exception {
    testAssymmetricFlows(D2DConnectionContextV0.PROTOCOL_VERSION);
  }

  @Test
  public void testAssymmetricFlows_V1() throws Exception {
    testAssymmetricFlows(D2DConnectionContextV1.PROTOCOL_VERSION);
  }

  public void testErrorWhenResponderResendsMessage(int protocolVersion) throws Exception {
    if (KeyEncoding.isLegacyCryptoRequired()) {
      // this means we're running on an old SDK, which doesn't support the
      // necessary crypto. Let's not test anything in this case.
      return;
    }

    initiatorCtx = createConnectionContext(protocolVersion, true /** isInitiator */);
    responderCtx = createConnectionContext(protocolVersion, false /** isInitiator */);

    byte[] pongMessage = responderCtx.encodeMessageToPeer(PONG);
    assertEquals(PONG, initiatorCtx.decodeMessageFromPeerAsString(pongMessage));

    try {
      // send pongMessage again to the initiator
      initiatorCtx.decodeMessageFromPeerAsString(pongMessage);
      fail("expected exception, but didn't get it");
    } catch (SignatureException expected) {
      assertTrue(expected.getMessage().contains("sequence"));
    }
  }

  @Test
  public void testErrorWhenResponderResendsMessage_V0() throws Exception {
    testErrorWhenResponderResendsMessage(D2DConnectionContextV0.PROTOCOL_VERSION);
  }

  @Test
  public void testErrorWhenResponderResendsMessage_V1() throws Exception {
    testErrorWhenResponderResendsMessage(D2DConnectionContextV1.PROTOCOL_VERSION);
  }

  protected void testErrorWhenResponderEchoesInitiatorMessage(
      int protocolVersion) throws Exception {
    if (KeyEncoding.isLegacyCryptoRequired()) {
      return;
    }

    initiatorCtx = createConnectionContext(protocolVersion, true /** isInitiator */);
    responderCtx = createConnectionContext(protocolVersion, false /** isInitiator */);

    byte[] pingMessage = initiatorCtx.encodeMessageToPeer(PING);
    assertEquals(PING, responderCtx.decodeMessageFromPeerAsString(pingMessage));

    try {
      initiatorCtx.decodeMessageFromPeerAsString(pingMessage);
      fail("expected exception, but didn't get it");
    } catch (SignatureException expected) {
    }
  }

  @Test
  public void testErrorWhenResponderEchoesInitiatorMessage_V0() throws Exception {
    testErrorWhenResponderEchoesInitiatorMessage(D2DConnectionContextV0.PROTOCOL_VERSION);
  }

  @Test
  public void testErrorWhenResponderEchoesInitiatorMessage_V1() throws Exception {
    testErrorWhenResponderEchoesInitiatorMessage(D2DConnectionContextV1.PROTOCOL_VERSION);
  }

  @Test
  public void testErrorUsingV1InitiatorWithV0Responder() throws SignatureException {
    if (KeyEncoding.isLegacyCryptoRequired()) {
      // this means we're running on an old SDK, which doesn't support the
      // necessary crypto. Let's not test anything in this case.
      return;
    }

    initiatorCtx = new D2DConnectionContextV1(INITIATOR_ENCODE_KEY, INITIATOR_DECODE_KEY, 1, 1);
    responderCtx = new D2DConnectionContextV0(INITIATOR_DECODE_KEY, 1);

    // Decoding the responder's message should succeed, because the decode key and sequence numbers
    // match.
    initiatorCtx.decodeMessageFromPeer(responderCtx.encodeMessageToPeer(PING));

    // Responder fails to decodes initiator's encoded message because keys do not match.
    try {
      responderCtx.decodeMessageFromPeer(initiatorCtx.encodeMessageToPeer(PONG));
      fail("Expected verification to fail.");
    } catch (SignatureException e) {
      // Exception expected.
    }
  }

  @Test
  public void testErrorWithV0InitiatorV1Responder() throws SignatureException {
    if (KeyEncoding.isLegacyCryptoRequired()) {
      // this means we're running on an old SDK, which doesn't support the
      // necessary crypto. Let's not test anything in this case.
      return;
    }

    initiatorCtx = new D2DConnectionContextV0(INITIATOR_ENCODE_KEY, 1);
    responderCtx = new D2DConnectionContextV1(INITIATOR_DECODE_KEY, INITIATOR_ENCODE_KEY, 1, 1);

    // Decoding the initiator's message should succeed, because the decode key and sequence numbers
    // match.
    responderCtx.decodeMessageFromPeer(initiatorCtx.encodeMessageToPeer(PING));

    // Initiator fails to decodes responder's encoded message because keys do not match.
    try {
      initiatorCtx.decodeMessageFromPeer(responderCtx.encodeMessageToPeer(PONG));
      fail("Expected verification to fail.");
    } catch (SignatureException e) {
      // Exception expected.
    }
  }

  protected void testSessionUnique(int protocolVersion) throws Exception {
    // Should be the same (we set them up with the same key and sequence number)
    initiatorCtx = createConnectionContext(protocolVersion, true /** isInitiator */);
    responderCtx = createConnectionContext(protocolVersion, false /** isInitiator */);
    Assert.assertArrayEquals(initiatorCtx.getSessionUnique(), responderCtx.getSessionUnique());

    // Change just the key (should not match)
    SecretKey wrongKey = new SecretKeySpec("wrong".getBytes("UTF8"), "AES");
    responderCtx = createConnectionContext(protocolVersion, false, wrongKey, wrongKey, 0, 1);
    assertFalse(Arrays.equals(initiatorCtx.getSessionUnique(), responderCtx.getSessionUnique()));

    // Change just the sequence number (should still match)
    responderCtx = createConnectionContext(
        protocolVersion, false, INITIATOR_ENCODE_KEY, INITIATOR_DECODE_KEY, 2, 2);
    Assert.assertArrayEquals(initiatorCtx.getSessionUnique(), responderCtx.getSessionUnique());
  }

  @Test
  public void testSessionUnique_V0() throws Exception {
    testSessionUnique(D2DConnectionContextV0.PROTOCOL_VERSION);
  }

  @Test
  public void testSessionUnique_V1() throws Exception {
    testSessionUnique(D2DConnectionContextV1.PROTOCOL_VERSION);
  }

  @Test
  public void testSessionUniqueValues_V0() throws Exception {
    // The key and the session unique value should match ones in the equivalent test in
    // @link {cs/Nearby/D2DCrypto/Tests/D2DConnectionContextTest.m}
    byte[] key =
        new byte[] {
          (byte) 0x01, (byte) 0x02, (byte) 0x03, (byte) 0x04, (byte) 0x05, (byte) 0x06, (byte) 0x07,
          (byte) 0x08, (byte) 0x09, (byte) 0x0a, (byte) 0x0b, (byte) 0x0c, (byte) 0x0d, (byte) 0x0e,
          (byte) 0x0f, (byte) 0x10, (byte) 0x11, (byte) 0x12, (byte) 0x13, (byte) 0x14, (byte) 0x15,
          (byte) 0x16, (byte) 0x17, (byte) 0x18, (byte) 0x19, (byte) 0x1a, (byte) 0x1b, (byte) 0x1c,
          (byte) 0x1d, (byte) 0x1e, (byte) 0x1f, (byte) 0x20
        };
    byte[] sessionUnique =
        new byte[] {
          (byte) 0x70, (byte) 0x7a, (byte) 0x17, (byte) 0x27, (byte) 0xa3, (byte) 0x0e, (byte) 0x68,
          (byte) 0x63, (byte) 0x38, (byte) 0xdf, (byte) 0x72, (byte) 0x62, (byte) 0xf4, (byte) 0xb0,
          (byte) 0x41, (byte) 0xac, (byte) 0x75, (byte) 0x8b, (byte) 0xca, (byte) 0x3b, (byte) 0x11,
          (byte) 0xd4, (byte) 0x09, (byte) 0x64, (byte) 0x96, (byte) 0x54, (byte) 0xb4, (byte) 0x9b,
          (byte) 0x43, (byte) 0xe6, (byte) 0x9b, (byte) 0xce
        };

    SecretKey secretKey = new SecretKeySpec(key, "AES");
    D2DConnectionContext context = new D2DConnectionContextV0(secretKey, 1);

    Assert.assertArrayEquals(context.getSessionUnique(), sessionUnique);
  }

  @Test
  public void testSessionUniqueValues_V1_Initiator() throws Exception {
    // The key and the session unique value should match ones in the equivalent test in
    // @link {cs/Nearby/D2DCrypto/Tests/D2DConnectionContextTest.m}
    byte[] sessionUnique =
        new byte[] {
          (byte) 0x91, (byte) 0xc7, (byte) 0xc9, (byte) 0x26, (byte) 0x2c, (byte) 0x17, (byte) 0x8a,
          (byte) 0xa0, (byte) 0x36, (byte) 0x9f, (byte) 0xf2, (byte) 0x05, (byte) 0x20, (byte) 0x98,
          (byte) 0x38, (byte) 0x53, (byte) 0xa5, (byte) 0x46, (byte) 0xab, (byte) 0x3a, (byte) 0x21,
          (byte) 0x3b, (byte) 0x76, (byte) 0x58, (byte) 0x59, (byte) 0x4e, (byte) 0xe7, (byte) 0xe3,
          (byte) 0xc1, (byte) 0x69, (byte) 0x87, (byte) 0xfa
        };

    D2DConnectionContext initiatorContext = new D2DConnectionContextV1(
        INITIATOR_ENCODE_KEY, INITIATOR_DECODE_KEY, 0, 1);
    D2DConnectionContext responderContext = new D2DConnectionContextV1(
        INITIATOR_DECODE_KEY, INITIATOR_ENCODE_KEY, 1, 0);

    // Both the initiator and responder must be the same.
    Assert.assertArrayEquals(initiatorContext.getSessionUnique(), sessionUnique);
    Assert.assertArrayEquals(responderContext.getSessionUnique(), sessionUnique);
  }

  @Test
  public void testSaveSessionV0() throws Exception {
    D2DConnectionContext initiatorCtx = new D2DConnectionContextV0(INITIATOR_ENCODE_KEY, 1);
    D2DConnectionContext responderCtx = new D2DConnectionContextV0(INITIATOR_ENCODE_KEY, 1);

    // Save the state
    byte[] initiatorSavedSessionState = initiatorCtx.saveSession();
    byte[] responderSavedSessionState = responderCtx.saveSession();

    // Try to rebuild the context
    initiatorCtx = D2DConnectionContext.fromSavedSession(initiatorSavedSessionState);
    responderCtx = D2DConnectionContext.fromSavedSession(responderSavedSessionState);

    // Sanity check
    assertEquals(1, initiatorCtx.getSequenceNumberForDecoding());
    assertEquals(1, responderCtx.getSequenceNumberForDecoding());
    Assert.assertArrayEquals(initiatorCtx.getSessionUnique(), responderCtx.getSessionUnique());

    // Make sure they can still talk to one another
    assertEquals(PING,
        responderCtx.decodeMessageFromPeerAsString(initiatorCtx.encodeMessageToPeer(PING)));
    assertEquals(PONG,
        initiatorCtx.decodeMessageFromPeerAsString(responderCtx.encodeMessageToPeer(PONG)));
  }

  @Test
  public void testSaveSessionV0_negativeSeqNumber() throws Exception {
    D2DConnectionContext initiatorCtx = new D2DConnectionContextV0(INITIATOR_ENCODE_KEY, -5);

    // Save the state
    byte[] initiatorSavedSessionState = initiatorCtx.saveSession();

    // Try to rebuild the context
    initiatorCtx = D2DConnectionContext.fromSavedSession(initiatorSavedSessionState);

    // Sanity check
    assertEquals(-5, initiatorCtx.getSequenceNumberForDecoding());
  }

  @Test
  public void testSaveSessionV0_shortKey() throws Exception {
    D2DConnectionContext initiatorCtx = new D2DConnectionContextV0(INITIATOR_ENCODE_KEY, -5);

    // Save the state
    byte[] initiatorSavedSessionState = initiatorCtx.saveSession();

    // Try to rebuild the context
    try {
      D2DConnectionContext.fromSavedSession(Arrays.copyOf(initiatorSavedSessionState,
          initiatorSavedSessionState.length - 1));
      fail("Expected failure as key is too short");
    } catch (IllegalArgumentException e) {
      // expected
    }
  }

  @Test
  public void testSaveSession_unknownProtocolVersion() throws Exception {
    D2DConnectionContext initiatorCtx = new D2DConnectionContextV0(INITIATOR_ENCODE_KEY, -5);

    // Save the state
    byte[] initiatorSavedSessionState = initiatorCtx.saveSession();

    // Mess with the protocol version
    initiatorSavedSessionState[0] = (byte) 0xff;

    // Try to rebuild the context
    try {
      D2DConnectionContext.fromSavedSession(initiatorSavedSessionState);
      fail("Expected failure as 0xff is not a valid protocol version");
    } catch (IllegalArgumentException e) {
      // expected
    }

    // Mess with the protocol version in the other direction
    initiatorSavedSessionState[0] = 2;

    // Try to rebuild the context
    try {
      D2DConnectionContext.fromSavedSession(initiatorSavedSessionState);
      fail("Expected failure as 2 is not a valid protocol version");
    } catch (IllegalArgumentException e) {
      // expected
    }
  }

  @Test
  public void testSaveSessionV1() throws Exception {
    D2DConnectionContext initiatorCtx = new D2DConnectionContextV1(INITIATOR_ENCODE_KEY,
        INITIATOR_DECODE_KEY, 0, 1);
    D2DConnectionContext responderCtx = new D2DConnectionContextV1(INITIATOR_DECODE_KEY,
        INITIATOR_ENCODE_KEY, 1, 0);

    // Save the state
    byte[] initiatorSavedSessionState = initiatorCtx.saveSession();
    byte[] responderSavedSessionState = responderCtx.saveSession();

    // Try to rebuild the context
    initiatorCtx = D2DConnectionContext.fromSavedSession(initiatorSavedSessionState);
    responderCtx = D2DConnectionContext.fromSavedSession(responderSavedSessionState);

    // Sanity check
    assertEquals(1, initiatorCtx.getSequenceNumberForDecoding());
    assertEquals(0, initiatorCtx.getSequenceNumberForEncoding());
    assertEquals(0, responderCtx.getSequenceNumberForDecoding());
    assertEquals(1, responderCtx.getSequenceNumberForEncoding());
    Assert.assertArrayEquals(initiatorCtx.getSessionUnique(), responderCtx.getSessionUnique());

    // Make sure they can still talk to one another
    assertEquals(PING,
        responderCtx.decodeMessageFromPeerAsString(initiatorCtx.encodeMessageToPeer(PING)));
    assertEquals(PONG,
        initiatorCtx.decodeMessageFromPeerAsString(responderCtx.encodeMessageToPeer(PONG)));
  }

  @Test
  public void testSaveSessionV1_negativeSeqNumbers() throws Exception {
    D2DConnectionContext initiatorCtx = new D2DConnectionContextV1(INITIATOR_ENCODE_KEY,
        INITIATOR_DECODE_KEY, -8, -10);

    // Save the state
    byte[] initiatorSavedSessionState = initiatorCtx.saveSession();

    // Try to rebuild the context
    initiatorCtx = D2DConnectionContext.fromSavedSession(initiatorSavedSessionState);

    // Sanity check
    assertEquals(-10, initiatorCtx.getSequenceNumberForDecoding());
    assertEquals(-8, initiatorCtx.getSequenceNumberForEncoding());
  }

  @Test
  public void testSaveSessionV1_tooShort() throws Exception {
    D2DConnectionContext initiatorCtx = new D2DConnectionContextV1(INITIATOR_ENCODE_KEY,
        INITIATOR_DECODE_KEY, -8, -10);

    // Save the state
    byte[] initiatorSavedSessionState = initiatorCtx.saveSession();

    // Try to rebuild the context
    try {
      D2DConnectionContext.fromSavedSession(
          Arrays.copyOf(initiatorSavedSessionState, initiatorSavedSessionState.length - 1));
      fail("Expected error as saved session is too short");
    } catch (IllegalArgumentException e) {
      // expected
    }

    // Sanity check
    assertEquals(-10, initiatorCtx.getSequenceNumberForDecoding());
    assertEquals(-8, initiatorCtx.getSequenceNumberForEncoding());
  }

  D2DConnectionContext createConnectionContext(int protocolVersion, boolean isInitiator) {
    return createConnectionContext(
        protocolVersion, isInitiator, INITIATOR_ENCODE_KEY, INITIATOR_DECODE_KEY, 0, 1);
  }

  D2DConnectionContext createConnectionContext(
      int protocolVersion, boolean isInitiator,
      SecretKey initiatorEncodeKey, SecretKey initiatorDecodeKey,
      int initiatorSequenceNumber, int responderSequenceNumber) {
    if (protocolVersion == D2DConnectionContextV0.PROTOCOL_VERSION) {
      return new D2DConnectionContextV0(initiatorEncodeKey, responderSequenceNumber);
    } else if (protocolVersion == D2DConnectionContextV1.PROTOCOL_VERSION) {
      return isInitiator
          ? new D2DConnectionContextV1(
              initiatorEncodeKey, initiatorDecodeKey,
              initiatorSequenceNumber, responderSequenceNumber)
          : new D2DConnectionContextV1(
              initiatorDecodeKey, initiatorEncodeKey,
              responderSequenceNumber, initiatorSequenceNumber);
    } else {
      throw new IllegalArgumentException("Unknown version: " + protocolVersion);
    }
  }
}
