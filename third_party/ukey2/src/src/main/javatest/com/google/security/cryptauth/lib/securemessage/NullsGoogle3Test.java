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

import com.google.common.testing.NullPointerTester;
import com.google.security.cryptauth.lib.securemessage.SecureMessageProto.SecureMessage;
import junit.framework.TestCase;

/**
 * Non-portable Google3-based test to check null pointer behavior.
 */
public class NullsGoogle3Test extends TestCase {

  /**
   *  We test all of the classes in one place to avoid a proliferation of similar test cases,
   * noting that {@link NullPointerTester} emits the name of the class where the breakge occurs.
   */
  public void testNulls() {
    final NullPointerTester tester = new NullPointerTester();
    tester.testAllPublicStaticMethods(CryptoOps.class);
    tester.testAllPublicStaticMethods(PublicKeyProtoUtil.class);

    tester.setDefault(SecureMessage.class, SecureMessage.getDefaultInstance());
    tester.testAllPublicStaticMethods(SecureMessageParser.class);

    tester.testAllPublicStaticMethods(SecureMessageBuilder.class);
    tester.testAllPublicConstructors(SecureMessageBuilder.class);
    tester.testAllPublicInstanceMethods(new SecureMessageBuilder());
  }
}
