---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/testing
  - Testing Home
- - /chromium-os/testing/test-code-labs
  - Test Code Labs
page_name: server-side-test
title: Server Side Test for Chromium OS Autotest Codelab
---

[TOC]

****## References****

    ****[Autotest Best
    Practices](https://chromium.googlesource.com/chromiumos/third_party/autotest/+/HEAD/docs/best-practices.md)****

    ****[Writing
    Autotests](http://www.chromium.org/chromium-os/testing/autotest-developer-faq#TOC-Writing-Autotests)****

    ****[Codelab for Writing an Autotest
    Test](https://wiki.corp.google.com/twiki/bin/view/Codelab/WritingAutotestTests)
    - This is a codelab teaching you how to write a generic test for
    Autotest.****

    ****[Autotest for Chromium OS
    developers](http://www.chromium.org/chromium-os/testing/autotest-user-doc)****

    ****[Chromium OS Developer
    Guide](http://www.chromium.org/chromium-os/developer-guide)****

****## Overview****

****In this codelab, you will build a server-side Autotest to test if the backlight brightness is restored to the original value on a device after the following operations:****

        ****user is logged out and logged back in****

    ****device is suspended and resumed****

****## Prerequisite****

****This Codelab is for Chromium OS developers and testers to learn how to write a server side Autotest. Assumptions:****

    ****You know the Python programming language.****

    ****You have a device under test (DUT) with a test image installed. See
    [Chromium OS Developer
    Guide](http://www.chromium.org/chromium-os/developer-guide#TOC-Installing-Chromium-OS-on-your-Device)
    for details.****

    ****You have Chromium OS development environment setup, see [Chromium OS
    Developer
    Guide](http://www.chromium.org/chromium-os/developer-guide#TOC-Get-the-Source)
    for details.****

****Objectives****

****By the end of this lab, you will know how to:****

    ****Write Chromium OS server side Autotest in Python****

    ****Manually run the server side Autotest against a DUT****

    ****Automatically run test as part of a suite****

****## Difference between client and server test****

****Autotest has a notion of both client-side tests and server-side tests. All tests are managed by an Autotest server machine that is typically the machine where [test_that](http://go/cros-test_that) is invoked. test_that (can be found in folder scripts) is a script to run autoserv process which deploys and executes tests on remote machines.****

****Code in a client-side test runs only on the device under test (DUT), and as such isn’t capable of maintaining state across reboots, handling a failed suspend/resume, or the like. If possible, an Autotest should be written as a client-side test, as it’s simpler and more reusable. Code lab for client side test can be found [here](http://www.chromium.org/chromium-os/testing/test-code-labs/autotest-client-tests).****

****A ‘server’ test runs on the Autotest server, and gets assigned a DUT, just like a client-side test. It can use various Autotest primitives (and library code written by the CrOS team) to manipulate that device. Most, if not all, tests that use [Servo](/chromium-os/servo) or remote power management should be written as server-side tests. As a good example, if the DUT needs to be rebooted during the test, it should be a server-side test.****

****Tests are located in 4 locations in the chromeos_public [third_party/autotest/files/](https://cs.corp.google.com/#chromeos_public/src/third_party/autotest/files/) tree:****

    ****client/site_tests - These are where most tests live. These are client
    tests that are Chromium OS specific.****

    ****client/tests - These are client tests that come from upstream Autotest,
    and most are general Linux tests (not Chromium OS specific).****

    ****server/site_tests - These are server tests that are Chromium OS
    specific.****

    ****server/tests - These are server tests that are general Linux tests (not
    Chromium OS specific).****

****Decide if your test is a client or server test and choose the appropriate directory from the above.****

****## Create a new server side test****

****[Adding a test](https://github.com/autotest/autotest/wiki/AddingTest) involves putting a control file and a properly-written test wrapper in the right place in the source tree. There are [conventions](http://www.chromium.org/chromium-os/testing/autotest-best-practices#TOC-Writing-tests) that must be followed, and a[ variety of primitives](https://github.com/autotest/autotest/wiki/AutotestApi) available for use. When writing any code, whether client-side test, server-side test, or library, have a strong bias towards using[ Autotest utility code](http://www.chromium.org/chromium-os/testing/existing-autotest-utilities). This keeps the codebase consistent and avoids duplicated effort.****

****In the subsections below, we will discuss the detail of following topics:****

    ****Syntax and style guide.****

    ****Code for a barebone server side test.****

    ****How to manually run a server side test on DUT.****

    ****Code to implement the test logic for this exercise.****

****### Syntax and style guide****

****Before we start writing any code, it’s a good idea to understand and follow the Python coding style enforced in Chromium OS project. For Autotest code in Chromium OS project, the Python coding style must follow the coding style specified at the top level of the Autotest repo. This style adds a few additions to [PEP-8](http://www.python.org/dev/peps/pep-0008/) style guide, e.g., formatting on comments and Docstrings. You may also consider to follow [Python Style Guidelines in Chromium Projects](http://www.chromium.org/chromium-os/python-style-guidelines), but coding style defined in Autotest repo and [PEP-8](http://www.python.org/dev/peps/pep-0008/) style guide must take precedence.****

****Some basic rules include:****

    ****Use 4 spaces instead of tab.****

    ****Use the % operator for formatting strings.****

    ****Read [this](http://www.python.org/dev/peps/pep-0008/#indentation) about
    alignment for function calls with multiple line arguments.****

    ****lower_case_with_underscores for local variable, function and method
    names.****

    ****CapitalizedWords for class names****

    ****__double_leading_underscore for private class attribute name.****

    ****UPPER_CASE_WITH_UNDERSCORES for static public class attribute name.****

    ****Separate top-level function and class definitions with two blank
    lines.****

    ****No extra space at the end of a line. Limit all lines to a maximum of 80
    characters.****

    ****Use single quote for strings if possible.****

    ****End comment with period.****

****[Here](http://www.chromium.org/developers/coding-style) is another resources talking about coding style, including languages other than Python. Keep in mind that the best practice is to keep the test file in pure Python i.e. one should avoid “shelling out” and generally use Python instead of tools like awk and grep.****

****### Step 1. Name your test****

****Create a folder in “src/third_party/autotest/files/server/site_tests/”. Name the folder with the test name, which is “power_MyBacklightTest”.****

****To name the test properly, first you need to decide the area wherein your test falls. It should be something like “power”, "platform", or "network" for instance. Take a look at the folder names in client/site_tests and server/site_tests, you can find some sample test area, and choose one area that’s proper for your test. This name is used to create the test name: e.g. "network_UnplugCable". Create a directory for your test at $(TEST_ROOT)/$(LOWERCASE_AREA)_$(TEST_NAME).****

****Try to find an example test that does something similar and copy it. You will create at least 2 files:****

****${TEST_ROOT}/${LOWERCASE_AREA}_${TEST_NAME}/control****

****${TEST_ROOT}/${LOWERCASE_AREA}_${TEST_NAME}/${LOWERCASE_AREA}_${TEST_NAME}.py****

****Your control file runs the test and sets up default parameters. The .py file is the test wrapper, and has the actual implementation of the test. You can create multiple control files for the same test that can take different arguments. As an example, power_SuspendStress test has multiple control files that serve multiple purpose, while sharing the same test logic with different configuration.****

****Inside the control file, the TEST_CLASS variable should be set to ${LOWERCASE_AREA}. The naming convention simply exists to make it easier to find other similar tests and measure the coverage in different areas of Chromium OS.****

****### Step 2. Create a barebone server side test****

****In this section we will discuss what’s the minimum code needed to create an empty server side test that provides all necessary files and overrides, but with no test logic in it.****

****#### Step 2.1. Create a Control file****

****A control file contains important metadata about the test (name, author, description, duration, what suite it’s in, etc) and then pulls in and executes the actual test code. Detailed description about each variable can be found in the [Autotest Best Practices](https://chromium.googlesource.com/chromiumos/third_party/autotest/+/HEAD/docs/best-practices.md). You can also find more information about control files [here](https://github.com/autotest/autotest/wiki/AddingTest).****

****In the folder power_MyBacklightTest you just created, create a control file named “control”, with the following content, set NAME to the test name of “power_MyBacklightTest”. Note that NAME must be exactly the same as the folder name. Set AUTHOR to your name or email address (\[user_name\]@chromium.org).****

****------------------------------****

****# Copyright information****
****AUTHOR = "your name, and at least one person (\[ldap\]@chromium.org) or a mailing list"****
****NAME = "power_MyBacklightTest" # test name****
****TIME = "SHORT" # can be MEDIUM, LONG****
****TEST_CATEGORY = "Benchmark" # can be General, Stress****
****TEST_CLASS = "power" # testing area, e.g., platform, network etc****
****TEST_TYPE = "server" *# indicating it's a server side test*****

SUITE = "bvt" *# if the test should be a part of the BVT*
# EXPERIMENTAL = "True" # Experiment is now not in use
DOC = """
This test measures the backlight level before and after certain system events
and verifies that the backlight level has not changed.
"""
def run_system_power_MyBacklightTest(machine):
job.run_test('power_MyBacklightTest', client_ip=machine)
# run the test in multiple machines
job.parallel_simple(run_system_power_MyBacklightTest, machines)

-----------------------------------------------------

This control file is used for Autotest server to run the test
“power_MyBacklightTest” in each test device. The test is defined in the test
wrapper file, which is explained in the next section.

Some important attributes are listed here:

    TIME: time duration to run the test, can be FAST (&lt;1 min), SHORT, MEDIUM,
    LONG or LENGTHY (&gt;30 min).

    TEST_CATEGORY: describes the category for your tests e.g. Stress, Functional

    TEST_TYPE: Client or Server, indicating it’s a client side or server side
    test.

Some optional attributes:

    SUITE: A comma-delimited string of suite names that this test should be a
    part of, e.g. bvt, regression, smoke

    [DEPENDENCIES](http://www.chromium.org/chromium-os/testing/test-dependencies-in-dynamic-suites):
    list, of, tags known to the HW test lab, e.g., webcam, blue-tooth

    (absolete) EXPERIMENTAL: if the test is experimental, value can be True or
    False. For experimental tests, failure is considered to be non-fatal. For
    the test you uploaded, EXPERIMENTAL should be set to True until the test
    passes reliably on all intended platforms in the lab.

#### Step 2.2. Create a test wrapper

The test wrapper file contains code for the test logic. Following is the source
code for a test wrapper for a barebone server side test that contains all the
necessary method overrides, but without any test logic. Basically the test
wrapper contains a class derived from base class “test” (server/test.py).
Autotest server will pick up this class and execute method “run_once” to do the
actual test work, and call method cleanup after all tests in this test wrapper
is done.

Note that the name of folder containing the control file, the NAME value in
control file, and the test wrapper must match the class name. To create the test
wrapper, in the folder power_MyBacklightTest, create test file named
“power_MyBacklightTest.py”.

-----------------------------------------------------

# Copyright information

from autotest_lib.server import test
class power_MyBacklightTest(test.test):

"""Test to check backlight brightness restored after various actions."""
version = 1

def initialize(self):

"""Initialize for this particular test."""
pass

def setup(self):
pass

def run_once(self, client_ip):

"""Where the actual test logic lives."""

pass
def cleanup(self):

"""Called at the end of the test."""
pass

-----------------------------------------------------

The test wrapper’s base class is set to test.test (server/test.py). Autotest
server will pick up this class and call following methods in order:

    **initialize**, where you can do initialization for this particular test.

    **run_once**, where actual test logic lives.

    **cleanup**, called at the end of the test.

Note that setup is not called during testing. Following are some details about
each method and attribute version.

*version*

You must define the class variable version.

*initialize*

The method initialize runs once for each job. The method is equivalent to
_init_. It is called before actual test is run. However, do not use _init_ for
initialization, use initialize instead. _init_ is defined and used in base class
(base_test) to initialize the attributes for the base class. Overriding _init_
will break the test.

*Setup*

This method is the only one called when you
`cros build-packages --withautotest`, while other methods are called during
testing.  The method is triggered when version attribute's value is changed or
the test package is built the first time to produce expected binaries.  If the
test is scheduled to run in a DUT in which the test has already been installed,
Autotest will check the version attribute's value of the test class.  If the
value is changed, the test will be reinstalled.  One sample usage of this method
can be found
[here](http://www.chromium.org/chromium-os/testing/autotest-developer-faq#TOC-Adding-binaries-for-your-tests-to-call-as-part-of-the-test).

*run_once*

You then must provide an override for the run_once() method in your class. The
method contains the actual test logic. The run_once method can accept any
arguments you desire. These are passed in as the \*dargs in the job.run_test()
method in the control file. As the test logic can be complicated, instead of a
lengthy run_once() method, it’s highly recommended to use member methods to
better organize the test code for easier review. At the end of this method, you
validate the test results in this method, and log any failure.

*Cleanup*

cleanup is the method where you do the custodial work, e.g., restore the
system’s status to its original state if you have cached that, in this test,
it’s the backlight brightness.

### Step 3. Running the server-side test

#### Run Autotest manually

After the control file and test wrapper file are created, you can try to
manually run your Autotest against DUT, which has a test image installed (follow
[this
instruction](http://www.chromium.org/chromium-os/developer-guide#TOC-Build-a-disk-image-for-your-board)
to build a test image using the “test” argument).

To get started, first you need to create or enter a chroot in your host machine.
Details can be found
[here](http://www.chromium.org/chromium-os/developer-guide#TOC-Create-a-chroot).
Go to chromiumos directory, run following command to create or enter a chroot.

./chromite/bin/cros_sdk

Then go to folder src/scripts, run following command to setup the board, that
is, the class of target device.

./setup_board --board=${BOARD}

Go to folder src/third_party/autotest/files, run following command to start
working on the board.

cros_workon --board=${BOARD} start .

Once you complete above steps, you can now try to run the server side test. Go
to src/scripts directory, run the following command to manually run your
Autotest:

test_that ${ClientIP} ${LOWERCASE_AREA}_${TEST_NAME}

${LOWERCASE_AREA}_${TEST_NAME} is the NAME in control file. If you set NAME
attribute to a different value, you need to use that value in test_that command.

For example:

test_that 192.168.1.20 power_MyBacklightTest

You need to replace the DUT’s IP address with your own configuration.

After the command is executed, the test should finish in about 30 seconds. You
should expect output with detailed information of each step the test is
executed, for example:

    Install autotest if it’s not found in DUT.

    Start autotestd_monitor and collect system information before and after the
    test.

    Execute the test logic defined in test wrapper.

At the end of the output, you will find the test result, like following:

------------------------------------------------------------------------------------------------------------

/tmp/test_that_results_Bdfrwv/results-1-experimental_power_MyBacklightTest \[
PASSED \]

/tmp/test_that_results_Bdfrwv/results-1-experimental_power_MyBacklightTest/power_MyBacklightTest
\[ PASSED \]

------------------------------------------------------------------------------------------------------------

Total PASS: 2/2 (100%)

18:16:28 INFO | Finished running tests. Results can be found in
/tmp/test_that_results_Bdfrwv

In a real world example, the test may take much longer to finish to execute all
the test logic.

#### Add test to an existing test suite

If you want Autotest to run your test automatically, you must add it to one or
more existing test suites. For example, to have it run as part of the BVT (build
validation test), you would edit the "SUITE" attribute in the control file, and
set it equal to 'bvt'. Autotest then will pick up this test and run it as part
of the bvt suite.

The control file and test wrapper must be stored in a subfolder of
src/third_party/autotest/files/server/site_tests/ folder. Note that the folder
name must match the class name of the test code (the class is the one defined in
test wrapper and derived from base class “test”). See the code lab for [Creating
and deploying Chromium OS Dynamic Test
Suites](/chromium-os/testing/test-code-labs/dynamic-suite-codelab).

### Step 4. Implement test logic in test wrapper

In the previous section, we show a test wrapper for a barebone server side test
that contains all necessary method overrides without any actual code in it. In
this section, we will implement the test logic which is to test if the backlight
brightness goes back to its original setting after various scenarios. That is,
you need to write code to accomplish the following individual tasks:

    Get/set backlight’s brightness

    Log out and log in user

    Suspend the device and resume

Code will be added to the test wrapper we created in the last section, that is
“power_MyBacklightTest.py”, in the folder power_MyBacklightTest. Follow each
steps in this section, and copy the code to the test wrapper
“power_MyBacklightTest.py”. At the end you will have a complete test to test the
backlight brightness of a DUT.

While writing a server side test, it is important to keep in mind that the test
execution is initiated in the server. To execute a command running on the DUT,
you need to run the command through a host object (self._client showing in the
code). Different from to a server side test, client side test code is copied to
the DUT and executed within the DUT.

#### Step 4.1. Import modules useful for Chromium OS specific test

Add the imports at the beginning of test wrapper.

import logging, os, subprocess, tempfile, time

from autotest_lib.client.common_lib import
[error](https://gerrit.chromium.org/gerrit/gitweb?p=chromiumos/third_party/autotest.git;a=blob;f=client/common_lib/error.py;h=d9854db796a812b86089773b4cfb365a8ddf0359;hb=HEAD)
from autotest_lib.server import
[autotest](https://gerrit.chromium.org/gerrit/gitweb?p=chromiumos/third_party/autotest.git;a=blob;f=server/autotest.py;h=56b578b08da7d61762dad1bec71608db84ed81d4;hb=HEAD)
from autotest_lib.server import
[hosts](https://gerrit.chromium.org/gerrit/gitweb?p=chromiumos/third_party/autotest.git;a=tree;f=server/hosts;h=c493f142abfaacc5a9db28c64cbc3fbbe9d08f75;hb=HEAD)
from autotest_lib.server import
[test](https://gerrit.chromium.org/gerrit/gitweb?p=chromiumos/third_party/autotest.git;a=blob;f=server/test.py;h=8fdba9d7c40addf23e139b060f558e17bb34e40b;hb=HEAD)
from autotest_lib.server import
[utils](https://gerrit.chromium.org/gerrit/gitweb?p=chromiumos/third_party/autotest.git;a=blob;f=server/utils.py;h=72f72f938f37b7c7cac729de05df446b0bbbd253;hb=HEAD)

These modules include some useful base classes to be used to run commands in a
test device or control a servo device. More details about how
[import](http://www.chromium.org/chromium-os/testing/autotest-developer-faq#TOC-Writing-Autotests)
is used in Autotest can be found
[here](/chromium-os/testing/autotest-developer-faq#TOC-A-word-about-imports).
Note that each line only imports one module and modules are ordered
alphabetically.

Step 4.2. Helper methods

Add following methods to class power_MyBacklightTest. They are some helper
methods to execute the commands from the DUT, or used to check the DUT’s power
status.

def _client_cmd(self, cmd):
"""Execute a command on the client.
@param cmd: command to execute on client.
@return: The output object, with stdout and stderr fields.
"""
logging.info('Client cmd: \[%s\]', cmd)
return self._client.run(cmd)
def _client_cmd_and_wait_for_restart(self, cmd):
boot_id = self._client.get_boot_id()
self._client_cmd('sh -c "sync; sleep 1; %s" &gt;/dev/null 2&gt;&1 &' % cmd)
self._client.wait_for_restart(old_boot_id=boot_id)
def _check_power_status(self):
cmd_result = self._client_cmd('status powerd')
if 'running' not in cmd_result.stdout:
raise error.TestError('powerd must be running.')

****result = self._client_cmd('power_supply_info | grep online')****

if 'yes' not in result.stdout:
raise error.TestError('power must be plugged in.')

#### Step 4.3. Get/Set brightness

Add following two methods in class power_MyBacklightTest. In these two methods,
a command is passed by _client_cmd method to get current brightness from test
device, and _set_brightness to set the device.

def _get_brightness(self):
"""Get brightness in integer value. It's not a percentage value and
may be over 100"""
result = self._client_cmd('backlight_tool --get_brightness')
return int(result.stdout.rstrip())

def _set_brightness_percent(self, brightness=100):
result = self._client_cmd('backlight_tool --set_brightness_percent %d'

% brightness)

#### Step 4.4. Actions being taken during the test

Add following methods in class power_MyBacklightTest. In these methods, command
is called to apply actions to DUT, including, logout/login, and power
suspend/resume.

def _do_logout(self):

self._client_cmd('restart ui')

def _do_suspend(self):

# The method calls a client side test 'power_resume' to do a power

# suspend on the DUT. It is easy yet powerful for one test to run

# another test to implement certain action without duplicating code.

self._client_at.run_test('power_Resume')

#### Step 4.5. Disable ALS(Auto Light Sensor)

Auto light sensor might interfere the brightness, so it needs to be disabled
before test starts, and re-enabled after the test finishes. Add following two
methods in class power_MyBacklightTest.

def _set_als_disable(self):
"""Turns off ALS in power manager. Saves the old has_ambient_light_sensor
flag if it exists.
"""

# Basic use of shell code via ssh run command is acceptable, rather than

# shipping over a small script to perform the same task.
als_path = '/var/lib/power_manager/has_ambient_light_sensor'
self._client_cmd('if \[ -e %s \]; then mv %s %s_backup; fi' %
(als_path, als_path, als_path))
self._client_cmd('echo 0 &gt; %s' % als_path)
self._client_cmd('restart powerd')
self._als_disabled = True

def _restore_als_disable(self):
"""Restore the has_ambient_light_sensor flag setting that was overwritten in
_set_als_disable.
"""
if not self._als_disabled:
return
als_path = '/var/lib/power_manager/has_ambient_light_sensor'
self._client_cmd('rm %s' % als_path)
self._client_cmd('if \[ -e %s_backup \]; then mv %s_backup %s; fi' %
(als_path, als_path, als_path))
self._client_cmd('restart powerd')

#### Step 4.6. Run test and validate test results

This is the method where you run the actual test logic. At the end of this
method, you validate the test results, and log any failure. It overrides the
run_once method in base class test.test.

# The dictionary of test scenarios and its corresponding method to

# apply each action to the DUT.

_transitions = {
'logout': _do_logout,
'suspend': _do_suspend,
}

def run_once(self, client_ip):
"""Run the test.

****For each system transition event in |_transitions|:****

Read the brightness.
Trigger transition event.
Wait for client to come back up.
Check new brightness against previous brightness.
@param client_ip: string of client's ip address (required)
"""
if not client_ip:
error.TestError("Must provide client's IP address to test")

# Create a custom host class for this machine, which is used to execute

# commands and other functions.
self._client = hosts.create_host(client_ip)

# Create an Autotest instance which you can run method like run_test,

# which can execute another client side test to facilitate this test.
self._client_at = autotest.Autotest(self._client)

****self._results = {}****

****self._check_power_status()****

self._set_als_disable()
# Save the original brightness, to be restored after the test ends.
self._original_brightness = self._get_brightness()
# Set the brightness to a random number which is different from
# system default value.
self._set_brightness_percent(71)
# Run the transition event tests.
for test_name in self._transitions:
self._old_brightness = self._get_brightness()

****self._transitions\[test_name\](self)****

# Save the before and after backlight values.
self._results\[test_name\] = { 'old': self._old_brightness,
'new': self._get_brightness() }

# Check results to make sure backlight levels were preserved across
# transition events.
num_failed = 0
for test_name in self._results:
old_brightness = self._results\[test_name\]\['old'\]
new_brightness = self._results\[test_name\]\['new'\]
if old_brightness == new_brightness:
logging.info('Transition event \[ PASSED \]: %s', test_name)
else:
logging.info('Transition event \[ FAILED \]: %s', test_name)
logging.info(' Brightness changed: %d -&gt; %d',
old_brightness, new_brightness)
num_failed += 1
if num_failed &gt; 0:
raise error.TestFail(('Failed to preserve backlight over %d '
'transition event(s).') % num_failed)

#### Step 4.7. Cleanup

“cleanup” is the method where you do the custodial work, e.g., restore the
system’s status to its original state if you have cached that, in this test,
it’s the backlight brightness. It overrides the cleanup method in base class
test.test.

def cleanup(self):
"""Restore DUT's condition before the test starts, and check the test
results.
"""
self._restore_als_disable()
self._set_brightness_percent(self._original_brightness)

For more information on writing your test, see the [frequently asked
questions](http://www.chromium.org/chromium-os/testing/autotest-developer-faq).

## Verify Test by Running Autotest Manually

After you complete the last section, you should have all the code ready to run
the test. As discussed in earlier section, once you configure the chroot
properly, you can try to run the server side test manually with following
command in src/scripts folder:

test_that ${ClientIP} ${LOWERCASE_AREA}_${TEST_NAME}

For example:

test_that 192.168.1.20 power_MyBacklightTest

You need to replace the board name and DUT’s IP address with your own
configuration.

If the test is successful, the following should happen in your test device in
order:

    User log out

    User log in

    Screen brightness stays at 71%.

    Screen goes black (system suspend/resume).

    System comes up again.

    Screen brightness stays at 71%.

    In the clean up procedure, screen brightness is reset to its original value.

All log generated from the test run can be found in a tmp folder, e.g.,
/tmp/test_that_results_WZ_eVZ. The following are some files useful for
troubleshooting an issue:

    status and status.log, contains the result of each test’s start/end time and
    test result.

    keyval, some setup information, e.g., drone, DUT’s hostname

    debug/autoserv.DEBUG, contains all logs collected by autoserv during the
    test.

When the test is completed, following message will be shown at the end of the
output to indicate all tests are passed:

------------------------------------------------------------------------------------------------------------
/tmp/test_that_results_Bdfrwv/results-1-experimental_power_MyBacklightTest \[
PASSED \]
/tmp/test_that_results_Bdfrwv/results-1-experimental_power_MyBacklightTest/power_MyBacklightTest
\[ PASSED \]
------------------------------------------------------------------------------------------------------------
Total PASS: 2/2 (100%) 18:16:28 INFO | Finished running tests. Results can be
found in /tmp/test_that_results_Bdfrwv

To review the test result later, you can always run generate_test_report to
retrieve above test result.

## Possible failure scenarios

### Device failed to reboot

If the device failed to reboot during the test, the test will eventually timed
out and consider failed. You need to verify the DUT is still connected to
network and can be used to test. A quick check you can do is to ssh to the DUT
as root user.

---

Code of the test wrapper

Reference:
/src/third_party/autotest/files/server/site_tests/power_BacklightServer

<https://chromium.googlesource.com/chromiumos/third_party/autotest/+/HEAD/server/site_tests/power_BacklightServer>

---

**# Copyright 2018 The ChromiumOS Authors.**
**# Use of this source code is governed by a BSD-style license that can be**
**# found in the LICENSE file.**
**import logging, os, subprocess, tempfile, time**
**from autotest_lib.client.common_lib import error**
**from autotest_lib.server import autotest**
**from autotest_lib.server import hosts**
**from autotest_lib.server import test**
**from autotest_lib.server import utils**
**class power_MyBacklightTest(test.test):**

**"""Test to check backlight brightness restored after various actions."""**
**version = 1**
**def _client_cmd(self, cmd):**
**"""Execute a command on the client.**
**@param cmd: command to execute on client.**
**@return: The output object, with stdout and stderr fields.**
**"""**
**logging.info('Client cmd: \[%s\]', cmd)**
**return self._client.run(cmd)**
**def _client_cmd_and_wait_for_restart(self, cmd):**
**boot_id = self._client.get_boot_id()**
**self._client_cmd('sh -c "sync; sleep 1; %s" &gt;/dev/null 2&gt;&1 &' % cmd)**
**self._client.wait_for_restart(old_boot_id=boot_id)**
**def _get_brightness(self):**
**"""Get brightness in integer value. It's not a percentage value and**
**may be over 100"""**
**result = self._client_cmd('backlight_tool --get_brightness')**
**return int(result.stdout.rstrip())**
**def _set_brightness_percent(self, brightness=100):**
**result = self._client_cmd('backlight_tool --set_brightness_percent %d'**
**% brightness)**
**def _check_power_status(self):**
**cmd_result = self._client_cmd('status powerd')**
**if 'running' not in cmd_result.stdout:**
**raise error.TestError('powerd must be running.')**
**result = self._client_cmd('power_supply_info | grep online')**
**if 'yes' not in result.stdout:**
**raise error.TestError('power must be plugged in.')**
**def _do_logout(self):**
**self._client_cmd('restart ui')**
**def _do_suspend(self):**

**# The method calls a client side test 'power_resume' to do a power**

**# suspend on the DUT. It is easy yet powerful for one test to run**

**# another test to implement certain action without duplicating code.**
**self._client_at.run_test('power_Resume')**
**_transitions = {**
**'logout': _do_logout,**
**'suspend': _do_suspend,**
**}**
**_als_disabled = False**
**def _set_als_disable(self):**
**"""Turns off ALS in power manager. Saves the old has_ambient_light_sensor flag if**
**it exists.**
**"""**
**als_path = '/var/lib/power_manager/has_ambient_light_sensor'**
**self._client_cmd('if \[ -e %s \]; then mv %s %s_backup; fi' %**
**(als_path, als_path, als_path))**
**self._client_cmd('echo 0 &gt; %s' % als_path)**
**self._client_cmd('restart powerd')**
**self._als_disabled = True**
**def _restore_als_disable(self):**
**"""Restore the has_ambient_light_sensor flag setting that was overwritten in**
**_set_als_disable.**
**"""**
**if not self._als_disabled:**
**return**
**als_path = '/var/lib/power_manager/has_ambient_light_sensor'**
**self._client_cmd('rm %s' % als_path)**
**self._client_cmd('if \[ -e %s_backup \]; then mv %s_backup %s; fi' %**
**(als_path, als_path, als_path))**
**self._client_cmd('restart powerd')**
**def run_once(self, client_ip):**
**"""Run the test.**
**For each system transition event in |_transitions|:**
**Read the brightness.**
**Trigger transition event.**
**Wait for client to come back up.**
**Check new brightness against previous brightness.**
**@param client_ip: string of client's ip address (required)**
**"""**
**if not client_ip:**
**error.TestError("Must provide client's IP address to test")**

**# Create a custom host class for this machine, which is used to execute**

**# commands and other functions.**

**self._client = hosts.create_host(client_ip)**
**# Create an Autotest instance which you can run method like run_test,**

**# which can execute another client side test to facilitate this test.**

**self._client_at = autotest.Autotest(self._client)**
**self._results = {}**
**self._check_power_status()**
**self._set_als_disable()**
**# Save the original brightness, to be restored after the test ends.**
**self._original_brightness = self._get_brightness()**
**# Set the brightness to a random number which is different from**
**# system default value.**
**self._set_brightness_percent(71)**
**# Run the transition event tests.**
**for test_name in self._transitions:**
**self._old_brightness = self._get_brightness()**
**self._transitions\[test_name\](self)**
**# Save the before and after backlight values.**
**self._results\[test_name\] = { 'old': self._old_brightness,**
**'new': self._get_brightness() }**
**def cleanup(self):**
**"""Restore DUT's condition before the test starts, and check the test**
**results.**
**"""**
**self._restore_als_disable()**
**self._set_brightness_percent(self._original_brightness)**
**# Check results to make sure backlight levels were preserved across**
**# transition events.**
**num_failed = 0**
**for test_name in self._results:**
**old_brightness = self._results\[test_name\]\['old'\]**
**new_brightness = self._results\[test_name\]\['new'\]**
**if old_brightness == new_brightness:**
**logging.info('Transition event \[ PASSED \]: %s', test_name)**
**else:**
**logging.info('Transition event \[ FAILED \]: %s', test_name)**
**logging.info(' Brightness changed: %d -&gt; %d',**
**old_brightness, new_brightness)**
**num_failed += 1**
**if num_failed &gt; 0:**
**raise error.TestFail(('Failed to preserve backlight over %d '**

****'transition event(s).') % num_failed)******
