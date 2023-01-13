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

import com.google.protobuf.InvalidProtocolBufferException;
import com.google.security.cryptauth.lib.securemessage.CryptoOps.EncType;
import com.google.security.cryptauth.lib.securemessage.CryptoOps.SigType;
import com.google.security.cryptauth.lib.securemessage.SecureMessageProto.GenericPublicKey;
import com.google.security.cryptauth.lib.securemessage.SecureMessageProto.Header;
import com.google.security.cryptauth.lib.securemessage.SecureMessageProto.HeaderAndBody;
import com.google.security.cryptauth.lib.securemessage.SecureMessageProto.SecureMessage;
import java.security.KeyFactory;
import java.security.KeyPair;
import java.security.NoSuchAlgorithmException;
import java.security.PrivateKey;
import java.security.PublicKey;
import java.security.spec.InvalidKeySpecException;
import java.security.spec.PKCS8EncodedKeySpec;
import java.util.Arrays;
import javax.crypto.KeyGenerator;
import javax.crypto.SecretKey;
import javax.crypto.spec.SecretKeySpec;
import junit.framework.TestCase;

/**
 * Tests the library against some very basic test vectors, to help ensure wire-format
 * compatibility is not broken.
 */
public class SecureMessageSimpleTestVectorTest extends TestCase {

  private static final KeyFactory EC_KEY_FACTORY;
  static {
    try {
      if (PublicKeyProtoUtil.isLegacyCryptoRequired()) {
        EC_KEY_FACTORY = null;
      } else {
        EC_KEY_FACTORY = KeyFactory.getInstance("EC");
      }
    } catch (Exception e) {
      throw new RuntimeException(e);
    }
  }

  private static final byte[] TEST_ASSOCIATED_DATA = {
    11, 22, 33, 44, 55
  };
  private static final byte[] TEST_METADATA = {
    10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28
  };
  private static final byte[] TEST_VKID  = {
    0, 0, 1
  };
  private static final byte[] TEST_DKID = {
    -1, -1, 0,
  };
  private static final byte[] TEST_MESSAGE = {
    0, 99, 1, 98, 2, 97, 3, 96, 4, 95, 5, 94, 6, 93, 7, 92, 8, 91, 9, 90
  };

  // The following fields are initialized below, in a static block that contains auto-generated test
  // vectors. Initialization can't just be done inline due to code that throws checked exceptions.
  private static final PublicKey TEST_EC_PUBLIC_KEY;
  private static final PrivateKey TEST_EC_PRIVATE_KEY;
  private static final SecretKey TEST_KEY1;
  private static final SecretKey TEST_KEY2;
  private static final byte[] TEST_VECTOR_ECDSA_ONLY;
  private static final byte[] TEST_VECTOR_ECDSA_AND_AES;
  private static final byte[] TEST_VECTOR_HMAC_AND_AES_SAME_KEYS;
  private static final byte[] TEST_VECTOR_HMAC_AND_AES_DIFFERENT_KEYS;

  public void testEcdsaOnly() throws Exception {
   if (PublicKeyProtoUtil.isLegacyCryptoRequired()) {
      // On older Android platforms we can't run this test.
      return;
    }
    SecureMessage testVector = SecureMessage.parseFrom(TEST_VECTOR_ECDSA_ONLY);
    Header unverifiedHeader = SecureMessageParser.getUnverifiedHeader(testVector);
    HeaderAndBody headerAndBody = SecureMessageParser.parseSignedCleartextMessage(
        testVector, TEST_EC_PUBLIC_KEY, SigType.ECDSA_P256_SHA256, TEST_ASSOCIATED_DATA);
    assertTrue(Arrays.equals(
        unverifiedHeader.toByteArray(),
        headerAndBody.getHeader().toByteArray()));
    assertTrue(Arrays.equals(TEST_MESSAGE, headerAndBody.getBody().toByteArray()));
    assertEquals(TEST_ASSOCIATED_DATA.length, unverifiedHeader.getAssociatedDataLength());
    assertTrue(Arrays.equals(TEST_METADATA, unverifiedHeader.getPublicMetadata().toByteArray()));
    assertTrue(Arrays.equals(TEST_VKID, unverifiedHeader.getVerificationKeyId().toByteArray()));
    assertFalse(unverifiedHeader.hasDecryptionKeyId());
  }

