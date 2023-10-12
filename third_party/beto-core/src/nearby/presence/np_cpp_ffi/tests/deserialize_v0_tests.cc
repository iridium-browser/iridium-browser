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

#include "gtest/gtest.h"

TEST(NpFfiDeserializeV0Tests, V0SingleDataElementTxPower) {
  nearby_protocol::RawAdvertisementPayload adv(
      nearby_protocol::ByteBuffer<255>({
          4,
          {0x00,       // Adv Header
           0x03,       // Public DE header
           0x15, 0x03} // Length 1 Tx Power DE with value 3
      }));

  auto maybe_credential_book = nearby_protocol::CredentialBook::TryCreate();
  ASSERT_TRUE(maybe_credential_book.ok());
  auto deserialize_result =
      nearby_protocol::Deserializer::DeserializeAdvertisement(
          adv, maybe_credential_book.value());

  ASSERT_EQ(deserialize_result.GetKind(),
            nearby_protocol::DeserializeAdvertisementResultKind::V0);
  auto v0_adv = deserialize_result.IntoV0();

  ASSERT_EQ(v0_adv.GetKind(),
            nearby_protocol::DeserializedV0AdvertisementKind::Legible);
  auto legible_adv = v0_adv.IntoLegible();
  auto identity = legible_adv.GetIdentity();
  ASSERT_EQ(identity.GetKind(),
            nearby_protocol::DeserializedV0IdentityKind::Plaintext);

  auto num_des = legible_adv.GetNumberOfDataElements();
  ASSERT_EQ(num_des, 1);
  auto payload = legible_adv.IntoPayload();

  auto de_result = payload.TryGetDataElement(0);
  ASSERT_TRUE(de_result.ok());
  auto de = de_result.value();

  ASSERT_EQ(de.GetKind(), nearby_protocol::V0DataElementKind::TxPower);
  auto tx_power = de.AsTxPower();
  ASSERT_EQ(tx_power.tx_power, 3);
}

TEST(NpFfiDeserializeV0Tests, V0LengthOneActionsDataElement) {
  nearby_protocol::RawAdvertisementPayload adv(
      nearby_protocol::ByteBuffer<255>({
          4,
          {0x00,       // Adv Header
           0x03,       // Public DE header
           0x16, 0x00} // Length 1 Actions DE
      }));

  auto maybe_credential_book = nearby_protocol::CredentialBook::TryCreate();
  ASSERT_TRUE(maybe_credential_book.ok());
  auto deserialize_result =
      nearby_protocol::Deserializer::DeserializeAdvertisement(
          adv, maybe_credential_book.value());

  ASSERT_EQ(deserialize_result.GetKind(),
            nearby_protocol::DeserializeAdvertisementResultKind::V0);
  auto v0_adv = deserialize_result.IntoV0();

  ASSERT_EQ(v0_adv.GetKind(),
            nearby_protocol::DeserializedV0AdvertisementKind::Legible);
  auto legible_adv = v0_adv.IntoLegible();
  auto identity = legible_adv.GetIdentity();
  ASSERT_EQ(identity.GetKind(),
            nearby_protocol::DeserializedV0IdentityKind::Plaintext);

  auto num_des = legible_adv.GetNumberOfDataElements();
  ASSERT_EQ(num_des, 1);
  auto payload = legible_adv.IntoPayload();

  auto de_result = payload.TryGetDataElement(0);
  ASSERT_TRUE(de_result.ok());
  auto de = de_result.value();

  ASSERT_EQ(de.GetKind(), nearby_protocol::V0DataElementKind::Actions);
  auto actions = de.AsActions();
  ASSERT_EQ(actions.GetAsU32(), 0);
}

