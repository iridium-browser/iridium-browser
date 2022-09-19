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

import com.google.security.cryptauth.lib.securegcm.DeviceToDeviceMessagesProto.InitiatorHello;
import com.google.security.cryptauth.lib.securegcm.DeviceToDeviceMessagesProto.ResponderHello;
import com.google.security.cryptauth.lib.securegcm.TransportCryptoOps.Payload;
import com.google.security.cryptauth.lib.securegcm.TransportCryptoOps.PayloadType;
import com.google.security.cryptauth.lib.securemessage.PublicKeyProtoUtil;
import java.nio.charset.Charset;
import java.security.KeyPair;
import java.security.PublicKey;
import java.security.SignatureException;
import javax.crypto.SecretKey;
import junit.framework.TestCase;
import org.junit.Assert;

/**
 * Android compatible tests for the {@link D2DDiffieHellmanKeyExchangeHandshake} class.
 */
public class D2DDiffieHellmanKeyExchangeHandshakeTest extends TestCase {

  private static final byte[] RESPONDER_HELLO_MESSAGE =
      "first payload".getBytes(Charset.forName("UTF-8"));

  private static final String PING = "ping";

  @Override
  protected void setUp() throws Exception {
    KeyEncodingTest.installSunEcSecurityProviderIfNecessary();
    super.setUp();
  }

  public void testHandshakeWithPayload() throws Exception {

    if (KeyEncoding.isLegacyCryptoRequired()) {
      // this means we're running on an old SDK, which doesn't support the
      // necessary crypto. Let's not test anything in this case.
      return;
    }

    // initiator:
    D2DHandshakeContext initiatorHandshakeContext =
        D2DDiffieHellmanKeyExchangeHandshake.forInitiator();
    assertFalse(initiatorHandshakeContext.canSendPayloadInHandshakeMessage());
    assertFalse(initiatorHandshakeContext.isHandshakeComplete());
    byte[] initiatorHello = initiatorHandshakeContext.getNextHandshakeMessage();
    assertFalse(initiatorHandshakeContext.isHandshakeComplete());
    // (send initiatorHello to responder)

    // responder:
    D2DHandshakeContext responderHandshakeContext =
        D2DDiffieHellmanKeyExchangeHandshake.forResponder();
    byte[] payload = responderHandshakeContext.parseHandshakeMessage(initiatorHello);
    assertEquals(0, payload.length);
    assertTrue(responderHandshakeContext.canSendPayloadInHandshakeMessage());
    assertFalse(responderHandshakeContext.isHandshakeComplete());
    byte[] responderHelloAndPayload = responderHandshakeContext.getNextHandshakeMessage(
        RESPONDER_HELLO_MESSAGE);
    assertTrue(responderHandshakeContext.isHandshakeComplete());
    D2DConnectionContext responderCtx = responderHandshakeContext.toConnectionContext();
    // (send responderHelloAndPayload to initiator)

    // initiator
    byte[] messageFromPayload =
        initiatorHandshakeContext.parseHandshakeMessage(responderHelloAndPayload);
    Assert.assertArrayEquals(RESPONDER_HELLO_MESSAGE, messageFromPayload);
    assertTrue(initiatorHandshakeContext.isHandshakeComplete());
    D2DConnectionContextV1 initiatorCtx =
        (D2DConnectionContextV1) initiatorHandshakeContext.toConnectionContext();

    // Test that that initiator and responder contexts are initialized correctly.
    checkInitializedConnectionContexts(initiatorCtx, responderCtx);
  }

