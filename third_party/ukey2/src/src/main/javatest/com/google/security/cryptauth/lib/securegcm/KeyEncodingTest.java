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

import com.google.security.cryptauth.lib.securemessage.PublicKeyProtoUtil;
import java.security.Key;
import java.security.KeyPair;
import java.security.PrivateKey;
import java.security.Provider;
import java.security.PublicKey;
import java.security.Security;
import java.security.interfaces.ECPrivateKey;
import java.security.interfaces.ECPublicKey;
import java.security.interfaces.RSAPrivateKey;
import java.security.interfaces.RSAPublicKey;
import java.security.spec.InvalidKeySpecException;
import java.util.Arrays;
import javax.crypto.interfaces.DHPrivateKey;
import javax.crypto.interfaces.DHPublicKey;
import junit.framework.TestCase;

/**
 * Android compatible tests for the {@link KeyEncoding} class.
 */
public class KeyEncodingTest extends TestCase {
  private static final byte[] RAW_KEY_BYTES = {
      1, 2, 3, 4, 5, 6, 7, 8, 9, 0,
      1, 2, 3, 4, 5, 6, 7, 8, 9, 0,
      1, 2, 3, 4, 5, 6, 7, 8, 9, 0,
      1, 2};

  private Boolean isLegacy;
  private KeyPair userKeyPair;

  @Override
  protected void setUp() throws Exception {
    installSunEcSecurityProviderIfNecessary();
    isLegacy = PublicKeyProtoUtil.isLegacyCryptoRequired();
    setUserKeyPair();
    super.setUp();
  }

  @Override
  protected void tearDown() throws Exception {
    KeyEncoding.setSimulateLegacyCrypto(false);
    isLegacy = PublicKeyProtoUtil.isLegacyCryptoRequired();
    super.tearDown();
  }

  private void setUserKeyPair() {
    userKeyPair = isLegacy ? PublicKeyProtoUtil.generateRSA2048KeyPair()
        : PublicKeyProtoUtil.generateEcP256KeyPair();
  }

  public void testSimulateLegacyCrypto() {
    if (isLegacy) {
      return;  // Nothing to test if we are already stuck in a legacy platform
    }
    assertFalse(KeyEncoding.isLegacyCryptoRequired());
    KeyEncoding.setSimulateLegacyCrypto(true);
    assertTrue(KeyEncoding.isLegacyCryptoRequired());
  }

  public void testMasterKeyEncoding() {
    // Require that master keys are encoded/decoded as raw byte arrays
    assertTrue(Arrays.equals(
        RAW_KEY_BYTES,
        KeyEncoding.encodeMasterKey(KeyEncoding.parseMasterKey(RAW_KEY_BYTES))));
  }

  public void testUserPublicKeyEncoding() throws InvalidKeySpecException {
    PublicKey pk = userKeyPair.getPublic();
    byte[] encodedPk = KeyEncoding.encodeUserPublicKey(pk);
    PublicKey decodedPk = KeyEncoding.parseUserPublicKey(encodedPk);
    assertKeysEqual(pk, decodedPk);
  }

  public void testUserPrivateKeyEncoding() throws InvalidKeySpecException {
    PrivateKey sk = userKeyPair.getPrivate();
    byte[] encodedSk = KeyEncoding.encodeUserPrivateKey(sk);
    PrivateKey decodedSk = KeyEncoding.parseUserPrivateKey(encodedSk, isLegacy);
    assertKeysEqual(sk, decodedSk);
  }

  public void testKeyAgreementPublicKeyEncoding() throws InvalidKeySpecException {
    KeyPair clientKeyPair = EnrollmentCryptoOps.generateEnrollmentKeyAgreementKeyPair(isLegacy);
    PublicKey pk = clientKeyPair.getPublic();
    byte[] encodedPk = KeyEncoding.encodeKeyAgreementPublicKey(pk);
    PublicKey decodedPk = KeyEncoding.parseKeyAgreementPublicKey(encodedPk);
    assertKeysEqual(pk, decodedPk);
  }

