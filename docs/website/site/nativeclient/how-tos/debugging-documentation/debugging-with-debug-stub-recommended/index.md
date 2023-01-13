---
breadcrumbs:
- - /nativeclient
  - Native Client
- - /nativeclient/how-tos
  - '2: How Tos'
- - /nativeclient/how-tos/debugging-documentation
  - Debugging Documentation
page_name: debugging-with-debug-stub-recommended
title: Debugging with debug stub (recommended)
---

Debug stub is a new way to debug NaCl applications. If --enable-nacl-debug and
--no-sandbox switches are passed to chrome or -g switch is passed to sel_ldr, a
local TCP port is opened that accepts incoming TCP connections from a debugger
via RSP protocol. This port is opened on localhost network interface, so one
need to forward this port to debug NaCl application from remote computer. This
method of debugging is preferred since debug stub inside NaCl process have an
intimate knowledge about NaCl application.
