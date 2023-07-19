// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UTIL_STD_UTIL_H_
#define UTIL_STD_UTIL_H_

#include <stddef.h>

#include <algorithm>
#include <map>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "absl/algorithm/container.h"
#include "util/stringprintf.h"

namespace openscreen {

template <typename T, size_t N>
constexpr size_t countof(T (&array)[N]) {
  return N;
}

// std::basic_string::data() has no mutable overload prior to C++17 [1].
// Hence this overload is provided.
// Note: str[0] is safe even for empty strings, as they are guaranteed to be
// null-terminated [2].
//
// [1] http://en.cppreference.com/w/cpp/string/basic_string/data
// [2] http://en.cppreference.com/w/cpp/string/basic_string/operator_at
template <typename CharT, typename Traits, typename Allocator>
CharT* data(std::basic_string<CharT, Traits, Allocator>& str) {
  return std::addressof(str[0]);
}

// Stringify a vector of objects that have an operator<< overload.
template <typename T>
std::string Join(const std::vector<T>& vec, const char* delimiter = ", ") {
  std::stringstream ss;

  auto it = vec.begin();
  ss << *it;
  for (++it; it != vec.end(); ++it) {
    ss << delimiter << *it;
  }

  return ss.str();
}

template <typename Key, typename Value>
void RemoveValueFromMap(std::map<Key, Value*>* map, Value* value) {
  for (auto it = map->begin(); it != map->end();) {
    if (it->second == value) {
      it = map->erase(it);
    } else {
      ++it;
    }
  }
}

template <typename ForwardIteratingContainer>
bool AreElementsSortedAndUnique(const ForwardIteratingContainer& c) {
  return absl::c_is_sorted(c) && (absl::c_adjacent_find(c) == c.end());
}

template <typename RandomAccessContainer>
void SortAndDedupeElements(RandomAccessContainer* c) {
  std::sort(c->begin(), c->end());
  const auto new_end = std::unique(c->begin(), c->end());
  c->erase(new_end, c->end());
}

// Append the provided elements together into a single vector. This can be
// useful when creating a vector of variadic templates in the ctor.
//
// This is the base case for the recursion
template <typename T>
std::vector<T>&& Append(std::vector<T>&& so_far) {
  return std::move(so_far);
}

// This is the recursive call. Depending on the number of remaining elements, it
// either calls into itself or into the above base case.
template <typename T, typename TFirst, typename... TOthers>
std::vector<T>&& Append(std::vector<T>&& so_far,
                        TFirst&& new_element,
                        TOthers&&... new_elements) {
  so_far.push_back(std::move(new_element));
  return Append(std::move(so_far), std::move(new_elements)...);
}

// Creates an empty vector with |size| elements reserved. Intended to be used as
// GetEmptyVectorOfSize<T>(sizeof...(variadic_input))
template <typename T>
std::vector<T> GetVectorWithCapacity(size_t size) {
  std::vector<T> results;
  results.reserve(size);
  return results;
}

// Returns true if an element equal to |element| is found in |container|.
// C.begin() must return an iterator to the beginning of C and C.end() must
// return an iterator to the end.
template <typename C, typename E>
bool Contains(const C& container, const E& element) {
  return std::find(container.begin(), container.end(), element) !=
         container.end();
}

// Returns true if any element in |container| returns true for |predicate|.
// C.begin() must return an iterator to the beginning of C and C.end() must
// return an iterator to the end.
template <typename C, typename P>
bool ContainsIf(const C& container, P predicate) {
  return std::find_if(container.begin(), container.end(),
                      std::move(predicate)) != container.end();
}

}  // namespace openscreen

#endif  // UTIL_STD_UTIL_H_
