// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "discovery/dnssd/public/dns_sd_instance_endpoint.h"

#include "discovery/dnssd/public/dns_sd_instance.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace openscreen {
namespace discovery {

TEST(DnsSdInstanceEndpointTests, ComparisonTests) {
  constexpr NetworkInterfaceIndex kIndex0 = 0;
  constexpr NetworkInterfaceIndex kIndex1 = 1;
  DnsSdInstance instance("instance", "_test._tcp", "local", {}, 80);
  DnsSdInstance instance2("instance", "_test._tcp", "local", {}, 79);
  IPEndpoint ep1{{192, 168, 80, 32}, 80};
  IPEndpoint ep2{{192, 168, 80, 32}, 79};
  IPEndpoint ep3{{192, 168, 80, 33}, 79};
  DnsSdInstanceEndpoint endpoint1(instance, kIndex1, ep1);
  DnsSdInstanceEndpoint endpoint2("instance", "_test._tcp", "local", {},
                                  kIndex1, ep1);
  DnsSdInstanceEndpoint endpoint3(instance2, kIndex1, ep2);
  DnsSdInstanceEndpoint endpoint4(instance2, kIndex0, ep2);
  DnsSdInstanceEndpoint endpoint5(instance2, kIndex1, ep3);
  DnsSdInstanceEndpoint endpoint6("instance", "_test._tcp", "local", {},
                                  kIndex1, ep1, "foo", "bar");
  DnsSdInstanceEndpoint endpoint7("instance", "_test._tcp", "local", {},
                                  kIndex1, ep1, "foo", "foobar");
  DnsSdInstanceEndpoint endpoint8("instance", "_test._tcp", "local", {},
                                  kIndex1, ep1, "foobar");

  EXPECT_EQ(static_cast<DnsSdInstance>(endpoint1),
            static_cast<DnsSdInstance>(endpoint2));
  EXPECT_EQ(endpoint1, endpoint2);
  EXPECT_GE(endpoint1, endpoint3);
  EXPECT_GE(endpoint1, endpoint4);
  EXPECT_LE(endpoint1, endpoint5);
  EXPECT_LE(endpoint1, endpoint6);
  EXPECT_LE(endpoint1, endpoint7);
  EXPECT_LE(endpoint1, endpoint8);

  EXPECT_GE(endpoint3, endpoint4);
  EXPECT_LE(endpoint3, endpoint5);

  EXPECT_LE(endpoint4, endpoint5);

  EXPECT_LE(endpoint6, endpoint7);
  EXPECT_GE(endpoint6, endpoint8);
  EXPECT_GE(endpoint7, endpoint8);
}

TEST(DnsSdInstanceEndpointTests, Constructors) {
  constexpr NetworkInterfaceIndex kIndex = 0;
  std::vector<std::string> subtypes{"foo", "bar", "foobar"};
  IPEndpoint endpoint1{{192, 168, 12, 21}, 80};
  IPEndpoint endpoint2{{227, 0, 0, 1}, 80};
  DnsSdInstance instance("instance", "_test._tcp", "local", {}, 80, subtypes);

  DnsSdInstanceEndpoint ep1(instance, kIndex, endpoint1, endpoint2);
  DnsSdInstanceEndpoint ep2(instance, kIndex,
                            std::vector<IPEndpoint>{endpoint1, endpoint2});
  DnsSdInstanceEndpoint ep3("instance", "_test._tcp", "local", {}, kIndex,
                            endpoint1, endpoint2, "foo", "bar", "foobar");
  DnsSdInstanceEndpoint ep4("instance", "_test._tcp", "local", {}, kIndex,
                            std::vector<IPEndpoint>{endpoint1, endpoint2},
                            subtypes);

  EXPECT_EQ(ep1, ep2);
  EXPECT_EQ(ep1, ep3);
  EXPECT_EQ(ep1, ep4);
}

}  // namespace discovery
}  // namespace openscreen
