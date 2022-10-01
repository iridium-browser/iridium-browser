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

import static org.hamcrest.CoreMatchers.containsString;
import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertThat;

import com.google.security.cryptauth.lib.securegcm.Ed25519.Ed25519Exception;
import java.math.BigInteger;
import junit.framework.TestCase;

/**
 * Android compatible tests for the {@link Ed25519} class.
 */
public class Ed25519Test extends TestCase {

  // Points on the curve
  private static final int HEX_RADIX = 16;
  private static final BigInteger[] KM = new BigInteger[] {
    new BigInteger("1981FB43F103290ECF9772022DB8B19BFAF389057ED91E8486EB368763435925", HEX_RADIX),
    new BigInteger("A714C34F3B588AAC92FD2587884A20964FD351A1F147D5C4BBF5C2F37A77C36", HEX_RADIX)};
  private static final BigInteger[] KN = new BigInteger[] {
    new BigInteger("201A184F47D9A7973891D148E3D1C864D8084547131C2C1CEFB7EEBD26C63567", HEX_RADIX),
    new BigInteger("6DA2D3B18EC4F9AA3B08E39C997CD8BF6E9948FFD4FEFFECAF8DD0B3D648B7E8", HEX_RADIX)};

  // Curve prime P
  private static final BigInteger P =
      new BigInteger("7FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFED", HEX_RADIX);

  // Test vectors obtain by multiplying KM by k by manually using the official implementation
  // see: http://ed25519.cr.yp.to/python/ed25519.py
  // k = 2
  private static final BigInteger[] KM_2 = new BigInteger[] {
    new BigInteger("718079972e63c2d62caf0ee93ec6f00337ceaff4e283181c04c4082b1d5e1ecf", HEX_RADIX),
    new BigInteger("143d18d393a8058c8614335bf36bf59364cc7c451db74726b322ce9d0b826d51", HEX_RADIX)
  };
  // k = 3
  private static final BigInteger[] KM_3 = new BigInteger[] {
    new BigInteger("39DA3C92EFC0577586B4D58F4A5C0BF65A6CC8F6BF358F38D70B2E6C28A31E8E", HEX_RADIX),
    new BigInteger("6D194F054B3FC2BE217F6A360BBEC747D2937FCEBD74B67FC3B20ED638ADD670", HEX_RADIX)
  };
  // k = 317698
  private static final BigInteger[] KM_317698 = new BigInteger[] {
    new BigInteger("7945D0ADEB568B16495476E81ADF281F4515439AE835914FBF6CEEAFEB9CD7E8", HEX_RADIX),
    new BigInteger("3631503DCDEBC0BF9BB1FFC3984A8CB52A34FFC2E77E9C19FD896DC6EE64A530", HEX_RADIX)
  };
  // k = P
  private static final BigInteger[] KM_HUGE = new BigInteger[] {
    new BigInteger("530162B05F440E00E219DFD3188524821C860C41FD87B9AC6AF2A283FDD585A1", HEX_RADIX),
    new BigInteger("48385A7D2BB858F3DB7F72E7CDFE218B9CA84DDA8BD64C3775AA43551D974F60", HEX_RADIX)
  };
  // k = P + 10000
  private static final BigInteger[] KM_XRAHUGE = new BigInteger[] {
    new BigInteger("16377E9F5EE2C0F4C70E17AC298EF670700A7CB186EEB0DA10CDD59635000AF8", HEX_RADIX),
    new BigInteger("5BD7921EEE662ACBAC3A96D8B6039D2356F154859FAF41FD2F0D99DF06CD2EAE", HEX_RADIX)
  };

  // Helpful constants
  private static final BigInteger ONE = BigInteger.ONE;
  private static final BigInteger ZERO = BigInteger.ZERO;

  // Identity element of the group (the zero) in affine and extended representations
  private static final BigInteger[] ID = new BigInteger[] {ZERO, ONE};
  private static final BigInteger[] ID_EX = new BigInteger[] {ZERO, ONE, ONE, ZERO};

