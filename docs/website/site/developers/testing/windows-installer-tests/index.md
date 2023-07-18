---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/testing
  - Testing and infrastructure
page_name: windows-installer-tests
title: Windows Installer Tests
---

## About

The mini_installer_tests build step is a full lifecycle test of Chrome's Windows
installer. The test does variations along the following theme:

*   Ensure the machine is in a sufficiently clean state such that Chrome
            can be installed.
*   Install Chrome.
*   Verify various state on the machine to validate that the installer
            succeeded.
*   Launch Chrome.
*   Verify that something resembling a browser window opens.
*   Quit Chrome.
*   Verify that all Chrome processes go away.
*   Uninstall Chrome.
*   Verify that various state on the machine is no longer present.

## Diagnosing Failures

The task output should show detailed installer and Chrome logs in case of
failure.

## Running the Tests Locally

First build the mini_installer_tests target. The simplest way to run the test
suite (from an elevated cmd prompt) is to first cd into your build output
directory and then run the tests like so:

vpython3 ..\\..\\chrome\\test\\mini_installer\\run_mini_installer_tests.py
\[test_name\]

Specify individual test names as, for example:
installer_test.InstallerTest.test_ChromeUserLevel.

Run it with --help for more options. Note that you should use the vpython that
comes with [depot_tools](/developers/how-tos/install-depot-tools) as other
versions might not work.

## Questions

Contact grt at chromium dot org.