TEST(NpFfiDeserializeV0Tests, V0LengthTwoActionsDataElement) {
  nearby_protocol::RawAdvertisementPayload adv(
      nearby_protocol::ByteBuffer<255>({
          5,
          {0x00,             // Adv Header
           0x03,             // Public DE header
           0x26, 0xD0, 0x46} // Length 2 Actions DE
      }));

  auto maybe_credential_book = nearby_protocol::CredentialBook::TryCreate();
  ASSERT_TRUE(maybe_credential_book.ok());
  auto deserialize_result =
      nearby_protocol::Deserializer::DeserializeAdvertisement(
          adv, maybe_credential_book.value());

  ASSERT_EQ(deserialize_result.GetKind(),
            nearby_protocol::DeserializeAdvertisementResultKind::V0);
  auto v0_adv = deserialize_result.IntoV0();

  ASSERT_EQ(v0_adv.GetKind(),
            nearby_protocol::DeserializedV0AdvertisementKind::Legible);
  auto legible_adv = v0_adv.IntoLegible();
  auto identity = legible_adv.GetIdentity();
  ASSERT_EQ(identity.GetKind(),
            nearby_protocol::DeserializedV0IdentityKind::Plaintext);

  auto num_des = legible_adv.GetNumberOfDataElements();
  ASSERT_EQ(num_des, 1);

  auto payload = legible_adv.IntoPayload();
  auto de_result = payload.TryGetDataElement(0);
  ASSERT_TRUE(de_result.ok());
  auto de = de_result.value();

  ASSERT_EQ(de.GetKind(), nearby_protocol::V0DataElementKind::Actions);
  auto actions = de.AsActions();
  ASSERT_EQ(actions.GetAsU32(), 0xD0460000);

  ASSERT_TRUE(
      actions.HasAction(nearby_protocol::BooleanActionType::NearbyShare));
  ASSERT_TRUE(actions.HasAction(nearby_protocol::BooleanActionType::Finder));
  ASSERT_TRUE(
      actions.HasAction(nearby_protocol::BooleanActionType::FastPairSass));

  ASSERT_FALSE(
      actions.HasAction(nearby_protocol::BooleanActionType::ActiveUnlock));
  ASSERT_FALSE(
      actions.HasAction(nearby_protocol::BooleanActionType::InstantTethering));
  ASSERT_FALSE(actions.HasAction(nearby_protocol::BooleanActionType::PhoneHub));
  ASSERT_FALSE(
      actions.HasAction(nearby_protocol::BooleanActionType::PresenceManager));

  ASSERT_EQ(actions.GetContextSyncSequenceNumber(), 0xD);
}

TEST(NpFfiDeserializeV0Tests, V0MultipleDataElements) {
  nearby_protocol::RawAdvertisementPayload adv(nearby_protocol::ByteBuffer<255>(
      {7,
       {
           0x00,             // Adv Header
           0x03,             // Public DE header
           0x15, 0x05,       // Tx Power value 5
           0x26, 0x00, 0x46, // Length 2 Actions
       }}));

  auto maybe_credential_book = nearby_protocol::CredentialBook::TryCreate();
  ASSERT_TRUE(maybe_credential_book.ok());
  auto deserialize_result =
      nearby_protocol::Deserializer::DeserializeAdvertisement(
          adv, maybe_credential_book.value());

  ASSERT_EQ(deserialize_result.GetKind(),
            nearby_protocol::DeserializeAdvertisementResultKind::V0);
  auto v0_adv = deserialize_result.IntoV0();

  ASSERT_EQ(v0_adv.GetKind(),
            nearby_protocol::DeserializedV0AdvertisementKind::Legible);
  auto legible_adv = v0_adv.IntoLegible();
  auto identity = legible_adv.GetIdentity();
  ASSERT_EQ(identity.GetKind(),
            nearby_protocol::DeserializedV0IdentityKind::Plaintext);

  auto num_des = legible_adv.GetNumberOfDataElements();
  ASSERT_EQ(num_des, 2);
  auto payload = legible_adv.IntoPayload();

  auto first_de_result = payload.TryGetDataElement(0);
  ASSERT_TRUE(first_de_result.ok());
  auto first_de = first_de_result.value();

  ASSERT_EQ(first_de.GetKind(), nearby_protocol::V0DataElementKind::TxPower);
  auto power = first_de.AsTxPower();
  ASSERT_EQ(power.tx_power, 5);

  auto second_de_result = payload.TryGetDataElement(1);
  ASSERT_TRUE(second_de_result.ok());
  auto second_de = second_de_result.value();

  ASSERT_EQ(second_de.GetKind(), nearby_protocol::V0DataElementKind::Actions);
  auto actions = second_de.AsActions();
  ASSERT_EQ(actions.GetAsU32(), 0x00460000);
  ASSERT_EQ(actions.GetContextSyncSequenceNumber(), 0);
}