  public void testEcdsaAndAes() throws Exception {
   if (PublicKeyProtoUtil.isLegacyCryptoRequired()) {
      // On older Android platforms we can't run this test.
      return;
    }
    SecureMessage testVector = SecureMessage.parseFrom(TEST_VECTOR_ECDSA_AND_AES);
    Header unverifiedHeader = SecureMessageParser.getUnverifiedHeader(testVector);
    HeaderAndBody headerAndBody = SecureMessageParser.parseSignCryptedMessage(
        testVector,
        TEST_EC_PUBLIC_KEY,
        SigType.ECDSA_P256_SHA256,
        TEST_KEY1,
        EncType.AES_256_CBC,
        TEST_ASSOCIATED_DATA);
    assertTrue(Arrays.equals(
        unverifiedHeader.toByteArray(),
        headerAndBody.getHeader().toByteArray()));
    assertTrue(Arrays.equals(TEST_MESSAGE, headerAndBody.getBody().toByteArray()));
    assertEquals(TEST_ASSOCIATED_DATA.length, unverifiedHeader.getAssociatedDataLength());
    assertTrue(Arrays.equals(TEST_METADATA, unverifiedHeader.getPublicMetadata().toByteArray()));
    assertTrue(Arrays.equals(TEST_VKID, unverifiedHeader.getVerificationKeyId().toByteArray()));
    assertTrue(Arrays.equals(TEST_DKID, unverifiedHeader.getDecryptionKeyId().toByteArray()));
  }

  public void testHmacAndAesSameKeys() throws Exception {
    SecureMessage testVector = SecureMessage.parseFrom(TEST_VECTOR_HMAC_AND_AES_SAME_KEYS);
    Header unverifiedHeader = SecureMessageParser.getUnverifiedHeader(testVector);

    HeaderAndBody headerAndBody = SecureMessageParser.parseSignCryptedMessage(
        testVector,
        TEST_KEY1,
        SigType.HMAC_SHA256,
        TEST_KEY1,
        EncType.AES_256_CBC,
        TEST_ASSOCIATED_DATA);
    assertTrue(Arrays.equals(
        unverifiedHeader.toByteArray(),
        headerAndBody.getHeader().toByteArray()));
    assertTrue(Arrays.equals(TEST_MESSAGE, headerAndBody.getBody().toByteArray()));
    assertEquals(TEST_ASSOCIATED_DATA.length, unverifiedHeader.getAssociatedDataLength());
    assertTrue(Arrays.equals(TEST_METADATA, unverifiedHeader.getPublicMetadata().toByteArray()));
    assertTrue(Arrays.equals(TEST_VKID, unverifiedHeader.getVerificationKeyId().toByteArray()));
    assertTrue(Arrays.equals(TEST_DKID, unverifiedHeader.getDecryptionKeyId().toByteArray()));
  }

  public void testHmacAndAesDifferentKeys() throws Exception {
    SecureMessage testVector = SecureMessage.parseFrom(TEST_VECTOR_HMAC_AND_AES_DIFFERENT_KEYS);
    Header unverifiedHeader = SecureMessageParser.getUnverifiedHeader(testVector);
    HeaderAndBody headerAndBody = SecureMessageParser.parseSignCryptedMessage(
        testVector,
        TEST_KEY1,
        SigType.HMAC_SHA256,
        TEST_KEY2,
        EncType.AES_256_CBC,
        TEST_ASSOCIATED_DATA);
    assertTrue(Arrays.equals(
        unverifiedHeader.toByteArray(),
        headerAndBody.getHeader().toByteArray()));
    assertTrue(Arrays.equals(TEST_MESSAGE, headerAndBody.getBody().toByteArray()));
    assertEquals(TEST_ASSOCIATED_DATA.length, unverifiedHeader.getAssociatedDataLength());
    assertTrue(Arrays.equals(TEST_METADATA, unverifiedHeader.getPublicMetadata().toByteArray()));
    assertTrue(Arrays.equals(TEST_VKID, unverifiedHeader.getVerificationKeyId().toByteArray()));
    assertTrue(Arrays.equals(TEST_DKID, unverifiedHeader.getDecryptionKeyId().toByteArray()));
  }

