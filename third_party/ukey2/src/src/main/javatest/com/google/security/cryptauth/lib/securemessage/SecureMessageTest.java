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

import com.google.protobuf.ByteString;
import com.google.protobuf.InvalidProtocolBufferException;
import com.google.protobuf.UninitializedMessageException;
import com.google.security.cryptauth.lib.securemessage.CryptoOps.EncType;
import com.google.security.cryptauth.lib.securemessage.CryptoOps.SigType;
import com.google.security.cryptauth.lib.securemessage.SecureMessageProto.EcP256PublicKey;
import com.google.security.cryptauth.lib.securemessage.SecureMessageProto.GenericPublicKey;
import com.google.security.cryptauth.lib.securemessage.SecureMessageProto.Header;
import com.google.security.cryptauth.lib.securemessage.SecureMessageProto.HeaderAndBody;
import com.google.security.cryptauth.lib.securemessage.SecureMessageProto.SecureMessage;
import com.google.security.cryptauth.lib.securemessage.SecureMessageProto.SimpleRsaPublicKey;
import java.security.InvalidKeyException;
import java.security.Key;
import java.security.KeyPair;
import java.security.NoSuchAlgorithmException;
import java.security.PrivateKey;
import java.security.PublicKey;
import java.security.SecureRandom;
import java.security.SignatureException;
import java.security.spec.InvalidKeySpecException;
import java.util.Arrays;
import java.util.List;
import javax.crypto.KeyGenerator;
import javax.crypto.SecretKey;
import junit.framework.TestCase;

/**
 * Tests for the SecureMessageBuilder and SecureMessageParser classes.
 */
public class SecureMessageTest extends TestCase {
  // Not to be used when generating cross-platform test vectors (due to default charset encoding)
  public static final byte[] TEST_MESSAGE =
      "Testing 1 2 3... Testing 1 2 3... Testing 1 2 3...".getBytes();

  private static final byte[] TEST_KEY_ID =
    { 0, 1, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0 };
  // Not to be used when generating cross-platform test vectors (due to default charset encoding)
  private static final byte[] TEST_METADATA = "Some protocol metadata string goes here".getBytes();
  private static final byte[] TEST_ASSOCIATED_DATA = {
    1, 0, 2, 0, 3, 0, 4, 0, 5, 0, 6, 0, 7, 0, 8, 0, 9, 0, 10, 0, 11 };
  private static final byte[] ZERO_BYTE = { 0 };
  private static final byte[] EMPTY_BYTES = { };

  private static final List<byte[]> MESSAGE_VALUES = Arrays.asList(
      EMPTY_BYTES,
      TEST_MESSAGE
      );

  private static final List<byte[]> KEY_ID_VALUES = Arrays.asList(
      null,
      EMPTY_BYTES,
      TEST_KEY_ID);

  private static final List<byte[]> METADATA_VALUES = Arrays.asList(
      null,
      EMPTY_BYTES,
      TEST_METADATA
      );

  private static final List<byte[]> ASSOCIATED_DATA_VALUES = Arrays.asList(
      null,
      ZERO_BYTE,
      TEST_ASSOCIATED_DATA);

  private byte[] message;
  private byte[] metadata;
  private byte[] verificationKeyId;
  private byte[] decryptionKeyId;
  private byte[] associatedData;
  private PublicKey ecPublicKey;
  private PrivateKey ecPrivateKey;
  private PublicKey rsaPublicKey;
  private PrivateKey rsaPrivateKey;
  private SecretKey aesEncryptionKey;
  private SecretKey hmacKey;
  private SecureMessageBuilder secureMessageBuilder = new SecureMessageBuilder();
  private SecureRandom rng = new SecureRandom();