TEST(NpFfiDeserializeV0Tests, V0EmptyPayload) {
  auto maybe_credential_book = nearby_protocol::CredentialBook::TryCreate();
  ASSERT_TRUE(maybe_credential_book.ok());
  auto deserialize_result =
      nearby_protocol::Deserializer::DeserializeAdvertisement(
          V0AdvEmpty, maybe_credential_book.value());

  ASSERT_EQ(deserialize_result.GetKind(),
            nearby_protocol::DeserializeAdvertisementResultKind::V0);
  auto v0_adv = deserialize_result.IntoV0();

  ASSERT_EQ(v0_adv.GetKind(),
            nearby_protocol::DeserializedV0AdvertisementKind::Legible);
  auto legible_adv = v0_adv.IntoLegible();
  auto identity = legible_adv.GetIdentity();
  ASSERT_EQ(identity.GetKind(),
            nearby_protocol::DeserializedV0IdentityKind::Plaintext);

  auto num_des = legible_adv.GetNumberOfDataElements();
  ASSERT_EQ(num_des, 0);
  auto payload = legible_adv.IntoPayload();

  auto result = payload.TryGetDataElement(0);
  ASSERT_FALSE(result.ok());
  ASSERT_TRUE(absl::IsOutOfRange(result.status()));
}

TEST(NpFfiDeserializeV0Tests, TestV0AdvMoveConstructor) {
  auto book = nearby_protocol::CredentialBook::TryCreate().value();
  auto result =
      nearby_protocol::Deserializer::DeserializeAdvertisement(V0AdvEmpty, book);
  ASSERT_EQ(result.GetKind(),
            np_ffi::internal::DeserializeAdvertisementResultKind::V0);
  auto adv = result.IntoV0();

  // Now move the adv into a new value, and make sure its still valid
  nearby_protocol::DeserializedV0Advertisement moved_adv(std::move(adv));
  ASSERT_EQ(moved_adv.GetKind(),
            np_ffi::internal::DeserializedV0AdvertisementKind::Legible);

  // trying to use the moved object should result in a use after free which
  // triggers an abort
  ASSERT_DEATH([[maybe_unused]] auto failure =
                   adv.IntoLegible(), // NOLINT(bugprone-use-after-move
               "");
  ASSERT_DEATH([[maybe_unused]] auto failure = adv.GetKind(), "");

  // moving again should still preserve the moved state and also lead to an
  // abort
  nearby_protocol::DeserializedV0Advertisement moved_again(std::move(adv));
  ASSERT_DEATH([[maybe_unused]] auto failure = moved_again.IntoLegible(), "");
  ASSERT_DEATH([[maybe_unused]] auto failure = moved_again.GetKind(), "");
}

