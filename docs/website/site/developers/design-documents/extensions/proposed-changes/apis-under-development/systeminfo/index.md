---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
- - /developers/design-documents/extensions
  - Extensions
- - /developers/design-documents/extensions/proposed-changes
  - Proposed & Proposing New Changes
- - /developers/design-documents/extensions/proposed-changes/apis-under-development
  - API Proposals (New APIs Start Here)
page_name: systeminfo
title: SystemInfo Extension API
---

Intel Open Technology Center

[hongbo.min@intel.com,](mailto:hongbo.min@intel.com,)
[ningxin.hu@intel.com](mailto:ningxin.hu@intel.com),
[james.p.ketrenos@intel.com](mailto:james.p.ketrenos@intel.com)

## Overview

This API is intended to provide extension with a set of interfaces to get
hardware devices information, such as CPU and memory status, OS information,
disk storage and network state etc.

The followings are some typical usage cases of SystemInfo API but not limited to
these:

1.  An extension or packaged app can query the number of processors to
            determine the optimal number of WebWork to be created.
2.  An extension or packaged app can decide which binary should be
            downloaded and used according to the CPU architecture, e.g. NaCl
            module.
3.  An extension can check if there is enough storage space to a huge
            file before downloading it from Internet.
4.  An extension or packaged app can popup a media gallery app if the
            new storage attached is a MTP-compatible device.
5.  A packaged app is allowed to create app window on the external
            display with display information.

#### SystemInfo in W3C

