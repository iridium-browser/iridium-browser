// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "discovery/dnssd/impl/dns_data_graph.h"

#include <utility>

#include "discovery/dnssd/impl/conversion_layer.h"
#include "discovery/dnssd/impl/instance_key.h"
#include "util/std_util.h"

namespace openscreen {
namespace discovery {
namespace {

ErrorOr<DnsSdInstanceEndpoint> CreateEndpoint(
    const DomainName& domain,
    const absl::optional<ARecordRdata>& a,
    const absl::optional<AAAARecordRdata>& aaaa,
    const SrvRecordRdata& srv,
    const TxtRecordRdata& txt,
    NetworkInterfaceIndex network_interface) {
  // Create the user-visible TXT record representation.
  ErrorOr<DnsSdTxtRecord> txt_or_error = CreateFromDnsTxt(txt);
  if (txt_or_error.is_error()) {
    return txt_or_error.error();
  }

  InstanceKey instance_id(domain);
  std::vector<IPEndpoint> endpoints;
  if (a.has_value()) {
    endpoints.push_back({a.value().ipv4_address(), srv.port()});
  }
  if (aaaa.has_value()) {
    endpoints.push_back({aaaa.value().ipv6_address(), srv.port()});
  }

  return DnsSdInstanceEndpoint(
      instance_id.instance_id(), instance_id.service_id(),
      instance_id.domain_id(), std::move(txt_or_error.value()),
      network_interface, std::move(endpoints));
}

class DnsDataGraphImpl : public DnsDataGraph {
 public:
  using DnsDataGraph::DomainChangeCallback;

  explicit DnsDataGraphImpl(NetworkInterfaceIndex network_interface)
      : network_interface_(network_interface) {}
  DnsDataGraphImpl(const DnsDataGraphImpl& other) = delete;
  DnsDataGraphImpl(DnsDataGraphImpl&& other) = delete;

  ~DnsDataGraphImpl() override { is_dtor_running_ = true; }

  DnsDataGraphImpl& operator=(const DnsDataGraphImpl& rhs) = delete;
  DnsDataGraphImpl& operator=(DnsDataGraphImpl&& rhs) = delete;

  // DnsDataGraph overrides.
  void StartTracking(const DomainName& domain,
                     DomainChangeCallback on_start_tracking) override;

  void StopTracking(const DomainName& domain,
                    DomainChangeCallback on_stop_tracking) override;

  std::vector<ErrorOr<DnsSdInstanceEndpoint>> CreateEndpoints(
      DomainGroup domain_group,
      const DomainName& name) const override;

  Error ApplyDataRecordChange(MdnsRecord record,
                              RecordChangedEvent event,
                              DomainChangeCallback on_start_tracking,
                              DomainChangeCallback on_stop_tracking) override;

  size_t GetTrackedDomainCount() const override { return nodes_.size(); }

  bool IsTracked(const DomainName& name) const override {
    return nodes_.find(name) != nodes_.end();
  }

 private:
  class NodeLifetimeHandler;

  using ScopedCallbackHandler = std::unique_ptr<NodeLifetimeHandler>;

  // A single node of the graph represented by this type.
  class Node {
   public:
    // NOE: This class is non-copyable, non-movable because either operation
    // would invalidate the pointer references or bidirectional edge states
    // maintained by instances of this class.
    Node(DomainName name, DnsDataGraphImpl* graph);
    Node(const Node& other) = delete;
    Node(Node&& other) = delete;

    ~Node();

    Node& operator=(const Node& rhs) = delete;
    Node& operator=(Node&& rhs) = delete;

    // Applies a record change for this node.
    Error ApplyDataRecordChange(MdnsRecord record, RecordChangedEvent event);

    // Returns the first rdata of a record with type matching |type| in this
    // node's |records_|, or absl::nullopt if no such record exists.
    template <typename T>
    absl::optional<T> GetRdata(DnsType type) {
      auto it = FindRecord(type);
      if (it == records_.end()) {
        return absl::nullopt;
      } else {
        return std::cref(absl::get<T>(it->rdata()));
      }
    }

    const DomainName& name() const { return name_; }
    const std::vector<Node*>& parents() const { return parents_; }
    const std::vector<Node*>& children() const { return children_; }
    const std::vector<MdnsRecord>& records() const { return records_; }