  @Override
  public void setUp() {
    message = TEST_MESSAGE;
    metadata = null;
    verificationKeyId = null;
    decryptionKeyId = null;
    associatedData = null;
    if (!PublicKeyProtoUtil.isLegacyCryptoRequired()) {
      KeyPair ecKeyPair = PublicKeyProtoUtil.generateEcP256KeyPair();
      ecPublicKey = ecKeyPair.getPublic();
      ecPrivateKey = ecKeyPair.getPrivate();
    }
    KeyPair rsaKeyPair = PublicKeyProtoUtil.generateRSA2048KeyPair();
    rsaPublicKey = rsaKeyPair.getPublic();
    rsaPrivateKey = rsaKeyPair.getPrivate();
    try {
      aesEncryptionKey = makeAesKey();
      hmacKey = makeAesKey();
    } catch (NoSuchAlgorithmException e) {
      e.printStackTrace();
      fail();
    }
    secureMessageBuilder.reset();
  }

  private SecureMessage sign(SigType sigType) throws NoSuchAlgorithmException, InvalidKeyException {
    return getPreconfiguredBuilder().buildSignedCleartextMessage(
        getSigningKeyFor(sigType), sigType, message);
  }

  private SecureMessage signCrypt(SigType sigType, EncType encType)
      throws NoSuchAlgorithmException, InvalidKeyException {
    return getPreconfiguredBuilder().buildSignCryptedMessage(
        getSigningKeyFor(sigType), sigType, aesEncryptionKey, encType, message);
  }

  private void verify(SecureMessage signed, SigType sigType)
      throws NoSuchAlgorithmException, InvalidKeyException, SignatureException,
      InvalidProtocolBufferException {
    HeaderAndBody headerAndBody = SecureMessageParser.parseSignedCleartextMessage(
        signed,
        getVerificationKeyFor(sigType),
        sigType,
        associatedData);
    consistencyCheck(signed, headerAndBody, sigType, EncType.NONE);
  }

  private void verifyDecrypt(SecureMessage encryptedAndSigned, SigType sigType, EncType encType)
      throws InvalidProtocolBufferException, InvalidKeyException, NoSuchAlgorithmException,
      SignatureException {
    HeaderAndBody headerAndBody = SecureMessageParser.parseSignCryptedMessage(
        encryptedAndSigned,
        getVerificationKeyFor(sigType),
        sigType,
        aesEncryptionKey,
        encType,
        associatedData);
    consistencyCheck(encryptedAndSigned, headerAndBody, sigType, encType);
  }

  // A collection of different kinds of "alterations" that can be made to SecureMessage protos.
  enum Alteration {
    DECRYPTION_KEY_ID,
    ENCTYPE,
    HEADER_AND_BODY_PROTO,
    MESSAGE,
    METADATA,
    RESIGNCRYPTION_ATTACK,
    SIGTYPE,
    VERIFICATION_KEY_ID,
    ASSOCIATED_DATA_LENGTH,
  }

  private void doSignAndVerify(SigType sigType) throws Exception {
    System.out.println("BEGIN_TEST -- Testing SigType: " + sigType + " with:");
    System.out.println("VerificationKeyId: " + Arrays.toString(verificationKeyId));
    System.out.println("Metadata: " + Arrays.toString(metadata));
    System.out.println("AssociatedData: " + Arrays.toString(associatedData));
    System.out.println("Message: " + Arrays.toString(message));
    // Positive test cases
    SecureMessage signed = sign(sigType);
    verify(signed, sigType);

    // Negative test cases
    for (Alteration altType : getAlterationsToTest()) {
      System.out.println("Testing alteration: " + altType.toString());
      SecureMessage modified = modifyMessage(signed, altType);
      try {
        verify(modified, sigType);
        fail(altType.toString());
      } catch (SignatureException e) {
        // We expect this
      }
    }

    // Try verifying with the wrong associated data
    if ((associatedData == null) || (associatedData.length == 0)) {
      associatedData = ZERO_BYTE;
    } else {
      associatedData = null;
    }
    try {
      verify(signed, sigType);
      fail("Expected verification to fail due to incorrect associatedData");
    } catch (SignatureException e) {
      // We expect this
    }

    System.out.println("PASS_TEST -- Testing SigType: " + sigType);
  }

