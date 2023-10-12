/*
 * Copyright 2022 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.google.android.gms.nearby.presence.hazmat;

/** JNI for LDT-XTS-AES128 with the "swap" mix function. */
class LdtNpJni {

  static {
    System.loadLibrary("ldt_np_jni");
  }

  /**
   * Create an instance of LDT-XTS-AES-128 using the Swap mix function.
   *
   * @param key key bytes, must be 4x AES key size = 64 bytes
   * @return 0 on error, and any other value for success
   */
  static native long createLdtCipher(byte[] key_seed, byte[] metadata_key_hmac_tag);

  /**
   * Close the native resources for an Ldt instance.
   *
   * @param ldtHandle An ldt handle returned from {@link LdtNpJni#createLdtCipher}.
   * @return 0 on success, <0 for any error
   */
  static native int closeLdtCipher(long ldtHandle);

  /**
   * Encrypt the data in place.
   *
   * @param ldtHandle An ldt handle returned from {@link LdtNpJni#createLdtCipher}.
   * @param salt big-endian salt to be expanded into bytes XORd into the LDT tweaks
   * @param data size must be between 16 and 31 bytes
   * @return 0 on success, -1 if the data size is wrong, or another negative number for any other
   *     error
   */
  static native int encrypt(long ldtHandle, char salt, byte[] data);

  /**
   * Decrypt the data in place using the default LDT tweak padding scheme.
   *
   * @param ldtHandle An ldt address returned from {@link LdtNpJni#createLdtCipher}.
   * @param salt big-endian salt to be expanded into bytes XORd into the LDT tweaks
   * @param data size must be between 16 and 31 bytes
   * @return 0 on success, -1 if the data size is wrong, -2 if the calculated hmac
   *     does not match the provided tag or another negative number for any other error
   */
  static native int decrypt_and_verify(long ldtHandle, char salt, byte[] data);
}
