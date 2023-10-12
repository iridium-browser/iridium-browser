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

/**
 * LDT-XTS-AES128 implementation using the default "swap" mix function. It is suitable for
 * encryption or decryption of individual Nearby Presence BLE 4.2 legacy format encrypted payloads.
 *
 * <p>To avoid leaking resources, call close() once an instance is no longer needed.
 *
 * <p>This class is not thread safe.
 */
public class LdtNpCipher implements AutoCloseable {

  /**
   * Size in bytes of key seed used to derive further keys used for in np ldt operations
   */
  private static final int KEY_SEED_SIZE = 32;
   /**
   * Size in bytes of the metadata keys calculated hmac tag
   */
  private static final int TAG_SIZE = 32;
  /** Block size of AES. */
  private static final int BLOCK_SIZE = 16;

  private final long ldtHandle;
  private boolean closed = false;

  private LdtNpCipher(long ldtHandle) {
    this.ldtHandle = ldtHandle;
  }

  /**
   * Create a new Ldt instance using LDT-XTS-AES128 with the "swap" mix function.
   *
   * @param key_seed 64-byte key material from the credential for the identity used to broadcast. The
   *     supplied byte[] can be zeroed out once this method returns, as the contents are copied.
   * @return an instance configured with the supplied key
   * @throws LdtException if the key is the wrong size
   * @throws LdtException if the tag is the wrong size
   * @throws LdtException if creating the instance fails
   */
  public static LdtNpCipher fromKey(byte[] key_seed, byte[] metadata_key_tag) throws LdtException {
    if (key_seed.length != KEY_SEED_SIZE) {
      throw new LdtException("Key must be " + KEY_SEED_SIZE + " bytes");
    }
    if (metadata_key_tag.length != TAG_SIZE) {
      throw new LdtException("Tag must be " + TAG_SIZE + " bytes");
    }

    long handle = LdtNpJni.createLdtCipher(key_seed, metadata_key_tag);
    if (handle == 0) {
      throw new LdtException("Creating Ldt native resources failed");
    }

    return new LdtNpCipher(handle);
  }

  /**
   * Encode a 2 byte salt as a big-endian char.
   *
   * @return a char with b1 in the high bits and b2 in the low bits
   */
  public static char saltAsChar(byte b1, byte b2) {
    // byte widening conversion to int sign-extends
    int highBits = b1 << 8;
    int lowBits = b2 & 0xFF;
    // narrowing conversion truncates to low 16 bits
    return (char) (highBits | lowBits);
  }

  /**
   * Encrypt data in place, XORing bytes derived from the salt into the LDT tweaks.
   *
   * @param salt the salt that will be used in the advertisement with this encrypted payload. See
   *     {@link LdtNpCipher#saltAsChar(byte, byte)} for constructing the char
   *     representation.
   * @param data plaintext to encrypt in place: the metadata key followed by the data elements to be
   *     encrypted. The length must be in [16, 31).
   * @throws IllegalStateException if this instance has already been closed
   * @throws IllegalArgumentException if data is the wrong length
   * @throws LdtException if encryption fails
   */
  public void encrypt(char salt, byte[] data) throws LdtException {
    checkPreconditions(data);

    int res = LdtNpJni.encrypt(ldtHandle, salt, data);
    if (res < 0) {
      // TODO is it possible for this to fail if the length is correct?
      throw new LdtException("Could not encrypt: error code " + res);
    }
  }

  /**
   * Decrypt the data in place, XORing the LDT tweak with the provided bytes.
   *
   * @param salt the salt extracted from the advertisement that contained this payload. See {@link
   *     LdtNpCipher#saltAsChar(byte, byte)} for constructing the char representation.
   * @param data ciphertext to decrypt in place: the metadata key followed by the data elements to
   *     be decrypted. The length must be in [16, 31).
   * @throws IllegalStateException if this instance has already been closed
   * @throws IllegalArgumentException if data is the wrong length
   * @throws LdtException if decryption fails
   */
  public void decrypt_and_verify(char salt, byte[] data) throws LdtException {
    checkPreconditions(data);

    int res = LdtNpJni.decrypt_and_verify(ldtHandle, salt, data);
    if (res < 0) {
      // TODO is it possible for this to fail if the length is correct?
      throw new LdtException("Could not decrypt: error code " + res);
    }
  }

  private void checkPreconditions(byte[] data) {
    if (closed) {
      throw new IllegalStateException("Instance has been closed");
    }
    if (data.length < BLOCK_SIZE || data.length >= BLOCK_SIZE * 2) {
      throw new IllegalArgumentException(
          "Data must be at least " + BLOCK_SIZE + " and less than " + BLOCK_SIZE * 2 + " bytes");
    }
  }

  /**
   * Releases native resources.
   *
   * <p>Once closed, an Ldt instance cannot be used further.
   */
  @Override
  public void close() {
    if (closed) {
      return;
    }
    closed = true;

    int res = LdtNpJni.closeLdtCipher(ldtHandle);
    if (res < 0) {
      throw new RuntimeException("Could not close Ldt: error code " + res);
    }
  }

  public static class LdtException extends Exception {
    LdtException(String message) {
      super(message);
    }
  }
}