The SysApps Working Group is working on defining a set of System-Level API for
web application, including [Device Capabilities
API](http://device-capabilities.sysapps.org/). The purpose is the same as
SystemInfo extension API which allows web app to access system information.

## Design and Considerations

### Namespace and Permission

The namespace for SystemInfo API is `chrome.system`, and currently has the
following categories:

*   chrome.system.cpu
*   chrome.system.storage
*   chrome.system.memory
*   chrome.system.display
*   …

Obviously, the `chrome.system` namespace can be easily extended in future if a
new system level API is introduced. The corresponding permission is also simple
and straightforward:

*   system.cpu
*   system.storage
*   system.memory
*   system.display

### Implementation Notes

The design goals of the new SystemInfo extension API introduction are:

1.  No need to pay the additional overhead if an extension or packaged
            app won't need to query system information at, e.g. no extra memory
            footprint at runtime.
2.  Efficiently handle too-frequent querying operation. Although it
            seems a corner case since few app will query the same information
            repeatly, we take it into consideration to avoid such an exception
            and make Chrome much robust.

A base class `SystemInfoProvider `is abstracted away for every system
information retrieval. It will:

1.  Maintain a callback queue for the incoming query request;
2.  Post the query operation to non-UI thread. The logic of how to query
            the requested information is implemented in each derived class of
            `SystemInfoProvider`;
3.  Forward the query result to each callback in the queue once the
            query is completed

In order to satisfy the minimal overhead required by design goal, each app using
the API won't pay for what it don't use, the lifetime of each concrete
`SystemInfoProvider `implementation is

1.  Created on demand. This mean the instance should be only created
            when it is needed
2.  Singleton instance. All apps using this API can share one
            InfoProvider.

Currently, we have `CpuInfoProvider `for `system.cpu` API set implementation,
`StorageInfoProvider `for `system.display`, `DisplayInfoProvider `for
`system.display`, and `MemoryInfoProvider `for `system.memory`.

## Extension API IDL

### CPU Info API

` dictionary CpuInfo {`
` // The number of logical processors.`
` long numOfProcessors;`
` // The architecture name of the processors.`
` DOMString archName;`
` // The model name of the processors.`
` DOMString modelName;`
` };`
` callback CpuInfoCallback = void (CpuInfo info);`
` interface Functions {`
` // Queries basic CPU information of the system.`
` static void getInfo(CpuInfoCallback callback);`
` };`

### Storage Info API:

`enum StorageUnitType {`
` // The storage has fixed media, e.g. hard disk or SSD.`
` fixed,`
` // The storage is removable, e.g. USB flash drive.`
` removable,`
` // The storage type is unknown.`
` unknown`
` };`
` dictionary StorageUnitInfo {`
` // The unique storage id. It will use the transient ID.`
` DOMString id;`
` // The name of the storage unit.`
` DOMString name;`
` // The media type of the storage unit.`
` StorageUnitType type;`
` // The total amount of the storage space, in bytes.`
` // Default value is 0 if query operation fails.`
` double capacity;`
` };`
` enum EjectDeviceResultCode {`
` // The ejection command is successful -- the application can prompt the user`
` // to remove the device.`
` success,`
` // The device is in use by another application. The ejection did not`
` // succeed; the user should not remove the device until the other`
` // application is done with the device.`
` in_use,`
` // There is no such device known.`
` no_such_device,`
` // The ejection command failed.`
` failure`
` };`
` callback StorageInfoCallback = void (StorageUnitInfo[] info);`
` callback EjectDeviceCallback = void (EjectDeviceResultCode result);`

` callback GetAvailableCapabilityCallback = void (double availableCapability);`

` interface Functions {`
` // Get the storage information from the system. The argument passed to the`
` // callback is an array of StorageUnitInfo objects.`
` static void getInfo(StorageInfoCallback callback);`
` // Ejects a removable storage device.`
` // Note: We plan to move this function into a namespace that indicates it`
` // that modifies the state of the system rather than just gathering`
` // information.`
` static void ejectDevice(DOMString id, EjectDeviceCallback callback);`
` };`
` interface Events {`
` // Fired when a new removable storage is attached to the system.`
` static void onAttached(StorageUnitInfo info);`
` // Fired when a removable storage is detached from the system.`
` static void onDetached(DOMString id);`

` // Get the available capability, in bytes`
` static void getAvailableCapacity(DOMString id, GetAvailableCapabilityCallback
callback);`

` };`

### Memory Info API

` dictionary MemoryInfo {`
` // The total amount of physical memory capacity, in bytes.`
` double capacity;`
` // The amount of available capacity, in bytes.`
` double availableCapacity;`
` };`
` callback MemoryInfoCallback = void (MemoryInfo info);`
` interface Functions {`
` // Get physical memory information.`
` static void getInfo(MemoryInfoCallback callback);`
` };`

### Display Info API

` dictionary Bounds {`
` // The x-coordinate of the upper-left corner.`
` long left;`
` // The y-coordinate of the upper-left corner.`
` long top;`
` // The width of the display in pixels.`
` long width;`
` // The height of the display in pixels.`
` long height;`
` };`
` dictionary Insets {`
` // The x-axis distance from the left bound.`
` long left;`
` // The y-axis distance from the top bound.`
` long top;`
` // The x-axis distance from the right bound.`
` long right;`
` // The y-axis distance from the bottom bound.`
` long bottom;`
` };`
` dictionary DisplayUnitInfo {`
` // The unique identifier of the display.`
` DOMString id;`
` // The user-friendly name (e.g. "HP LCD monitor").`
` DOMString name;`
` // Identifier of the display that is being mirrored on the display unit.`
` // If mirroring is not in progress, set to an empty string.`
` // Currently exposed only on ChromeOS. Will be empty string on other`
` // platforms.`
` DOMString mirroringSourceId;`
` // True if this is the primary display.`
` boolean isPrimary;`
` // True if this is an internal display.`
` boolean isInternal;`
` // True if this display is enabled.`
` boolean isEnabled;`
` // The number of pixels per inch along the x-axis.`
` double dpiX;`
` // The number of pixels per inch along the y-axis.`
` double dpiY;`
` // The display's clockwise rotation in degrees relative to the vertical`
` // position.`
` // Currently exposed only on ChromeOS. Will be set to 0 on other platforms.`
` long rotation;`
` // The display's logical bounds.`
` Bounds bounds;`
` // The display's insets within its screen's bounds.`
` // Currently exposed only on ChromeOS. Will be set to empty insets on`
` // other platforms.`
` Insets overscan;`
` // The usable work area of the display within the display bounds. The work`
` // area excludes areas of the display reserved for OS, for example taskbar`
` // and launcher.`
` Bounds workArea;`
` };`
` dictionary DisplayProperties {`
` // If set and not empty, starts mirroring between this and the display with`
` // the provided id (the system will determine which of the displays is`
` // actually mirrored).`
` // If set and not empty, stops mirroring between this and the display with`
` // the specified id (if mirroring is in progress).`
` // If set, no other parameter may be set.`
` DOMString? mirroringSourceId;`
` // If set to true, makes the display primary. No-op if set to false.`
` boolean? isPrimary;`
` // If set, sets the display's overscan insets to the provided values. Note`
` // that overscan values may not be negative or larger than a half of the`
` // screen's size. Overscan cannot be changed on the internal monitor.`
` // It's applied after <code>isPrimary</code> parameter.`
` Insets? overscan;`
` // If set, updates the display's rotation.`
` // Legal values are [0, 90, 180, 270]. The rotation is set clockwise,`
` // relative to the display's vertical position.`
` // It's applied after <code>overscan</code> paramter.`
` long? rotation;`
` // If set, updates the display's logical bounds origin along x-axis. Applied`
` // together with <code>boundsOriginY</code>, if <code>boundsOriginY</code>`
` // is set. Note that, when updating the display origin, some constraints`
` // will be applied, so the final bounds origin may be different than the one`
` // set. The final bounds can be retrieved using $ref:getInfo.`
` // The bounds origin is applied after <code>rotation</code>.`
` // The bounds origin cannot be changed on the primary display. Note that is`
` // also invalid to set bounds origin values if <code>isPrimary</code> is`
` // also set (as <code>isPrimary</code> parameter is applied first).`
` long? boundsOriginX;`
` // If set, updates the display's logical bounds origin along y-axis.`
` // See documentation for <code>boundsOriginX</code> parameter.`
` long? boundsOriginY;`
` };`
` callback DisplayInfoCallback = void (DisplayUnitInfo[] displayInfo);`
` callback SetDisplayUnitInfoCallback = void();`
` interface Functions {`
` // Get the information of all attached display devices.`
` static void getInfo(DisplayInfoCallback callback);`
` // Updates the properties for the display specified by |id|, according to`
` // the information provided in |info|. On failure, $ref:runtime.lastError`
` // will be set.`
` // |id|: The display's unique identifier.`
` // |info|: The information about display properties that should be changed.`
` // A property will be changed only if a new value for it is specified in`
` // |info|.`
` // |callback|: Empty function called when the function finishes. To find out`
` // whether the function succeeded, $ref:runtime.lastError should be`
` // queried.`
` static void setDisplayProperties(`
` DOMString id,`
` DisplayProperties info,`
` optional SetDisplayUnitInfoCallback callback);`
` };`
` interface Events {`
` // Fired when anything changes to the display configuration.`
` static void onDisplayChanged();`
` };`
