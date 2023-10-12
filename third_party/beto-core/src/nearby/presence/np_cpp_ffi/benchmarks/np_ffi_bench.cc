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

#include "np_cpp_ffi_functions.h"
#include "np_cpp_ffi_types.h"

#include "benchmark/benchmark.h"

// Internal C struct representation of a v0 payload
static np_ffi::internal::RawAdvertisementPayload payload{
    {7,
     {
         0x00,             // Adv Header
         0x03,             // Public DE header
         0x15, 0x05,       // Tx Power value 5
         0x26, 0x00, 0x46, // Length 2 Actions
     }}};

static nearby_protocol::RawAdvertisementPayload
    v0_adv(nearby_protocol::ByteBuffer(payload.bytes));

class NpCppBenchmark : public benchmark::Fixture {
  void SetUp(const ::benchmark::State &state) override {}

  void TearDown(const ::benchmark::State &state) override {}
};

BENCHMARK_DEFINE_F(NpCppBenchmark, V0PlaintextAdvertisement)
(benchmark::State &state) {
  auto cred_book = nearby_protocol::CredentialBook::TryCreate();
  assert(cred_book.ok());
  auto num_ciphers = state.range(0);

  for ([[maybe_unused]] auto _ : state) {
    for (int i = 0; i < num_ciphers; i++) {
      auto result = nearby_protocol::Deserializer::DeserializeAdvertisement(
          v0_adv, cred_book.value());
      assert(result.GetKind() ==
             nearby_protocol::DeserializeAdvertisementResultKind::V0);
    }
  }
}

BENCHMARK_REGISTER_F(NpCppBenchmark, V0PlaintextAdvertisement)
    ->RangeMultiplier(10)
    ->Range(1, 1000)
    ->Unit(benchmark::kMicrosecond);

class NpCBenchmark : public benchmark::Fixture {
  void SetUp(const ::benchmark::State &state) override {}

  void TearDown(const ::benchmark::State &state) override {}
};

BENCHMARK_DEFINE_F(NpCBenchmark, V0PlaintextAdvertisement)
(benchmark::State &state) {
  auto num_ciphers = state.range(0);
  auto book_result = np_ffi::internal::np_ffi_create_credential_book();
  assert(
      np_ffi::internal::np_ffi_CreateCredentialBookResult_kind(book_result) ==
      np_ffi::internal::CreateCredentialBookResultKind::Success);
  auto book = np_ffi::internal::np_ffi_CreateCredentialBookResult_into_SUCCESS(
      book_result);

  for ([[maybe_unused]] auto _ : state) {
    for (int i = 0; i < num_ciphers; i++) {
      auto result =
          np_ffi::internal::np_ffi_deserialize_advertisement({payload}, book);
      assert(np_ffi::internal::np_ffi_DeserializeAdvertisementResult_kind(
                 result) ==
             np_ffi::internal::DeserializeAdvertisementResultKind::V0);
      auto deallocate_result =
          np_ffi::internal::np_ffi_deallocate_deserialize_advertisement_result(
              result);
      assert(deallocate_result == np_ffi::internal::DeallocateResult::Success);
    }
  }

  auto deallocate_result =
      np_ffi::internal::np_ffi_deallocate_credential_book(book);
  assert(deallocate_result == np_ffi::internal::DeallocateResult::Success);
}

BENCHMARK_REGISTER_F(NpCBenchmark, V0PlaintextAdvertisement)
    ->RangeMultiplier(10)
    ->Range(1, 1000)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_MAIN();