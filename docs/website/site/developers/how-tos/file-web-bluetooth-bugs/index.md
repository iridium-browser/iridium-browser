---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/how-tos
  - How-Tos
page_name: file-web-bluetooth-bugs
title: How to file Web Bluetooth bugs
---

[TOC]

[Stack Overflow #web-bluetooth](https://stackoverflow.com/questions/tagged/web-bluetooth?tab=Active)
is a great community for diagnosing issues.  It is helpful to form
[well-asked questions](https://stackoverflow.com/help/how-to-ask).
The tools on this page can help you diagnose issues.

If you find an implementation bug, please [file a Web Bluetooth
bug](https://bugs.chromium.org/p/chromium/issues/entry?components=Blink%3EBluetooth&source=chromium.org),
and attach Bluetooth logs (both Chrome and platform logs) to help
debug the issue.

[Specification bugs are on GitHub](https://github.com/WebBluetoothCG/web-bluetooth/issues).


## General Bluetooth Inspection Tools

These general tools will help you explore bluetooth devices that you are
attempting to communicate with:

1.  chrome://bluetooth-internals
    *   Displays Bluetooth information Chrome is able to access,
                allowing you to experiment without going through the device
                chooser dialog, blocklist, or other web platform security
                aspects of Web Bluetooth.
2.  [nRF Connect for
            Android](https://play.google.com/store/apps/details?id=no.nordicsemi.android.mcp&hl=en)
            or [nRF Connect for
            iOS](https://itunes.apple.com/us/app/nrf-connect/id1054362403?mt=8)
    *   Generic tool that allows you to scan, advertise and explore your
                Bluetooth low energy (BLE) devices and communicate with them.

## Manual Bluetooth Tests

https://github.com/WebBluetoothCG/manual-tests contains a suite of tests that
use the Web Bluetooth API. These tests differ from those already utilized by
browsers in that they rely on actual Bluetooth hardware, and are manually driven.

Reproducing an error by forking and modifying one of these tests will enable
other developers to reproduce the same error more rapidly because they will
not need to acquire another specific or possibly proprietary device.

## Bluetooth Devices to Test With

These devices are some that can be used to act as peripheral devices.

1.  [BLE Peripheral
            Simulator](https://play.google.com/store/apps/details?id=io.github.webbluetoothcg.bletestperipheral)
            Android app
    *   Simulates a devices with Battery, Heart Rate, Health Thermometer
                services. Developers can connect to the app to Read and Write
                Characteristics, Subscribe to Notifications for when the
                Characteristics change, and Read and Write Descriptors.
    *   Code at:
                https://github.com/WebBluetoothCG/ble-test-peripheral-android
2.  [nRF
            Connect](https://www.nordicsemi.com/Software-and-tools/Development-Tools/nRF-Connect-for-mobile/GetStarted)
            Can also simulate a GATT server.
3.  [Espruino](https://www.espruino.com/) devices ([Puck
            JS](https://www.puck-js.com/) etc)
    *   Programable in JavaScript with a
                [WebIDE](https://www.espruino.com/ide/).

## chrome://device-log

**chrome://device-log** displays a filtered view of logs on multiple platforms.

*   Change the **Log level** to **Debug**
*   **Filter** to only **Bluetooth**.

However, more detailed platform logs may be necessary to diagnose issues, see
below:

## Mac

#### Chrome logs

1.  Quit any running instance of Chrome.
2.  Launch /Applications/Utilities/Terminal.app
3.  At the command prompt enter:
    /Applications/Google\\ Chrome.app/Contents/MacOS/Google\\ Chrome
    --enable-logging=stderr --vmodule=\*bluetooth\*=2

#### Platform logs

1.  Install Apple's [Additional Tools for
            XCode](https://developer.apple.com/download/more/?name=Additional%20Tools%20for%20XCode)
            if you have XCode 8+, otherwise install Apple's [Hardware IO Tools
            for
            Xcode](https://developer.apple.com/downloads/?name=Hardware%20IO%20Tools).
2.  Use **Bluetooth Explorer** in that package to try to discover and
            connect to your device.
    *   Devices | Low Energy Devices will let you scan for and connect
                to BLE devices. If you can't find or use your device with
                Bluetooth Explorer, there's likely a bug in MacOS rather than
                Chrome. Please file a bug in [Apple's bug
                tracker](https://bugreport.apple.com/).
    *   "Tools" and select "Device Cache Explorer", from there you can
                click "Delete All" button to clear **device cache** if needed to
                help reproduce errors.
3.  Use **PacketLogger** to capture a log of Bluetooth packets while
            you're reproducing your bug. Remember to clear the packet log before
            you start so you send us mostly relevant packets.

## Linux

#### Chrome logs

1.  Quit any running instance of chrome.
2.  Execute in a console:
    google-chrome --enable-logging=stderr --vmodule=\*bluetooth\*=9
    --enable-experimental-web-platform-features

#### Platform logs

1.  Get BlueZ version with dpkg -l bluez
2.  Get Linux kernel version with uname -r
3.  Start monitoring bluetooth via sudo btmon or record traces in
            btsnoop format via sudo btmon -w ~/Downloads/btmon.btsnoop

## Chrome OS

#### Chrome logs

1.  Put the device into [Developer
            Mode](/chromium-os/chromiumos-design-docs/developer-mode) so you can
            get a Root Shell
2.  Open Chrome OS Shell with \[Ctrl\] + \[Alt\] + T
3.  Enter shell and run these shell commands:
    sudo su
    cp /etc/chrome_dev.conf /usr/local/
    mount --bind /usr/local/chrome_dev.conf /etc/chrome_dev.conf
    echo "--vmodule=\*bluetooth\*=9" &gt;&gt; /etc/chrome_dev.conf
4.  Restart the UI via:
    `restart ui`
5.  Check out messages logged by Chrome at
            file:///home/chronos/user/log/chrome

#### Platform logs

Check out System Logs:

1.  Go to file:///var/log/messages
2.  Look for "bluetooth"

Enter a Bluetooth debugging console:

1.  Open Chrome OS Shell with \[Ctrl\] + \[Alt\] + T
2.  Enter bt_console and type help to get to know commands

Monitor Bluetooth traffic:

1.  Put the device into [Developer
            Mode](/chromium-os/chromiumos-design-docs/developer-mode) so you can
            get a Root Shell
2.  Open Chrome OS Shell with \[Ctrl\] + \[Alt\] + T
3.  Enter shell
4.  Start monitoring bluetooth via sudo btmon or record traces in
            btsnoop format via sudo btmon -w ~/Downloads/btmon.btsnoop

Monitor BlueZ messages going through a D-Bus message bus:

1.  Put the device into [Developer
            Mode](/chromium-os/chromiumos-design-docs/developer-mode) so you can
            get a Root Shell
2.  Open Chrome OS Shell with \[Ctrl\] + \[Alt\] + T
3.  Enter shell
4.  Start monitoring with dbus-monitor --system
            "interface=org.freedesktop.DBus.Properties"
            "interface=org.freedesktop.DBus.ObjectManager"

Getting the BlueZ package version:

1.  Put the device into [Developer
            Mode](/chromium-os/chromiumos-design-docs/developer-mode) so you can
            get a Root Shell
2.  Open Chrome OS Shell with \[Ctrl\] + \[Alt\] + T
3.  Enter shell
4.  Get the version with more
            /etc/portage/make.profile/package.provided/chromeos-base.packages |
            grep bluez

And if you're enthusiastic, you can also build [BlueZ](http://www.bluez.org/)
from source on your Chromebook in [Developer
Mode](/chromium-os/poking-around-your-chrome-os-device), with
[crouton](https://github.com/dnschneid/crouton) and run it by following these
[instructions](https://github.com/beaufortfrancois/sandbox/blob/gh-pages/web-bluetooth/Bluez.md).

## Android

#### ADB logs

*   If you have Android SDK installed
    *   Use adb logcat, perhaps filtering it through grep to highlight
                interesting bits for you:
        *   adb logcat -v time | grep -E "
                    |\[Bb\]luetooth|cr.Bluetooth|BtGatt|BluetoothGatt|bt_btif_config"
*   If you do not have Android SDK installed
    *   [Enable USB
                Debugging](http://developer.android.com/tools/device.html) via
                Developer options
        *   Settings &gt; About phone and tap Build number seven times.
                    Return to the previous screen to find Developer options.
        *   Enable USB Debugging
    *   Take a bug report
        *   Settings &gt; Debeloper options &gt; Take a bug report
        *   Send the bug report to **yourself**, it contains more
                    information than we need.
    *   Extract the bug report .zip file
    *   Search within the bug report text file to find the logcat
                output.
    *   Copy the relevant section that includes Bluetooth information to
                a new file, and send that.

## Windows

**Packet capture**

1.  Install the [Windows Driver
            Kit](https://docs.microsoft.com/en-us/windows-hardware/drivers/download-the-wdk)
            for Windows 10.
2.  Open an administrator command prompt.
3.  Run the command: logman create trace "bth_hci" -ow -o
            C:\\bth_hci.etl -p {8a1f9517-3a8c-4a9e-a018-4f17a200f277}
            0xffffffffffffffff 0xff -nb 16 16 -bs 1024 -mode Circular -f bincirc
            -max 4096 -ets
4.  Reproduce your issue.
5.  Run the command: logman stop "bth_hci" -ets
6.  Find BtEtlParse.exe in the WDK and run the command: btetlparse
            C:\\bth_hci.etl
7.  The resulting file can be opened using Wireshark.
