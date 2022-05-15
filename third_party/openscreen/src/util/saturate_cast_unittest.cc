// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "util/saturate_cast.h"

#include "gtest/gtest.h"

namespace openscreen {
namespace {

TEST(SaturateCastTest, LargerToSmallerSignedInteger) {
  struct ValuePair {
    int64_t from;
    int32_t to;
  };
  constexpr ValuePair kValuePairs[] = {
      {std::numeric_limits<int64_t>::max(),
       std::numeric_limits<int32_t>::max()},
      {std::numeric_limits<int64_t>::max() / 2 + 42,
       std::numeric_limits<int32_t>::max()},
      {std::numeric_limits<int32_t>::max(),
       std::numeric_limits<int32_t>::max()},
      {42, 42},
      {0, 0},
      {-42, -42},
      {std::numeric_limits<int32_t>::min(),
       std::numeric_limits<int32_t>::min()},
      {std::numeric_limits<int64_t>::min() / 2 - 42,
       std::numeric_limits<int32_t>::min()},
      {std::numeric_limits<int64_t>::min(),
       std::numeric_limits<int32_t>::min()},
  };
  for (const ValuePair& value_pair : kValuePairs) {
    EXPECT_EQ(value_pair.to, saturate_cast<int32_t>(value_pair.from));
  }
}

TEST(SaturateCastTest, LargerToSmallerUnsignedInteger) {
  struct ValuePair {
    uint64_t from;
    uint32_t to;
  };
  constexpr ValuePair kValuePairs[] = {
      {std::numeric_limits<uint64_t>::max(),
       std::numeric_limits<uint32_t>::max()},
      {std::numeric_limits<uint64_t>::max() / 2 + 42,
       std::numeric_limits<uint32_t>::max()},
      {std::numeric_limits<uint32_t>::max(),
       std::numeric_limits<uint32_t>::max()},
      {42, 42},
      {0, 0},
  };
  for (const ValuePair& value_pair : kValuePairs) {
    EXPECT_EQ(value_pair.to, saturate_cast<uint32_t>(value_pair.from));
  }
}

TEST(SaturateCastTest, LargerSignedToSmallerUnsignedInteger) {
  struct ValuePair {
    int64_t from;
    uint32_t to;
  };
  constexpr ValuePair kValuePairs[] = {
      {std::numeric_limits<int64_t>::max(),
       std::numeric_limits<uint32_t>::max()},
      {std::numeric_limits<int64_t>::max() / 2 + 42,
       std::numeric_limits<uint32_t>::max()},
      {std::numeric_limits<uint32_t>::max(),
       std::numeric_limits<uint32_t>::max()},
      {42, 42},
      {0, 0},
      {-42, 0},
      {std::numeric_limits<int64_t>::min() / 2 - 42, 0},
      {std::numeric_limits<int64_t>::min(), 0},
  };
  for (const ValuePair& value_pair : kValuePairs) {
    EXPECT_EQ(value_pair.to, saturate_cast<uint32_t>(value_pair.from));
  }
}

TEST(SaturateCastTest, LargerUnsignedToSmallerSignedInteger) {
  struct ValuePair {
    uint64_t from;
    int32_t to;
  };
  constexpr ValuePair kValuePairs[] = {
      {std::numeric_limits<uint64_t>::max(),
       std::numeric_limits<int32_t>::max()},
      {std::numeric_limits<uint64_t>::max() / 2 + 42,
       std::numeric_limits<int32_t>::max()},
      {std::numeric_limits<int32_t>::max(),
       std::numeric_limits<int32_t>::max()},
      {42, 42},
      {0, 0},
  };
  for (const ValuePair& value_pair : kValuePairs) {
    EXPECT_EQ(value_pair.to, saturate_cast<int32_t>(value_pair.from));
  }
}

TEST(SaturateCastTest, SignedToUnsigned32BitInteger) {
  struct ValuePair {
    int32_t from;
    uint32_t to;
  };
  constexpr ValuePair kValuePairs[] = {
      {std::numeric_limits<int32_t>::max(),
       std::numeric_limits<int32_t>::max()},
      {42, 42},
      {0, 0},
      {-42, 0},
      {std::numeric_limits<int32_t>::min(), 0},
  };
  for (const ValuePair& value_pair : kValuePairs) {
    EXPECT_EQ(value_pair.to, saturate_cast<uint32_t>(value_pair.from));
  }
}

TEST(SaturateCastTest, UnsignedToSigned32BitInteger) {
  struct ValuePair {
    uint32_t from;
    int32_t to;
  };
  constexpr ValuePair kValuePairs[] = {
      {std::numeric_limits<uint32_t>::max(),
       std::numeric_limits<int32_t>::max()},
      {std::numeric_limits<uint32_t>::max() / 2 + 42,
       std::numeric_limits<int32_t>::max()},
      {std::numeric_limits<int32_t>::max(),
       std::numeric_limits<int32_t>::max()},
      {42, 42},
      {0, 0},
  };
  for (const ValuePair& value_pair : kValuePairs) {
    EXPECT_EQ(value_pair.to, saturate_cast<int32_t>(value_pair.from));
  }
}

TEST(SaturateCastTest, SignedToUnsigned64BitInteger) {
  struct ValuePair {
    int64_t from;
    uint64_t to;
  };
  constexpr ValuePair kValuePairs[] = {
      {std::numeric_limits<int64_t>::max(),
       std::numeric_limits<int64_t>::max()},
      {42, 42},
      {0, 0},
      {-42, 0},
      {std::numeric_limits<int64_t>::min(), 0},
  };
  for (const ValuePair& value_pair : kValuePairs) {
    EXPECT_EQ(value_pair.to, saturate_cast<uint64_t>(value_pair.from));
  }
}

TEST(SaturateCastTest, UnsignedToSigned64BitInteger) {
  struct ValuePair {
    uint64_t from;
    int64_t to;
  };
  constexpr ValuePair kValuePairs[] = {
      {std::numeric_limits<uint64_t>::max(),
       std::numeric_limits<int64_t>::max()},
      {std::numeric_limits<uint64_t>::max() / 2 + 42,
       std::numeric_limits<int64_t>::max()},
      {std::numeric_limits<int64_t>::max(),
       std::numeric_limits<int64_t>::max()},
      {42, 42},
      {0, 0},
  };
  for (const ValuePair& value_pair : kValuePairs) {
    EXPECT_EQ(value_pair.to, saturate_cast<int64_t>(value_pair.from));
  }
}

TEST(SaturateCastTest, Float32ToSigned32) {
  struct ValuePair {
    float from;
    int32_t to;
  };
  constexpr float kFloatMax = std::numeric_limits<float>::max();
  // Note: kIntMax is one larger because float cannot represent the exact value.
  constexpr float kIntMax =
      static_cast<float>(std::numeric_limits<int32_t>::max());
  constexpr float kIntMin = std::numeric_limits<int32_t>::min();
  const ValuePair kValuePairs[] = {
      {kFloatMax, std::numeric_limits<int32_t>::max()},
      {std::nextafter(kIntMax, kFloatMax), std::numeric_limits<int32_t>::max()},
      {kIntMax, std::numeric_limits<int32_t>::max()},
      {std::nextafter(kIntMax, 0.f), 2147483520},
      {42, 42},
      {0, 0},
      {-42, -42},
      {std::nextafter(kIntMin, 0.f), -2147483520},
      {kIntMin, std::numeric_limits<int32_t>::min()},
      {std::nextafter(kIntMin, -kFloatMax),
       std::numeric_limits<int32_t>::min()},
      {-kFloatMax, std::numeric_limits<int32_t>::min()},
  };
  for (const ValuePair& value_pair : kValuePairs) {
    EXPECT_EQ(value_pair.to, saturate_cast<int32_t>(value_pair.from));
  }
}

TEST(SaturateCastTest, Float32ToSigned64) {
  struct ValuePair {
    float from;
    int64_t to;
  };
  constexpr float kFloatMax = std::numeric_limits<float>::max();
  // Note: kIntMax is one larger because float cannot represent the exact value.
  constexpr float kIntMax =
      static_cast<float>(std::numeric_limits<int64_t>::max());
  constexpr float kIntMin = std::numeric_limits<int64_t>::min();
  const ValuePair kValuePairs[] = {
      {kFloatMax, std::numeric_limits<int64_t>::max()},
      {std::nextafter(kIntMax, kFloatMax), std::numeric_limits<int64_t>::max()},
      {kIntMax, std::numeric_limits<int64_t>::max()},
      {std::nextafter(kIntMax, 0.f), INT64_C(9223371487098961920)},
      {42, 42},
      {0, 0},
      {-42, -42},
      {std::nextafter(kIntMin, 0.f), INT64_C(-9223371487098961920)},
      {kIntMin, std::numeric_limits<int64_t>::min()},
      {std::nextafter(kIntMin, -kFloatMax),
       std::numeric_limits<int64_t>::min()},
      {-kFloatMax, std::numeric_limits<int64_t>::min()},
  };
  for (const ValuePair& value_pair : kValuePairs) {
    EXPECT_EQ(value_pair.to, saturate_cast<int64_t>(value_pair.from));
  }
}

TEST(SaturateCastTest, Float64ToSigned32) {
  struct ValuePair {
    double from;
    int32_t to;
  };
  constexpr double kDoubleMax = std::numeric_limits<double>::max();
  constexpr double kIntMax = std::numeric_limits<int32_t>::max();
  constexpr double kIntMin = std::numeric_limits<int32_t>::min();
  const ValuePair kValuePairs[] = {
      {kDoubleMax, std::numeric_limits<int32_t>::max()},
      {std::nextafter(kIntMax, kDoubleMax),
       std::numeric_limits<int32_t>::max()},
      {kIntMax, std::numeric_limits<int32_t>::max()},
      {std::nextafter(kIntMax, 0.0), std::numeric_limits<int32_t>::max() - 1},
      {42, 42},
      {0, 0},
      {-42, -42},
      {std::nextafter(kIntMin, 0.0), std::numeric_limits<int32_t>::min() + 1},
      {kIntMin, std::numeric_limits<int32_t>::min()},
      {std::nextafter(kIntMin, -kDoubleMax),
       std::numeric_limits<int32_t>::min()},
      {-kDoubleMax, std::numeric_limits<int32_t>::min()},
  };
  for (const ValuePair& value_pair : kValuePairs) {
    EXPECT_EQ(value_pair.to, saturate_cast<int32_t>(value_pair.from));
  }
}

TEST(SaturateCastTest, Float64ToSigned64) {
  struct ValuePair {
    double from;
    int64_t to;
  };
  constexpr double kDoubleMax = std::numeric_limits<double>::max();
  // Note: kIntMax is one larger because double cannot represent the exact
  // value.
  constexpr double kIntMax =
      static_cast<double>(std::numeric_limits<int64_t>::max());
  constexpr double kIntMin = std::numeric_limits<int64_t>::min();
  const ValuePair kValuePairs[] = {
      {kDoubleMax, std::numeric_limits<int64_t>::max()},
      {std::nextafter(kIntMax, kDoubleMax),
       std::numeric_limits<int64_t>::max()},
      {kIntMax, std::numeric_limits<int64_t>::max()},
      {std::nextafter(kIntMax, 0.0), INT64_C(9223372036854774784)},
      {42, 42},
      {0, 0},
      {-42, -42},
      {std::nextafter(kIntMin, 0.0), INT64_C(-9223372036854774784)},
      {kIntMin, std::numeric_limits<int64_t>::min()},
      {std::nextafter(kIntMin, -kDoubleMax),
       std::numeric_limits<int64_t>::min()},
      {-kDoubleMax, std::numeric_limits<int64_t>::min()},
  };
  for (const ValuePair& value_pair : kValuePairs) {
    EXPECT_EQ(value_pair.to, saturate_cast<int64_t>(value_pair.from));
  }
}

TEST(SaturateCastTest, Float32ToUnsigned64) {
  struct ValuePair {
    float from;
    uint64_t to;
  };
  constexpr float kFloatMax = std::numeric_limits<float>::max();
  // Note: kIntMax is one larger because float cannot represent the exact value.
  constexpr float kIntMax =
      static_cast<float>(std::numeric_limits<uint64_t>::max());
  const ValuePair kValuePairs[] = {
      {kFloatMax, std::numeric_limits<uint64_t>::max()},
      {std::nextafter(kIntMax, kFloatMax),
       std::numeric_limits<uint64_t>::max()},
      {kIntMax, std::numeric_limits<uint64_t>::max()},
      {std::nextafter(kIntMax, 0.f), UINT64_C(18446742974197923840)},
      {42, 42},
      {0, 0},
      {-42, 0},
      {-kFloatMax, 0},
  };
  for (const ValuePair& value_pair : kValuePairs) {
    EXPECT_EQ(value_pair.to, saturate_cast<uint64_t>(value_pair.from));
  }
}

TEST(SaturateCastTest, RoundingFloat32ToSigned64) {
  struct ValuePair {
    float from;
    int64_t to;
  };
  constexpr float kFloatMax = std::numeric_limits<float>::max();
  // Note: kIntMax is one larger because float cannot represent the exact value.
  constexpr float kIntMax =
      static_cast<float>(std::numeric_limits<int64_t>::max());
  constexpr float kIntMin = std::numeric_limits<int64_t>::min();
  const ValuePair kValuePairs[] = {
      {kFloatMax, std::numeric_limits<int64_t>::max()},
      {std::nextafter(kIntMax, kFloatMax), std::numeric_limits<int64_t>::max()},
      {kIntMax, std::numeric_limits<int64_t>::max()},
      {std::nextafter(kIntMax, 0.f), INT64_C(9223371487098961920)},
      {41.9, 42},
      {42, 42},
      {42.6, 43},
      {42.5, 43},
      {42.4, 42},
      {0.5, 1},
      {0.1, 0},
      {0, 0},
      {-0.1, 0},
      {-0.5, -1},
      {-41.9, -42},
      {-42, -42},
      {-42.4, -42},
      {-42.5, -43},
      {-42.6, -43},
      {std::nextafter(kIntMin, 0.f), INT64_C(-9223371487098961920)},
      {kIntMin, std::numeric_limits<int64_t>::min()},
      {std::nextafter(kIntMin, -kFloatMax),
       std::numeric_limits<int64_t>::min()},
      {-kFloatMax, std::numeric_limits<int64_t>::min()},
  };
  for (const ValuePair& value_pair : kValuePairs) {
    EXPECT_EQ(value_pair.to, rounded_saturate_cast<int64_t>(value_pair.from));
  }
}

}  // namespace
}  // namespace openscreen