  public void testHandshakeWithoutPayload() throws Exception {

    if (KeyEncoding.isLegacyCryptoRequired()) {
      // this means we're running on an old SDK, which doesn't support the
      // necessary crypto. Let's not test anything in this case.
      return;
    }

    // initiator:
    D2DHandshakeContext initiatorHandshakeContext =
        D2DDiffieHellmanKeyExchangeHandshake.forInitiator();
    byte[] initiatorHello = initiatorHandshakeContext.getNextHandshakeMessage();
    // (send initiatorHello to responder)

    // responder:
    D2DHandshakeContext responderHandshakeContext =
        D2DDiffieHellmanKeyExchangeHandshake.forResponder();
    responderHandshakeContext.parseHandshakeMessage(initiatorHello);
    byte[] responderHelloAndPayload = responderHandshakeContext.getNextHandshakeMessage();
    assertTrue(responderHandshakeContext.isHandshakeComplete());
    D2DConnectionContext responderCtx = responderHandshakeContext.toConnectionContext();
    // (send responderHelloAndPayload to initiator)

    // initiator
    byte[] messageFromPayload =
        initiatorHandshakeContext.parseHandshakeMessage(responderHelloAndPayload);
    assertEquals(0, messageFromPayload.length);
    assertTrue(initiatorHandshakeContext.isHandshakeComplete());
    D2DConnectionContext initiatorCtx = initiatorHandshakeContext.toConnectionContext();

    // Test that that initiator and responder contexts are initialized correctly.
    checkInitializedConnectionContexts(initiatorCtx, responderCtx);
  }

  public void testErrorWhenInitiatorOrResponderSendTwice() throws Exception {

    if (KeyEncoding.isLegacyCryptoRequired()) {
      // this means we're running on an old SDK, which doesn't support the
      // necessary crypto. Let's not test anything in this case.
      return;
    }

    // initiator:
    D2DHandshakeContext initiatorHandshakeContext =
        D2DDiffieHellmanKeyExchangeHandshake.forInitiator();
    byte[] initiatorHello = initiatorHandshakeContext.getNextHandshakeMessage();
    try {
      initiatorHandshakeContext.getNextHandshakeMessage();
      fail("Expected error as initiator has no more initiator messages to send");
    } catch (HandshakeException expected) {
      assertTrue(expected.getMessage().contains("Cannot get next message"));
    }
    // (send initiatorHello to responder)

    // responder:
    D2DHandshakeContext responderHandshakeContext =
        D2DDiffieHellmanKeyExchangeHandshake.forResponder();
    responderHandshakeContext.parseHandshakeMessage(initiatorHello);
    responderHandshakeContext.getNextHandshakeMessage();
    try {
      responderHandshakeContext.getNextHandshakeMessage();
      fail("Expected error as initiator has no more responder messages to send");
    } catch (HandshakeException expected) {
      assertTrue(expected.getMessage().contains("Cannot get"));
    }
  }

  public void testInitiatorOrResponderFailOnEmptyMessage() throws Exception {

    if (KeyEncoding.isLegacyCryptoRequired()) {
      // this means we're running on an old SDK, which doesn't support the
      // necessary crypto. Let's not test anything in this case.
      return;
    }

    D2DHandshakeContext handshakeContext =
        D2DDiffieHellmanKeyExchangeHandshake.forInitiator();
    try {
      handshakeContext.parseHandshakeMessage(null);
      fail("Expected to crash on null message");
    } catch (HandshakeException expected) {
      assertTrue(expected.getMessage().contains("short"));
    }
    try {
      handshakeContext.parseHandshakeMessage(new byte[0]);
      fail("Expected to crash on empty message");
    } catch (HandshakeException expected) {
      assertTrue(expected.getMessage().contains("short"));
    }
  }

