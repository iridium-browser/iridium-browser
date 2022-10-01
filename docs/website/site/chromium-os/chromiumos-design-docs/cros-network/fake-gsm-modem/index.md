---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/chromiumos-design-docs
  - Design Documents
- - /chromium-os/chromiumos-design-docs/cros-network
  - 'CrOS network: notes on ChromiumOS networking'
page_name: fake-gsm-modem
title: fake-gsm-modem
---

*   Implemented in
            [fake-gsm-modem](http://code.google.com/searchframe#wZuuyuB8jKQ/src/third_party/flimflam/test/fake-gsm-modem).
*   Seems to serve as a replacement for modemmanager (classic). I.e., I
            think it provides a mock implementation of the MM D-Bus APIs. In
            contrast,
            [fakemodem](/chromium-os/chromiumos-design-docs/cros-network/fakemodem)
            provides a mock implementation of the AT command interface.
*   For data forwarding, you have to use an existing ethernet interface.
            (See comments at top of
            [fake-gsm-modem](http://code.google.com/searchframe#wZuuyuB8jKQ/src/third_party/flimflam/test/fake-gsm-modem).)
*   To test shill (as opposed to flimflam) against fake-gsm-modem,
            follow these steps:

```none
$ stop flimflam
$ stop cromo
$ /usr/local/lib/flimflam/test/veth setup pseudomodem0 172.16.1
$ /usr/local/lib/flimflam/test/fake-gsm-modem -c tmobile --shill
$ start flimflam
```

T-Mobile should now appear as a connectable network in the 'Internet Connection'
Settings UI. Furthermore, running 'modem status' in crosh should display
information on TestModem.

Once done with the tests, run the following for cleanup:

```none
$ stop flimflam
$ # kill the fake-gsm-modem process
$ /usr/local/lib/flimflam/test/veth teardown pseudomodem0
$ start cromo
$ start flimflam
```

shill will now interact with cromo. Bear in mind that, as of the writing of this
entry, the shill daemon is still referred to as flimflam, which is why we call
`start flimflam` and not `start shill`. This will be changed in the future.

The above steps apply to Chromebooks using a Gobi modem. For any other modem,
replace `cromo` with the correct modem manager.
