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
import com.google.security.cryptauth.lib.securegcm.SecureGcmProto.GcmDeviceInfo;
import com.google.security.cryptauth.lib.securemessage.PublicKeyProtoUtil;
import com.google.security.cryptauth.lib.securemessage.SecureMessageProto.GenericPublicKey;
import java.security.KeyPair;
import java.security.PublicKey;
import java.util.Arrays;
import javax.crypto.SecretKey;
import junit.framework.TestCase;

/**
 * Android compatible tests for the {@link EnrollmentCryptoOps} class.
 */
public class EnrollmentCryptoOpsTest extends TestCase {

  private static final long DEVICE_ID = 1234567890L;
  private static final byte[] GCM_REGISTRATION_ID = { -0x80, 0, -0x80, 0, -0x80, 0 };
  private static final String DEVICE_MODEL = "TEST DEVICE";
  private static final String LOCALE = "en";
  private static final byte[] SESSION_ID = { 5, 5, 4, 4, 3, 3, 2, 2, 1, 1 };
  private static final String OAUTH_TOKEN = "1/23456etc";

  @Override
  protected void setUp() throws Exception {
    KeyEncodingTest.installSunEcSecurityProviderIfNecessary();
    assertEquals(
        PublicKeyProtoUtil.isLegacyCryptoRequired(), KeyEncoding.isLegacyCryptoRequired());
    super.setUp();
  }

  @Override
  protected void tearDown() throws Exception {
    KeyEncoding.setSimulateLegacyCrypto(false);
    super.tearDown();
  }


  public void testSimulatedEnrollment() throws Exception {
    boolean isLegacy = KeyEncoding.isLegacyCryptoRequired();
    // Step 1: Server generates an ephemeral DH key pair, saves the private key, and sends
    //         the public key to the client as server_ephemeral_key.
    KeyPair serverEphemeralKeyPair =
        EnrollmentCryptoOps.generateEnrollmentKeyAgreementKeyPair(isLegacy);
    byte[] savedServerPrivateKey =
        KeyEncoding.encodeKeyAgreementPrivateKey(serverEphemeralKeyPair.getPrivate());
    byte[] serverEphemeralKey = KeyEncoding.encodeKeyAgreementPublicKey(
        serverEphemeralKeyPair.getPublic());

    // Step 2a: Client generates an ephemeral DH key pair, and completes the DH key exchange
    //          to derive the master key.
    KeyPair clientEphemeralKeyPair =
        EnrollmentCryptoOps.generateEnrollmentKeyAgreementKeyPair(isLegacy);
    byte[] clientEphemeralKey = KeyEncoding.encodeKeyAgreementPublicKey(
        clientEphemeralKeyPair.getPublic());
    SecretKey clientMasterKey = EnrollmentCryptoOps.doKeyAgreement(
        clientEphemeralKeyPair.getPrivate(),
        KeyEncoding.parseKeyAgreementPublicKey(serverEphemeralKey));

    // Step 2b: Client generates its user key pair, and fills in a GcmDeviceInfo message containing
    //          the enrollment request (which includes the user public key).
    KeyPair userKeyPair = isLegacy ? PublicKeyProtoUtil.generateRSA2048KeyPair()
        : PublicKeyProtoUtil.generateEcP256KeyPair();
    GcmDeviceInfo clientInfo = createGcmDeviceInfo(userKeyPair.getPublic(), clientMasterKey);

    // Step 2c: Client signcrypts the enrollment request to the server, using a combination of the
    //          master key and its user signing key.
    byte[] enrollmentMessage = EnrollmentCryptoOps.encryptEnrollmentMessage(
        clientInfo, clientMasterKey, userKeyPair.getPrivate());


    // Step 3a: Server receives the client's DH public key and completes the key exchange using
    //          the saved DH private key.
    SecretKey serverMasterKey = EnrollmentCryptoOps.doKeyAgreement(
        KeyEncoding.parseKeyAgreementPrivateKey(savedServerPrivateKey, isLegacy),
        KeyEncoding.parseKeyAgreementPublicKey(clientEphemeralKey));

    // Step 3b: Server uses the exchanged master key to de-signcrypt the enrollment request
    //          (which also provides the user public key in the clear).
    GcmDeviceInfo serverInfo = EnrollmentCryptoOps.decryptEnrollmentMessage(
        enrollmentMessage, serverMasterKey, isLegacy);

    // Verify that the server sees the client's original enrollment request
    assertTrue(Arrays.equals(clientInfo.toByteArray(), serverInfo.toByteArray()));

    // Confirm that the server can recover a valid user PublicKey from the enrollment
    PublicKey serverUserPublicKey = KeyEncoding.parseUserPublicKey(
        serverInfo.getUserPublicKey().toByteArray());
    assertTrue(serverUserPublicKey.equals(userKeyPair.getPublic()));
  }

  public void testSimulatedEnrollmentWithForcedLegacy() throws Exception {
    if (PublicKeyProtoUtil.isLegacyCryptoRequired()) {
      // We already test with legacy in this case
      return;
    }
    KeyEncoding.setSimulateLegacyCrypto(true);
    testSimulatedEnrollment();
  }

  private GcmDeviceInfo createGcmDeviceInfo(PublicKey userPublicKey, SecretKey masterKey) {
    // One possible method of generating a key handle:
    GenericPublicKey encodedUserPublicKey = PublicKeyProtoUtil.encodePublicKey(userPublicKey);
    byte[] keyHandle = EnrollmentCryptoOps.sha256(encodedUserPublicKey.toByteArray());

    return GcmDeviceInfo.newBuilder()
        .setAndroidDeviceId(DEVICE_ID)
        .setGcmRegistrationId(ByteString.copyFrom(GCM_REGISTRATION_ID))
        .setDeviceMasterKeyHash(
            ByteString.copyFrom(EnrollmentCryptoOps.getMasterKeyHash(masterKey)))
        .setUserPublicKey(ByteString.copyFrom(KeyEncoding.encodeUserPublicKey(userPublicKey)))
        .setDeviceModel(DEVICE_MODEL)
        .setLocale(LOCALE)
        .setKeyHandle(ByteString.copyFrom(keyHandle))
        .setEnrollmentSessionId(ByteString.copyFrom(SESSION_ID))
        .setOauthToken(OAUTH_TOKEN)
        .build();
  }
}