  public void testPrematureConversionToConnection() throws Exception {
    if (KeyEncoding.isLegacyCryptoRequired()) {
      // this means we're running on an old SDK, which doesn't support the
      // necessary crypto. Let's not test anything in this case.
      return;
    }

    // initiator:
    D2DHandshakeContext initiatorHandshakeContext =
        D2DDiffieHellmanKeyExchangeHandshake.forInitiator();
    try {
      initiatorHandshakeContext.toConnectionContext();
      fail("Expected to crash: initiator hasn't done anything to deserve full connection");
    } catch (HandshakeException expected) {
      assertTrue(expected.getMessage().contains("not complete"));
    }

    byte[] initiatorHello = initiatorHandshakeContext.getNextHandshakeMessage();
    try {
      initiatorHandshakeContext.toConnectionContext();
      fail("Expected to crash: initiator hasn't yet received responder's key");
    } catch (HandshakeException expected) {
      assertTrue(expected.getMessage().contains("not complete"));
    }
    // (send initiatorHello to responder)

    // responder:
    D2DHandshakeContext responderHandshakeContext =
        D2DDiffieHellmanKeyExchangeHandshake.forResponder();
    responderHandshakeContext.parseHandshakeMessage(initiatorHello);
    try {
      initiatorHandshakeContext.toConnectionContext();
      fail("Expected to crash: responder hasn't yet send their key");
    } catch (HandshakeException expected) {
      assertTrue(expected.getMessage().contains("not complete"));
    }
  }

  public void testCannotReuseHandshakeContext() throws Exception {

    if (KeyEncoding.isLegacyCryptoRequired()) {
      // this means we're running on an old SDK, which doesn't support the
      // necessary crypto. Let's not test anything in this case.
      return;
    }

    // initiator:
    D2DHandshakeContext initiatorHandshakeContext =
        D2DDiffieHellmanKeyExchangeHandshake.forInitiator();
    byte[] initiatorHello = initiatorHandshakeContext.getNextHandshakeMessage();
    // (send initiatorHello to responder)

    // responder:
    D2DHandshakeContext responderHandshakeContext =
        D2DDiffieHellmanKeyExchangeHandshake.forResponder();
    responderHandshakeContext.parseHandshakeMessage(initiatorHello);
    byte[] responderHelloAndPayload = responderHandshakeContext.getNextHandshakeMessage();
    D2DConnectionContext responderCtx = responderHandshakeContext.toConnectionContext();
    // (send responderHelloAndPayload to initiator)

    // initiator
    initiatorHandshakeContext.parseHandshakeMessage(responderHelloAndPayload);
    D2DConnectionContext initiatorCtx = initiatorHandshakeContext.toConnectionContext();

    // Test that that initiator and responder contexts are initialized correctly.
    checkInitializedConnectionContexts(initiatorCtx, responderCtx);

    // Try to get another full context
    try {
      initiatorHandshakeContext.toConnectionContext();
      fail("Expected crash: initiator context has already been used");
    } catch (HandshakeException expected) {
      assertTrue(expected.getMessage().contains("used"));
    }
    try {
      responderHandshakeContext.toConnectionContext();
      fail("Expected crash: responder context has already been used");
    } catch (HandshakeException expected) {
      assertTrue(expected.getMessage().contains("used"));
    }
  }

  public void testErrorWhenInitiatorEchosResponderHello() throws Exception {
    if (KeyEncoding.isLegacyCryptoRequired()) {
      // this means we're running on an old SDK, which doesn't support the
      // necessary crypto. Let's not test anything in this case.
      return;
    }

    // Initiator echoing back responder's first packet:
    D2DDiffieHellmanKeyExchangeHandshake partialInitiatorContext =
        D2DDiffieHellmanKeyExchangeHandshake.forInitiator();
    byte[] initiatorHello = partialInitiatorContext.getNextHandshakeMessage();

    D2DDiffieHellmanKeyExchangeHandshake partialResponderCtx =
        D2DDiffieHellmanKeyExchangeHandshake.forResponder();
    partialResponderCtx.parseHandshakeMessage(initiatorHello);
    byte[] responderHelloAndPayload =
        partialResponderCtx.getNextHandshakeMessage(RESPONDER_HELLO_MESSAGE);
    D2DConnectionContext responderCtx = partialResponderCtx.toConnectionContext();

    try {
      // initiator sends responderHelloAndPayload to responder
      responderCtx.decodeMessageFromPeerAsString(responderHelloAndPayload);
      fail("expected exception, but didn't get it");
    } catch (SignatureException expected) {
      assertTrue(expected.getMessage().contains("Signature failed verification"));
    }
  }

