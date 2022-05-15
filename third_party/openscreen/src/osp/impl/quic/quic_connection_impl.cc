// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "osp/impl/quic/quic_connection_impl.h"

#include <limits>
#include <memory>

#include "absl/types/optional.h"
#include "osp/impl/quic/quic_connection_factory_impl.h"
#include "platform/base/error.h"
#include "third_party/chromium_quic/src/net/third_party/quic/platform/impl/quic_chromium_clock.h"
#include "util/osp_logging.h"
#include "util/trace_logging.h"

namespace openscreen {
namespace osp {

UdpTransport::UdpTransport(UdpSocket* socket, const IPEndpoint& destination)
    : socket_(socket), destination_(destination) {
  OSP_DCHECK(socket_);
}

UdpTransport::UdpTransport(UdpTransport&&) noexcept = default;
UdpTransport::~UdpTransport() = default;

UdpTransport& UdpTransport::operator=(UdpTransport&&) noexcept = default;

int UdpTransport::Write(const char* buffer,
                        size_t buffer_length,
                        const PacketInfo& info) {
  TRACE_SCOPED(TraceCategory::kQuic, "UdpTransport::Write");
  socket_->SendMessage(buffer, buffer_length, destination_);
  OSP_DCHECK_LE(buffer_length,
                static_cast<size_t>(std::numeric_limits<int>::max()));
  return static_cast<int>(buffer_length);
}

QuicStreamImpl::QuicStreamImpl(QuicStream::Delegate* delegate,
                               ::quic::QuartcStream* stream)
    : QuicStream(delegate, stream->id()), stream_(stream) {
  stream_->SetDelegate(this);
}

QuicStreamImpl::~QuicStreamImpl() = default;

void QuicStreamImpl::Write(const uint8_t* data, size_t data_size) {
  TRACE_SCOPED(TraceCategory::kQuic, "QuicStreamImpl::Write");
  OSP_DCHECK(!stream_->write_side_closed());
  stream_->WriteOrBufferData(
      ::quic::QuicStringPiece(reinterpret_cast<const char*>(data), data_size),
      false, nullptr);
}

void QuicStreamImpl::CloseWriteEnd() {
  TRACE_SCOPED(TraceCategory::kQuic, "QuicStreamImpl::CloseWriteEnd");
  if (!stream_->write_side_closed())
    stream_->FinishWriting();
}

void QuicStreamImpl::OnReceived(::quic::QuartcStream* stream,
                                const char* data,
                                size_t data_size) {
  TRACE_SCOPED(TraceCategory::kQuic, "QuicStreamImpl::OnReceived");
  delegate_->OnReceived(this, data, data_size);
}

void QuicStreamImpl::OnClose(::quic::QuartcStream* stream) {
  TRACE_SCOPED(TraceCategory::kQuic, "QuicStreamImpl::OnClose");
  delegate_->OnClose(stream->id());
}

void QuicStreamImpl::OnBufferChanged(::quic::QuartcStream* stream) {}

// Passes a received UDP packet to the QUIC implementation.  If this contains
// any stream data, it will be passed automatically to the relevant
// QuicStream::Delegate objects.
void QuicConnectionImpl::OnRead(UdpSocket* socket, ErrorOr<UdpPacket> data) {
  TRACE_SCOPED(TraceCategory::kQuic, "QuicConnectionImpl::OnRead");
  if (data.is_error()) {
    TRACE_SET_RESULT(data.error());
    return;
  }

  session_->OnTransportReceived(
      reinterpret_cast<const char*>(data.value().data()), data.value().size());
}

void QuicConnectionImpl::OnSendError(UdpSocket* socket, Error error) {
  // TODO(crbug.com/openscreen/67): Implement this method.
  OSP_UNIMPLEMENTED();
}

void QuicConnectionImpl::OnError(UdpSocket* socket, Error error) {
  // TODO(crbug.com/openscreen/67): Implement this method.
  OSP_UNIMPLEMENTED();
}

QuicConnectionImpl::QuicConnectionImpl(
    QuicConnectionFactoryImpl* parent_factory,
    QuicConnection::Delegate* delegate,
    std::unique_ptr<UdpTransport> udp_transport,
    std::unique_ptr<::quic::QuartcSession> session)
    : QuicConnection(delegate),
      parent_factory_(parent_factory),
      session_(std::move(session)),
      udp_transport_(std::move(udp_transport)) {
  TRACE_SCOPED(TraceCategory::kQuic, "QuicConnectionImpl::QuicConnectionImpl");
  session_->SetDelegate(this);
  session_->OnTransportCanWrite();
  session_->StartCryptoHandshake();
}

QuicConnectionImpl::~QuicConnectionImpl() = default;

std::unique_ptr<QuicStream> QuicConnectionImpl::MakeOutgoingStream(
    QuicStream::Delegate* delegate) {
  TRACE_SCOPED(TraceCategory::kQuic, "QuicConnectionImpl::MakeOutgoingStream");
  ::quic::QuartcStream* stream = session_->CreateOutgoingDynamicStream();
  return std::make_unique<QuicStreamImpl>(delegate, stream);
}

void QuicConnectionImpl::Close() {
  TRACE_SCOPED(TraceCategory::kQuic, "QuicConnectionImpl::Close");
  session_->CloseConnection("closed");
}

void QuicConnectionImpl::OnCryptoHandshakeComplete() {
  TRACE_SCOPED(TraceCategory::kQuic,
               "QuicConnectionImpl::OnCryptoHandshakeComplete");
  delegate_->OnCryptoHandshakeComplete(session_->connection_id());
}

void QuicConnectionImpl::OnIncomingStream(::quic::QuartcStream* stream) {
  TRACE_SCOPED(TraceCategory::kQuic, "QuicConnectionImpl::OnIncomingStream");
  auto public_stream = std::make_unique<QuicStreamImpl>(
      delegate_->NextStreamDelegate(session_->connection_id(), stream->id()),
      stream);
  streams_.push_back(public_stream.get());
  delegate_->OnIncomingStream(session_->connection_id(),
                              std::move(public_stream));
}

void QuicConnectionImpl::OnConnectionClosed(
    ::quic::QuicErrorCode error_code,
    const ::quic::QuicString& error_details,
    ::quic::ConnectionCloseSource source) {
  TRACE_SCOPED(TraceCategory::kQuic, "QuicConnectionImpl::OnConnectionClosed");
  parent_factory_->OnConnectionClosed(this);
  delegate_->OnConnectionClosed(session_->connection_id());
}

}  // namespace osp
}  // namespace openscreen
