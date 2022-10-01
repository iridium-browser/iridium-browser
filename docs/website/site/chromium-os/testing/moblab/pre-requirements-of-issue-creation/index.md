---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/testing
  - Testing Home
- - /chromium-os/testing/moblab
  - MobLab
page_name: pre-requirements-of-issue-creation
title: Pre-requirements of issue creation
---

Before filing an issue, please confirm you’ve completed actions below:

    Your moblab is using recommended equipments. (
    https://www.chromium.org/chromium-os/testing/moblab/overview-of-moblab?pli=1
    )

    Update Moblab to latest version and powerwashed.

    DUTs are loaded with test images.

    Moblab and DUTs are in dev mode (able to enter VT2 by Ctrl-Alt-F2).

    (Optional if Servo board is involved in test) DUT can reboot after
    "dut-control warm_reset:on".

Please have all screenshots ready and rename to descriptions in bold.

Screenshot of **Moblab admin** (
\[YOUR_MOBLAB_IP\]/moblab_setup/#tab_id=config_Wizard )

Screenshot of **Host List** (if DUT state is concerned.
\[YOUR_MOBLAB_IP\]/moblab_setup/#tab_id=dut_manage)

Screenshot of **MobMonitor**

Download Moblab logs via Mob\*Monitor page by clicking "Collect Log"

Now you're ready to file issue.