TEST(NpFfiDeserializeResultTests, TestV0AdvMoveAssignment) {
  auto book = nearby_protocol::CredentialBook::TryCreate().value();
  auto result =
      nearby_protocol::Deserializer::DeserializeAdvertisement(V0AdvEmpty, book);
  ASSERT_EQ(result.GetKind(),
            np_ffi::internal::DeserializeAdvertisementResultKind::V0);
  auto adv = result.IntoV0();

  // create a second result
  auto another_result =
      nearby_protocol::Deserializer::DeserializeAdvertisement(V0AdvEmpty, book);
  ASSERT_EQ(another_result.GetKind(),
            np_ffi::internal::DeserializeAdvertisementResultKind::V0);
  auto adv2 = another_result.IntoV0();

  // move adv2 into adv, the original should be deallocated by assignment
  adv2 = std::move(adv);
  ASSERT_EQ(adv2.GetKind(),
            np_ffi::internal::DeserializedV0AdvertisementKind::Legible);

  // original result should now be invalid, using it will trigger a use after
  // free abort.
  ASSERT_DEATH([[maybe_unused]] auto failure =
                   adv.IntoLegible(), // NOLINT(bugprone-use-after-move)
               "");
  ASSERT_DEATH([[maybe_unused]] auto failure = adv.GetKind(), "");

  // moving again should still lead to an error
  auto moved_again = std::move(adv);
  ASSERT_DEATH([[maybe_unused]] auto failure = moved_again.IntoLegible(), "");
  ASSERT_DEATH([[maybe_unused]] auto failure = moved_again.GetKind(), "");
}

TEST(NpFfiDeserializeV0Tests, V0AdvDestructor) {
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

    // Calling IntoV0() should move the underlying resources into the v0 object
    // when both go out of scope only one should be freed
    auto v0_adv = deserialize_result.IntoV0();
  }

  // Now that the first v0 adv is out of scope, it should be de-allocated which
  // will create room for one more to be created.
  auto deserialize_result3 =
      nearby_protocol::Deserializer::DeserializeAdvertisement(
          V0AdvEmpty, book_result.value());
  ASSERT_EQ(deserialize_result3.GetKind(),
            np_ffi::internal::DeserializeAdvertisementResultKind::V0);
}

TEST(NpFfiDeserializeV0Tests, V0AdvUseAfterMove) {
  auto maybe_credential_book = nearby_protocol::CredentialBook::TryCreate();
  ASSERT_TRUE(maybe_credential_book.ok());
  auto deserialize_result =
      nearby_protocol::Deserializer::DeserializeAdvertisement(
          V0AdvSimple, maybe_credential_book.value());

  ASSERT_EQ(deserialize_result.GetKind(),
            nearby_protocol::DeserializeAdvertisementResultKind::V0);
  auto v0_adv = deserialize_result.IntoV0();

  ASSERT_EQ(v0_adv.GetKind(),
            nearby_protocol::DeserializedV0AdvertisementKind::Legible);

  // Moves the adv into a legible adv, so the original v0_adv is no longer valid
  [[maybe_unused]] auto legible_adv = v0_adv.IntoLegible();
  ASSERT_DEATH([[maybe_unused]] auto failure = v0_adv.GetKind(), "");
  ASSERT_DEATH([[maybe_unused]] auto failure = v0_adv.IntoLegible(), "");
}

TEST(NpFfiDeserializeV0Tests, TestLegibleAdvMoveConstructor) {
  auto book = nearby_protocol::CredentialBook::TryCreate().value();
  auto result =
      nearby_protocol::Deserializer::DeserializeAdvertisement(V0AdvEmpty, book);
  ASSERT_EQ(result.GetKind(),
            np_ffi::internal::DeserializeAdvertisementResultKind::V0);
  auto legible = result.IntoV0().IntoLegible();

  // Now move the adv into a new value, and make sure its still valid
  nearby_protocol::LegibleDeserializedV0Advertisement moved(std::move(legible));
  ASSERT_EQ(moved.GetNumberOfDataElements(), 0);
  ASSERT_EQ(moved.GetIdentity().GetKind(),
            np_ffi::internal::DeserializedV0IdentityKind::Plaintext);

  // trying to use the moved object should result in a use after free which
  // triggers an abort
  ASSERT_DEATH([[maybe_unused]] auto failure =
                   legible.GetIdentity(), // NOLINT(bugprone-use-after-move
               "");
  ASSERT_DEATH(
      [[maybe_unused]] auto failure = legible.GetNumberOfDataElements(), "");
  ASSERT_DEATH([[maybe_unused]] auto failure = legible.IntoPayload(), "");

  // moving again should still preserve the moved state and also lead to an
  // abort
  nearby_protocol::LegibleDeserializedV0Advertisement moved_again(
      std::move(legible));
  ASSERT_DEATH([[maybe_unused]] auto failure = moved_again.GetIdentity(), "");
  ASSERT_DEATH([[maybe_unused]] auto failure =
                   moved_again.GetNumberOfDataElements(),
               "");
  ASSERT_DEATH([[maybe_unused]] auto failure = moved_again.IntoPayload(), "");
}

