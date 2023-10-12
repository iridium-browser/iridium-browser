// Copyright 2023 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "nearby_protocol.h"
#include "shared_test_util.h"

#include <iostream>

#include "gtest/gtest.h"

TEST(NpFfiGlobalConfigTests, TestPanicHandler) {
  ASSERT_TRUE(
      nearby_protocol::GlobalConfig::SetPanicHandler(test_panic_handler));
  auto book = nearby_protocol::CredentialBook::TryCreate().value();
  auto deserialize_result =
      nearby_protocol::Deserializer::DeserializeAdvertisement(V0AdvEmpty, book);
  ASSERT_EQ(deserialize_result.GetKind(),
            np_ffi::internal::DeserializeAdvertisementResultKind::V0);

  // Now try to cast the result into the wrong type and verify the process
  // aborts
  ASSERT_DEATH({ [[maybe_unused]] auto failure = deserialize_result.IntoV1(); },
               "");
}

TEST(NpFfiGlobalConfigTests, TestPanicHandlerTwice) {
  ASSERT_TRUE(
      nearby_protocol::GlobalConfig::SetPanicHandler(test_panic_handler));

  // Second time trying to set should fail
  ASSERT_FALSE(
      nearby_protocol::GlobalConfig::SetPanicHandler(test_panic_handler));
}

// There is not much we can actually test here since this will affect memory
// consumption. This is more of just a simple check that things still work after
// configuring this
TEST(NpFfiGlobalConfigTests, TestSetMaxShardsDefault) {
  // 0 should still work as default behavior
  nearby_protocol::GlobalConfig::SetNumShards(0);

  auto book = nearby_protocol::CredentialBook::TryCreate().value();
  auto book2 = nearby_protocol::CredentialBook::TryCreate().value();
  auto book3 = nearby_protocol::CredentialBook::TryCreate().value();
  auto deserialize_result =
      nearby_protocol::Deserializer::DeserializeAdvertisement(V0AdvEmpty, book);

  // Should still work
  ASSERT_EQ(deserialize_result.GetKind(),
            np_ffi::internal::DeserializeAdvertisementResultKind::V0);

  // Call again with a lower number, should have no effect. books 2 and 3 should
  // still work.
  nearby_protocol::GlobalConfig::SetNumShards(1);
  auto deserialize_result_2 =
      nearby_protocol::Deserializer::DeserializeAdvertisement(V0AdvEmpty,
                                                              book2);
  ASSERT_EQ(deserialize_result_2.GetKind(),
            np_ffi::internal::DeserializeAdvertisementResultKind::V0);
  auto deserialize_result_3 =
      nearby_protocol::Deserializer::DeserializeAdvertisement(V0AdvEmpty,
                                                              book3);
  ASSERT_EQ(deserialize_result_3.GetKind(),
            np_ffi::internal::DeserializeAdvertisementResultKind::V0);
}

TEST(NpFfiGlobalConfigTests, TestSetMaxShardsSmall) {
  nearby_protocol::GlobalConfig::SetNumShards(1);
  auto book = nearby_protocol::CredentialBook::TryCreate().value();

  // should still be able to parse 2 payloads with only one shard
  auto deserialize_result1 =
      nearby_protocol::Deserializer::DeserializeAdvertisement(V0AdvEmpty, book);
  ASSERT_EQ(deserialize_result1.GetKind(),
            np_ffi::internal::DeserializeAdvertisementResultKind::V0);
  auto deserialize_result2 =
      nearby_protocol::Deserializer::DeserializeAdvertisement(V0AdvEmpty, book);
  ASSERT_EQ(deserialize_result2.GetKind(),
            np_ffi::internal::DeserializeAdvertisementResultKind::V0);
}

TEST(NpFfiGlobalConfigTests, TestSetMaxCredBooks) {
  nearby_protocol::GlobalConfig::SetMaxNumCredentialBooks(1);
  auto book1_result = nearby_protocol::CredentialBook::TryCreate();
  ASSERT_TRUE(book1_result.ok());

  auto book2_result = nearby_protocol::CredentialBook::TryCreate();
  ASSERT_FALSE(book2_result.ok());
  ASSERT_TRUE(absl::IsResourceExhausted(book2_result.status()));
}

TEST(NpFfiGlobalConfigTests, TestSetMaxCredBooksAfterFirstCall) {
  auto book = nearby_protocol::CredentialBook::TryCreate().value();
  auto book2 = nearby_protocol::CredentialBook::TryCreate().value();
  auto book3 = nearby_protocol::CredentialBook::TryCreate().value();

  // setting this after books have already been created should have no affect
  nearby_protocol::GlobalConfig::SetMaxNumCredentialBooks(1);
  auto book4_result = nearby_protocol::CredentialBook::TryCreate();
  ASSERT_TRUE(book4_result.ok());
}

