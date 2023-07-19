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

#include <openssl/aes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "np_ldt.h"

// Fuzz decrypting data that doesn't match the hmac
int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  // data:
  // 32 byte key seed
  // 2 byte salt
  // 31 byte plaintext
  // 1 byte length of plaintext to use
  // 32 byte metadata key hmac
  if (size < 98) {
    return -1;
  }

  NpLdtKeySeed key_seed;
  memcpy(&key_seed.bytes, data, 32);

  NpLdtSalt salt;
  memcpy(&salt.bytes, data + 32, 2);

  uint8_t payload[31];
  memcpy(&payload, data + 32 + 2, 31);

  uint8_t payload_len;
  memcpy(&payload_len, data + 32 + 2 + 31, 1);
  // length in [16, 31]
  payload_len = 16 + (payload_len % 16);

  NpMetadataKeyHmac metadata_key_hmac;
  memcpy(&metadata_key_hmac.bytes, data + 32 + 2 + 31 + 1, 32);

  // create a cipher
  NpLdtEncryptHandle enc_handle = NpLdtEncryptCreate(key_seed);
  if (enc_handle.handle == 0) {
    printf("Error: create LDT failed\n");
    __builtin_trap();
    return 0;
  }

  // encrypt with it
  NP_LDT_RESULT result = NpLdtEncrypt(enc_handle, payload, payload_len, salt);
  if (result != 0) {
    printf("Error: encrypt\n");
    __builtin_trap();
    return 0;
  }

  NpLdtDecryptHandle dec_handle = NpLdtDecryptCreate(key_seed, metadata_key_hmac);
  if (dec_handle.handle == 0) {
    printf("Error: create LDT failed\n");
    __builtin_trap();
    return 0;
  }

  // decrypt & verify -- we expect mac mismatch since we're using a random mac
  result = NpLdtDecryptAndVerify(dec_handle, payload, payload_len, salt);
  if (result != -2) {
    printf("Error: decryption didn't fail with the expected MAC mismatch\n");
    __builtin_trap();
    return 0;
  }

  // deallocate the cipher
  result = NpLdtEncryptClose(enc_handle);
  if (result) {
    printf("Error: close cipher\n");
    __builtin_trap();
    return result;
  }

  result = NpLdtDecryptClose(dec_handle);
  if (result) {
    printf("Error: close cipher failed\n");
    __builtin_trap();
    return result;
  }

  return 0;  // Values other than 0 and -1 are reserved for future use.
}
