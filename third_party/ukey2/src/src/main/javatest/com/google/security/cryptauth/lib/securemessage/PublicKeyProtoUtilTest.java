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

package com.google.security.cryptauth.lib.securemessage;

import com.google.common.io.BaseEncoding;
import com.google.protobuf.ByteString;
import com.google.security.annotations.SuppressInsecureCipherModeCheckerNoReview;
import com.google.security.cryptauth.lib.securemessage.SecureMessageProto.DhPublicKey;
import com.google.security.cryptauth.lib.securemessage.SecureMessageProto.EcP256PublicKey;
import com.google.security.cryptauth.lib.securemessage.SecureMessageProto.GenericPublicKey;
import com.google.security.cryptauth.lib.securemessage.SecureMessageProto.SimpleRsaPublicKey;
import java.math.BigInteger;
import java.security.KeyFactory;
import java.security.KeyPair;
import java.security.KeyPairGenerator;
import java.security.NoSuchAlgorithmException;
import java.security.PrivateKey;
import java.security.PublicKey;
import java.security.interfaces.ECPublicKey;
import java.security.spec.ECPoint;
import java.security.spec.ECPublicKeySpec;
import java.security.spec.InvalidKeySpecException;
import java.util.Arrays;
import javax.crypto.KeyAgreement;
import javax.crypto.interfaces.DHPrivateKey;
import javax.crypto.interfaces.DHPublicKey;
import junit.framework.TestCase;

/** Tests for the PublicKeyProtoUtil class. */
public class PublicKeyProtoUtilTest extends TestCase {

  private static final byte[] ZERO_BYTE = {0};
  private PublicKey ecPublicKey;
  private PublicKey rsaPublicKey;

  /**
   * Diffie Hellman {@link PublicKey}s require special treatment, so we store them specifically as a
   * {@link DHPublicKey} to minimize casting.
   */
  private DHPublicKey dhPublicKey;

  @Override
  public void setUp() {
    if (!isAndroidOsWithoutEcSupport()) {
      ecPublicKey = PublicKeyProtoUtil.generateEcP256KeyPair().getPublic();
    }
    rsaPublicKey = PublicKeyProtoUtil.generateRSA2048KeyPair().getPublic();
    dhPublicKey = (DHPublicKey) PublicKeyProtoUtil.generateDh2048KeyPair().getPublic();
  }

  public void testPublicKeyProtoSpecificEncodeParse() throws Exception {
    if (!isAndroidOsWithoutEcSupport()) {
      assertEquals(
          ecPublicKey,
          PublicKeyProtoUtil.parseEcPublicKey(PublicKeyProtoUtil.encodeEcPublicKey(ecPublicKey)));
    }

    assertEquals(
        rsaPublicKey,
        PublicKeyProtoUtil.parseRsa2048PublicKey(
            PublicKeyProtoUtil.encodeRsa2048PublicKey(rsaPublicKey)));

    // DHPublicKey objects don't seem to properly implement equals(), so we have to test that
    // the individual y and p values match (it is safe to assume g = 2 is used if p is correct).
    DHPublicKey parsedDHPublicKey =
        PublicKeyProtoUtil.parseDh2048PublicKey(
            PublicKeyProtoUtil.encodeDh2048PublicKey(dhPublicKey));
    assertEquals(dhPublicKey.getY(), parsedDHPublicKey.getY());
    assertEquals(dhPublicKey.getParams().getP(), parsedDHPublicKey.getParams().getP());
    assertEquals(dhPublicKey.getParams().getG(), parsedDHPublicKey.getParams().getG());
  }

