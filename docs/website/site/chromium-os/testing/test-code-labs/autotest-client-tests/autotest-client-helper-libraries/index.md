---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/testing
  - Testing Home
- - /chromium-os/testing/test-code-labs
  - Test Code Labs
- - /chromium-os/testing/test-code-labs/autotest-client-tests
  - Autotest Client Tests
page_name: autotest-client-helper-libraries
title: Autotest Client Helper Libraries
---

This is a listing and brief explanation of the various helper methods available
through autotest_lib.client.\*. It is in no way comprehensive, and is only meant
as an introduction to the topic:

cros

|-audio
|-camera
|-cellular
|---pseudomodem
|-dhcp_test_data
|-i2c
|-rf
|-saft

auth_server     - auth

dns_server      - auth
httpd           - auth
constants       - constants
crash_test      - crash programs, get dumps
cros_logging    - logging
cros_ui         - ui, session manager, login
cryptohome      - cryptohome vaults

login           - wait_for_(browser, window manager, ownership)
network         - Modem, IP
pkcs11          - chapsd, TPM
power_\*                - power
storage         - storage devices
sys_power       - wakeup, suspend

common_lib

|-cros

autoupdater

devserver
|-hosts (Autotest Host base classes)
|-perf_expectations
|-test_utils

autotemp                - autotest temp dir creation helpers

base_utils              - ‘run’ and other system level helpers

error            - different error classes

pexpect

utils

bin

|-input
|-net
|-self-test
|---tests

base_utils      - linux sysadmin helper: grep, disk, cpu, environ

site_utils      - systems level helper: ping hosts, board type.
utils           - base+site utils

test            - main test class

unit_test       - main unit_test class

If you have come this far, you may also be interested in reading the autotest
client tests
[codelab](/chromium-os/testing/test-code-labs/autotest-client-tests).
