---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/how-tos-and-troubleshooting
  - How Tos and Troubleshooting
- - /chromium-os/how-tos-and-troubleshooting/debugging-3g
  - Debugging a cellular modem
page_name: modem-debugging-with-mmcli
title: Modem debugging with mmcli (from the modemmanager-next package)
---

Listing modems/finding the index number of a modem (The modem index frequently
changes during a suspend-resume cycle):

mmcli -L

(Further examples will assume modem index 0. Substitute your own modem index
where necessary.)

See the status of a modem:

mmcli -m 0

Enable a modem (useful side effects usually include registering on a network and
getting an operator ID and name)

mmcli -m 0 -e

Minimal connection and disconnection:

mmcli -m 0 --simple-connect="apn=foo.carrier.com"

mmcli -m 0 --simple-disconnect

Set logging level to maximium (equivalent to the old mm_debug debug):

mmcli -G DEBUG

Set logging level to minimum (equivalent to the old mm_debug err):

mmcli -G ERR

Arbitrary AT commands - available if ModemManager is started with the --debug
flag:

This is a good test command to see if ModemManager is speaking to the modem. The
modem should already be in state E0 (no command echo); you should immediately
get an empty reply. You can check /var/log/messages for the message exchange if
you've previously turned up the logging level to maximum.

mmcli -m 0 --command="E0"

Do a network scan and list the carriers found - this requires a
longer-than-default timeout.

mmcli -m 0 --command-timeout=120 --command="+COPS?"
