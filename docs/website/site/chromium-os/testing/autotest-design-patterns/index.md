---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/testing
  - Testing Home
page_name: autotest-design-patterns
title: Autotest Design Patterns
---

[TOC]

This is a collection of patterns to help aid test developers in finding useful
parts of Autotest and common pathways when writing tests.

## Adding retries to Flaky tests

We have a mechanism to retry tests that have flaky failures. A retries count can
be specified, and a failing test will be re-attempted until it either passes or
the number of failed retries reaches the retry count, in which case the test
will be considered failed.

To add retries to your test, you simply need to add a JOB_RETRIES = N attribute
to the control file for the test you would like to retry, where N is an integer
giving the maximum number of times the test should be retried.

> Note that a previous method of retrying flaky tests was to use the RETRIES
> attribute. While this attribute is no longer in use, one might find this in
> commits merged in or before Aug 2018. [This
> bug](https://bugs.chromium.org/p/chromium/issues/detail?id=873716) tracks the
> migration from RETRIES to JOB_RETRIES.

## Wrapping a GTest test in Autotest

## Wrapping a Browerstest in Autotest

## Wrapping a Telemetry test in Autotest

Telemetry is replacing Pyauto as the new performance testing framework for
Chrome as well as the new key way to write tests that interact with the UI.
Writing a test that uses telemetry is a 2 step process:

*   Write a telemetry test or benchmark in the chromium source.
    *   Telemetry's source code, tests, and performance benchmarks are
                all platform independent. Thus they all exist within the
                chromium not the chromium-os source.
    *   If one wants to write a new test, they first should add it as a
                new telemetry test.
    *   They can run these tests locally against their Chrome-OS
                machines but provided the arguments --browser=cros-chrome and
                --remote=&lt;IP&gt; to the telemetry run_tests script that kicks
                off your new tests.
    *   Once the test works and is checked in, it can be ran as a new
                autotest test.
        *   It will take a day or two for the chrome tip-of-tree to be
                    rev-ed into the next build
    *   Please see <http://www.chromium.org/developers/telemetry> for
                more information.
*   Write an autotest test to kick off the new telemetry test.
    *   Once your new telemetry test/benchmark is checked into chrome
                and working you need to write an autotest server-side test to
                kick it off.
    *   In your new test script you need the following code:

```none
from autotest_lib.server.cros import telemetry_runner
def run_once(self, host=None):
        """Run the telemetry scrolling benchmark.
        @param host: host we are running telemetry on.
        """
        telemetry = telemetry_runner.TelemetryRunner(host)
        result = telemetry.run_telemetry_test('my_new_telemetry_test')
        
        # OR
        result = telemetry.run_telemetry_benchmark('my_new_telemetry_benchmark', 'my_page_set')
        # If collecting performance keyvals to be sent to the perf database:
        result = telemetry.run_telemetry_benchmark('my_new_telemetry_benchmark', 'my_page_set', keyval_writer=self)
```

    *   Alternatively there currently exists a telemetry_benchmarks test
                and to add a new benchmark you simple need to add a new control
                file to this test. I.E. for octane, you add a control file named
                control.octane and kick off the telemetry_benchmarks test with
                the current parameters:

```none
def run_benchmark(machine):
    host = hosts.create_host(machine)
    job.run_test("telemetry_benchmarks", host=host, benchmark="octane", page_sets=["octane.json"])
parrallel_simple(run_benchmark, machine)
```

## Locally Testing a Wrapped Telemetry Test:

Because the telemetry runner code requires the full lab infrastructure, you need
to set up a local autotest frontend and devserver in order to locally test your
telemetry changes.

1.  First set up a local AFE and database by following the instructions
            here:
            <http://www.chromium.org/chromium-os/testing/autotest-developer-faq/setup-autotest-server>
2.  Kick off a local devserver.

    ```none
    cd src/platform/dev # From you local chromium-os checkout
    # Kick off the devserver
    ./devserver.py --static_dir=static
    ```

3.  Edit your local AFE to use your local devserver.
    1.  Add the following to
                src/third_party/autotest/files/shadow_config.ini:

        ```none
        [CROS]
        dev_server: http://[YOUR IP ADDRESS]:8080
        ```

4.  Restart apache and kick off the scheduler:

    ```none
    sudo /etc/init.d/apache2 restart
    /usr/local/autotest/scheduler/monitor_db.py /usr/local/autotest/results
    ```

5.  Add your device to your setup (this requires you to be able to log
            into your machine as root).

    ```none
    src/third_party/autotest/files/cli/atest host create [ip of your host]
    ```

6.  Setup a device with the image you want (important as the database
            will label your machine with its current image). Do this by kicking
            off the dummy suite.

    ```none
    src/third_party/autotest/files/site_utils/run_suite.py --board=[board] --build=[image you want to use] --suite=dummy
    ```

7.  Now you should be able to kick off your server side telemetry tests
            through the AFE (go to localhost in your browser)! But in case your
            new test doesn't show up, run test importer. Then refresh the create
            job's page with cntl+shift+r

    ```none
    src/third_party/autotest/files/utils/test_importer.py
    ```

Note that the test will try to ssh to the dev_server, so make sure that 'ssh
YOUR_IP_ADDRESS' works without prompting a password. If it doesn't, you'll need
to setup public keys for your local machine. Following these steps:

1.  First generate RSA ssh keys, see [this
            link](https://help.github.com/articles/generating-ssh-keys) if you
            don't know how to. Assume you've created ~/.ssh/id_rsa,pub and
            ~/.ssh/id_rsa
2.  Add the following to ~/.ssh/config

    ```none
    Host YOUR_IP_ADDRESS
     User YOUR_USER_NAME   
    IdentityFile /path/to/home/.ssh/id_rsa
    ```

3.  Add the content of ~/.ssh/id_rsa to ~/.ssh/authorized_keys

    ```none
    cat ~/.ssh/id_rsa >> ~/.ssh/authorized_keys
    ```

4.  Restart ssh-agent

    ```none
    username@localhost~$ killall ssh-agent; eval `ssh-agent`
    ```

5.  Allow ssh to your local machine using RSA keys

    ```none
    username@localhost~$ sudo vim /etc/ssh/sshd_config
    # Turn on RSAAuthentication
    RSAAuthentication yes
    ```

6.  Restart ssh server

    ```none
    username@localhost~$ sudo service ssh restart
    ```

7.  Test you can ssh to your local machine without password

    ```none
    username@localhost~$ ssh YOUR_IP_ADDRESS
    ```

## Logging into Chrome from an Autotest (using Telemetry)

From an Autotest, you can use Telemetry to log into and out of Chrome using
context management in Python (the "with/as" construct). To do so, your autotest
must include the following import:

```none
from autotest_lib.client.common_lib.cros import chrome
```

and then use the "with/as" construct like so:

```none
with chrome.Chrome() as cr:
    do_stuff()  # Will be logged into Chrome here.
# Will be logged out at this point.
```

In this example, cr is a Telemetry Browser object. Wrapping your code inside of
a "with/as" ensures that Chrome will be logged in at the start of the construct,
and logged out at the end of the construct.

## Rebooting a machine in a server-side test

This handles the simple reboot case:

```none
def run_once(self, host=None):
    host.reboot()
    # If the reboot fails, it will raise error.AutoservRebootError
    # If you need to recover from a reboot failure, note that calls
    # to host.run() will almost certainly fail with a timeout.
```

If you have custom code that brings the DUT down, and then brings it back up
again, you will need to use a slightly different flow:

```none
def run_once(self, host=None):
    boot_id = host.get_boot_id()
    self._trigger_complicated_dut_restart()
    host.wait_for_restart(old_boot_id=boot_id)
```

## Running a client side test as part of a server side test

Inside a server job you can utilize the host object to create a client object to
run tests.

```none
def run_once(self, host, job_repo_url=None):
    client_at = autotest.Autotest(host)
    client_at.run_test('sleeptest')
```

\*The status of this test is communicated through the status.log, if this test
fails, the overall run of the job will be considered a failure.

## Writing code to automatically generate a label for a DUT

Many tests have requirements for specific hardware characteristics, specified by
setting DEPENDENCIES. So, imagine there's a new *fubar* class of device present
on some new hardware. You have tests that exercise the driver for the new
devices. Obviously, your test shouldn't run unless a *fubar* device is present.
So, your control file will say this:

```none
DEPENDENCIES = "fubar"
```

Alternatively, imagine that there are three kinds of *fubar*, types A, B, with
the possibility of other types that might be added in future. There are boards
with types A and B to be tested. You then have two control files, one for each
type.

For Type A:

```none
DEPENDENCIES = "fubar_type:type_a"
```

For Type B:

```none
DEPENDENCIES = "fubar_type:type_b"
```

These hardware characteristics are meant be detected automatically when the DUT
is added into the lab database. To make the detection happen, you have to write
detection code. Here's how to do it.

The detection function must be added as a method in class `CrosHost`, found in
file server/hosts/cros_host.py. The method must be decorated with
`@label_decorator`. The return value is a string with the label name as it will
appear in the `DEPENDENCIES` setting.

A sample of a simple binary label:

```none
@label_decorator('fubar')
def has_fubar(self):
    result = self.run('test -d /sys/class/fubar', ignore_status=True)
    if result.exit_code != 0:
        return None
    return 'fubar'
```

A sample of a label with multiple values:

```none
@label_decorator('fubar_type')
def get_fubar_type(self):
    result = self.run('get_fubar_type', ignore_status=True)
    typestring = result.stdout.strip()
    if typestring == 'Type A':
        return 'fubar_type:type_a'
    if typestring == 'Type B':
        return 'fubar_type:type_b'
    # typestring 'Type C' is a proposed industry standard, but
    # it hasn't been finalized.  There may be other as yet
    # unknown types in the future.
    return None
```

One point about labels with multiple values bears emphasis: It may be that you
will find that some valid values of a label aren't yet needed for any test. In
that case, return `None` for the unused case. **Do not return a label value that
is not needed by any existing test.** Generally, adding labels is easy, but
removing labels is hard. So, don't create a label name until you know
**exactly** where and how it will be used.

## Use Chrome Driver in ChromeOS Autotest Test

### About Chrome Driver

Chrome Driver is a standalone server which implements WebDriver's wire protocol
for Chromium. Through Chrome Driver, you can easily interact with Chromium
browser with the power of browser automation provided by WebDriver. A list of
WebDriver calls you can make can be found in [Selenium 2.0
Documentation](http://selenium.googlecode.com/svn/trunk/docs/api/py/webdriver_remote/selenium.webdriver.remote.webdriver.html).
Some useful calls include:

    [get](http://selenium.googlecode.com/svn/trunk/docs/api/py/_modules/selenium/webdriver/remote/webdriver.html#WebDriver.get)(url):
    Loads a web page in the current browser session.

    [execute_script](http://selenium.googlecode.com/svn/trunk/docs/api/py/_modules/selenium/webdriver/remote/webdriver.html#WebDriver.execute_script)(script,
    \*args): Synchronously Executes JavaScript in the current window/frame.

    [find_element_by_name](http://selenium.googlecode.com/svn/trunk/docs/api/py/_modules/selenium/webdriver/remote/webdriver.html#WebDriver.find_element_by_name)(name):
    Finds an element by name.

### Chrome Driver Binary

All ChromeOS test images shall have Chrome Driver binary installed in
/usr/local/chromedriver/. The binary is updated to the same version of Chrome in
that test image. That is, you will always be using the latest build of Chrome
Driver.

If your test expects to run against a “stable” build of Chrome Driver binary,
you will need to write your own code in your test to download the desired binary
and replace the binary in /usr/local/chromedriver/.

### How to use Chrome Driver in an Autotest test

Writing a test that uses Chrome Driver to interact with Chrome is easy. There is
[a wrapper
class](https://chromium.googlesource.com/chromiumos/third_party/autotest/+/HEAD/client/common_lib/cros/chromedriver.py)
for using Chrome Driver available in ChromeOS/Autotest. The wrapper class, as a
context manager type, and handles the following tasks for you:

    Logs in ChromeOS using Telemetry.

    Starts Chrome Driver with Remote mode on the Device under Test (DUT) and
    connects to the remote debug port of the Chrome instance after log in.

    Exposes a driver instance for you to make any Chrome Driver calls.

    Shutdowns Chrome Driver process, and logs out of ChromeOS.

To write a test, you can follow the example of test
[desktopui_UrlFetchWithChromeDriver](https://chromium.googlesource.com/chromiumos/third_party/autotest/+/HEAD/client/site_tests/desktopui_UrlFetchWithChromeDriver/desktopui_UrlFetchWithChromeDriver.py).
All you need to do to get started are basically:

    import the wrapper class

```none
from autotest_lib.client.common_lib.cros import chromedriver
```

    Create an instance of the Chrome Driver, and make calls.

```none
with chromedriver.chromedriver() as chromedriver_instance:
```

```none
    driver = chromedriver_instance.driver
```

```none
    # Here you can make standard Chrome Driver calls through the driver instance.
```

```none
           # For example, browse a given url with |driver.get(url)|
```