  public void testPublicKeyProtoGenericEncodeParse() throws Exception {
    if (!isAndroidOsWithoutEcSupport()) {
      assertEquals(
          ecPublicKey,
          PublicKeyProtoUtil.parsePublicKey(
              PublicKeyProtoUtil.encodePaddedEcPublicKey(ecPublicKey)));
      assertEquals(
          ecPublicKey,
          PublicKeyProtoUtil.parsePublicKey(PublicKeyProtoUtil.encodePublicKey(ecPublicKey)));
    }

    assertEquals(
        rsaPublicKey,
        PublicKeyProtoUtil.parsePublicKey(PublicKeyProtoUtil.encodePublicKey(rsaPublicKey)));

    // See above explanation for why we treat DHPublicKey objects differently.
    DHPublicKey parsedDHPublicKey =
        PublicKeyProtoUtil.parseDh2048PublicKey(
            PublicKeyProtoUtil.encodeDh2048PublicKey(dhPublicKey));
    assertEquals(dhPublicKey.getY(), parsedDHPublicKey.getY());
    assertEquals(dhPublicKey.getParams().getP(), parsedDHPublicKey.getParams().getP());
    assertEquals(dhPublicKey.getParams().getG(), parsedDHPublicKey.getParams().getG());
  }

  public void testPaddedECPublicKeyEncodeHasPaddedNullByte() throws Exception {
    if (isAndroidOsWithoutEcSupport()) {
      return;
    }

    // Key where the x coordinate is 33 bytes, y coordinate is 32 bytes
    ECPublicKey maxXByteLengthKey =
        buildEcPublicKey(
            BaseEncoding.base64().decode("AM730WQL7ZAmvyAJX4euNdr3+nAIueGlYYGXE6p732h6"),
            BaseEncoding.base64().decode("JEnmaDpKn0fH4/0kKGb97qUSwI2uT+ta0GLe3V7REfk="));
    // Key where both coordinates are 33 bytes
    ECPublicKey maxByteLengthKey =
        buildEcPublicKey(
            BaseEncoding.base64().decode("AOg9TQCxFfVdXv7lO/6UVDyiPsu8XDkEWQIPUfqX6UHP"),
            BaseEncoding.base64().decode("AP/RW8uVyu6QImpbza51CqG1mtBTh5c9pjv9CUwOuB7E"));
    // Key where both coordinates are 32 bytes
    ECPublicKey notMaxByteLengthKey =
        buildEcPublicKey(
            BaseEncoding.base64().decode("M35bxV8HKr0e8v7f4zuXgw6TYFawvikFdI71u9S1ONI="),
            BaseEncoding.base64().decode("OXR+xCpD8AR0VR8TeBXA00eIr3rWE6sV6KrOM6MoWsc="));
    GenericPublicKey encodedMaxXByteLengthKey =
        PublicKeyProtoUtil.encodePublicKey(maxXByteLengthKey);
    GenericPublicKey paddedEncodedMaxXByteLengthKey =
        PublicKeyProtoUtil.encodePaddedEcPublicKey(maxXByteLengthKey);
    GenericPublicKey encodedMaxByteLengthKey = PublicKeyProtoUtil.encodePublicKey(maxByteLengthKey);
    GenericPublicKey paddedEncodedMaxByteLengthKey =
        PublicKeyProtoUtil.encodePaddedEcPublicKey(maxByteLengthKey);
    GenericPublicKey encodedNotMaxByteLengthKey =
        PublicKeyProtoUtil.encodePublicKey(notMaxByteLengthKey);
    GenericPublicKey paddedEncodedNotMaxByteLengthKey =
        PublicKeyProtoUtil.encodePaddedEcPublicKey(notMaxByteLengthKey);

    assertEquals(maxXByteLengthKey, PublicKeyProtoUtil.parsePublicKey(encodedMaxXByteLengthKey));
    assertEquals(
        maxXByteLengthKey, PublicKeyProtoUtil.parsePublicKey(paddedEncodedMaxXByteLengthKey));
    assertEquals(maxByteLengthKey, PublicKeyProtoUtil.parsePublicKey(encodedMaxByteLengthKey));
    assertEquals(
        maxByteLengthKey, PublicKeyProtoUtil.parsePublicKey(paddedEncodedMaxByteLengthKey));
    assertEquals(
        notMaxByteLengthKey, PublicKeyProtoUtil.parsePublicKey(paddedEncodedNotMaxByteLengthKey));
    assertEquals(
        notMaxByteLengthKey, PublicKeyProtoUtil.parsePublicKey(encodedNotMaxByteLengthKey));

    assertEquals(33, paddedEncodedMaxXByteLengthKey.getEcP256PublicKey().getX().size());
    assertEquals(33, paddedEncodedMaxXByteLengthKey.getEcP256PublicKey().getY().size());
    assertEquals(0, paddedEncodedMaxXByteLengthKey.getEcP256PublicKey().getX().byteAt(0));
    assertEquals(0, paddedEncodedMaxXByteLengthKey.getEcP256PublicKey().getY().byteAt(0));
    assertEquals(33, encodedMaxXByteLengthKey.getEcP256PublicKey().getX().size());
    assertEquals(32, encodedMaxXByteLengthKey.getEcP256PublicKey().getY().size());

    assertEquals(33, paddedEncodedMaxByteLengthKey.getEcP256PublicKey().getX().size());
    assertEquals(33, paddedEncodedMaxByteLengthKey.getEcP256PublicKey().getY().size());
    assertEquals(0, paddedEncodedMaxByteLengthKey.getEcP256PublicKey().getX().byteAt(0));
    assertEquals(0, paddedEncodedMaxByteLengthKey.getEcP256PublicKey().getY().byteAt(0));
    assertEquals(33, encodedMaxByteLengthKey.getEcP256PublicKey().getX().size());
    assertEquals(33, encodedMaxByteLengthKey.getEcP256PublicKey().getY().size());

    assertEquals(32, encodedNotMaxByteLengthKey.getEcP256PublicKey().getX().size());
    assertEquals(32, encodedNotMaxByteLengthKey.getEcP256PublicKey().getY().size());
    assertEquals(0, paddedEncodedNotMaxByteLengthKey.getEcP256PublicKey().getX().byteAt(0));
    assertEquals(0, paddedEncodedNotMaxByteLengthKey.getEcP256PublicKey().getY().byteAt(0));
    assertEquals(33, paddedEncodedNotMaxByteLengthKey.getEcP256PublicKey().getX().size());
    assertEquals(33, paddedEncodedNotMaxByteLengthKey.getEcP256PublicKey().getY().size());
  }

