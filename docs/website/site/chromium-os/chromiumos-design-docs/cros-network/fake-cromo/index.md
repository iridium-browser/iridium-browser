---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/chromiumos-design-docs
  - Design Documents
- - /chromium-os/chromiumos-design-docs/cros-network
  - 'CrOS network: notes on ChromiumOS networking'
page_name: fake-cromo
title: fake-cromo
---

*   Implemented in
            [fake-cromo](http://code.google.com/searchframe#wZuuyuB8jKQ/src/third_party/flimflam/test/fake-cromo).
*   Seems to provide a mock implementation of modem-manager (classic)
            APIs, focusing on CDMA.
*   Works with
            [activation-server](http://code.google.com/searchframe#wZuuyuB8jKQ/src/third_party/flimflam/test/activation-server),
            which provides emulation of carrier activation sequence.

Usage:

Start the web server that emulates the carrier activation portal.

# /usr/local/lib/flimflam/test/activation-server &

Rename eth0 to the name of the cellular device, as reported to shill

by the fake modemmanager. (The fake modemmanager is implemented in

[the Modem class in
flimflam_test](http://code.google.com/searchframe#wZuuyuB8jKQ/src/third_party/flimflam/test/flimflam_test.py&l=106).)

This means that we'll route "cellular" data traffic out our Ethernet

connection. Note, however, that fake-cromo let's us implement a

"captive portal" like mode. (That's implemented using iptables.)

Note further that the name of the cellular device must be pseudo-modem0,

as that's [hard-coded in
fake-cromo](http://code.google.com/searchframe#wZuuyuB8jKQ/src/third_party/flimflam/test/fake-cromo&l=61)
(and also used in a [default
argument](http://code.google.com/searchframe#wZuuyuB8jKQ/src/third_party/flimflam/test/flimflam_test.py&l=128)

in the Modem class.)

# /usr/local/lib/flimflam/test/backchannel setup eth0 pseudo-modem0

Start fake-cromo

# /usr/local/lib/flimflam/test/fake-cromo &