  private List<Alteration> getAlterationsToTest() {
    if (isRunningInAndroid()) {
      // Android is very slow. Only try one alteration attack, intead of all of them.
      int randomAlteration = Math.abs(rng.nextInt()) % Alteration.values().length;
      return Arrays.asList(Alteration.values()[randomAlteration]);
    } else {
      // Just try all of them
      return Arrays.asList(Alteration.values());
    }
  }

  private void doSignCryptAndVerifyDecrypt(SigType sigType) throws Exception {
    // For now, EncType is always AES_256_CBC
    EncType encType = EncType.AES_256_CBC;
    System.out.println("BEGIN_TEST -- Testing SigType: " + sigType
        + " EncType: " + encType + " with:");
    System.out.println("DecryptionKeyId: " + Arrays.toString(decryptionKeyId));
    System.out.println("VerificationKeyId: " + Arrays.toString(verificationKeyId));
    System.out.println("Metadata: " + Arrays.toString(metadata));
    System.out.println("AssociatedData: " + Arrays.toString(associatedData));
    System.out.println("Message: " + Arrays.toString(message));
    SecureMessage encryptedAndSigned = null;
    encryptedAndSigned = signCrypt(sigType, encType);
    verifyDecrypt(encryptedAndSigned, sigType, encType);

    // Negative test cases
    for (Alteration altType : getAlterationsToTest()) {
      if (skipAlterationTestFor(altType, sigType)) {
        System.out.println("Skipping alteration test: " + altType.toString());
        continue;
      }

      System.out.println("Testing alteration: " + altType.toString());
      SecureMessage modified = modifyMessage(encryptedAndSigned, altType);
      try {
        verifyDecrypt(modified, sigType, encType);
        fail();
      } catch (SignatureException e) {
        // We expect this
      }
    }
    System.out.println("PASS_TEST -- Testing SigType: " + sigType + " EncType: " + encType);
  }

  private boolean skipAlterationTestFor(Alteration altType, SigType sigType) {
    // The RESIGNCRYPTION_ATTACK may be allowed to succeed iff the same symmetric key
    // is being reused for both signature and encryption.
    return (altType == Alteration.RESIGNCRYPTION_ATTACK)
        // Intentionally testing equality of object address here
        && (getVerificationKeyFor(sigType) == aesEncryptionKey);
  }