  /**
   * This code emits the test vectors to {@code System.out}. It will not generate fresh test
   * vectors unless an existing test vector is set to {@code null}, but it contains all of the code
   * used to the generate the test vector values. Ideally, existing test vectors should never be
   * regenerated, but having this code available should make it easier to add new test vectors.
   */
  public void testGenerateTestVectorsPseudoTest() throws Exception {
   if (PublicKeyProtoUtil.isLegacyCryptoRequired()) {
      // On older Android platforms we can't run this test.
      return;
    }
    System.out.printf("  static {\n    try {\n");
    String indent = "      ";
    PublicKey testEcPublicKey = TEST_EC_PUBLIC_KEY;
    PrivateKey testEcPrivateKey = TEST_EC_PRIVATE_KEY;
    if (testEcPublicKey == null) {
      KeyPair testEcKeyPair = PublicKeyProtoUtil.generateEcP256KeyPair();
      testEcPublicKey = testEcKeyPair.getPublic();
      testEcPrivateKey = testEcKeyPair.getPrivate();
    }
    System.out.printf("%s%s = parsePublicKey(new byte[] %s);\n",
        indent,
        "TEST_EC_PUBLIC_KEY",
        byteArrayToJavaCode(indent, encodePublicKey(testEcPublicKey)));
    System.out.printf("%s%s = parseEcPrivateKey(new byte[] %s);\n",
        indent,
        "TEST_EC_PRIVATE_KEY",
        byteArrayToJavaCode(indent, encodeEcPrivateKey(testEcPrivateKey)));

    SecretKey testKey1 = TEST_KEY1;
    if (testKey1 == null) {
      testKey1 = makeAesKey();
    }
    System.out.printf("%s%s = new SecretKeySpec(new byte[] %s, \"AES\");\n",
        indent,
        "TEST_KEY1",
        byteArrayToJavaCode(indent, testKey1.getEncoded()));

    SecretKey testKey2 = TEST_KEY2;
    if (testKey2 == null) {
      testKey2 = makeAesKey();
    }
    System.out.printf("%s%s = new SecretKeySpec(new byte[] %s, \"AES\");\n",
        indent,
        "TEST_KEY2",
        byteArrayToJavaCode(indent, testKey2.getEncoded()));

    byte[] testVectorEcdsaOnly = TEST_VECTOR_ECDSA_ONLY;
    if (testVectorEcdsaOnly == null) {
      testVectorEcdsaOnly = new SecureMessageBuilder()
          .setAssociatedData(TEST_ASSOCIATED_DATA)
          .setPublicMetadata(TEST_METADATA)
          .setVerificationKeyId(TEST_VKID)
          .buildSignedCleartextMessage(
              testEcPrivateKey, SigType.ECDSA_P256_SHA256, TEST_MESSAGE).toByteArray();
    }
    printInitializerFor(indent, "TEST_VECTOR_ECDSA_ONLY", testVectorEcdsaOnly);

    byte[] testVectorEcdsaAndAes = TEST_VECTOR_ECDSA_AND_AES;
    if (testVectorEcdsaAndAes == null) {
      testVectorEcdsaAndAes = new SecureMessageBuilder()
          .setAssociatedData(TEST_ASSOCIATED_DATA)
          .setDecryptionKeyId(TEST_DKID)
          .setPublicMetadata(TEST_METADATA)
          .setVerificationKeyId(TEST_VKID)
          .buildSignCryptedMessage(
              testEcPrivateKey,
              SigType.ECDSA_P256_SHA256,
              testKey1,
              EncType.AES_256_CBC,
              TEST_MESSAGE).toByteArray();
    }
    printInitializerFor(indent, "TEST_VECTOR_ECDSA_AND_AES", testVectorEcdsaAndAes);

    byte[] testVectorHmacAndAesSameKeys = TEST_VECTOR_HMAC_AND_AES_SAME_KEYS;
    if (testVectorHmacAndAesSameKeys == null) {
      testVectorHmacAndAesSameKeys = new SecureMessageBuilder()
          .setAssociatedData(TEST_ASSOCIATED_DATA)
          .setDecryptionKeyId(TEST_DKID)
          .setPublicMetadata(TEST_METADATA)
          .setVerificationKeyId(TEST_VKID)
          .buildSignCryptedMessage(
              testKey1,
              SigType.HMAC_SHA256,
              testKey1,
              EncType.AES_256_CBC,
              TEST_MESSAGE).toByteArray();
    }
    printInitializerFor(indent, "TEST_VECTOR_HMAC_AND_AES_SAME_KEYS", testVectorHmacAndAesSameKeys);

    byte[] testVectorHmacAndAesDifferentKeys = TEST_VECTOR_HMAC_AND_AES_DIFFERENT_KEYS;
    if (testVectorHmacAndAesDifferentKeys == null) {
      testVectorHmacAndAesDifferentKeys = new SecureMessageBuilder()
          .setAssociatedData(TEST_ASSOCIATED_DATA)
          .setDecryptionKeyId(TEST_DKID)
          .setPublicMetadata(TEST_METADATA)
          .setVerificationKeyId(TEST_VKID)
          .buildSignCryptedMessage(
              testKey1,
              SigType.HMAC_SHA256,
              testKey2,
              EncType.AES_256_CBC,
              TEST_MESSAGE).toByteArray();
    }
    printInitializerFor(
        indent, "TEST_VECTOR_HMAC_AND_AES_DIFFERENT_KEYS", testVectorHmacAndAesDifferentKeys);

    System.out.printf(
        "    } catch (Exception e) {\n      throw new RuntimeException(e);\n    }\n  }\n");
  }