  public void testErrorWhenInitiatorResendsMessage() throws Exception {
    if (KeyEncoding.isLegacyCryptoRequired()) {
      // this means we're running on an old SDK, which doesn't support the
      // necessary crypto. Let's not test anything in this case.
      return;
    }

    // Initiator repeating the same packet twice
    D2DDiffieHellmanKeyExchangeHandshake partialInitiatorContext =
        D2DDiffieHellmanKeyExchangeHandshake.forInitiator();
    byte[] initiatorHello = partialInitiatorContext.getNextHandshakeMessage();

    D2DDiffieHellmanKeyExchangeHandshake partialResponderCtx =
        D2DDiffieHellmanKeyExchangeHandshake.forResponder();
    partialResponderCtx.parseHandshakeMessage(initiatorHello);
    byte[] responderHelloAndPayload =
        partialResponderCtx.getNextHandshakeMessage(RESPONDER_HELLO_MESSAGE);
    D2DConnectionContext responderCtx = partialResponderCtx.toConnectionContext();

    partialInitiatorContext.parseHandshakeMessage(responderHelloAndPayload);
    D2DConnectionContext initiatorCtx = partialInitiatorContext.toConnectionContext();

    byte[] pingMessage = initiatorCtx.encodeMessageToPeer(PING);
    assertEquals(PING, responderCtx.decodeMessageFromPeerAsString(pingMessage));

    try {
      // send pingMessage to responder again
      responderCtx.decodeMessageFromPeerAsString(pingMessage);
      fail("expected exception, but didn't get it");
    } catch (SignatureException expected) {
      assertTrue(expected.getMessage().contains("sequence"));
    }
  }

  public void testErrorWhenResponderResendsFirstMessage() throws Exception {
    if (KeyEncoding.isLegacyCryptoRequired()) {
      // this means we're running on an old SDK, which doesn't support the
      // necessary crypto. Let's not test anything in this case.
      return;
    }

    D2DDiffieHellmanKeyExchangeHandshake partialInitiatorContext =
        D2DDiffieHellmanKeyExchangeHandshake.forInitiator();
    byte[] initiatorHello = partialInitiatorContext.getNextHandshakeMessage();

    D2DDiffieHellmanKeyExchangeHandshake partialResponderCtx =
        D2DDiffieHellmanKeyExchangeHandshake.forResponder();
    partialResponderCtx.parseHandshakeMessage(initiatorHello);
    byte[] responderHelloAndPayload =
        partialResponderCtx.getNextHandshakeMessage(RESPONDER_HELLO_MESSAGE);

    partialInitiatorContext.parseHandshakeMessage(responderHelloAndPayload);
    D2DConnectionContext initiatorCtx = partialInitiatorContext.toConnectionContext();

    try {
      // Send the responderHelloAndPayload again. This time, the initiator will
      // process it as a normal message.
      initiatorCtx.decodeMessageFromPeerAsString(responderHelloAndPayload);
      fail("expected exception, but didn't get it");
    } catch (SignatureException expected) {
      assertTrue(expected.getMessage().contains("wrong message type"));
    }
  }