  @SuppressInsecureCipherModeCheckerNoReview
  public void testWrongPublicKeyType() throws Exception {
    KeyPairGenerator dsaGen = KeyPairGenerator.getInstance("DSA");
    dsaGen.initialize(512);
    PublicKey pk = dsaGen.generateKeyPair().getPublic();

    if (!isAndroidOsWithoutEcSupport()) {
      // Try to encode it as EC
      try {
        PublicKeyProtoUtil.encodeEcPublicKey(pk);
        fail();
      } catch (IllegalArgumentException expected) {
      }

      try {
        PublicKeyProtoUtil.encodePaddedEcPublicKey(pk);
        fail();
      } catch (IllegalArgumentException expected) {
      }
    }

    // Try to encode it as RSA
    try {
      PublicKeyProtoUtil.encodeRsa2048PublicKey(pk);
      fail();
    } catch (IllegalArgumentException expected) {
    }

    // Try to encode it as DH
    try {
      PublicKeyProtoUtil.encodeDh2048PublicKey(pk);
      fail();
    } catch (IllegalArgumentException expected) {
    }

    // Try to encode it as Generic
    try {
      PublicKeyProtoUtil.encodePublicKey(pk);
      fail();
    } catch (IllegalArgumentException expected) {
    }
  }

  public void testEcPublicKeyProtoInvalidEncoding() throws Exception {
    if (isAndroidOsWithoutEcSupport()) {
      return;
    }

    EcP256PublicKey validProto = PublicKeyProtoUtil.encodeEcPublicKey(ecPublicKey);
    EcP256PublicKey.Builder invalidProto = EcP256PublicKey.newBuilder(validProto);

    // Mess up the X coordinate by repeating it twice
    byte[] newX =
        CryptoOps.concat(validProto.getX().toByteArray(), validProto.getX().toByteArray());
    checkParsingFailsFor(invalidProto.setX(ByteString.copyFrom(newX)).build());

    // Mess up the Y coordinate by erasing it
    invalidProto = EcP256PublicKey.newBuilder(validProto);
    checkParsingFailsFor(invalidProto.setY(ByteString.EMPTY).build());

    // Pick a point that is likely not on the curve by copying X over Y
    invalidProto = EcP256PublicKey.newBuilder(validProto);
    checkParsingFailsFor(invalidProto.setY(validProto.getX()).build());

    // Try the point (0, 0)
    invalidProto = EcP256PublicKey.newBuilder(validProto);
    checkParsingFailsFor(
        invalidProto
            .setX(ByteString.copyFrom(ZERO_BYTE))
            .setY(ByteString.copyFrom(ZERO_BYTE))
            .build());
  }

