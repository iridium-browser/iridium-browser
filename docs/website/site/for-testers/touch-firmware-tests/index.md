---
breadcrumbs:
- - /for-testers
  - For Testers
page_name: touch-firmware-tests
title: Touch Firmware Tests
---

[TOC]

[ <img alt="image" src="/for-testers/touch-firmware-tests/TouchFWTests.png"
height=94 width=400>](/for-testers/touch-firmware-tests/TouchFWTests.png)

## Preparing the Testing Host

These instructions describe how to set up the code and its dependencies on the
host, the workstation you will actually be running the tests *from*. Usually the
host is a normal development machine. It is not the device under test (DUT).

### Initial setup

The first time you configure the host to run touch firmware tests, the following
steps are required:

1.  Clone the git repo that contains the test scripts. It is possible to
            clone the repo without a full Chrome OS checkout, by running the
            following: `git clone
            <https://chromium.googlesource.com/chromiumos/platform/touch_firmware_test.git>`
            This command creates a new `touch_firmware_test` directory in your
            working directory, containing a number of Python scripts.
2.  Install the [PyQtGraph](http://www.pyqtgraph.org/) plotting library,
            which is a dependency for the touch firmware tests. Use the provided
            installers for Ubuntu and Windows and follow the instructions on
            [www.pyqtgraph.org](http://www.pyqtgraph.org) to install this
            library.

### Updating to the Latest Version

If you have previously checked out the test source code, simply pull the latest
patches from the git server by navigating to the `touch_firmware_test` directory
and running the `git pull` command.

## Preparing the DUT

The firmware test works for both Android and Chrome devices. However,
configuration steps vary according to the type of DUT you are testing.

### Android Devices

1.  Verify `adb` is installed on the testing host. You can follow the
            steps [described on this
            page](https://docs.google.com/document/d/1nRsKr6wZi5sWmdgzCy9Cx7yPykcuxu9gFccXroRLxBI)
            or use a different procedure that works for your environment.
2.  Connect your Android device to the testing host.On the host, you
            must be able to connect to the DUT via `adb`. In the simple case the
            DUT is physically connected to the host via USB. Other
            configurations, such as adb-over-wifi, are also possible, as long as
            `adb` can communicate with the DUT.
3.  Enable USB Debugging on the DUT. Using [these
            instructions](http://www.greenbot.com/article/2457986/how-to-enable-developer-options-on-your-android-phone-or-tablet.html)
            or similar steps, enable Developer options on the Android device. In
            the Developer options panel, select USB debugging. The exact
            procedure depends on the device. If you cannot locate these features
            on your device, search the Internet for “USB debugging
            &lt;*your-device-type*&gt;”.
4.  Verify the configuration of your DUT and testing host by running
            `adb shell` in a terminal window on the host. It should open a
            terminal connection to the DUT with a command prompt.

### Chrome Devices

1.  Install a Chrome OS test image on the DUT. A test image is required
            (versus a normal Chrome OS image) to allow the firmware test to use
            ssh, evtest, and other testing utilities not included in release
            images. To get a test image, run the following:

    ```bash
    cros build-packages --board=$BOARD
    cros build-image --board=$BOARD --no-enable-rootfs-verification test
    ```

1.  Connect the DUT to a network the testing host can access. A
            USB-Ethernet adapter connected to a LAN is recommended, to minimize
            lag and dropped connections. It is also feasible to use a wireless
            connection.
2.  Verify the configuration of your DUT and testing host by running the
            command `ssh root@$DUT_IP` to connect to the DUT via SSH. If
            prompted for a password, use `test0000`. A connection to the DUT
            should be established.

## Running the Test

1.  On the testing host, navigate to the `touch_firmware_test` directory
            and run either:

> `python main.py -t chromeos -a $IP_ADDRESS `

> for Chrome devices, or:

> `python main.py -t android [-a $DEVICE_STRING] `

> for Android devices. To display additional command line parameters that
> configure the way the test runs, run `python main.py --help` .

1.  Follow the screen prompts and perform the gestures indicated on the
            touchpad or touchscreen of the DUT.
2.  When all the tests have run, a file named `report.html` is
            generated. Open this file in a browser to view the results.
