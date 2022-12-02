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

import static org.junit.Assert.assertThrows;

import com.google.security.cryptauth.lib.securemessage.CryptoOps.EncType;
import com.google.security.cryptauth.lib.securemessage.CryptoOps.SigType;
import java.util.Arrays;
import javax.crypto.KeyGenerator;
import javax.crypto.SecretKey;
import javax.crypto.spec.SecretKeySpec;
import junit.framework.TestCase;

/**
 * Unit tests for the CryptoOps class
 */
public class CryptoOpsTest extends TestCase {

  /** HKDF Test Case 1 IKM from RFC 5869 */
  private static final byte[] HKDF_CASE1_IKM = {
    0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
    0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
    0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
    0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
    0x0b, 0x0b
  };

  /** HKDF Test Case 1 salt from RFC 5869 */
  private static final byte[] HKDF_CASE1_SALT = {
    0x00, 0x01, 0x02, 0x03, 0x04,
    0x05, 0x06, 0x07, 0x08, 0x09,
    0x0a, 0x0b, 0x0c
  };

  /** HKDF Test Case 1 info from RFC 5869 */
  private static final byte[] HKDF_CASE1_INFO = {
    (byte) 0xf0, (byte) 0xf1, (byte) 0xf2, (byte) 0xf3, (byte) 0xf4,
    (byte) 0xf5, (byte) 0xf6, (byte) 0xf7, (byte) 0xf8, (byte) 0xf9
  };

  /** First 32 bytes of HKDF Test Case 1 OKM (output) from RFC 5869 */
  private static final byte[] HKDF_CASE1_OKM = {
    (byte) 0x3c, (byte) 0xb2, (byte) 0x5f, (byte) 0x25, (byte) 0xfa,
    (byte) 0xac, (byte) 0xd5, (byte) 0x7a, (byte) 0x90, (byte) 0x43,
    (byte) 0x4f, (byte) 0x64, (byte) 0xd0, (byte) 0x36, (byte) 0x2f,
    (byte) 0x2a, (byte) 0x2d, (byte) 0x2d, (byte) 0x0a, (byte) 0x90,
    (byte) 0xcf, (byte) 0x1a, (byte) 0x5a, (byte) 0x4c, (byte) 0x5d,
    (byte) 0xb0, (byte) 0x2d, (byte) 0x56, (byte) 0xec, (byte) 0xc4,
    (byte) 0xc5, (byte) 0xbf, (byte) 0x34, (byte) 0x00, (byte) 0x72,
    (byte) 0x08, (byte) 0xd5, (byte) 0xb8, (byte) 0x87, (byte) 0x18,
    (byte) 0x58, (byte) 0x65
  };

  private SecretKey aesKey1;
  private SecretKey aesKey2;

  @Override
  protected void setUp() throws Exception {
    KeyGenerator aesKeygen = KeyGenerator.getInstance("AES");
    aesKeygen.init(256);
    aesKey1 = aesKeygen.generateKey();
    aesKey2 = aesKeygen.generateKey();
    super.setUp();
  }

  public void testNoPurposeConflicts() {
    // Ensure that signature algorithms and encryption algorithms are not given identical purposes
    // (this prevents confusion of derived keys).
    for (SigType sigType : SigType.values()) {
      for (EncType encType : EncType.values()) {
        assertFalse(CryptoOps.getPurpose(sigType).equals(CryptoOps.getPurpose(encType)));
      }
    }
  }

  public void testDeriveAes256KeyFor() throws Exception {
    // Test that deriving with the same key and purpose twice is deterministic
    assertTrue(Arrays.equals(CryptoOps.deriveAes256KeyFor(aesKey1, "A").getEncoded(),
                             CryptoOps.deriveAes256KeyFor(aesKey1, "A").getEncoded()));
    // Test that derived keys with different purposes differ
    assertFalse(Arrays.equals(CryptoOps.deriveAes256KeyFor(aesKey1, "A").getEncoded(),
                              CryptoOps.deriveAes256KeyFor(aesKey1, "B").getEncoded()));
    // Test that derived keys with the same purpose but different master keys differ
    assertFalse(Arrays.equals(CryptoOps.deriveAes256KeyFor(aesKey1, "A").getEncoded(),
                              CryptoOps.deriveAes256KeyFor(aesKey2, "A").getEncoded()));
  }