TEST(NpFfiDeserializeResultTests, TestLegibleAdvMoveAssignment) {
  auto book = nearby_protocol::CredentialBook::TryCreate().value();
  auto result =
      nearby_protocol::Deserializer::DeserializeAdvertisement(V0AdvEmpty, book);
  ASSERT_EQ(result.GetKind(),
            np_ffi::internal::DeserializeAdvertisementResultKind::V0);
  auto legible = result.IntoV0().IntoLegible();

  // create a second result
  auto another_result =
      nearby_protocol::Deserializer::DeserializeAdvertisement(V0AdvEmpty, book);
  ASSERT_EQ(another_result.GetKind(),
            np_ffi::internal::DeserializeAdvertisementResultKind::V0);
  auto legible2 = another_result.IntoV0().IntoLegible();

  // move adv2 into adv, the original should be deallocated by assignment
  legible2 = std::move(legible);
  ASSERT_EQ(legible2.GetIdentity().GetKind(),
            np_ffi::internal::DeserializedV0IdentityKind::Plaintext);

  // original result should now be invalid, using it will trigger a use after
  // free abort.
  ASSERT_DEATH([[maybe_unused]] auto failure =
                   legible.GetIdentity(), // NOLINT(bugprone-use-after-move)
               "");
  ASSERT_DEATH(
      [[maybe_unused]] auto failure = legible.GetNumberOfDataElements(), "");
  ASSERT_DEATH([[maybe_unused]] auto failure = legible.IntoPayload(), "");

  // moving again should still lead to an error
  auto moved_again = std::move(legible);
  ASSERT_DEATH([[maybe_unused]] auto failure = moved_again.IntoPayload(), "");
  ASSERT_DEATH([[maybe_unused]] auto failure = moved_again.GetIdentity(), "");
  ASSERT_DEATH([[maybe_unused]] auto failure =
                   moved_again.GetNumberOfDataElements(),
               "");
}

nearby_protocol::LegibleDeserializedV0Advertisement
CreateLegibleAdv(nearby_protocol::CredentialBook &book) {
  auto adv = nearby_protocol::Deserializer::DeserializeAdvertisement(
      V0AdvSimple, book);
  auto v0_adv = adv.IntoV0();
  return v0_adv.IntoLegible();
}

TEST(NpFfiDeserializeV0Tests, V0LegibleAdvUseAfterMove) {
  auto book = nearby_protocol::CredentialBook::TryCreate().value();
  auto legible_adv = CreateLegibleAdv(book);

  // Should be able to use the valid legible adv even though its original parent
  // is now out of scope.
  ASSERT_EQ(legible_adv.GetIdentity().GetKind(),
            nearby_protocol::DeserializedV0IdentityKind::Plaintext);
  ASSERT_EQ(legible_adv.GetNumberOfDataElements(), 1);
  [[maybe_unused]] auto payload = legible_adv.IntoPayload();

  // now that the legible adv has moved into the payload it should no longer be
  // valid
  ASSERT_DEATH([[maybe_unused]] auto failure = legible_adv.GetIdentity(), "");
  ASSERT_DEATH([[maybe_unused]] auto failure =
                   legible_adv.GetNumberOfDataElements(),
               "");
  ASSERT_DEATH([[maybe_unused]] auto failure = legible_adv.IntoPayload(), "");
}

