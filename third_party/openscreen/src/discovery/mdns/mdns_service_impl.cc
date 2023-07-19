// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "discovery/mdns/mdns_service_impl.h"

#include <memory>
#include <utility>
#include <vector>

#include "discovery/common/reporting_client.h"
#include "discovery/mdns/mdns_records.h"
#include "discovery/mdns/public/mdns_constants.h"

namespace openscreen {
namespace discovery {

// static
std::unique_ptr<MdnsService> MdnsService::Create(
    TaskRunner* task_runner,
    ReportingClient* reporting_client,
    const Config& config,
    const InterfaceInfo& network_info) {
  return std::make_unique<MdnsServiceImpl>(
      task_runner, Clock::now, reporting_client, config, network_info);
}

MdnsServiceImpl::MdnsServiceImpl(TaskRunner* task_runner,
                                 ClockNowFunctionPtr now_function,
                                 ReportingClient* reporting_client,
                                 const Config& config,
                                 const InterfaceInfo& network_info)
    : task_runner_(task_runner),
      now_function_(now_function),
      reporting_client_(reporting_client),
      receiver_(config),
      interface_(network_info.index) {
  OSP_DCHECK(task_runner_);
  OSP_DCHECK(reporting_client_);

  // Create all UDP sockets needed for this object. They should not yet be bound
  // so that they do not send or receive data until the objects on which their
  // callback depends is initialized.
  // NOTE: we bind to the Any addresses here because traffic is filtered by
  // the multicast join calls.
  if (network_info.GetIpAddressV4()) {
    ErrorOr<std::unique_ptr<UdpSocket>> socket = UdpSocket::Create(
        task_runner, this,
        IPEndpoint{IPAddress::kAnyV4(), kDefaultMulticastPort});
    OSP_DCHECK(!socket.is_error());
    OSP_DCHECK(socket.value().get());
    OSP_DCHECK(socket.value()->IsIPv4());

    socket_v4_ = std::move(socket.value());
  }

  if (network_info.GetIpAddressV6()) {
    ErrorOr<std::unique_ptr<UdpSocket>> socket = UdpSocket::Create(
        task_runner, this,
        IPEndpoint{IPAddress::kAnyV6(), kDefaultMulticastPort});
    OSP_DCHECK(!socket.is_error());
    OSP_DCHECK(socket.value().get());
    OSP_DCHECK(socket.value()->IsIPv6());

    socket_v6_ = std::move(socket.value());
  }

  // Initialize objects which depend on the above sockets.
  UdpSocket* socket_ptr =
      socket_v4_.get() ? socket_v4_.get() : socket_v6_.get();
  OSP_DCHECK(socket_ptr);
  sender_ = std::make_unique<MdnsSender>(socket_ptr);
  if (config.enable_querying) {
    querier_ = std::make_unique<MdnsQuerier>(
        sender_.get(), &receiver_, task_runner_, now_function_, &random_delay_,
        reporting_client_, config);
  }
  if (config.enable_publication) {
    probe_manager_ = std::make_unique<MdnsProbeManagerImpl>(
        sender_.get(), &receiver_, &random_delay_, task_runner_, now_function_);
    publisher_ =
        std::make_unique<MdnsPublisher>(sender_.get(), probe_manager_.get(),
                                        task_runner_, now_function_, config);
    responder_ = std::make_unique<MdnsResponder>(
        publisher_.get(), probe_manager_.get(), sender_.get(), &receiver_,
        task_runner_, now_function_, &random_delay_, config);
  }

  receiver_.Start();

  // Initialize all sockets to start sending/receiving data. Now that the above
  // objects have all been created, it they should be able to safely do so.
  // NOTE: Although only one of these sockets is used for sending, both will be
  // used for reading on the mDNS v4 and v6 addresses and ports.
  if (socket_v4_) {
    socket_v4_->Bind();
  }
  if (socket_v6_) {
    socket_v6_->Bind();
  }
}

MdnsServiceImpl::~MdnsServiceImpl() = default;

void MdnsServiceImpl::StartQuery(const DomainName& name,
                                 DnsType dns_type,
                                 DnsClass dns_class,
                                 MdnsRecordChangedCallback* callback) {
  return querier_->StartQuery(name, dns_type, dns_class, callback);
}

void MdnsServiceImpl::StopQuery(const DomainName& name,
                                DnsType dns_type,
                                DnsClass dns_class,
                                MdnsRecordChangedCallback* callback) {
  return querier_->StopQuery(name, dns_type, dns_class, callback);
}

void MdnsServiceImpl::ReinitializeQueries(const DomainName& name) {
  querier_->ReinitializeQueries(name);
}

Error MdnsServiceImpl::StartProbe(MdnsDomainConfirmedProvider* callback,
                                  DomainName requested_name,
                                  IPAddress address) {
  return probe_manager_->StartProbe(callback, std::move(requested_name),
                                    std::move(address));
}

Error MdnsServiceImpl::RegisterRecord(const MdnsRecord& record) {
  return publisher_->RegisterRecord(record);
}

Error MdnsServiceImpl::UpdateRegisteredRecord(const MdnsRecord& old_record,
                                              const MdnsRecord& new_record) {
  return publisher_->UpdateRegisteredRecord(old_record, new_record);
}

Error MdnsServiceImpl::UnregisterRecord(const MdnsRecord& record) {
  return publisher_->UnregisterRecord(record);
}

void MdnsServiceImpl::OnError(UdpSocket* socket, Error error) {
  reporting_client_->OnFatalError(error);
}

void MdnsServiceImpl::OnSendError(UdpSocket* socket, Error error) {
  sender_->OnSendError(socket, error);
}

void MdnsServiceImpl::OnRead(UdpSocket* socket, ErrorOr<UdpPacket> packet) {
  receiver_.OnRead(socket, std::move(packet));
}

void MdnsServiceImpl::OnBound(UdpSocket* socket) {
  // Socket configuration must occur after the socket has been bound
  // successfully.
  if (socket == socket_v4_.get()) {
    socket_v4_->SetMulticastOutboundInterface(interface_);
    socket_v4_->JoinMulticastGroup(kDefaultMulticastGroupIPv4, interface_);
    socket_v4_->JoinMulticastGroup(kDefaultSiteLocalGroupIPv4, interface_);
  } else if (socket == socket_v6_.get()) {
    socket_v6_->SetMulticastOutboundInterface(interface_);
    socket_v6_->JoinMulticastGroup(kDefaultMulticastGroupIPv6, interface_);
    socket_v6_->JoinMulticastGroup(kDefaultSiteLocalGroupIPv6, interface_);
  } else {
    // Sanity check: we shouldn't be called for sockets we haven't subscribed
    // to.
    OSP_NOTREACHED();
  }
}

}  // namespace discovery
}  // namespace openscreen
