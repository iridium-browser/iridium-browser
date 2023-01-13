// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/common/channel/namespace_router.h"

#include <utility>

#include "cast/common/channel/cast_message_handler.h"
#include "cast/common/channel/proto/cast_channel.pb.h"
#include "cast/common/channel/testing/fake_cast_socket.h"
#include "cast/common/channel/testing/mock_cast_message_handler.h"
#include "cast/common/channel/virtual_connection_router.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace openscreen {
namespace cast {
namespace {

using ::cast::channel::CastMessage;
using ::testing::_;
using ::testing::Invoke;

class NamespaceRouterTest : public ::testing::Test {
 public:
 protected:
  CastSocket* socket() { return &fake_socket_.socket; }

  FakeCastSocket fake_socket_;
  VirtualConnectionRouter vc_router_;
  NamespaceRouter router_;
};

}  // namespace

TEST_F(NamespaceRouterTest, NoHandlersNoop) {
  CastMessage message;
  message.set_namespace_("anzrfcnpr");
  router_.OnMessage(&vc_router_, socket(), std::move(message));
}

TEST_F(NamespaceRouterTest, MultipleHandlers) {
  MockCastMessageHandler media_handler;
  MockCastMessageHandler auth_handler;
  MockCastMessageHandler connection_handler;

  router_.AddNamespaceHandler("media", &media_handler);
  router_.AddNamespaceHandler("auth", &auth_handler);
  router_.AddNamespaceHandler("connection", &connection_handler);

  EXPECT_CALL(media_handler, OnMessage(_, _, _)).Times(0);
  EXPECT_CALL(auth_handler, OnMessage(_, _, _))
      .WillOnce(Invoke([](VirtualConnectionRouter* router, CastSocket*,
                          CastMessage message) {
        EXPECT_EQ(message.namespace_(), "auth");
      }));
  EXPECT_CALL(connection_handler, OnMessage(_, _, _))
      .WillOnce(Invoke([](VirtualConnectionRouter* router, CastSocket*,
                          CastMessage message) {
        EXPECT_EQ(message.namespace_(), "connection");
      }));

  CastMessage auth_message;
  auth_message.set_namespace_("auth");
  router_.OnMessage(&vc_router_, socket(), std::move(auth_message));

  CastMessage connection_message;
  connection_message.set_namespace_("connection");
  router_.OnMessage(&vc_router_, socket(), std::move(connection_message));
}

TEST_F(NamespaceRouterTest, RemoveHandler) {
  MockCastMessageHandler handler1;
  MockCastMessageHandler handler2;

  router_.AddNamespaceHandler("one", &handler1);
  router_.AddNamespaceHandler("two", &handler2);

  router_.RemoveNamespaceHandler("one");

  EXPECT_CALL(handler1, OnMessage(_, _, _)).Times(0);
  EXPECT_CALL(handler2, OnMessage(_, _, _))
      .WillOnce(Invoke(
          [](VirtualConnectionRouter* router, CastSocket* socket,
             CastMessage message) { EXPECT_EQ("two", message.namespace_()); }));

  CastMessage message1;
  message1.set_namespace_("one");
  router_.OnMessage(&vc_router_, socket(), std::move(message1));

  CastMessage message2;
  message2.set_namespace_("two");
  router_.OnMessage(&vc_router_, socket(), std::move(message2));
}

}  // namespace cast
}  // namespace openscreen