TEST(NpFfiDeserializeV0Tests, LegibleAdvDestructor) {
  auto book = nearby_protocol::CredentialBook::TryCreate().value();
  nearby_protocol::GlobalConfig::SetMaxNumDeserializedV0Advertisements(1);
  {
    auto legible_adv = CreateLegibleAdv(book);

    // check that legible adv is valid.
    ASSERT_EQ(legible_adv.GetIdentity().GetKind(),
              nearby_protocol::DeserializedV0IdentityKind::Plaintext);
    ASSERT_EQ(legible_adv.GetNumberOfDataElements(), 1);

    // allocation slots should be full
    ASSERT_EQ(nearby_protocol::Deserializer::DeserializeAdvertisement(
                  V0AdvSimple, book)
                  .GetKind(),
              nearby_protocol::DeserializeAdvertisementResultKind::Error);
  }

  // Verify the handle was de-allocated when legible adv went out of scope
  auto result = nearby_protocol::Deserializer::DeserializeAdvertisement(
      V0AdvSimple, book);
  ASSERT_EQ(result.GetKind(),
            nearby_protocol::DeserializeAdvertisementResultKind::V0);
}

nearby_protocol::V0Payload
CreatePayload(nearby_protocol::CredentialBook &book) {
  auto legible_adv = CreateLegibleAdv(book);
  return legible_adv.IntoPayload();
}

TEST(NpFfiDeserializeV0Tests, V0PayloadDestructor) {
  auto book = nearby_protocol::CredentialBook::TryCreate().value();
  nearby_protocol::GlobalConfig::SetMaxNumDeserializedV0Advertisements(1);
  {
    auto payload = CreatePayload(book);

    // check that payload adv is valid even though its parent is out of scope
    ASSERT_TRUE(payload.TryGetDataElement(0).ok());

    // allocation slots should be full
    ASSERT_EQ(nearby_protocol::Deserializer::DeserializeAdvertisement(
                  V0AdvSimple, book)
                  .GetKind(),
              nearby_protocol::DeserializeAdvertisementResultKind::Error);
  }

  // Now that the payload is out of scope its destructor should have been called
  // freeing the parent handle
  auto result = nearby_protocol::Deserializer::DeserializeAdvertisement(
      V0AdvSimple, book);
  ASSERT_EQ(result.GetKind(),
            nearby_protocol::DeserializeAdvertisementResultKind::V0);
}

TEST(NpFfiDeserializeV0Tests, TestV0PayloadMoveConstructor) {
  auto book = nearby_protocol::CredentialBook::TryCreate().value();
  auto result = nearby_protocol::Deserializer::DeserializeAdvertisement(
      V0AdvSimple, book);
  ASSERT_EQ(result.GetKind(),
            np_ffi::internal::DeserializeAdvertisementResultKind::V0);
  auto payload = result.IntoV0().IntoLegible().IntoPayload();

  // Now move the adv into a new value, and make sure its still valid
  nearby_protocol::V0Payload moved(std::move(payload));
  ASSERT_TRUE(moved.TryGetDataElement(0).ok());
  ASSERT_TRUE(absl::IsOutOfRange(moved.TryGetDataElement(1).status()));

  // trying to use the moved object should result in a use after free which
  // triggers an abort
  ASSERT_DEATH([[maybe_unused]] auto failure = payload.TryGetDataElement(
                   0), // NOLINT(bugprone-use-after-move
               "");

  // moving again should still preserve the moved state and also lead to an
  // abort
  nearby_protocol::V0Payload moved_again(std::move(payload));
  ASSERT_DEATH([[maybe_unused]] auto failure = moved_again.TryGetDataElement(0),
               "");
}