TEST(NpFfiGlobalConfigTests, TestSetMaxV0Advs) {
  nearby_protocol::GlobalConfig::SetMaxNumDeserializedV0Advertisements(1);
  auto book_result = nearby_protocol::CredentialBook::TryCreate();
  ASSERT_TRUE(book_result.ok());

  {
    auto deserialize_result =
        nearby_protocol::Deserializer::DeserializeAdvertisement(
            V0AdvEmpty, book_result.value());
    ASSERT_EQ(deserialize_result.GetKind(),
              np_ffi::internal::DeserializeAdvertisementResultKind::V0);

    // Going over max amount should result in error
    auto deserialize_result2 =
        nearby_protocol::Deserializer::DeserializeAdvertisement(
            V0AdvEmpty, book_result.value());
    ASSERT_EQ(deserialize_result2.GetKind(),
              np_ffi::internal::DeserializeAdvertisementResultKind::Error);
  }

  // Now that the first v0 adv is out of scope, it will be de-allocated which
  // will create room for one more to be created.
  auto deserialize_result3 =
      nearby_protocol::Deserializer::DeserializeAdvertisement(
          V0AdvEmpty, book_result.value());
  ASSERT_EQ(deserialize_result3.GetKind(),
            np_ffi::internal::DeserializeAdvertisementResultKind::V0);
}

TEST(NpFfiGlobalConfigTests, TestSetMaxV1Advs) {
  nearby_protocol::GlobalConfig::SetMaxNumDeserializedV1Advertisements(1);
  auto book_result = nearby_protocol::CredentialBook::TryCreate();
  ASSERT_TRUE(book_result.ok());

  {
    auto deserialize_result =
        nearby_protocol::Deserializer::DeserializeAdvertisement(
            V1AdvSimple, book_result.value());
    ASSERT_EQ(deserialize_result.GetKind(),
              np_ffi::internal::DeserializeAdvertisementResultKind::V1);

    // Going over max amount should result in error
    auto deserialize_result2 =
        nearby_protocol::Deserializer::DeserializeAdvertisement(
            V1AdvSimple, book_result.value());
    ASSERT_EQ(deserialize_result2.GetKind(),
              np_ffi::internal::DeserializeAdvertisementResultKind::Error);
  }

  // Now that the first v1 adv is out of scope, it will be de-allocated which
  // will create room for one more to be created.
  auto deserialize_result3 =
      nearby_protocol::Deserializer::DeserializeAdvertisement(
          V1AdvSimple, book_result.value());
  ASSERT_EQ(deserialize_result3.GetKind(),
            np_ffi::internal::DeserializeAdvertisementResultKind::V1);
}

// Same test case as above, but verifies that the de-allocation still succeeds
// after calling IntoV1() and that no double frees occur.
TEST(NpFfiGlobalConfigTests, TestSetMaxV1AdvsFreeAfterInto) {
  nearby_protocol::GlobalConfig::SetMaxNumDeserializedV1Advertisements(1);
  auto book_result = nearby_protocol::CredentialBook::TryCreate();
  ASSERT_TRUE(book_result.ok());

  {
    auto deserialize_result =
        nearby_protocol::Deserializer::DeserializeAdvertisement(
            V1AdvSimple, book_result.value());
    ASSERT_EQ(deserialize_result.GetKind(),
              np_ffi::internal::DeserializeAdvertisementResultKind::V1);

    // Going over max amount should result in error
    auto deserialize_result2 =
        nearby_protocol::Deserializer::DeserializeAdvertisement(
            V1AdvSimple, book_result.value());
    ASSERT_EQ(deserialize_result2.GetKind(),
              np_ffi::internal::DeserializeAdvertisementResultKind::Error);

    // Calling IntoV1() should move the underlying resources into the v0 object
    // when both go out of scope only one should be freed
    auto v0_adv = deserialize_result.IntoV1();
  }

  // Now that the first v1 adv is out of scope, it will be de-allocated which
  // will create room for one more to be created.
  auto deserialize_result3 =
      nearby_protocol::Deserializer::DeserializeAdvertisement(
          V1AdvSimple, book_result.value());
  ASSERT_EQ(deserialize_result3.GetKind(),
            np_ffi::internal::DeserializeAdvertisementResultKind::V1);
}