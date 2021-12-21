// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UTIL_HASHING_H_
#define UTIL_HASHING_H_

namespace openscreen {

// Computes the aggregate hash of the provided hashable objects.
// Seed must initially use a large prime between 2^63 and 2^64 as a starting
// value, or the result of a previous call to this function.
template <typename... T>
uint64_t ComputeAggregateHash(uint64_t seed, const T&... objs) {
  auto hash_combiner = [](uint64_t seed, uint64_t hash_value) -> uint64_t {
    static const uint64_t kMultiplier = UINT64_C(0x9ddfea08eb382d69);
    uint64_t a = (hash_value ^ seed) * kMultiplier;
    a ^= (a >> 47);
    uint64_t b = (seed ^ a) * kMultiplier;
    b ^= (b >> 47);
    b *= kMultiplier;
    return b;
  };

  uint64_t result = seed;
  std::vector<uint64_t> hashes{std::hash<T>()(objs)...};
  for (uint64_t hash : hashes) {
    result = hash_combiner(result, hash);
  }
  return result;
}

template <typename... T>
uint64_t ComputeAggregateHash(const T&... objs) {
  // This value is taken from absl::Hash implementation.
  constexpr uint64_t default_seed = UINT64_C(0xc3a5c85c97cb3127);
  return ComputeAggregateHash(default_seed, objs...);
}

struct PairHash {
  template <typename TFirst, typename TSecond>
  size_t operator()(const std::pair<TFirst, TSecond>& pair) const {
    return ComputeAggregateHash(pair.first, pair.second);
  }
};

}  // namespace openscreen

#endif  // UTIL_HASHING_H_
