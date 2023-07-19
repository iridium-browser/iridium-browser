/*
 * Copyright 2023 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.google.android.gms.nearby.presence.hazmat

import org.junit.jupiter.api.Assertions
import org.junit.jupiter.api.Test

class LdtNpJniTests {
  @Test
  fun ldtRoundTripTest() {
    // Data taken from first test case in ldt_np_adv/resources/test/np_adv_test_vectors.json
    val key_seed = "CCDB2489E9FCAC42B39348B8941ED19A1D360E75E098C8C15E6B1CC2B620CD39".decodeHex()
    val hmac_tag = "DFB90A1F9B1FE28D18BBCCA52240B5CC2CCB5F8D5289A3CB64EB3541CA614BB4".decodeHex()
    val plaintext = "CD683FE1A1D1F846543D0A13D4AEA40040C8D67B".decodeHex()
    val salt_bytes = "32EE".decodeHex()
    val expected_ciphertext = "04344411F1E57C841FE0F7150636BC782455059A".decodeHex()
    val salt = LdtNpCipher.saltAsChar(salt_bytes[0], salt_bytes[1])

    val data = plaintext.copyOf()
    val LdtCipher = LdtNpCipher.fromKey(key_seed, hmac_tag)
    LdtCipher.encrypt(salt, data)
    Assertions.assertArrayEquals(expected_ciphertext, data)

    LdtCipher.decrypt_and_verify(salt, data)
    Assertions.assertArrayEquals(plaintext, data)
  }
}

private fun ByteArray.toHex(): String =
  joinToString(separator = "") { eachByte -> "%02x".format(eachByte) }

private fun String.decodeHex(): ByteArray {
  check(length % 2 == 0)
  return chunked(2)
    .map { it.toInt(16).toByte() }
    .toByteArray()
}