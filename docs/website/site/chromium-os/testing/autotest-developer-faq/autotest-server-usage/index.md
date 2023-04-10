---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/testing
  - Testing Home
- - /chromium-os/testing/autotest-developer-faq
  - Autotest Developer FAQ
page_name: autotest-server-usage
title: Autotest Server Usage Guide
---

Note these steps are to be ran after you have setup Autotest:
<https://www.chromium.org/chromium-os/testing/autotest-developer-faq/setup-autotest-server>
and have the local Autotest Web Frontend running. To verify simply navigate to
<http://localhost>

## Add Hosts

Add hosts. *Note you need to ensure that you have SSH Access Setup Properly:
<https://chromium.googlesource.com/chromiumos/docs/+/HEAD/developer_guide.md#Set-up-SSH-connection-between-chroot-and-DUT>*

```none
/usr/local/autotest/cli/atest host create hostname
```

## Start the Autotest Test Scheduler

Start the scheduler.

\*You may want to run the scheduler in ***screen***

```none
/usr/local/autotest/scheduler/monitor_db.py /usr/local/autotest/results
```
