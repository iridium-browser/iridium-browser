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

// Constants used for convolve implementations.
// This will be included inside an anonymous namespace on files where these are
// necessary.

constexpr int kIntermediateAllocWidth = kMaxSuperBlockSizeInPixels;
constexpr int kIntermediateStride = 8;
constexpr int kHorizontalOffset = 3;
constexpr int kFilterIndexShift = 6;
