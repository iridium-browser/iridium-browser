// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "util/url.h"

#include <utility>

#include "gtest/gtest.h"

namespace openscreen {

TEST(UrlTest, Validity) {
  const char* valid_cases[] = {
      "http://google.com",
      "unknown://google.com",
      "http://user:pass@google.com",
      "http://google.com:12345",
      "http://google.com/path",
      "http://google.com//path",
      "http://google.com?k=v#fragment",
      "http://user:pass@google.com:12345/path?k=v#fragment",
      "http:/path",
      "http:path",
  };
  constexpr int valid_case_count = sizeof(valid_cases) / sizeof(valid_cases[0]);
  for (size_t i = 0; i < valid_case_count; i++) {
    EXPECT_TRUE(Url(valid_cases[i]).is_valid()) << "Case: " << valid_cases[i];
  }

  const char* invalid_cases[] = {
      "http://?k=v",      "http:://google.com",
      "http//google.com", "http://google.com:12three45",
      "://google.com",    "path",
  };
  constexpr int invalid_case_count =
      sizeof(invalid_cases) / sizeof(invalid_cases[0]);
  for (size_t i = 0; i < invalid_case_count; i++) {
    EXPECT_FALSE(Url(invalid_cases[i]).is_valid())
        << "Case: " << invalid_cases[i];
  }
}

TEST(UrlTest, Components) {
  Url url("http://user:pass@google.com:99/foo;bar?q=a#ref");

  EXPECT_TRUE(url.is_valid());
  EXPECT_TRUE(url.has_host());
  EXPECT_TRUE(url.has_port());
  EXPECT_TRUE(url.has_path());
  EXPECT_TRUE(url.has_query());

  EXPECT_EQ("http", url.scheme());
  EXPECT_EQ("google.com", url.host());
  EXPECT_EQ(99, url.port());
  EXPECT_EQ("/foo;bar", url.path());
  EXPECT_EQ("q=a", url.query());
}

TEST(UrlTest, Copy) {
  Url url1("http://user:pass@google.com:99/foo;bar?q=a#ref");
  Url url2(url1);

  EXPECT_TRUE(url1.is_valid());
  EXPECT_TRUE(url1.has_host());
  EXPECT_TRUE(url1.has_port());
  EXPECT_TRUE(url1.has_path());
  EXPECT_TRUE(url1.has_query());

  EXPECT_EQ("http", url1.scheme());
  EXPECT_EQ("google.com", url1.host());
  EXPECT_EQ(99, url1.port());
  EXPECT_EQ("/foo;bar", url1.path());
  EXPECT_EQ("q=a", url1.query());

  EXPECT_TRUE(url2.is_valid());
  EXPECT_TRUE(url2.has_host());
  EXPECT_TRUE(url2.has_port());
  EXPECT_TRUE(url2.has_path());
  EXPECT_TRUE(url2.has_query());

  EXPECT_EQ("http", url2.scheme());
  EXPECT_EQ("google.com", url2.host());
  EXPECT_EQ(99, url2.port());
  EXPECT_EQ("/foo;bar", url2.path());
  EXPECT_EQ("q=a", url2.query());
}

TEST(UrlTest, Move) {
  Url url1("http://user:pass@google.com:99/foo;bar?q=a#ref");
  Url url2(std::move(url1));

  EXPECT_FALSE(url1.is_valid());

  EXPECT_TRUE(url2.is_valid());
  EXPECT_TRUE(url2.has_host());
  EXPECT_TRUE(url2.has_port());
  EXPECT_TRUE(url2.has_path());
  EXPECT_TRUE(url2.has_query());

  EXPECT_EQ("http", url2.scheme());
  EXPECT_EQ("google.com", url2.host());
  EXPECT_EQ(99, url2.port());
  EXPECT_EQ("/foo;bar", url2.path());
  EXPECT_EQ("q=a", url2.query());
}

TEST(UrlTest, Assign) {
  Url url1("http://user:pass@google.com:99/foo;bar?q=a#ref");
  Url url2("https://example.com");
  Url url3("https://example.com");

  url2 = url1;

  EXPECT_TRUE(url1.is_valid());
  EXPECT_TRUE(url1.has_host());
  EXPECT_TRUE(url1.has_port());
  EXPECT_TRUE(url1.has_path());
  EXPECT_TRUE(url1.has_query());

  EXPECT_EQ("http", url1.scheme());
  EXPECT_EQ("google.com", url1.host());
  EXPECT_EQ(99, url1.port());
  EXPECT_EQ("/foo;bar", url1.path());
  EXPECT_EQ("q=a", url1.query());

  EXPECT_TRUE(url2.is_valid());
  EXPECT_TRUE(url2.has_host());
  EXPECT_TRUE(url2.has_port());
  EXPECT_TRUE(url2.has_path());
  EXPECT_TRUE(url2.has_query());

  EXPECT_EQ("http", url2.scheme());
  EXPECT_EQ("google.com", url2.host());
  EXPECT_EQ(99, url2.port());
  EXPECT_EQ("/foo;bar", url2.path());
  EXPECT_EQ("q=a", url2.query());

  url3 = std::move(url1);

  EXPECT_FALSE(url1.is_valid());

  EXPECT_TRUE(url3.is_valid());
  EXPECT_TRUE(url3.has_host());
  EXPECT_TRUE(url3.has_port());
  EXPECT_TRUE(url3.has_path());
  EXPECT_TRUE(url3.has_query());

  EXPECT_EQ("http", url3.scheme());
  EXPECT_EQ("google.com", url3.host());
  EXPECT_EQ(99, url3.port());
  EXPECT_EQ("/foo;bar", url3.path());
  EXPECT_EQ("q=a", url3.query());
}

TEST(UrlTest, Cast) {
  std::string cast(
      "cast:233637DE?capabilities=video_out%2Caudio_out&clientId="
      "157299995976393660&autoJoinPolicy=tab_and_origin_scoped&"
      "defaultActionPolicy=create_session&launchTimeout=30000");
  Url url(cast);
  EXPECT_TRUE(url.is_valid());
  EXPECT_TRUE(url.has_query());
  EXPECT_EQ(url.scheme(), "cast");
  EXPECT_EQ(url.path(), "233637DE");
  EXPECT_EQ(url.query(), cast.c_str() + 14);
}

}  // namespace openscreen