TEST(NpFfiDeserializeResultTests, TestV0PayloadMoveAssignment) {
  auto book = nearby_protocol::CredentialBook::TryCreate().value();
  auto result = nearby_protocol::Deserializer::DeserializeAdvertisement(
      V0AdvSimple, book);
  ASSERT_EQ(result.GetKind(),
            np_ffi::internal::DeserializeAdvertisementResultKind::V0);
  auto payload = result.IntoV0().IntoLegible().IntoPayload();

  // create a second result
  auto another_result = nearby_protocol::Deserializer::DeserializeAdvertisement(
      V0AdvSimple, book);
  ASSERT_EQ(another_result.GetKind(),
            np_ffi::internal::DeserializeAdvertisementResultKind::V0);
  auto payload2 = another_result.IntoV0().IntoLegible().IntoPayload();

  // original should be deallocated by assignment
  payload2 = std::move(payload);
  ASSERT_TRUE(payload2.TryGetDataElement(0).ok());

  // original result should now be invalid, using it will trigger a use after
  // free abort.
  ASSERT_DEATH([[maybe_unused]] auto failure = payload.TryGetDataElement(
                   0), // NOLINT(bugprone-use-after-move)
               "");

  // moving again should still lead to an error
  auto moved_again = std::move(payload);
  ASSERT_DEATH([[maybe_unused]] auto failure = moved_again.TryGetDataElement(0),
               "");
}

TEST(NpFfiDeserializeV0Tests, InvalidDataElementCast) {
  ASSERT_TRUE(
      nearby_protocol::GlobalConfig::SetPanicHandler(test_panic_handler));

  auto maybe_credential_book = nearby_protocol::CredentialBook::TryCreate();
  ASSERT_TRUE(maybe_credential_book.ok());
  auto deserialize_result =
      nearby_protocol::Deserializer::DeserializeAdvertisement(
          V0AdvSimple, maybe_credential_book.value());

  ASSERT_EQ(deserialize_result.GetKind(),
            nearby_protocol::DeserializeAdvertisementResultKind::V0);
  auto v0_adv = deserialize_result.IntoV0();
  ASSERT_EQ(v0_adv.GetKind(),
            nearby_protocol::DeserializedV0AdvertisementKind::Legible);
  auto legible_adv = v0_adv.IntoLegible();
  auto identity = legible_adv.GetIdentity();
  ASSERT_EQ(identity.GetKind(),
            nearby_protocol::DeserializedV0IdentityKind::Plaintext);

  auto num_des = legible_adv.GetNumberOfDataElements();
  ASSERT_EQ(num_des, 1);
  auto payload = legible_adv.IntoPayload();

  auto de_result = payload.TryGetDataElement(0);
  ASSERT_TRUE(de_result.ok());
  auto de = de_result.value();

  ASSERT_EQ(de.GetKind(), nearby_protocol::V0DataElementKind::TxPower);
  ASSERT_DEATH([[maybe_unused]] auto failure = de.AsActions();, "");
}

TEST(NpFfiDeserializeV0Tests, InvalidDataElementIndex) {
  auto maybe_credential_book = nearby_protocol::CredentialBook::TryCreate();
  ASSERT_TRUE(maybe_credential_book.ok());
  auto deserialize_result =
      nearby_protocol::Deserializer::DeserializeAdvertisement(
          V0AdvSimple, maybe_credential_book.value());

  ASSERT_EQ(deserialize_result.GetKind(),
            nearby_protocol::DeserializeAdvertisementResultKind::V0);
  auto v0_adv = deserialize_result.IntoV0();

  ASSERT_EQ(v0_adv.GetKind(),
            nearby_protocol::DeserializedV0AdvertisementKind::Legible);
  auto legible_adv = v0_adv.IntoLegible();
  auto identity = legible_adv.GetIdentity();
  ASSERT_EQ(identity.GetKind(),
            nearby_protocol::DeserializedV0IdentityKind::Plaintext);

  auto num_des = legible_adv.GetNumberOfDataElements();
  ASSERT_EQ(num_des, 1);
  auto payload = legible_adv.IntoPayload();

  auto de_result = payload.TryGetDataElement(1);
  ASSERT_FALSE(de_result.ok());
  ASSERT_TRUE(absl::IsOutOfRange(de_result.status()));
}
