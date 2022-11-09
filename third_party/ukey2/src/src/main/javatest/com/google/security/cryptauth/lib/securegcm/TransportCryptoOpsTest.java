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

import com.google.security.cryptauth.lib.securegcm.SecureGcmProto.Tickle;
import com.google.security.cryptauth.lib.securegcm.TransportCryptoOps.Payload;
import com.google.security.cryptauth.lib.securegcm.TransportCryptoOps.PayloadType;
import com.google.security.cryptauth.lib.securemessage.PublicKeyProtoUtil;
import java.security.KeyPair;
import java.security.PublicKey;
import java.util.Arrays;
import javax.crypto.SecretKey;
import javax.crypto.spec.SecretKeySpec;
import junit.framework.TestCase;

/**
 * Android compatible tests for the {@link TransportCryptoOps} class.
 */
public class TransportCryptoOpsTest extends TestCase {
  private static final byte[] KEY_BYTES = {
      1, 2, 3, 4, 5, 6, 7, 8, 9, 0,
      1, 2, 3, 4, 5, 6, 7, 8, 9, 0,
      1, 2, 3, 4, 5, 6, 7, 8, 9, 0,
      1, 2
  };
  private static final byte[] KEY_HANDLE = { 9 };

  private SecretKey masterKey;

  @Override
  protected void setUp() throws Exception {
    KeyEncodingTest.installSunEcSecurityProviderIfNecessary();
    masterKey = new SecretKeySpec(KEY_BYTES, "AES");
    super.setUp();
  }

  public void testServerMessage() throws Exception {
    long tickleExpiry = 12345L;
    Tickle tickle = Tickle.newBuilder()
        .setExpiryTime(tickleExpiry)
        .build();

    // Simulate sending a message
    byte[] signcryptedMessage = TransportCryptoOps.signcryptServerMessage(
        new Payload(PayloadType.TICKLE, tickle.toByteArray()),
        masterKey,
        KEY_HANDLE);

    // Simulate the process of receiving the message
    assertTrue(Arrays.equals(KEY_HANDLE, TransportCryptoOps.getKeyHandleFor(signcryptedMessage)));
    Payload received = TransportCryptoOps.verifydecryptServerMessage(signcryptedMessage, masterKey);
    assertEquals(PayloadType.TICKLE, received.getPayloadType());
    Tickle receivedTickle = Tickle.parseFrom(received.getMessage());
    assertEquals(tickleExpiry, receivedTickle.getExpiryTime());
  }

  public void testClientMessage() throws Exception {
    if (PublicKeyProtoUtil.isLegacyCryptoRequired()) {
      return;  // This test isn't for legacy crypto
    }
    KeyPair userKeyPair = PublicKeyProtoUtil.generateEcP256KeyPair();
    doTestClientMessageWith(userKeyPair);
  }

  public void testClientMessageWithLegacyCrypto() throws Exception {
    KeyPair userKeyPair = PublicKeyProtoUtil.generateRSA2048KeyPair();
    doTestClientMessageWith(userKeyPair);
  }

  private void doTestClientMessageWith(KeyPair userKeyPair) throws Exception {
    PublicKey userPublicKey = userKeyPair.getPublic();
    // Will use a Tickle for the test message, even though that would normally
    // only be sent from the server to the client
    long tickleExpiry = 12345L;
    Tickle tickle = Tickle.newBuilder()
        .setExpiryTime(tickleExpiry)
        .build();

    // Simulate sending a message
    byte[] signcryptedMessage = TransportCryptoOps.signcryptClientMessage(
        new Payload(PayloadType.TICKLE, tickle.toByteArray()),
        userKeyPair,
        masterKey);

    // Simulate the process of receiving the message
    byte[] encodedUserPublicKey = TransportCryptoOps.getEncodedUserPublicKeyFor(signcryptedMessage);
    assertTrue(Arrays.equals(KeyEncoding.encodeUserPublicKey(userPublicKey), encodedUserPublicKey));
    userPublicKey = KeyEncoding.parseUserPublicKey(encodedUserPublicKey);
    // At this point the server would have looked up the masterKey for this userPublicKey

    Payload received = TransportCryptoOps.verifydecryptClientMessage(
        signcryptedMessage, userPublicKey, masterKey);

    assertEquals(PayloadType.TICKLE, received.getPayloadType());
    Tickle receivedTickle = Tickle.parseFrom(received.getMessage());
    assertEquals(tickleExpiry, receivedTickle.getExpiryTime());
  }
}