  private SecretKey makeAesKey() throws NoSuchAlgorithmException {
    KeyGenerator aesKeygen = KeyGenerator.getInstance("AES");
    aesKeygen.init(256);
    return aesKeygen.generateKey();
  }

  private void printInitializerFor(String indent, String name, byte[] value) {
    System.out.printf("%s%s = new byte[] %s;\n",
        indent,
        name,
        byteArrayToJavaCode(indent, value));
  }

  private static String byteArrayToJavaCode(String lineIndent, byte[] array) {
    String newline = "\n" + lineIndent + "    ";
    String unwrappedArray = Arrays.toString(array).replace("[", "").replace("]", "");
    int wrapAfter = 16;
    int count = wrapAfter;
    StringBuilder result = new StringBuilder("{");
    for (String entry : unwrappedArray.split(" ")) {
      if (++count > wrapAfter) {
        result.append(newline);
        count = 0;
      } else {
        result.append(" ");
      }
      result.append(entry);
    }
    result.append(" }");
    return result.toString();
  }

  private static byte[] encodePublicKey(PublicKey pk) {
    return PublicKeyProtoUtil.encodePublicKey(pk).toByteArray();
  }

  private static PublicKey parsePublicKey(byte[] encodedPk)
      throws InvalidKeySpecException, InvalidProtocolBufferException {
    GenericPublicKey gpk = GenericPublicKey.parseFrom(encodedPk);
    if (PublicKeyProtoUtil.isLegacyCryptoRequired()
        && gpk.getType() == SecureMessageProto.PublicKeyType.EC_P256) {
      return null;
    }
    return PublicKeyProtoUtil.parsePublicKey(gpk);
  }

  private static byte[] encodeEcPrivateKey(PrivateKey sk) {
    return sk.getEncoded();
  }

  private static PrivateKey parseEcPrivateKey(byte[] sk) throws InvalidKeySpecException {
    if (PublicKeyProtoUtil.isLegacyCryptoRequired()) {
      return null;
    }
    return EC_KEY_FACTORY.generatePrivate(new PKCS8EncodedKeySpec(sk));
  }

