---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/chromiumos-design-docs
  - Design Documents
- - /chromium-os/chromiumos-design-docs/cros-network
  - 'CrOS network: notes on ChromiumOS networking'
page_name: chrome-network-debugging
title: chrome network debugging
---

chrome has several debugging tools:

*   netlog (chrome://net-internals -&gt; events). see [NetLog: Chrome’s
            network logging
            system](/developers/design-documents/network-stack/netlog), or [the
            list of netlog events and their
            meanings](http://src.chromium.org/viewvc/chrome/trunk/src/net/base/net_log_event_type_list.h)
*   chrome://net-internals/#chromeos -&gt; controls to enable chromeos
            network debugging and to package up log files
*   chrome://net-internals/#tests -&gt; test loading a particular URL
*   network debugger app (in development; contact ebeach@)