  private SecureMessage modifyMessage(SecureMessage original, Alteration altType) throws Exception {
    ByteString bogus = ByteString.copyFromUtf8("BOGUS");
    HeaderAndBody origHAB = HeaderAndBody.parseFrom(original.getHeaderAndBody());
    HeaderAndBody.Builder newHAB = HeaderAndBody.newBuilder(origHAB);
    Header.Builder newHeader = Header.newBuilder(origHAB.getHeader());
    Header origHeader = origHAB.getHeader();
    SecureMessage.Builder result = SecureMessage.newBuilder(original);
    switch (altType) {
      case DECRYPTION_KEY_ID:
        if (origHeader.hasDecryptionKeyId()) {
          newHeader.clearDecryptionKeyId();
        } else {
          newHeader.setDecryptionKeyId(ByteString.copyFrom(TEST_KEY_ID));
        }
        break;
      case ENCTYPE:
        if (origHeader.getEncryptionScheme() == SecureMessageProto.EncScheme.NONE) {
          newHeader.setEncryptionScheme(SecureMessageProto.EncScheme.AES_256_CBC);
        } else {
          newHeader.setEncryptionScheme(SecureMessageProto.EncScheme.NONE);
        }
        break;
      case HEADER_AND_BODY_PROTO:
        // Substitute a junk byte string instead of the HeeaderAndBody proto message
        return result.setHeaderAndBody(bogus).build();
      case MESSAGE:
        byte[] origBody = origHAB.getBody().toByteArray();
        if (origBody.length > 0) {
          // Lop off trailing byte of the body
          byte[] truncatedBody = CryptoOps.subarray(origBody, 0, origBody.length - 1);
          newHAB.setBody(ByteString.copyFrom(truncatedBody));
        } else {
          newHAB.setBody(bogus);
        }
        break;
      case METADATA:
        if (origHeader.hasPublicMetadata()) {
          newHeader.clearPublicMetadata();
        } else {
          newHeader.setPublicMetadata(bogus);
        }
        break;
      case RESIGNCRYPTION_ATTACK:
        // Simulate stripping a signature, and re-signing a message to see if it will be decrypted.
        newHeader
            .setVerificationKeyId(bogus)
            // In case original was cleartext
            .setEncryptionScheme(SecureMessageProto.EncScheme.AES_256_CBC);

        // Now that we've mildly changed the header, compute a new signature for it.
        newHAB.setHeader(newHeader.build());
        byte[] headerAndBodyBytes = newHAB.build().toByteArray();
        result.setHeaderAndBody(ByteString.copyFrom(headerAndBodyBytes));
        SigType sigType = SigType.valueOf(origHeader.getSignatureScheme());
        // Note that in all cases where this attack applies, the associatedData is not normally
        // used directly inside the signature (but rather inside the inner ciphertext).
        result.setSignature(ByteString.copyFrom(CryptoOps.sign(
            sigType, getSigningKeyFor(sigType), rng, headerAndBodyBytes)));
        return result.build();
      case SIGTYPE:
        if (origHeader.getSignatureScheme() == SecureMessageProto.SigScheme.ECDSA_P256_SHA256) {
          newHeader
              .setSignatureScheme(SecureMessageProto.SigScheme.HMAC_SHA256);
        } else {
          newHeader
              .setSignatureScheme(SecureMessageProto.SigScheme.ECDSA_P256_SHA256);
        }
        break;
      case VERIFICATION_KEY_ID:
        if (origHeader.hasVerificationKeyId()) {
          newHeader.clearVerificationKeyId();
        } else {
          newHeader.setVerificationKeyId(
              ByteString.copyFrom(TEST_KEY_ID));
        }
        break;
      case ASSOCIATED_DATA_LENGTH:
        int adLength = origHeader.getAssociatedDataLength();
        switch (adLength) {
          case 0:
            newHeader.setAssociatedDataLength(1);
            break;
          case 1:
            newHeader.setAssociatedDataLength(0);
            break;
          default:
            newHeader.setAssociatedDataLength(adLength - 1);
        }
        break;
      default:
        fail("Forgot to implement an alteration attack: " + altType);
        break;
    }
    // Set the header.
    newHAB.setHeader(newHeader.build());

    return result.setHeaderAndBody(ByteString.copyFrom(newHAB.build().toByteArray()))
        .build();
  }

  public void testEcDsaSignedOnly() throws Exception {
    doTestSignedOnly(SigType.ECDSA_P256_SHA256);
  }

  public void testRsaSignedOnly() throws Exception {
    doTestSignedOnly(SigType.RSA2048_SHA256);
  }

  public void testHmacSignedOnly() throws Exception {
    doTestSignedOnly(SigType.HMAC_SHA256);
  }

  private void doTestSignedOnly(SigType sigType) throws Exception {
    if (isUnsupported(sigType)) {
      return;
    }

    // decryptionKeyId must be left null for signature-only operation
    for (byte[] vkId : KEY_ID_VALUES) {
      verificationKeyId = vkId;
      for (byte[] md : METADATA_VALUES) {
        metadata = md;
        for (byte[] ad : ASSOCIATED_DATA_VALUES) {
          associatedData = ad;
          for (byte[] msg : MESSAGE_VALUES) {
            message = msg;
            doSignAndVerify(sigType);
          }
        }
      }
    }

    // Test that use of a DecryptionKeyId is not allowed for signature-only
    try {
      decryptionKeyId = TEST_KEY_ID;  // Should trigger a failure
      doSignAndVerify(sigType);
      fail();
    } catch (IllegalStateException expected) {
    }
}

