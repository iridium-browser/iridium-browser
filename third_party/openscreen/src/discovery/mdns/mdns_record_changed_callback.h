// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DISCOVERY_MDNS_MDNS_RECORD_CHANGED_CALLBACK_H_
#define DISCOVERY_MDNS_MDNS_RECORD_CHANGED_CALLBACK_H_

#include <vector>

#include "discovery/mdns/mdns_records.h"
#include "util/osp_logging.h"

namespace openscreen {
namespace discovery {

enum class RecordChangedEvent {
  kCreated,
  kUpdated,
  kExpired,
};

class MdnsRecordChangedCallback;

struct PendingQueryChange {
  enum ChangeType { kStartQuery, kStopQuery };
  DomainName name;
  DnsType dns_type;
  DnsClass dns_class;
  MdnsRecordChangedCallback* callback;
  ChangeType change_type;
};

class MdnsRecordChangedCallback {
 public:
  virtual ~MdnsRecordChangedCallback() = default;

  // Called when |record| has been changed.
  // NOTE: This callback may not modify the instance from which it is called.
  // The return value of this function must be the set of all record changes to
  // be made once the operation completes.
  virtual std::vector<PendingQueryChange> OnRecordChanged(
      const MdnsRecord& record,
      RecordChangedEvent event) = 0;
};

inline std::ostream& operator<<(std::ostream& output,
                                RecordChangedEvent event) {
  switch (event) {
    case RecordChangedEvent::kCreated:
      return output << "Create";
    case RecordChangedEvent::kUpdated:
      return output << "Update";
    case RecordChangedEvent::kExpired:
      return output << "Expiry";
  }
  OSP_NOTREACHED();
}

}  // namespace discovery
}  // namespace openscreen

#endif  // DISCOVERY_MDNS_MDNS_RECORD_CHANGED_CALLBACK_H_