  public void testHandshakeWithInitiatorV1AndResponderV0() throws Exception {
    if (KeyEncoding.isLegacyCryptoRequired()) {
      // this means we're running on an old SDK, which doesn't support the
      // necessary crypto. Let's not test anything in this case.
      return;
    }

    // Initialize initiator side.
    D2DHandshakeContext initiatorHandshakeContext =
        D2DDiffieHellmanKeyExchangeHandshake.forInitiator();
    byte[] initiatorHello = initiatorHandshakeContext.getNextHandshakeMessage();

    // Set up keys used by the responder.
    PublicKey initiatorPublicKey = PublicKeyProtoUtil.parsePublicKey(
        InitiatorHello.parseFrom(initiatorHello).getPublicDhKey());
    KeyPair responderKeyPair = PublicKeyProtoUtil.generateEcP256KeyPair();
    SecretKey sharedKey =
        EnrollmentCryptoOps.doKeyAgreement(responderKeyPair.getPrivate(), initiatorPublicKey);

    // Construct a responder hello message without the version field, whose payload is encrypted
    // with the shared key.
    byte[] responderHello = D2DCryptoOps.signcryptPayload(
          new Payload(
              PayloadType.DEVICE_TO_DEVICE_RESPONDER_HELLO_PAYLOAD,
              D2DConnectionContext.createDeviceToDeviceMessage(new byte[] {}, 1).toByteArray()),
          sharedKey,
          ResponderHello.newBuilder()
              .setPublicDhKey(
                  PublicKeyProtoUtil.encodePublicKey(responderKeyPair.getPublic()))
              .build().toByteArray());

    // Handle V0 responder hello message.
    initiatorHandshakeContext.parseHandshakeMessage(responderHello);
    D2DConnectionContext initiatorCtx = initiatorHandshakeContext.toConnectionContext();

    assertEquals(D2DConnectionContextV0.PROTOCOL_VERSION, initiatorCtx.getProtocolVersion());
    assertEquals(1, initiatorCtx.getSequenceNumberForEncoding());
    assertEquals(1, initiatorCtx.getSequenceNumberForDecoding());
  }

  public void testHandshakeWithInitiatorV0AndResponderV1() throws Exception {
    if (KeyEncoding.isLegacyCryptoRequired()) {
      // this means we're running on an old SDK, which doesn't support the
      // necessary crypto. Let's not test anything in this case.
      return;
    }

    // Construct an initiator hello message without the version field.
    byte[] initiatorHello = InitiatorHello.newBuilder()
        .setPublicDhKey(PublicKeyProtoUtil.encodePublicKey(
            PublicKeyProtoUtil.generateEcP256KeyPair().getPublic()))
        .build()
        .toByteArray();

    // Handle V0 initiator hello message.
    D2DHandshakeContext responderHandshakeContext =
        D2DDiffieHellmanKeyExchangeHandshake.forResponder();
    responderHandshakeContext.parseHandshakeMessage(initiatorHello);
    responderHandshakeContext.getNextHandshakeMessage();
    D2DConnectionContext responderCtx = responderHandshakeContext.toConnectionContext();

    assertEquals(D2DConnectionContextV0.PROTOCOL_VERSION, responderCtx.getProtocolVersion());
    assertEquals(1, responderCtx.getSequenceNumberForEncoding());
    assertEquals(1, responderCtx.getSequenceNumberForDecoding());
  }

  private void checkInitializedConnectionContexts(
      D2DConnectionContext initiatorCtx, D2DConnectionContext responderCtx) {
    assertNotNull(initiatorCtx);
    assertNotNull(responderCtx);
    assertEquals(D2DConnectionContextV1.PROTOCOL_VERSION, initiatorCtx.getProtocolVersion());
    assertEquals(D2DConnectionContextV1.PROTOCOL_VERSION, responderCtx.getProtocolVersion());
    assertEquals(initiatorCtx.getEncodeKey(), responderCtx.getDecodeKey());
    assertEquals(initiatorCtx.getDecodeKey(), responderCtx.getEncodeKey());
    assertEquals(0, initiatorCtx.getSequenceNumberForEncoding());
    assertEquals(1, initiatorCtx.getSequenceNumberForDecoding());
    assertEquals(1, responderCtx.getSequenceNumberForEncoding());
    assertEquals(0, responderCtx.getSequenceNumberForDecoding());
  }
}