  public void testEncryptedAndMACed() throws Exception {
    for (byte[] dkId : KEY_ID_VALUES) {
      decryptionKeyId = dkId;
      for (byte[] vkId : KEY_ID_VALUES) {
        verificationKeyId = vkId;
        for (byte[] md : METADATA_VALUES) {
          metadata = md;
          for (byte[] ad : ASSOCIATED_DATA_VALUES) {
            associatedData = ad;
            for (byte[] msg : MESSAGE_VALUES) {
              message = msg;
              doSignCryptAndVerifyDecrypt(SigType.HMAC_SHA256);
            }
          }
        }
      }
    }
  }

  public void testEncryptedAndMACedWithSameKey() throws Exception {
    hmacKey = aesEncryptionKey; // Re-use the same key for both
    testEncryptedAndMACed();
  }

  public void testEncryptedAndEcdsaSigned() throws Exception {
    doTestEncryptedAndSigned(SigType.ECDSA_P256_SHA256);
  }

  public void testEncryptedAndRsaSigned() throws Exception {
    doTestEncryptedAndSigned(SigType.RSA2048_SHA256);
  }

  public void doTestEncryptedAndSigned(SigType sigType) throws Exception {
    if (isUnsupported(sigType)) {
      return;  // EC operations aren't supported on older Android releases
    }

    for (byte[] dkId : KEY_ID_VALUES) {
      decryptionKeyId = dkId;
      for (byte[] vkId : KEY_ID_VALUES) {
        verificationKeyId = vkId;
        if ((verificationKeyId == null) && sigType.isPublicKeyScheme()) {
          continue;  // Null verificationKeyId is not allowed with public key signcryption
        }
        for (byte[] md : METADATA_VALUES) {
          metadata = md;
          for (byte[] ad : ASSOCIATED_DATA_VALUES) {
            associatedData = ad;
            for (byte[] msg : MESSAGE_VALUES) {
              message = msg;
              doSignCryptAndVerifyDecrypt(sigType);
            }
          }
        }
      }
    }

    // Verify that a missing verificationKeyId is not allowed here
    try {
      verificationKeyId = null;  // Should trigger a failure
      signCrypt(sigType, EncType.AES_256_CBC);
      fail();
    } catch (IllegalStateException expected) {
    }
  }

  public void testSignCryptionRequiresEncryption() throws Exception {
    try {
      signCrypt(SigType.RSA2048_SHA256, EncType.NONE);
    } catch (IllegalArgumentException expected) {
    }
  }