  private void checkParsingFailsFor(EcP256PublicKey invalid) {
    try {
      // Should fail to decode
      PublicKeyProtoUtil.parseEcPublicKey(invalid);
      fail();
    } catch (InvalidKeySpecException expected) {
    }
  }

  public void testSimpleRsaPublicKeyProtoInvalidEncoding() throws Exception {
    SimpleRsaPublicKey validProto = PublicKeyProtoUtil.encodeRsa2048PublicKey(rsaPublicKey);
    SimpleRsaPublicKey.Builder invalidProto;

    // Double the number of bits in the modulus
    invalidProto = SimpleRsaPublicKey.newBuilder(validProto);
    byte[] newN =
        CryptoOps.concat(validProto.getN().toByteArray(), validProto.getN().toByteArray());
    checkParsingFailsFor(invalidProto.setN(ByteString.copyFrom(newN)).build());

    // Set the modulus to 0
    invalidProto = SimpleRsaPublicKey.newBuilder(validProto);
    checkParsingFailsFor(invalidProto.setN(ByteString.copyFrom(ZERO_BYTE)).build());

    // Set the modulus to 65537 (way too small)
    invalidProto = SimpleRsaPublicKey.newBuilder(validProto);
    checkParsingFailsFor(
        invalidProto.setN(ByteString.copyFrom(BigInteger.valueOf(65537).toByteArray())).build());
  }

  private static void checkParsingFailsFor(SimpleRsaPublicKey invalid) {
    try {
      // Should fail to decode
      PublicKeyProtoUtil.parseRsa2048PublicKey(invalid);
      fail();
    } catch (InvalidKeySpecException expected) {
    }
  }

  public void testSimpleDhPublicKeyProtoInvalidEncoding() throws Exception {
    DhPublicKey validProto = PublicKeyProtoUtil.encodeDh2048PublicKey(dhPublicKey);
    DhPublicKey.Builder invalidProto;

    // Double the number of bits in the public element encoding
    invalidProto = DhPublicKey.newBuilder(validProto);
    byte[] newY =
        CryptoOps.concat(validProto.getY().toByteArray(), validProto.getY().toByteArray());
    checkParsingFailsFor(invalidProto.setY(ByteString.copyFrom(newY)).build());

    // Set the public element to 0
    invalidProto = DhPublicKey.newBuilder(validProto);
    checkParsingFailsFor(invalidProto.setY(ByteString.copyFrom(ZERO_BYTE)).build());
  }

  private static void checkParsingFailsFor(DhPublicKey invalid) {
    try {
      // Should fail to decode
      PublicKeyProtoUtil.parseDh2048PublicKey(invalid);
      fail();
    } catch (InvalidKeySpecException expected) {
    }
  }

  public void testDhKeyAgreementWorks() throws Exception {
    int minExpectedSecretLength = (PublicKeyProtoUtil.DH_P.bitLength() / 8) - 4;

    KeyPair clientKeyPair = PublicKeyProtoUtil.generateDh2048KeyPair();
    KeyPair serverKeyPair = PublicKeyProtoUtil.generateDh2048KeyPair();
    BigInteger clientY = ((DHPublicKey) clientKeyPair.getPublic()).getY();
    BigInteger serverY = ((DHPublicKey) serverKeyPair.getPublic()).getY();
    assertFalse(clientY.equals(serverY)); // DHPublicKeys should not be equal

    // Run client side of the key exchange
    byte[] clientSecret = doDhAgreement(clientKeyPair.getPrivate(), serverKeyPair.getPublic());
    assert (clientSecret.length >= minExpectedSecretLength);

    // Run the server side of the key exchange
    byte[] serverSecret = doDhAgreement(serverKeyPair.getPrivate(), clientKeyPair.getPublic());
    assert (serverSecret.length >= minExpectedSecretLength);

    assertTrue(Arrays.equals(clientSecret, serverSecret));
  }

