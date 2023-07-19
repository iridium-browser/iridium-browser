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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "np_ldt.h"

static const uint8_t KEY_SEED_BYTES[] = {204, 219, 36, 137, 233, 252, 172, 66, 179, 147, 72, 184, 148, 30, 209, 154, 29, 54, 14, 117, 224, 152, 200, 193, 94, 107, 28, 194, 182, 32, 205, 57};
static const uint8_t KNOWN_HMAC_BYTES[] = {223, 185, 10, 31, 155, 31, 226, 141, 24, 187, 204, 165, 34, 64, 181, 204, 44, 203, 95, 141, 82, 137, 163, 203, 100, 235, 53, 65, 202, 97, 75, 180};
static const uint8_t TEST_DATA_BYTES[] = {205, 104, 63, 225, 161, 209, 248, 70, 84, 61, 10, 19, 212, 174, 164, 0, 64, 200, 214, 123};


int main()
{
    // Create test data
    NpLdtKeySeed* key_seed = malloc(sizeof(NpLdtKeySeed));
    memcpy(key_seed->bytes, KEY_SEED_BYTES, 32);

    NpMetadataKeyHmac* known_hmac = malloc(sizeof(NpMetadataKeyHmac));
    memcpy(known_hmac->bytes, KNOWN_HMAC_BYTES, 32);

    uint8_t* plaintext = malloc(20 * sizeof(uint8_t));
    memcpy(plaintext, TEST_DATA_BYTES, 20);

    NpLdtSalt salt  = {
        {12, 15}
    };

    // Create handle for encryption
    NpLdtEncryptHandle enc_handle = NpLdtEncryptCreate(*key_seed);
    if (enc_handle.handle == 0)
    {
        return -1;
    }

    // Print original plaintext data bytes
    printf("\n Plaintext data: ");
    int i;
    for (i = 0; *(plaintext + i) != 0x00; i++)
        printf("%X ", *(plaintext + i));
    printf("\n");

    // Encrypt the data and print it
    NP_LDT_RESULT result = NpLdtEncrypt(enc_handle, plaintext, 24, salt);
    if (result)
    {
        printf("Error in NpLdtEncrypt: %d\n", result);
        return result;
    }

    printf("\n Encrypted data: ");
    for (i = 0; *(plaintext + i) != 0x00; i++)
        printf("%X ", *(plaintext + i));
    printf("\n");

    // Create handle for encryption
    NpLdtDecryptHandle dec_handle = NpLdtDecryptCreate(*key_seed, *known_hmac);
    if (enc_handle.handle == 0)
    {
        return -1;
    }

    // Decrypt the data and print its bytes
    result = NpLdtDecryptAndVerify(dec_handle, plaintext, 24, salt);
    if (result)
    {
        printf("Error in NpDecryptAndVerify: %d\n", result);
        return result;
    }

    printf("\n Decrypted data: ");
    for (i = 0; *(plaintext + i) != 0x00; i++)
        printf("%X ", *(plaintext + i));
    printf("\n");

    // Call NpLdtClose to free resources
    result = NpLdtEncryptClose(enc_handle);
    if (result)
    {
        printf("Error in NpLdtClose: %d\n", result);
        return result;
    }

    // Call NpLdtClose to free resources
    result = NpLdtDecryptClose(dec_handle);
    if (result)
    {
        printf("Error in NpLdtClose: %d\n", result);
        return result;
    }

    return 0;
}