  public void testValidPoints() throws Exception {
    if (KeyEncoding.isLegacyCryptoRequired()) {
      // this means we're running on an old SDK, which doesn't support the
      // necessary crypto. Let's not test anything in this case.
      return;
    }

    // We've got a couple of valid points
    Ed25519.validateAffinePoint(KM);
    Ed25519.validateAffinePoint(KN);
    
    // And a bunch of invalid ones
    try {
      Ed25519.validateAffinePoint(new BigInteger[] {ZERO, ONE});
      fail("Validate point not catching zero x coordinates");
    } catch (Ed25519Exception e) {
      assertThat(e.getMessage(), containsString("positive"));
    }
    
    try {
      Ed25519.validateAffinePoint(new BigInteger[] {ONE, ZERO});
      fail("Validate point not catching zero y coordinates");
    } catch (Ed25519Exception e) {
      assertThat(e.getMessage(), containsString("positive"));
    }
    
    try {
      Ed25519.validateAffinePoint(new BigInteger[] {new BigInteger("-1"), ONE});
      fail("Validate point not catching negative x coordinates");
    } catch (Ed25519Exception e) {
      assertThat(e.getMessage(), containsString("positive"));
    }
    
    try {
      Ed25519.validateAffinePoint(new BigInteger[] {ONE, new BigInteger("-1")});
      fail("Validate point not catching negative y coordinates");
    } catch (Ed25519Exception e) {
      assertThat(e.getMessage(), containsString("positive"));
    }
    
    try {
      Ed25519.validateAffinePoint(new BigInteger[] {ONE, ONE});
      fail("Validate point not catching points that are not on curve");
    } catch (Ed25519Exception e) {
      assertThat(e.getMessage(), containsString("expected curve"));
    }
  }

  public void testAffineExtendedConversion() throws Exception {
    BigInteger[] km1 = Ed25519.toAffine(Ed25519.toExtended(KM));
    BigInteger[] kn1 = Ed25519.toAffine(Ed25519.toExtended(KN));

    assertArrayEquals(KM, km1);
    assertArrayEquals(KN, kn1);

    assertArrayEquals(ID, Ed25519.toAffine(ID_EX));
    assertArrayEquals(ID_EX, Ed25519.toExtended(ID));
  }

  public void testRepresentationCheck() throws Exception {
    Ed25519.checkPointIsInAffineRepresentation(KM);
    Ed25519.checkPointIsInExtendedRepresentation(ID_EX);

    try {
      Ed25519.checkPointIsInExtendedRepresentation(KM);
      fail("Point is not really in extended representation, expected failure");
    } catch (Ed25519Exception e) {      
      assertThat(e.getMessage(), containsString("not in extended"));
    }

    try {
      Ed25519.checkPointIsInAffineRepresentation(Ed25519.toExtended(KM));
      fail("Point is not really in affine representation, expected failure");
    } catch (Ed25519Exception e) {     
      assertThat(e.getMessage(), containsString("not in affine"));
    }
  }

  public void testAddSubtractExtendedPoints() throws Exception {
    // Adding/subtracting identity to/from itself should yield the identity point
    assertArrayEquals(ID, Ed25519.addAffinePoints(ID, ID));
    assertArrayEquals(ID, Ed25519.subtractAffinePoints(ID, ID));

    // In fact adding/subtracting the identity point to/from any point should yield that point
    assertArrayEquals(KM, Ed25519.addAffinePoints(KM, ID));
    assertArrayEquals(KM, Ed25519.subtractAffinePoints(KM, ID));

    // Subtracting a point from itself should yield the identity element
    assertArrayEquals(ID, Ed25519.subtractAffinePoints(KM, KM));
    assertArrayEquals(ID, Ed25519.subtractAffinePoints(KN, KN));

    // Adding and subtracting should yield the same point
    assertArrayEquals(KM, Ed25519.subtractAffinePoints(Ed25519.addAffinePoints(KM, KN), KN));
    assertArrayEquals(KN, Ed25519.subtractAffinePoints(Ed25519.addAffinePoints(KN, KM), KM));
  }

  public void testScalarMultiplyExtendedPoints() throws Exception {
    // A point times one is the point itself
    assertArrayEquals(KM, Ed25519.scalarMultiplyAffinePoint(KM, ONE));
    assertArrayEquals(KN, Ed25519.scalarMultiplyAffinePoint(KN, ONE));

    // A point times zero is the identity point
    assertArrayEquals(ID, Ed25519.scalarMultiplyAffinePoint(KM, ZERO));
    assertArrayEquals(ID, Ed25519.scalarMultiplyAffinePoint(KN, ZERO));

    // The identity times a scalar is the identity
    assertArrayEquals(ID, Ed25519.scalarMultiplyAffinePoint(ID, BigInteger.valueOf(317698)));

    // Use test vectors
    assertArrayEquals(KM_2, Ed25519.scalarMultiplyAffinePoint(KM, BigInteger.valueOf(2)));
    assertArrayEquals(KM_3, Ed25519.scalarMultiplyAffinePoint(KM, BigInteger.valueOf(3)));
    assertArrayEquals(KM_317698, Ed25519.scalarMultiplyAffinePoint(KM, BigInteger.valueOf(317698)));
    assertArrayEquals(KM_HUGE, Ed25519.scalarMultiplyAffinePoint(KM, P));
    assertArrayEquals(KM_XRAHUGE,
        Ed25519.scalarMultiplyAffinePoint(KM, P.add(BigInteger.valueOf(10000))));
  }
}