  public void testAssociatedData() throws Exception {
    // How much extra room might the encoding of AssociatedDataLength take up?
    int maxAssociatedDataOverheadBytes = 4;
    // How many bytes might normally vary in the encoding length for SecureMessages generated with
    // fresh randomness but identical contents (e.g., due to MSBs being 0)
    int maxJitter = 2;
    verificationKeyId = TEST_KEY_ID;  // So that public key signcryption will work
    message = TEST_MESSAGE;

    for (SigType sigType : SigType.values()) {
      if (isUnsupported(sigType)) {
        continue;
      }
      associatedData = null;
      SecureMessage signed = sign(sigType);
      int signedLength = signed.toByteArray().length;
      associatedData = EMPTY_BYTES;
      // Check that EMPTY_BYTES is equivalent to null associated data under verification
      verify(signed, sigType);
      // We already tested that incorrect associated data fails elsewhere in negative test cases
      associatedData = TEST_ASSOCIATED_DATA;
      SecureMessage signedWithAssociatedData = sign(sigType);
      int signedWithAssociatedDataLength = signedWithAssociatedData.toByteArray().length;
      String logInfo = "Testing associated data overhead for signature using: " + sigType
          + " signedLength=" + signedLength
          + " signedWithAssociatedDataLength=" + signedWithAssociatedDataLength;
      System.out.println(logInfo);
      assertTrue(logInfo,
          signedWithAssociatedData.toByteArray().length
          <= signed.toByteArray().length + maxAssociatedDataOverheadBytes + maxJitter);
    }

    for (SigType sigType : SigType.values()) {
      if (isUnsupported(sigType)) {
        continue;
      }
      associatedData = null;
      SecureMessage signCrypted = signCrypt(sigType, EncType.AES_256_CBC);
      int signCryptedLength = signCrypted.toByteArray().length;
      // Check that EMPTY_BYTES is equivalent to null associated data under verification
      associatedData = EMPTY_BYTES;
      verifyDecrypt(signCrypted, sigType, EncType.AES_256_CBC);
      // We already tested that incorrect associated data fails elsewhere in negative test cases
      associatedData = TEST_ASSOCIATED_DATA;
      SecureMessage signCryptedWithAssociatedData = signCrypt(sigType, EncType.AES_256_CBC);
      int signCryptedWithAssociatedDataLength = signCryptedWithAssociatedData.toByteArray().length;
      String logInfo = "Testing associated data overhead for signcryption using: " + sigType
          + " signCryptedLength=" + signCryptedLength
          + " signCryptedWithAssociatedDataLength=" + signCryptedWithAssociatedDataLength;
      System.out.println(logInfo);
      assertTrue(logInfo,
          signCryptedWithAssociatedData.toByteArray().length
          <= signCrypted.toByteArray().length + maxAssociatedDataOverheadBytes + maxJitter);
    }
  }

  public void testEncryptedAndEcdsaSignedUsingPublicKeyProto() throws Exception {
    if (isUnsupported(SigType.ECDSA_P256_SHA256)) {
      return;
    }

    // Safest usage of SignCryption is to set the VerificationKeyId to an actual representation of
    // the verification key.
    verificationKeyId = PublicKeyProtoUtil.encodeEcPublicKey(ecPublicKey).toByteArray();
    SecureMessage encryptedAndSigned = signCrypt(SigType.ECDSA_P256_SHA256, EncType.AES_256_CBC);

    // Simulate extracting the verification key ID from the SecureMessage (non-standard usage)
    ecPublicKey =
        PublicKeyProtoUtil.parseEcPublicKey(
            EcP256PublicKey.parseFrom(
                SecureMessageParser.getUnverifiedHeader(encryptedAndSigned)
                    .getVerificationKeyId()));

    // Note that this verification uses the encoded/decoded ecPublicKey value
    verifyDecrypt(encryptedAndSigned, SigType.ECDSA_P256_SHA256, EncType.AES_256_CBC);
  }

  public void testEncryptedAndRsaSignedUsingPublicKeyProto() throws Exception {
    // Safest usage of SignCryption is to set the VerificationKeyId to an actual representation of
    // the verification key.
    verificationKeyId = PublicKeyProtoUtil.encodeRsa2048PublicKey(rsaPublicKey).toByteArray();
    SecureMessage encryptedAndSigned = signCrypt(SigType.RSA2048_SHA256, EncType.AES_256_CBC);

    // Simulate extracting the verification key ID from the SecureMessage (non-standard usage)
    rsaPublicKey =
        PublicKeyProtoUtil.parseRsa2048PublicKey(
            SimpleRsaPublicKey.parseFrom(
                SecureMessageParser.getUnverifiedHeader(encryptedAndSigned)
                    .getVerificationKeyId()));

    // Note that this verification uses the encoded/decoded SimpleRsaPublicKey value
    verifyDecrypt(encryptedAndSigned, SigType.RSA2048_SHA256, EncType.AES_256_CBC);
  }