   private:
    // Adds or removes an edge in |graph_|.
    // NOTE: The same edge may be added multiple times, and one call to remove
    // is needed for every such call.
    void AddChild(Node* child);
    void RemoveChild(Node* child);

    // Applies the specified change to domain |child| for this node.
    void ApplyChildChange(DomainName child_name, RecordChangedEvent event);

    // Finds an iterator to the record of the provided type, or to
    // records_.end() if no such record exists.
    std::vector<MdnsRecord>::iterator FindRecord(DnsType type);

    // The domain with which the data records stored in this node are
    // associated.
    const DomainName name_;

    // Currently extant mDNS Records at |name_|.
    std::vector<MdnsRecord> records_;

    // Nodes which contain records pointing to this node's |name|.
    std::vector<Node*> parents_;

    // Nodes containing records pointed to by the records in this node.
    std::vector<Node*> children_;

    // Graph containing this node.
    DnsDataGraphImpl* graph_;
  };

  // Wrapper to handle the creation and deletion callbacks. When the object is
  // created, it sets the callback to use, and erases the callback when it goes
  // out of scope. This class allows all node creations to complete before
  // calling the user-provided callback to ensure there are no race-conditions.
  class NodeLifetimeHandler {
   public:
    NodeLifetimeHandler(DomainChangeCallback* callback_ptr,
                        DomainChangeCallback callback);

    // NOTE: The copy and delete ctors and operators must be deleted because
    // they would invalidate the pointer logic used here.
    NodeLifetimeHandler(const NodeLifetimeHandler& other) = delete;
    NodeLifetimeHandler(NodeLifetimeHandler&& other) = delete;

    ~NodeLifetimeHandler();

    NodeLifetimeHandler operator=(const NodeLifetimeHandler& other) = delete;
    NodeLifetimeHandler operator=(NodeLifetimeHandler&& other) = delete;

   private:
    std::vector<DomainName> domains_changed;

    DomainChangeCallback* callback_ptr_;
    DomainChangeCallback callback_;
  };

  // Helpers to create the ScopedCallbackHandlers for creation and deletion
  // callbacks.
  ScopedCallbackHandler GetScopedCreationHandler(
      DomainChangeCallback creation_callback);
  ScopedCallbackHandler GetScopedDeletionHandler(
      DomainChangeCallback deletion_callback);

  // Determines whether the provided node has the necessary records to be a
  // valid node at the specified domain level.
  static bool IsValidAddressNode(Node* node);
  static bool IsValidSrvAndTxtNode(Node* node);

  // Calculates the set of DnsSdInstanceEndpoints associated with the PTR
  // records present at the given |node|.
  std::vector<ErrorOr<DnsSdInstanceEndpoint>> CalculatePtrRecordEndpoints(
      Node* node) const;

  // Denotes whether the dtor for this instance has been called. This is
  // required for validation of Node instance functionality. See the
  // implementation of DnsDataGraph::Node::~Node() for more details.
  bool is_dtor_running_ = false;

  // Map from domain name to the node containing all records associated with the
  // name.
  std::map<DomainName, std::unique_ptr<Node>> nodes_;

  const NetworkInterfaceIndex network_interface_;

