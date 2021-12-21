// Copyright 2020 The libgav1 Authors
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

// Constants used for cdef implementations.
// This will be included inside an anonymous namespace on files where these are
// necessary.

const int8_t (*const kCdefDirections)[2][2] = kCdefDirectionsPadded + 2;

// Mirror values and pad to 16 elements.
alignas(16) constexpr uint32_t kCdefDivisionTable[] = {
    840, 420, 280, 210, 168, 140, 120, 105,
    120, 140, 168, 210, 280, 420, 840, 0};

// Used when calculating odd |cost[x]| values to mask off unwanted elements.
// Holds elements 1 3 5 X 5 3 1 X
alignas(16) constexpr uint32_t kCdefDivisionTableOdd[] = {420, 210, 140, 0,
                                                          140, 210, 420, 0};