  // TODO(shabsi): The test was only corrupting header but wasn't setting the body. With protolite,
  // not setting a required field causes problems. Modify the SecureMessageParser test and
  // enable/remove this test.
  /*
  public void testCorruptUnverifiedHeader() throws Exception {
    // Create a sample message
    SecureMessage original = signCrypt(SigType.HMAC_SHA256, EncType.AES_256_CBC);
    HeaderAndBody originalHAB = HeaderAndBody.parseFrom(original.getHeaderAndBody().toByteArray());
    for (CorruptHeaderType corruptionType : CorruptHeaderType.values()) {
      // Mess with the HeaderAndBody field
      HeaderAndBody.Builder corruptHAB = HeaderAndBody.newBuilder(originalHAB);
      try {
        corruptHeaderWith(corruptionType, corruptHAB);
        // Construct the corrupted message using the modified HeaderAndBody
        SecureMessage.Builder corrupt = SecureMessage.newBuilder(original);
          corrupt.setHeaderAndBody(ByteString.copyFrom(corruptHAB.build().toByteArray())).build();
          SecureMessageParser.getUnverifiedHeader(corrupt.build());
          fail("Corrupt header type " + corruptionType + " parsed without error");
      } catch (InvalidProtocolBufferException expected) {
      }
    }
  }
  */

  public void testParseEmptyMessage() throws Exception {
    byte[] bogusData = new byte[0];

    try {
      SecureMessageParser.parseSignedCleartextMessage(
          SecureMessage.parseFrom(bogusData),
          aesEncryptionKey,
          SigType.HMAC_SHA256);
      fail("Empty message verified without error");
    } catch (SignatureException | UninitializedMessageException
        | InvalidProtocolBufferException expected) {
    }
  }

  public void testParseKeyInvalidInputs() throws Exception {
    GenericPublicKey[] badKeys = new GenericPublicKey[] {
        GenericPublicKey.newBuilder().setType(SecureMessageProto.PublicKeyType.EC_P256).build(),
        GenericPublicKey.newBuilder().setType(SecureMessageProto.PublicKeyType.RSA2048).build(),
        GenericPublicKey.newBuilder().setType(SecureMessageProto.PublicKeyType.DH2048_MODP).build(),
    };
    for (int i = 0; i < badKeys.length; i++) {
      GenericPublicKey key = badKeys[i];
      try {
        PublicKeyProtoUtil.parsePublicKey(key);
        fail(String.format("%sth key was parsed without exceptions", i));
      } catch (InvalidKeySpecException expected) {
      }
    }
  }

  enum CorruptHeaderType {
    EMPTY,
    // TODO(shabsi): Remove these test cases and modify code in SecureMessageParser appropriately.
    // UNSET,
    // JUNK,
  }

  private void corruptHeaderWith(CorruptHeaderType corruptionType,
      HeaderAndBody.Builder protoToModify) {
    switch (corruptionType) {
      case EMPTY:
        protoToModify.setHeader(Header.getDefaultInstance());
        break;
      /*
      case JUNK:
        Header.Builder junk = Header.newBuilder();
        junk.setDecryptionKeyId(ByteString.copyFromUtf8("fooooo"));
        junk.setIv(ByteString.copyFromUtf8("bar"));
        // Don't set signature scheme.
        junk.setVerificationKeyId(ByteString.copyFromUtf8("bazzzzz"));
        protoToModify.setHeader(junk.build());
        break;
      case UNSET:
        protoToModify.clearHeader();
        break;
      */
      default:
        throw new RuntimeException("Broken test code");
    }
  }

  private void consistencyCheck(
      SecureMessage secmsg, HeaderAndBody headerAndBody, SigType sigType, EncType encType)
      throws InvalidProtocolBufferException {
    Header header = SecureMessageParser.getUnverifiedHeader(secmsg);
    checkHeader(header, sigType, encType);  // Checks that the "unverified header" looks right
    checkHeaderAndBody(header, headerAndBody);  // Matches header vs. the "verified" headerAndBody
  }

