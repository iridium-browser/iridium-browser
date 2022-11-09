---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/how-tos-and-troubleshooting
  - How Tos and Troubleshooting
page_name: force-out-of-box-experience-oobe
title: How to force the out-of-box experience (OOBE)
---

You can force your device to redo the out-of-box experience (OOBE) as follows:

*   Boot to login screen
*   Switch to VT2 (press ctrl-alt-F2 -- only possible in Dev mode)
*   Login as root (or chronos and use sudo)
*   stop ui
*   rm -rf /home/chronos/Local\\ State
*   rm -rf /home/chronos/.oobe_completed
*   reboot