  public void testHkdf() throws Exception {
    SecretKey inputKey = new SecretKeySpec(HKDF_CASE1_IKM, "AES");
    byte[] result = CryptoOps.hkdf(inputKey, HKDF_CASE1_SALT, HKDF_CASE1_INFO);
    byte[] expectedResult = Arrays.copyOf(HKDF_CASE1_OKM, 32);
    assertTrue(Arrays.equals(result, expectedResult));
  }

  public void testHkdfLongOutput() throws Exception {
    SecretKey inputKey = new SecretKeySpec(HKDF_CASE1_IKM, "AES");
    byte[] result = CryptoOps.hkdf(inputKey, HKDF_CASE1_SALT, HKDF_CASE1_INFO, 42);
    byte[] expectedResult = Arrays.copyOf(HKDF_CASE1_OKM, 42);
    assertTrue(Arrays.equals(result, expectedResult));
  }

  public void testHkdfShortOutput() throws Exception {
    SecretKey inputKey = new SecretKeySpec(HKDF_CASE1_IKM, "AES");
    byte[] result = CryptoOps.hkdf(inputKey, HKDF_CASE1_SALT, HKDF_CASE1_INFO, 12);
    byte[] expectedResult = Arrays.copyOf(HKDF_CASE1_OKM, 12);
    assertTrue(Arrays.equals(result, expectedResult));
  }

  public void testHkdfInvalidLengths() throws Exception {
    SecretKey inputKey = new SecretKeySpec(HKDF_CASE1_IKM, "AES");

    // Negative length
    assertThrows(
        IllegalArgumentException.class,
        () -> CryptoOps.hkdf(inputKey, HKDF_CASE1_SALT, HKDF_CASE1_INFO, -5));

    // Too long, would be more than 256 blocks
    assertThrows(
        IllegalArgumentException.class,
        () -> CryptoOps.hkdf(inputKey, HKDF_CASE1_SALT, HKDF_CASE1_INFO, 32 * 256 + 1));
  }

  public void testConcat() {
    byte[] a = { 1, 2, 3, 4};
    byte[] b = { 5 , 6 };
    byte[] expectedResult = { 1, 2, 3, 4, 5, 6 };
    byte[] result = CryptoOps.concat(a, b);
    assertEquals(a.length + b.length, result.length);
    assertTrue(Arrays.equals(expectedResult, result));

    byte[] empty =  { };
    assertEquals(0, CryptoOps.concat(empty, empty).length);
    assertTrue(Arrays.equals(a, CryptoOps.concat(a, empty)));
    assertTrue(Arrays.equals(a, CryptoOps.concat(empty, a)));

    assertEquals(0, CryptoOps.concat(null, null).length);
    assertTrue(Arrays.equals(a, CryptoOps.concat(a, null)));
    assertTrue(Arrays.equals(a, CryptoOps.concat(null, a)));
  }

  public void testSubarray() {
    byte[] in = { 1, 2, 3, 4, 5, 6, 7 };
    assertTrue(Arrays.equals(in, CryptoOps.subarray(in, 0, in.length)));
    assertEquals(0, CryptoOps.subarray(in, 0, 0).length);
    byte[] expectedResult1 = { 1 };
    assertTrue(Arrays.equals(expectedResult1, CryptoOps.subarray(in, 0, 1)));
    byte[] expectedResult34 = { 3, 4 };
    assertTrue(Arrays.equals(expectedResult34, CryptoOps.subarray(in, 2, 4)));
    assertThrows(IndexOutOfBoundsException.class, () -> CryptoOps.subarray(in, 0, in.length + 1));
    assertThrows(IndexOutOfBoundsException.class, () -> CryptoOps.subarray(in, -1, in.length));
    assertThrows(
        IndexOutOfBoundsException.class, () -> CryptoOps.subarray(in, in.length, in.length));
    assertThrows(
        IndexOutOfBoundsException.class,
        () -> CryptoOps.subarray(in, Integer.MIN_VALUE, in.length));
    assertThrows(
        IndexOutOfBoundsException.class, () -> CryptoOps.subarray(in, 1, Integer.MIN_VALUE));
  }
}