  private Header checkHeader(Header header, SigType sigType, EncType encType) {
    assertEquals(sigType.getSigScheme(), header.getSignatureScheme());
    assertEquals(encType.getEncScheme(), header.getEncryptionScheme());
    checkKeyIdsAndMetadata(verificationKeyId, decryptionKeyId, metadata, associatedData, header);
    return header;
  }

  private void checkHeaderAndBody(Header header, HeaderAndBody headerAndBody) {
    assertTrue(header.equals(headerAndBody.getHeader()));
    assertTrue(Arrays.equals(message, headerAndBody.getBody().toByteArray()));
  }

  private void checkKeyIdsAndMetadata(byte[] verificationKeyId, byte[] decryptionKeyId,
      byte[] metadata, byte[] associatedData, Header header) {
    if (verificationKeyId == null) {
      assertFalse(header.hasVerificationKeyId());
    } else {
      assertTrue(Arrays.equals(verificationKeyId, header.getVerificationKeyId().toByteArray()));
    }
    if (decryptionKeyId == null) {
      assertFalse(header.hasDecryptionKeyId());
    } else {
      assertTrue(Arrays.equals(decryptionKeyId, header.getDecryptionKeyId().toByteArray()));
    }
    if (metadata == null) {
      assertFalse(header.hasPublicMetadata());
    } else {
      assertTrue(Arrays.equals(metadata, header.getPublicMetadata().toByteArray()));
    }
    if (associatedData == null) {
      assertFalse(header.hasAssociatedDataLength());
    } else {
      assertEquals(associatedData.length, header.getAssociatedDataLength());
    }
  }

  private SecretKey makeAesKey() throws NoSuchAlgorithmException {
    KeyGenerator aesKeygen = KeyGenerator.getInstance("AES");
    aesKeygen.init(256);
    return aesKeygen.generateKey();
  }

  private Key getSigningKeyFor(SigType sigType) {
    if (sigType == SigType.ECDSA_P256_SHA256) {
      return ecPrivateKey;
    }
    if (sigType == SigType.RSA2048_SHA256) {
      return rsaPrivateKey;
    }
    if (sigType == SigType.HMAC_SHA256) {
      return hmacKey;
    }
    return null;  // This should not happen
  }

  private Key getVerificationKeyFor(SigType sigType) {
    try {
      if (sigType == SigType.ECDSA_P256_SHA256) {
        return PublicKeyProtoUtil.parseEcPublicKey(
            PublicKeyProtoUtil.encodeEcPublicKey(ecPublicKey));
      }
      if (sigType == SigType.RSA2048_SHA256) {
        return PublicKeyProtoUtil.parseRsa2048PublicKey(
            PublicKeyProtoUtil.encodeRsa2048PublicKey(rsaPublicKey));
      }
    } catch (InvalidKeySpecException e) {
      throw new AssertionError(e);
    }

    assertFalse(sigType.isPublicKeyScheme());
    // For symmetric key schemes
    return getSigningKeyFor(sigType);
  }

  private SecureMessageBuilder getPreconfiguredBuilder() {
    // Re-use a single instance of SecureMessageBuilder for efficiency.
    SecureMessageBuilder builder = secureMessageBuilder.reset();
    if (verificationKeyId != null) {
      builder.setVerificationKeyId(verificationKeyId);
    }
    if (decryptionKeyId != null) {
      builder.setDecryptionKeyId(decryptionKeyId);
    }
    if (metadata != null) {
      builder.setPublicMetadata(metadata);
    }
    if (associatedData != null) {
      builder.setAssociatedData(associatedData);
    }
    return builder;
  }

  private static boolean isUnsupported(SigType sigType) {
    // EC operations aren't supported on older Android releases
    return PublicKeyProtoUtil.isLegacyCryptoRequired()
        && (sigType == SigType.ECDSA_P256_SHA256);
  }

  private static boolean isRunningInAndroid() {
    try {
      ClassLoader.getSystemClassLoader().loadClass("android.os.Build$VERSION");
      return true;
    } catch (ClassNotFoundException e) {
      // Not running on Android
      return false;
    }
  }
}