  // The methods to be called when a domain name either starts or stops being
  // referenced. These will only be set when a record change is ongoing, and act
  // as a single source of truth for the creation and deletion callbacks that
  // should be used during that operation.
  DomainChangeCallback on_node_creation_;
  DomainChangeCallback on_node_deletion_;
};

DnsDataGraphImpl::Node::Node(DomainName name, DnsDataGraphImpl* graph)
    : name_(std::move(name)), graph_(graph) {
  OSP_DCHECK(graph_);

  graph_->on_node_creation_(name_);
}

DnsDataGraphImpl::Node::~Node() {
  // A node should only be deleted when it has no parents. The only case where
  // a deletion can occur when parents are still extant is during destruction of
  // the holding graph. In that case, the state of the graph no longer matters
  // and all nodes will be deleted, so no need to consider the child pointers.
  if (!graph_->is_dtor_running_) {
    OSP_DCHECK(
        !ContainsIf(parents_, [this](Node* parent) { return parent != this; }));

    // Erase all childrens' parent pointers to this node.
    for (Node* child : children_) {
      RemoveChild(child);
    }

    OSP_DCHECK(graph_->on_node_deletion_);
    graph_->on_node_deletion_(name_);
  }
}

Error DnsDataGraphImpl::Node::ApplyDataRecordChange(MdnsRecord record,
                                                    RecordChangedEvent event) {
  OSP_DCHECK(record.name() == name_);

  // The child domain to which the changed record points, or none. This is only
  // applicable for PTR and SRV records, and is empty in all other cases.
  DomainName child_name;

  // The location of the current record. In the case of PTR records, multiple
  // records are allowed for the same domain. In all other cases, this is not
  // valid.
  std::vector<MdnsRecord>::iterator it;

  if (record.dns_type() == DnsType::kPTR) {
    child_name = absl::get<PtrRecordRdata>(record.rdata()).ptr_domain();
    it = std::find_if(records_.begin(), records_.end(),
                      [record](const MdnsRecord& rhs) {
                        return record.IsReannouncementOf(rhs);
                      });
  } else {
    if (record.dns_type() == DnsType::kSRV) {
      child_name = absl::get<SrvRecordRdata>(record.rdata()).target();
    }
    it = FindRecord(record.dns_type());
  }

  // Validate that the requested change is allowed and apply it.
  switch (event) {
    case RecordChangedEvent::kCreated:
      if (it != records_.end()) {
        return Error::Code::kItemAlreadyExists;
      }
      records_.push_back(std::move(record));
      break;

    case RecordChangedEvent::kUpdated:
      if (it == records_.end()) {
        return Error::Code::kItemNotFound;
      }
      *it = std::move(record);
      break;

    case RecordChangedEvent::kExpired:
      if (it == records_.end()) {
        return Error::Code::kItemNotFound;
      }
      records_.erase(it);
      break;
  }

  // Apply any required edge changes to the graph. This is only applicable if
  // a |child| was found earlier. Note that the same child can be added multiple
  // times to the |children_| vector, which simplifies the code dramatically.
  if (!child_name.empty()) {
    ApplyChildChange(std::move(child_name), event);
  }

  return Error::None();
}

void DnsDataGraphImpl::Node::ApplyChildChange(DomainName child_name,
                                              RecordChangedEvent event) {
  if (event == RecordChangedEvent::kCreated) {
    const auto pair =
        graph_->nodes_.emplace(child_name, std::unique_ptr<Node>());
    if (pair.second) {
      auto new_node = std::make_unique<Node>(std::move(child_name), graph_);
      pair.first->second.swap(new_node);
    }

    AddChild(pair.first->second.get());
  } else if (event == RecordChangedEvent::kExpired) {
    const auto it = graph_->nodes_.find(child_name);
    OSP_DCHECK(it != graph_->nodes_.end());
    RemoveChild(it->second.get());
  }
}

void DnsDataGraphImpl::Node::AddChild(Node* child) {
  OSP_DCHECK(child);
  children_.push_back(child);
  child->parents_.push_back(this);
}

void DnsDataGraphImpl::Node::RemoveChild(Node* child) {
  OSP_DCHECK(child);

  auto it = std::find(children_.begin(), children_.end(), child);
  OSP_DCHECK(it != children_.end());
  children_.erase(it);

  it = std::find(child->parents_.begin(), child->parents_.end(), this);
  OSP_DCHECK(it != child->parents_.end());
  child->parents_.erase(it);

  // If the node has been orphaned, remove it.
  it = std::find_if(child->parents_.begin(), child->parents_.end(),
                    [child](Node* parent) { return parent != child; });
  if (it == child->parents_.end()) {
    DomainName child_name = child->name();
    const size_t count = graph_->nodes_.erase(child_name);
    OSP_DCHECK(child == this || count);
  }
}

std::vector<MdnsRecord>::iterator DnsDataGraphImpl::Node::FindRecord(
    DnsType type) {
  return std::find_if(
      records_.begin(), records_.end(),
      [type](const MdnsRecord& record) { return record.dns_type() == type; });
}

DnsDataGraphImpl::NodeLifetimeHandler::NodeLifetimeHandler(
    DomainChangeCallback* callback_ptr,
    DomainChangeCallback callback)
    : callback_ptr_(callback_ptr), callback_(callback) {
  OSP_DCHECK(callback_ptr_);
  OSP_DCHECK(callback);
  OSP_DCHECK(*callback_ptr_ == nullptr);
  *callback_ptr = [this](DomainName domain) {
    domains_changed.push_back(std::move(domain));
  };
}

DnsDataGraphImpl::NodeLifetimeHandler::~NodeLifetimeHandler() {
  *callback_ptr_ = nullptr;
  for (DomainName& domain : domains_changed) {
    callback_(domain);
  }
}

DnsDataGraphImpl::ScopedCallbackHandler
DnsDataGraphImpl::GetScopedCreationHandler(
    DomainChangeCallback creation_callback) {
  return std::make_unique<NodeLifetimeHandler>(&on_node_creation_,
                                               std::move(creation_callback));
}

DnsDataGraphImpl::ScopedCallbackHandler
DnsDataGraphImpl::GetScopedDeletionHandler(
    DomainChangeCallback deletion_callback) {
  return std::make_unique<NodeLifetimeHandler>(&on_node_deletion_,
                                               std::move(deletion_callback));
}

void DnsDataGraphImpl::StartTracking(const DomainName& domain,
                                     DomainChangeCallback on_start_tracking) {
  ScopedCallbackHandler creation_handler =
      GetScopedCreationHandler(std::move(on_start_tracking));

  auto pair = nodes_.emplace(domain, std::make_unique<Node>(domain, this));

  OSP_DCHECK(pair.second);
  OSP_DCHECK(nodes_.find(domain) != nodes_.end());
}

void DnsDataGraphImpl::StopTracking(const DomainName& domain,
                                    DomainChangeCallback on_stop_tracking) {
  ScopedCallbackHandler deletion_handler =
      GetScopedDeletionHandler(std::move(on_stop_tracking));

  auto it = nodes_.find(domain);
  OSP_CHECK(it != nodes_.end());
  OSP_DCHECK(it->second->parents().empty());
  it->second.reset();
  const size_t erased_count = nodes_.erase(domain);
  OSP_DCHECK(erased_count);
}

Error DnsDataGraphImpl::ApplyDataRecordChange(
    MdnsRecord record,
    RecordChangedEvent event,
    DomainChangeCallback on_start_tracking,
    DomainChangeCallback on_stop_tracking) {
  ScopedCallbackHandler creation_handler =
      GetScopedCreationHandler(std::move(on_start_tracking));
  ScopedCallbackHandler deletion_handler =
      GetScopedDeletionHandler(std::move(on_stop_tracking));

  auto it = nodes_.find(record.name());
  if (it == nodes_.end()) {
    return Error::Code::kOperationCancelled;
  }

  const auto result =
      it->second->ApplyDataRecordChange(std::move(record), event);

  return result;
}

std::vector<ErrorOr<DnsSdInstanceEndpoint>> DnsDataGraphImpl::CreateEndpoints(
    DomainGroup domain_group,
    const DomainName& name) const {
  const auto it = nodes_.find(name);
  if (it == nodes_.end()) {
    return {};
  }
  Node* target_node = it->second.get();

  // NOTE: One of these will contain no more than one element, so iterating over
  // them both will be fast.
  std::vector<Node*> srv_and_txt_record_nodes;
  std::vector<Node*> address_record_nodes;

  switch (domain_group) {
    case DomainGroup::kAddress:
      if (!IsValidAddressNode(target_node)) {
        return {};
      }

      address_record_nodes.push_back(target_node);
      srv_and_txt_record_nodes = target_node->parents();
      break;

    case DomainGroup::kSrvAndTxt:
      if (!IsValidSrvAndTxtNode(target_node)) {
        return {};
      }

      srv_and_txt_record_nodes.push_back(target_node);
      address_record_nodes = target_node->children();
      break;

    case DomainGroup::kPtr:
      return CalculatePtrRecordEndpoints(target_node);

    default:
      return {};
  }

  // Iterate across all node pairs and create all possible DnsSdInstanceEndpoint
  // objects.
  std::vector<ErrorOr<DnsSdInstanceEndpoint>> endpoints;
  for (Node* srv_and_txt : srv_and_txt_record_nodes) {
    for (Node* address : address_record_nodes) {
      // First, there has to be a SRV record present (to provide the port
      // number), and the target of that SRV record has to be the node where the
      // address records are sourced from.
      const absl::optional<SrvRecordRdata> srv =
          srv_and_txt->GetRdata<SrvRecordRdata>(DnsType::kSRV);
      if (!srv.has_value() || srv.value().target() != address->name()) {
        continue;
      }

      // Next, a TXT record must be present to provide additional connection
      // information about the service per RFC 6763.
      const absl::optional<TxtRecordRdata> txt =
          srv_and_txt->GetRdata<TxtRecordRdata>(DnsType::kTXT);
      if (!txt.has_value()) {
        continue;
      }

      // Last, at least one address record must be present to provide an
      // endpoint for this instance.
      const absl::optional<ARecordRdata> a =
          address->GetRdata<ARecordRdata>(DnsType::kA);
      const absl::optional<AAAARecordRdata> aaaa =
          address->GetRdata<AAAARecordRdata>(DnsType::kAAAA);
      if (!a.has_value() && !aaaa.has_value()) {
        continue;
      }

      // Then use the above info to create an endpoint object. If an error
      // occurs, this is only related to the one endpoint and its possible that
      // other endpoints may still be valid, so only the one endpoint is treated
      // as failing. For instance, a bad TXT record for service A will not
      // affect the endpoints for service B.
      ErrorOr<DnsSdInstanceEndpoint> endpoint =
          CreateEndpoint(srv_and_txt->name(), a, aaaa, srv.value(), txt.value(),
                         network_interface_);
      endpoints.push_back(std::move(endpoint));
    }
  }

  return endpoints;
}

// static
bool DnsDataGraphImpl::IsValidAddressNode(Node* node) {
  const absl::optional<ARecordRdata> a =
      node->GetRdata<ARecordRdata>(DnsType::kA);
  const absl::optional<AAAARecordRdata> aaaa =
      node->GetRdata<AAAARecordRdata>(DnsType::kAAAA);
  return a.has_value() || aaaa.has_value();
}

// static
bool DnsDataGraphImpl::IsValidSrvAndTxtNode(Node* node) {
  const absl::optional<SrvRecordRdata> srv =
      node->GetRdata<SrvRecordRdata>(DnsType::kSRV);
  const absl::optional<TxtRecordRdata> txt =
      node->GetRdata<TxtRecordRdata>(DnsType::kTXT);

  return srv.has_value() && txt.has_value();
}

std::vector<ErrorOr<DnsSdInstanceEndpoint>>
DnsDataGraphImpl::CalculatePtrRecordEndpoints(Node* node) const {
  // PTR records aren't actually part of the generated endpoint objects, so
  // call this method recursively on all children and
  std::vector<ErrorOr<DnsSdInstanceEndpoint>> endpoints;
  for (const MdnsRecord& record : node->records()) {
    if (record.dns_type() != DnsType::kPTR) {
      continue;
    }

    const DomainName domain =
        absl::get<PtrRecordRdata>(record.rdata()).ptr_domain();
    const Node* child = nodes_.find(domain)->second.get();
    std::vector<ErrorOr<DnsSdInstanceEndpoint>> child_endpoints =
        CreateEndpoints(DomainGroup::kSrvAndTxt, child->name());
    for (auto& endpoint_or_error : child_endpoints) {
      endpoints.push_back(std::move(endpoint_or_error));
    }
  }
  return endpoints;
}

}  // namespace

DnsDataGraph::~DnsDataGraph() = default;

// static
std::unique_ptr<DnsDataGraph> DnsDataGraph::Create(
    NetworkInterfaceIndex network_interface) {
  return std::make_unique<DnsDataGraphImpl>(network_interface);
}

// static
DnsDataGraphImpl::DomainGroup DnsDataGraph::GetDomainGroup(DnsType type) {
  switch (type) {
    case DnsType::kA:
    case DnsType::kAAAA:
      return DnsDataGraphImpl::DomainGroup::kAddress;
    case DnsType::kSRV:
    case DnsType::kTXT:
      return DnsDataGraphImpl::DomainGroup::kSrvAndTxt;
    case DnsType::kPTR:
      return DnsDataGraphImpl::DomainGroup::kPtr;
    default:
      OSP_NOTREACHED();
  }
}

// static
DnsDataGraphImpl::DomainGroup DnsDataGraph::GetDomainGroup(
    const MdnsRecord record) {
  return GetDomainGroup(record.dns_type());
}

}  // namespace discovery
}  // namespace openscreen