  public void testKeyAgreementPrivateKeyEncoding() throws InvalidKeySpecException {
    KeyPair clientKeyPair = EnrollmentCryptoOps.generateEnrollmentKeyAgreementKeyPair(isLegacy);
    PrivateKey sk = clientKeyPair.getPrivate();
    byte[] encodedSk = KeyEncoding.encodeKeyAgreementPrivateKey(sk);
    PrivateKey decodedSk = KeyEncoding.parseKeyAgreementPrivateKey(encodedSk, isLegacy);
    assertKeysEqual(sk, decodedSk);
  }

  public void testEncodingsWithForcedLegacy() throws InvalidKeySpecException {
    if (PublicKeyProtoUtil.isLegacyCryptoRequired()) {
      // We already test with legacy in this case
      return;
    }
    KeyEncoding.setSimulateLegacyCrypto(true);
    isLegacy = true;
    setUserKeyPair();
    testUserPublicKeyEncoding();
    testUserPrivateKeyEncoding();
    testKeyAgreementPublicKeyEncoding();
    testKeyAgreementPrivateKeyEncoding();
  }

  public void testSigningPublicKeyEncoding() throws InvalidKeySpecException {
    KeyPair keyPair = PublicKeyProtoUtil.generateEcP256KeyPair();
    PublicKey pk = keyPair.getPublic();
    byte[] encodedPk = KeyEncoding.encodeSigningPublicKey(pk);
    PublicKey decodedPk = KeyEncoding.parseSigningPublicKey(encodedPk);
    assertKeysEqual(pk, decodedPk);
  }

  public void testSigningPrivateKeyEncoding() throws InvalidKeySpecException {
    KeyPair keyPair = PublicKeyProtoUtil.generateEcP256KeyPair();
    PrivateKey sk = keyPair.getPrivate();
    byte[] encodedSk = KeyEncoding.encodeSigningPrivateKey(sk);
    PrivateKey decodedSk = KeyEncoding.parseSigningPrivateKey(encodedSk);
    assertKeysEqual(sk, decodedSk);
  }

  public void testDeviceSyncPublicKeyEncoding() throws InvalidKeySpecException {
    KeyPair keyPair = PublicKeyProtoUtil.generateEcP256KeyPair();
    PublicKey pk = keyPair.getPublic();
    byte[] encodedPk = KeyEncoding.encodeDeviceSyncGroupPublicKey(pk);
    PublicKey decodedPk = KeyEncoding.parseDeviceSyncGroupPublicKey(encodedPk);
    assertKeysEqual(pk, decodedPk);
  }

  void assertKeysEqual(Key a, Key b) {
    if ((a instanceof ECPublicKey)
        || (a instanceof ECPrivateKey)
        || (a instanceof RSAPublicKey)
        || (a instanceof RSAPrivateKey)) {
      assertNotNull(a.getEncoded());
      assertTrue(Arrays.equals(a.getEncoded(), b.getEncoded()));
    }
    if (a instanceof DHPublicKey) {
      DHPublicKey ya = (DHPublicKey) a;
      DHPublicKey yb = (DHPublicKey) b;
      assertEquals(ya.getY(), yb.getY());
      assertEquals(ya.getParams().getG(), yb.getParams().getG());
      assertEquals(ya.getParams().getP(), yb.getParams().getP());
    }
    if (a instanceof DHPrivateKey) {
      DHPrivateKey xa = (DHPrivateKey) a;
      DHPrivateKey xb = (DHPrivateKey) b;
      assertEquals(xa.getX(), xb.getX());
      assertEquals(xa.getParams().getG(), xb.getParams().getG());
      assertEquals(xa.getParams().getP(), xb.getParams().getP());
    }
  }

  /**
   * Registers the SunEC security provider if no EC security providers are currently registered.
   */
  // TODO(shabsi): Remove this method when b/7891565 is fixed
  static void installSunEcSecurityProviderIfNecessary() {
    if (Security.getProviders("KeyPairGenerator.EC") == null) {
      try {
        Class<?> providerClass = Class.forName("sun.security.ec.SunEC");
        Security.addProvider((Provider) providerClass.newInstance());
      } catch (Exception e) {
        // SunEC is not available, nothing we can do
      }
    }
  }
}