  public void testDh2048PrivateKeyEncoding() throws Exception {
    KeyPair testPair = PublicKeyProtoUtil.generateDh2048KeyPair();
    DHPrivateKey sk = (DHPrivateKey) testPair.getPrivate();
    DHPrivateKey skParsed =
        PublicKeyProtoUtil.parseDh2048PrivateKey(PublicKeyProtoUtil.encodeDh2048PrivateKey(sk));
    assertEquals(sk.getX(), skParsed.getX());
    assertEquals(sk.getParams().getP(), skParsed.getParams().getP());
    assertEquals(sk.getParams().getG(), skParsed.getParams().getG());
  }

  public void testParseEcPublicKeyOnLegacyPlatform() {
    if (!PublicKeyProtoUtil.isLegacyCryptoRequired()) {
      return; // This test only runs on legacy platforms
    }
    byte[] pointBytes = {
      1, 2, 3, 4, 5, 6, 7, 8, 9, 0,
      1, 2, 3, 4, 5, 6, 7, 8, 9, 0,
      1, 2, 3, 4, 5, 6, 7, 8, 9, 0,
      1, 2
    };

    try {
      PublicKeyProtoUtil.parseEcPublicKey(
          EcP256PublicKey.newBuilder()
              .setX(ByteString.copyFrom(pointBytes))
              .setY(ByteString.copyFrom(pointBytes))
              .build());
      fail();
    } catch (InvalidKeySpecException expected) {
      // Should get this specific exception when EC doesn't work
    }
  }

  public void testIsLegacyCryptoRequired() {
    assertEquals(isAndroidOsWithoutEcSupport(), PublicKeyProtoUtil.isLegacyCryptoRequired());
  }

  /** @return true if running on an Android OS that doesn't support Elliptic Curve algorithms */
  public static boolean isAndroidOsWithoutEcSupport() {
    try {
      Class<?> clazz = ClassLoader.getSystemClassLoader().loadClass("android.os.Build$VERSION");
      int sdkVersion = clazz.getField("SDK_INT").getInt(null);
      if (sdkVersion < PublicKeyProtoUtil.ANDROID_HONEYCOMB_SDK_INT) {
        return true;
      }
    } catch (ClassNotFoundException e) {
      // Not running on Android
      return false;
    } catch (SecurityException e) {
      throw new AssertionError(e);
    } catch (NoSuchFieldException e) {
      throw new AssertionError(e);
    } catch (IllegalArgumentException e) {
      throw new AssertionError(e);
    } catch (IllegalAccessException e) {
      throw new AssertionError(e);
    }
    return false;
  }

  @SuppressInsecureCipherModeCheckerNoReview
  private static byte[] doDhAgreement(PrivateKey secretKey, PublicKey peerKey) throws Exception {
    KeyAgreement agreement = KeyAgreement.getInstance("DH");
    agreement.init(secretKey);
    agreement.doPhase(peerKey, true);
    return agreement.generateSecret();
  }

  private static ECPublicKey buildEcPublicKey(byte[] encodedX, byte[] encodedY) throws Exception {
    try {
      BigInteger wX = new BigInteger(encodedX);
      BigInteger wY = new BigInteger(encodedY);
      return (ECPublicKey)
          KeyFactory.getInstance("EC")
              .generatePublic(
                  new ECPublicKeySpec(
                      new ECPoint(wX, wY),
                      ((ECPublicKey) PublicKeyProtoUtil.generateEcP256KeyPair().getPublic())
                          .getParams()));
    } catch (NoSuchAlgorithmException e) {
      throw new RuntimeException(e);
    }
  }
}