  // The following block of code was automatically generated by cut and pasting the output of the
  // generateTestVectorsPseudoTest, which should reliably emit this same test vectors. Please
  // DO NOT DELETE any of these existing test vectors unless you _really_ know what you are doing.
  //
  // --- AUTO GENERATED CODE BEGINS HERE ---
  static {
    try {
      TEST_EC_PUBLIC_KEY = parsePublicKey(new byte[] {
          8, 1, 18, 70, 10, 33, 0, -109, 9, 5, 8, -89, -3, -68, -86, -19, 17,
          -126, -11, -95, 35, 101, 102, -57, -84, -118, 73, 83, 66, -62, -49, -91, 71, -19,
          52, 123, 113, 119, 45, 18, 33, 0, -65, -19, 83, -66, -12, 62, 102, -67, 116,
          64, 42, 55, -84, -101, 90, -106, 113, -89, -30, 57, -112, 96, -99, -126, 14, 83,
          41, 95, -24, -114, 23, -5 });
      TEST_EC_PRIVATE_KEY = parseEcPrivateKey(new byte[] {
          48, 65, 2, 1, 0, 48, 19, 6, 7, 42, -122, 72, -50, 61, 2, 1, 6,
          8, 42, -122, 72, -50, 61, 3, 1, 7, 4, 39, 48, 37, 2, 1, 1, 4,
          32, 26, -82, -61, -86, -59, -8, 2, -62, -17, -20, 122, 3, 85, -102, -76, 81,
          51, 39, -9, 12, 99, -117, 127, 19, 121, 109, -31, -49, 110, 121, 76, -107 });
      TEST_KEY1 = new SecretKeySpec(new byte[] {
          -89, 105, 62, -41, -75, 78, 70, 110, -62, -58, -80, -81, -99, -62, 39, 38, 37,
          -7, -112, -83, 81, 23, 125, -72, -100, 103, -34, -23, -68, 21, -46, -104 }, "AES");
      TEST_KEY2 = new SecretKeySpec(new byte[] {
          -6, 48, 107, 61, -99, -89, 111, 33, 70, 54, -13, 111, 81, -120, 50, 89, -119,
          -113, -114, 63, 12, -68, 40, 42, -77, -58, -49, 18, 69, 91, -20, -65 }, "AES");
      TEST_VECTOR_ECDSA_ONLY = new byte[] {
          10, 56, 10, 32, 8, 2, 16, 1, 26, 3, 0, 0, 1, 50, 19, 10, 11,
          12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28,
          56, 5, 18, 20, 0, 99, 1, 98, 2, 97, 3, 96, 4, 95, 5, 94, 6,
          93, 7, 92, 8, 91, 9, 90, 18, 72, 48, 70, 2, 33, 0, -79, 59, 50,
          21, 54, 61, -92, 77, -34, -77, -45, -105, 107, -28, -19, 91, -78, 120, 68, 33,
          11, -76, -1, 50, 64, -127, -78, 6, 108, 115, -13, 126, 2, 33, 0, -72, -44,
          52, 93, 105, 109, -127, -111, 11, 33, -111, 97, -114, 9, 117, -68, -45, 64, 63,
          43, 60, -44, -89, -107, -59, -45, 56, 100, -66, -40, 46, -60 };
      TEST_VECTOR_ECDSA_AND_AES = new byte[] {
          10, 107, 10, 55, 8, 2, 16, 2, 26, 3, 0, 0, 1, 34, 3, -1, -1,
          0, 42, 16, -86, 16, 55, -8, -85, -47, -77, -36, -127, 44, -10, -44, -63, 115,
          -111, 26, 50, 19, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22,
          23, 24, 25, 26, 27, 28, 56, 5, 18, 48, -110, 23, -67, 122, -118, 96, -4,
          32, -113, -104, -107, -16, 76, 37, -61, -67, -63, 90, 38, 96, -47, -105, 56, -34,
          50, -30, 82, 25, 100, 36, 69, 50, 68, 60, 38, 96, -108, -49, -73, -10, -62,
          -76, -45, -105, -86, 93, 28, 34, 18, 70, 48, 68, 2, 33, 0, -87, -103, 11,
          -70, 34, 33, -41, 90, -83, -74, 19, -13, 127, -43, -116, -32, 88, -13, 125, -122,
          56, -21, 79, 47, 101, 89, -80, -43, 102, 92, 4, -15, 2, 31, 109, -69, 35,
          21, 44, -27, -77, 32, 17, -90, -68, 113, 55, -24, -122, 40, 81, 51, 0, -84,
          -29, -12, -26, 73, 105, -32, 116, -28, 84, -116, -117 };
      TEST_VECTOR_HMAC_AND_AES_SAME_KEYS = new byte[] {
          10, 91, 10, 55, 8, 1, 16, 2, 26, 3, 0, 0, 1, 34, 3, -1, -1,
          0, 42, 16, -110, 48, 67, 67, -31, 24, -42, 13, -44, -109, 6, 113, 34, -70,
          121, 6, 50, 19, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22,
          23, 24, 25, 26, 27, 28, 56, 5, 18, 32, -44, -102, -16, 123, 113, -75, 88,
          -33, 118, 25, 60, -65, 109, 26, -70, -123, 58, -114, 126, 8, 106, -28, 65, -38,
          -4, 68, -78, -91, 49, -13, 22, -122, 18, 32, 20, -120, -113, -76, 85, -35, -53,
          37, -18, 66, -38, 32, 10, 30, 89, 112, -39, -27, 24, 93, -36, -100, -127, -79,
          94, -7, -19, -41, -47, -29, 1, 12 };
      TEST_VECTOR_HMAC_AND_AES_DIFFERENT_KEYS = new byte[] {
          10, 107, 10, 55, 8, 1, 16, 2, 26, 3, 0, 0, 1, 34, 3, -1, -1,
          0, 42, 16, -96, -7, 39, 79, -37, 40, 1, -30, 97, 0, 123, -7, -124, -75,
          -127, -18, 50, 19, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22,
          23, 24, 25, 26, 27, 28, 56, 5, 18, 48, 90, 40, -48, -113, 84, -32, 47,
          98, 54, -128, 127, 115, 32, 87, -86, 4, -26, 99, 9, -88, 13, 77, 127, 114,
          -48, -117, -94, 96, -86, -105, -123, 11, 116, -69, -83, -110, 3, -10, 0, -34, 72,
          10, -58, 3, -119, -94, 23, -114, 18, 32, -25, -126, 95, 125, -110, -62, -36, -78,
          97, 72, -54, -114, 97, -68, -46, 107, 53, 55, -57, 88, 127, -20, -23, 80, -9,
          -91, 115, 42, 24, 49, -76, -111 };
    } catch (Exception e) {
      throw new RuntimeException(e);
    }
  }
}
