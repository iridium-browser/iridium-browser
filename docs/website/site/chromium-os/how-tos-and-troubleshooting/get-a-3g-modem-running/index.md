---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/how-tos-and-troubleshooting
  - How Tos and Troubleshooting
page_name: get-a-3g-modem-running
title: Cellular modem FAQ
---

[TOC]

Chrome OS supports Gobi modems via Cromo, and various other modems via Modem
Manager. Chrome OS only supports cellular modems that have a cdc_ether (or
similar) network interface. This page documents what is useful to know if you
wish to bring up new modems and/or GSM carriers.

## How do I verify that a modem is seen by Modem Manager?

There are several modem tools that are part of the flimflam-testscripts package.
This package is install on test builds. The tools can be found in
/usr/local/lib/flimflam/test. The first step is to verify that Chrome OS can see
the modem.

> `PATH=${PATH}:/usr/local/lib/flimflam/test`

> `mm-list-modems`

You should see something like:

> `/org/freedesktop/ModemManager/Modems/0`
> ` or`
> ` /org/chromium/Cromo/Gobi/0`

## What if the modem is not seen by Modem Manager?

*   make sure kernel has proper drivers
*   do you need to run modem-modeswitch?
*   do you need to eject the modem?
*   Can you write some udev rules to automatically eject/modeswtich the
            modem?

## How do I get more information about a modem?

Enable the modem and use the mm-status command to get more information about the
modem.

> `mm-enable`

> `modem status`

The output will look like:

> `Path: /org/freedesktop/ModemManager/Modems/0`

> `Iface org.freedesktop.ModemManager.Modem`

> `MasterDevice = /sys/devices/pci0000:00/0000:00:1d.0/usb2/2-1`

> `UnlockRequired = `

> `Enabled = 1`

> `IpMethod = 0`

> `Driver = option1`

> `Device = ttyUSB0`

> `Type = 2`

> `GetInfo()`

> `---------`

> `Manufacturer: NOVATEL WIRELESS INCORPORATED`

> `Modem: MC760 SPRINT`

> `Version: Q6085BDRAGONFLY_S122 [Dec 09 2008 18:00:00]`

> `GetEsn()`

> `--------`

> `esn = 0x5B96D0FB`

> `GetRegistrationState()`

> `----------------------`

> `CDMA-1x state: 1`

> `EVDO state: 1`

> `GetServingSystem()`

> `------------------`

> `Serving System band class: 2`

> `Serving System band: B`

> `Serving System id: 4106`

> `GetSignalQuality()`

> `------------------`

> `Signal Quality: 100`

## How do I manually connect a modem?

You can use the mm-connect script to manually connect a modem to a 3G network.
If you are using a GSM network you may need to edit mm-connect to add your
carrier (APN, number, pin) to the dictionary of connect arguments. Connecting a
modem though in most cases only establishes a PDP context between the modem and
the carrier. You'll then need to run dhclient on the associated interface.

## How do I fix flimflam to recognize my GSM carrier?

Flimflam uses the mobile-broadband-provider-info to find APNs. You can enter
your own APN on the UI, or if you find that your APN is missing from the monbile
broadband provider info database, file a but at https://bugzilla.gnome.org/

## How do I manually connect via flimflam?

The advantage of using flimflam to connect is that you flimflam will take care
of running dhcpcd for you. Using the flimflam scripts one can list the devices,
and services. You should be able to verify that a cellular device is listed, and
that there is a cellular service. To ask flimflam to connect via the cellular
service issues the connect-service command (from /usr/local/lib/flimflam/test):

```none
list-devices
connect-service cell
```

There are still some issues in the support of the cdc_ether driver. If the
connect-service call fails, try to disconnect, and reissue the connect call. In
most cases this will succeed in connecting your 3G modem.

```none
disconnect-service cell
connect-service cell
```

## How do I debug the Gobi kernel module?

To debug the Gobi kernel module, you need to set the gobi_debug module parameter
on the gobi module. There are two ways to do this:

To set the parameter temporarily (until the module is unloaded, such as when you
reboot), you can use sysfs:

```none
localhost ~ # echo 1 > /sys/module/gobi/parameters/gobi_debug
```

To set the parameter permanently, you can configure modprobe to set it
automatically:

```none
localhost ~ # echo 'options gobi gobi_debug=1' > /etc/modprobe.d/gobi_debug.conf
```

You can use either 1 or 2 for the debug level. 1 will show most
userspace-relevant error conditions, but won't trace most normal behavior; 2
will dump every control packet to and from the card, and will give more detail
about normal behavior.

## How do I see the GPS stream on a Gobi modem?

To see the GPS stream on a Gobi modem you first have to enable automatic
tracking.

```none
 localhost ~ # echo -e -n '$GPS_START' > /dev/ttyUSB2
    localhost ~ # dbus-send --system --print-reply \
     --dest=org.chromium.ModemManager \
     /org/chromium/ModemManager/Gobi/0 \
     org.chromium.ModemManager.Modem.Gobi.SetAutomaticTracking \
     boolean:true boolean:true
localhost ~ # cat /dev/ttyUSB2
```

and observe the NMEA stream printed on the console.

Then interrupt the stream (^c), and enter

```none
localhost ~ # dbus-send --system --print-reply \
     --dest=org.chromium.ModemManager \
     /org/chromium/ModemManager/Gobi/0 \
     org.chromium.ModemManager.Modem.Gobi.SetAutomaticTracking \
     boolean:false boolean:false
localhost ~ # cat /dev/ttyUSB2
```

and observe that the stream has been stopped.

Note that we have an autotest which does all that, it should be invoked as
follows:

```none
./run_remote_tests.sh --board=<board> --remote=<taregt ip> hardware_GPS/control
```

## What do all those PIN codes do?

There are 8 different kinds of standard PIN codes defined for GSM devices. Two
of these apply to the SIM card, and the others apply to the device itself. The
following table describes each kind of PIN. Most PINs allow some number of
incorrect entries, after which the PIN becomes "blocked". At that point the PIN
must be "unblocked" by entering a PUK code. Each PIN type has its own
corresponding type of PUK.

SIM locks can be disabled, which is different than unlocking. Unlocking only
lasts until the next power cycle. Disabling prevents the SIM from being locked,
even following a power cycle. To disable a SIM lock, the SIM PIN must be
supplied.

Device locks, also known as personalization locks, restrict usage of the device,
as described below. For personalization, there is no distinction between
unlocking and disabling. For each type of personalization, there is a code that
must be entered. If the correct code is entered, the corresponding device
restriction is lifted.

Refer to 3GPP spec [TS
22.022](http://www.etsi.org/deliver/etsi_ts/122000_122099/122022/10.00.00_60/ts_122022v100000p.pdf)
for full details.

[<img alt="image"
src="/chromium-os/how-tos-and-troubleshooting/get-a-3g-modem-running/GSM%20PIN%20code%20types.png">](/chromium-os/how-tos-and-troubleshooting/get-a-3g-modem-running/GSM%20PIN%20code%20types.png)

## How do I run tests on cell modems?

You can run autotests against a live carrier network, or you can use an
emulated-network testbed. See [Running and Writing Autotests for Cell
Emulators](https://docs.google.com/a/google.com/document/pub?id=1FTSAO-5hVTAV9t7RNZWJRN_QOOqpl5qZJua4k3POwKc)
and [Chrome OS Cellular Testbed
Setup](https://docs.google.com/a/google.com/document/pub?id=1yG7j8Iw9PnQTH-93zP5BqB0qQRU08az11A_eN0acd70).
