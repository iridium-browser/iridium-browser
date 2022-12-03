---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/how-tos
  - How-Tos
page_name: file-web-hid-bugs
title: How to file WebHID bugs
---

[TOC]

When [filing a WebHID
bug](https://bugs.chromium.org/p/chromium/issues/entry?components=Blink%3EHID&source=chromium.org),
please attach some information so we can debug the issue.

## Product Information

Many issues only occur with a particular device. When filing a WebHID API bug,
please provide a link to a page describing the device you are using so that
other developers can identify it.

Examples:

* A link to an online store page where the product can be purchased.
* A link to the manufacturer's page describing the product.
* A link to a photo containing the product packaging with the product and brand
  names visible.

## chrome://device-log

`chrome://device-log` displays a filtered view of logs on multiple platforms.

* Change the **Log level** to **Debug**
* **Filter** to only **HID**.

If there are any messages with type **HID**, please include them in the bug.

## General HID Inspection Tools

These general tools will help you explore devices that you are attempting to
communicate with.

### Web

Use [HID Explorer](https://nondebug.github.io/webhid-explorer/) to capture
information about your device and add it to the bug.

If your device has multiple HID interfaces it will appear as multiple devices in
HID Explorer, please include information for all device interfaces.

If you can connect to your device in the browser and capture its information
with HID Explorer it is usually not necessary to use the other tools listed in
this section as they provide the same information.

### Windows

Use [winhiddump](https://github.com/todbot/win-hid-dump/releases/) to return a
report descriptor reconstructed from the information available through the
Windows HID API. This will likely differ from the actual report descriptor.

Please include the `winhiddump` output in the bug report.

If the device is connected with USB, use [UsbTreeView](https://www.uwe-sieber.de/usbtreeview_e.html)
to capture device information. Select your device in the tree view pane and copy
the device information into the bug.

### macOS

Use the [macOS I/O Registry](https://developer.apple.com/library/archive/documentation/DeviceDrivers/Conceptual/IOKitFundamentals/TheRegistry/TheRegistry.html)
to output information about all connected HID devices:

```
ioreg -c IOHIDInterface -r -d 1 > deviceinfo.txt
```

The device can be identified by its product name ("Product" key) or its vendor and product IDs ("VendorID" and "ProductID" keys).

```
IOHIDInterface  <class IOHIDInterface, id 0x10000e745, registered, matched, active, busy 0 (11 ms), retain 10>
{
  "Transport" = "Bluetooth Low Energy"
  "HIDDefaultBehavior" = Yes
  "Manufacturer" = "Logitech"
  "Product" = "MX Master 3"
  "MaxInputReportSize" = 20
  "DeviceUsagePairs" = ({"DeviceUsagePage"=1,"DeviceUsage"=6},{"DeviceUsagePage"=1,"DeviceUsage"=2},{"DeviceUsagePage"=1,"DeviceUsage"=1},{"DeviceUsagePage"=65347,"DeviceUsage"=514})
  "VendorIDSource" = 2
  "MaxOutputReportSize" = 20
  "ReportDescriptor" = <05010906a1018501050719e029e71500250175019508810295067508150026a400050719002aa4008100c005010902a10185020901a1009510750115002501050919012910810205011601f826ff07750c95020930093181061581257f75089501093881069501050c0a38028106c0c00643ff0a0202a101851175089513150026ff000902810009029100c0>
  "CountryCode" = 0
  "VendorID" = 1133
  "VersionNumber" = 19
  "IOServiceDEXTEntitlements" = ("com.apple.developer.driverkit.transport.hid")
  "IODEXTMatchCount" = 1
  "PrimaryUsage" = 6
  "ProductID" = 45091
  "ReportInterval" = 8000
  "PrimaryUsagePage" = 1
  "DeviceOpenedByEventSystem" = Yes
  "MaxFeatureReportSize" = 1
}
```

Please include in your bug report:

* **Transport**
* **Manufacturer**
* **Product**
* **VendorID**
* **ProductID**
* **ReportDescriptor**
* **VersionNumber**

### Linux

Use `lsusb -v` to output verbose information about USB devices, if your device
is USB.

The Linux kernel exposes the HID report descriptors for connected devices
through a debug filesystem interface. Find your device in the list:

```
sudo ls /sys/kernel/debug/hid
```

Directories in `/sys/kernel/debug/hid` represent HID interfaces on connected
devices. The name of each directory has the form `aaaa:bbbb:cccc.dddd` where:

* `aaaa` is the bus type (`0003` for USB, `0005` for Bluetooth),
* `bbbb` and `cccc` are the vendor and product IDs as 16-bit hexadecimal values, and
* `dddd` is a device index that increments for each newly-connected device.

The report descriptor is available as a file named `rdesc` inside the directory:

```
sudo cat /sys/kernel/debug/hid/0005:046D:B023.006F/rdesc
```

Please include the `lsusb -v` output, edited to only include information for
your device. Please also include `rdesc` for each HID interface exposed by the
device.
