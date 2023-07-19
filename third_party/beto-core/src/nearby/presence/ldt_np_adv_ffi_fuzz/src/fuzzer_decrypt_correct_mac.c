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

#include <openssl/core.h>
#include <openssl/core_names.h>
#include <openssl/hmac.h>
#include <openssl/kdf.h>
#include <openssl/obj_mac.h>
#include <openssl/params.h>
#include <stddef.h>
#include <string.h>

#include "np_ldt.h"

// Fuzz decrypting data that doesn't match the hmac
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  // data:
  // 32 byte key seed
  // 2 byte salt
  // 31 byte plaintext
  // 1 byte length of plaintext to use
  if (size < 66) {
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

  // calculate metadata key hmac key by HKDFing key seed
  // HKDF code from https://www.openssl.org/docs/man3.0/man7/EVP_KDF-HKDF.html
  EVP_KDF *kdf;
  EVP_KDF_CTX *kctx;
  // 32 byte HMAC-SHA256 key
  uint8_t metadata_key_hmac_key[32] = {0};
  OSSL_PARAM params[5], *p = params;

  kdf = EVP_KDF_fetch(NULL, "HKDF", NULL);
  if (kdf == NULL) {
    printf("Couldn't allocate KDF\n");
    __builtin_trap();
    return 0;
  }
  kctx = EVP_KDF_CTX_new(kdf);
  EVP_KDF_free(kdf);

  *p++ = OSSL_PARAM_construct_utf8_string(OSSL_KDF_PARAM_DIGEST, SN_sha256,
                                          strlen(SN_sha256));
  *p++ = OSSL_PARAM_construct_octet_string(OSSL_KDF_PARAM_KEY, &key_seed.bytes,
                                           (size_t)32);
  *p++ = OSSL_PARAM_construct_octet_string(
      OSSL_KDF_PARAM_INFO, "Legacy metadata key verification HMAC key",
      (size_t)41);
  *p++ = OSSL_PARAM_construct_octet_string(OSSL_KDF_PARAM_SALT, "Google Nearby",
                                           (size_t)13);
  *p = OSSL_PARAM_construct_end();
  if (EVP_KDF_derive(kctx, metadata_key_hmac_key, sizeof(metadata_key_hmac_key),
                     params) <= 0) {
    printf("HKDF error\n");
    __builtin_trap();
    return 0;
  }

  EVP_KDF_CTX_free(kctx);

  // calculate metadata key hmac using hkdf'd hmac key
  NpMetadataKeyHmac metadata_key_hmac = {.bytes = {0}};
  // will be written to by HMAC call, but it will always be
  // 32 because that's what SHA256 outputs
  unsigned int md_len = 32;
  // first 14 bytes of payload are metadata key
  HMAC(EVP_sha256(), metadata_key_hmac_key, 32, payload, 14,
       metadata_key_hmac.bytes, &md_len);

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
  if (result != 0) {
    printf("Error: decryption failed\n");
    __builtin_trap();
    return 0;
  }

  // deallocate the ciphers
  result = NpLdtEncryptClose(enc_handle);
  if (result) {
    printf("Error: close cipher failed\n");
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
