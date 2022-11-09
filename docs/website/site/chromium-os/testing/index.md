---
breadcrumbs:
- - /chromium-os
  - Chromium OS
page_name: testing
title: Testing Home
---

<div class="two-column-container">
<div class="column">

## This is the home page for testing. You should find everything related to writing tests, modifying existing tests, and running tests here. Please feel free to send an email to [chromium-os-dev@chromium.org ](mailto:chromium-os-dev@chromium.org)if you have any questions.

### Unit Tests

We run unit tests using the Portage test FEATURE. For instructions on how to run
and add your own unit tests, follow these
[instructions](https://chromium.googlesource.com/chromiumos/docs/+/HEAD/testing/running_unit_tests.md).

### Testing Workflows

Here's a description of some common [developer
workflows](/chromium-os/testing/common-testing-workflows)

When iterating on a test, you might save some time by using the
[autotest_quickmerge](/system/errors/NodeNotFound) tool.

### **Testing Components**

These documents discuss test cases and (eventually) test suites.

*   [Power testing](/chromium-os/testing/power-testing)
*   [Touch Firmware Tests](/for-testers/touch-firmware-tests)
*   [FAFT](/for-testers/faft) (Fully Automated Firmware Tests)

## Autotest

[Autotest User Doc](/chromium-os/testing/autotest-user-doc) describes the
integration of autotest into the build system, along with FAQ and specific
descriptions of various tasks.

This has additional info on how to [run the smoke test suite on a Virtual
Machine](/chromium-os/testing/running-smoke-suite-on-a-vm-image).

Before you check in a change, you should also do [a trybot run
locally](/chromium-os/build/local-trybot-documentation), which will run unit
tests and smoke tests in the same way that the builders do.

*   [AFE RPC
            Infrastructure](/chromium-os/testing/afe-rpc-infrastructure)
*   [Autoserv Packaging](/chromium-os/testing/autoserv-packaging)
*   [Collecting Stats for
            Graphite](/chromium-os/testing/collecting-stats-for-graphite)
*   [GS Offloader](/chromium-os/testing/gs-offloader)
*   [Autotest Keyvals](/chromium-os/testing/autotest-keyvals)
*   [Performance Tests and Dashboard](/chromium-os/testing/perf-data)

</div>
<div class="column">

### Codelabs

*   [Server Side tests
            codelab](/chromium-os/testing/test-code-labs/server-side-test)
*   [Client side
            codelab](/chromium-os/testing/test-code-labs/autotest-client-tests)
*   [Dynamic suite
            codelab](/chromium-os/testing/test-code-labs/dynamic-suite-codelab)

### Writing Tests & Suites

*   [Autotest Best
            Practices](https://chromium.googlesource.com/chromiumos/third_party/autotest/+/HEAD/docs/best-practices.md)
            - Start here when writing any code for Autotest.
*   [Autotest Developer
            FAQ](/chromium-os/testing/autotest-developer-faq) - Working on test
            cases within Autotest.
*   [Autotest Design
            Patterns](/chromium-os/testing/autotest-design-patterns) - Recipes
            for completing specific tasks in Autotest
*   [Existing Autotest
            Utilities](/chromium-os/testing/existing-autotest-utilities) -
            Coming Soon!
*   [Using Test Suites](/chromium-os/testing/test-suites) - How to write
            and modify Test Suites

### MobLab

MobLab is a self-contained automated test environment running on a Chromebox.

*   ==[MobLab Home](/chromium-os/testing/moblab)==

### Related Links

*   [Slides](https://docs.google.com/present/edit?id=0AXd3jph7Jzc6ZGQ4NjI1el8waGszOXZwZmI)
            for testing in Chromium OS
*   [Autotest API
            docs](https://github.com/autotest/autotest/wiki/TestDeveloper)

### Design Docs

*   [Dynamic Test Suite
            Implementation](/chromium-os/testing/dynamic-test-suites)
*   [Test Dependencies in Dynamic
            Suites](/chromium-os/testing/test-dependencies-in-dynamic-suites)
*   [Suite Scheduler AKA Test Scheduler
            V2](/chromium-os/testing/suite_scheduler-1)

## Servo

[Servo](https://chromium.googlesource.com/chromiumos/third_party/hdctools/+/HEAD/docs/servo.md) is a debug board for Chromium OS test and development. It can connect to
most Chrome devices through a debug header on the mainboard.

## Chamelium

*   [Chamelium](/chromium-os/testing/chamelium) automates external
            display testing across VGA, HDMI, and DisplayPort (DP).
*   [Chamelium with audio
            board](/chromium-os/testing/chamelium-audio-board) automate audio
            testing across 3.5mm headphone/mic, internal speaker, internal
            microphone, HDMI, Bluetooth A2DP/HSP, USB audio.
*   [Chamelium capturing and streaming
            tool](/chromium-os/testing/chamelium-audio-streaming) to monitor
            audio underruns.
*   [Chamelium USB audio](/chromium-os/testing/chamelium-usb-audio) to
            setup external USB audio devices

</div>
</div>
