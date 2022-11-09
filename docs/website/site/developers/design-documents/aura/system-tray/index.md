---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
- - /developers/design-documents/aura
  - Aura
page_name: system-tray
title: System Tray
---

## Summary

The system tray is part of the Ash status area. It displays system status and
settings, and expands into a menu with more information and settings controls.
Each system tray item may have any subset of the 4 view types:

*   **Tray view.** This is the view that shows in the status area, such
            as the clock or the Wi-Fi indicator image.
*   **Default view.** This is the view that appears in the list when the
            tray bubble is opened.
*   **Detailed view.** This view appears by itself. If the tray bubble
            is already open, the detailed view is displayed within the existing
            window for the tray bubble.
*   **Notification view.** Obsolete. The SMS notification is shown as a
            notification view. Other tray items use the message center or their
            detailed view to show notifications.

This document summarizes the behavior of each system tray item. Some system tray
items are only available while logged in.

## List of system tray items

<table>
<tr>
<td>System tray item</td>
<td>Tray view</td>
<td>Default view</td>
<td>Detailed view</td>
<td>Notifications</td>
<td>Chrome OS only</td>
<td>Visible in sign-in screen, lock screen, OOBE</td>
</tr>
<tr>
<td><b>ScreenCaptureTrayItem</b></td>
<td>Yes</td>
<td>Yes</td>
<td>No</td>
<td>Message center</td>
<td>Yes</td>
<td>No? </td>
</tr>
<tr>
<td><b>ScreenShareTrayItem</b></td>
<td>Yes</td>
<td>Yes</td>
<td>No</td>
<td>Message center</td>
<td>Yes</td>
<td>No? </td>
</tr>
<tr>
<td><b>TrayAccessibility</b></td>
<td>When accessible option is enabled</td>
<td>Yes, unless disabled in settings</td>
<td>Yes (from default view)</td>
<td>Custom popup</td>
<td>No (Windows, Linux)</td>
<td>Yes</td>
</tr>
<tr>
<td><b>TrayAudio</b></td>
<td>When muted</td>
<td>Yes</td>
<td>Pops up when volume changes</td>
<td>Detailed view popup</td>
<td>Yes</td>
</tr>
<tr>
<td><b>TrayAudioChromeOs</b></td>
<td>When muted</td>
<td>Yes</td>
<td>Pops up when volume changes</td>
<td>Detailed view popup</td>
<td>Yes</td>
<td>Yes</td>
</tr>
<tr>
<td><b>TrayAudioWin</b></td>
<td>No</td>
<td>Yes</td>
<td>Yes</td>
<td>No</td>
<td>No (Windows)</td>
<td>N/A</td>
</tr>
<tr>
<td><b>TrayBluetooth</b></td>
<td>No</td>
<td>Yes</td>
<td>Yes (from default view)</td>
<td>No</td>
<td>No (Linux)</td>
<td>Yes</td>
</tr>
<tr>
<td><b>TrayBrightness</b></td>
<td>No</td>
<td>In TouchView</td>
<td>Pops up when brightness changes</td>
<td>Detailed view popup</td>
<td>Yes </td>
<td>Yes</td>
</tr>
<tr>
<td><b>TrayCapsLock</b></td>
<td>When enabled</td>
<td>When enabled</td>
<td>Pops up when caps lock is enabled</td>
<td>Detailed view popup</td>
<td>Yes</td>
<td>Yes</td>
</tr>
<tr>
<td><b>TrayDate</b></td>
<td>Yes</td>
<td>Yes</td>
<td>No</td>
<td>No</td>
<td>No (Windows, Linux)</td>
<td>Yes</td>
</tr>
<tr>
<td><b>TrayDisplay</b></td>
<td>No</td>
<td>When display changes or is mirrored, extended or docked</td>
<td>No</td>
<td>Message center</td>
<td>Yes</td>
<td>Yes</td>
</tr>
<tr>
<td><b>TrayDrive</b></td>
<td>No</td>
<td>For the owner when waiting on Drive</td>
<td>Yes (from default view)</td>
<td>No</td>
<td>No (Linux)</td>
<td>No</td>
</tr>
<tr>
<td><b>TrayEnterprise</b></td>
<td>No</td>
<td>When device is managed (non-public user)</td>
<td>No</td>
<td>No</td>
<td>Yes </td>
<td>Yes</td>
</tr>
<tr>
<td><b>TrayIME</b></td>
<td>If multiple input methods are enabled</td>
<td>If multiple input methods are enabled</td>
<td>Yes (from default view)</td>
<td>No</td>
<td>No (Linux)</td>
<td>Yes</td>
</tr>
<tr>
<td><b>TrayLocallyManagedUser</b></td>
<td>No</td>
<td>For locally managed user</td>
<td>No</td>
<td>Message center</td>
<td>Yes</td>
<td>No</td>
</tr>
<tr>
<td><b>TrayNetwork</b></td>
<td>Sometimes</td>
<td>Yes, once initialized</td>
<td>Yes (from default view)</td>
<td>Detailed view popup or message center</td>
<td>Yes</td>
<td>Yes</td>
</tr>
<tr>
<td><b>TrayPower</b></td>
<td>If battery present</td>
<td>No</td>
<td>No (part of TraySettings)</td>
<td>Message center</td>
<td>Yes</td>
<td>Yes</td>
</tr>
<tr>
<td><b>TrayRotationLock</b></td>
<td>In TouchView</td>
<td>In TouchView</td>
<td>No</td>
<td>No</td>
<td>Yes</td>
<td>Yes</td>
</tr>
<tr>
<td><b>TraySessionLengthLimit</b></td>
<td>When limited</td>
<td>No</td>
<td>No</td>
<td>Message center</td>
<td>Yes</td>
<td>No</td>
</tr>
<tr>
<td><b>TraySettings</b></td>
<td>No</td>
<td>Yes</td>
<td>No</td>
<td>No</td>
<td>Yes</td>
<td>Sometimes</td>
</tr>
<tr>
<td><b>TraySms</b></td>
<td>When messages available</td>
<td>When messages available</td>
<td>When messages available</td>
<td>Notification view</td>
<td>Yes</td>
<td>?</td>
</tr>
<tr>
<td><b>TrayTracing</b></td>
<td>When tracing is enabled</td>
<td>When tracing is enabled</td>
<td>No</td>
<td>No</td>
<td>Yes</td>
<td>Yes</td>
</tr>
<tr>
<td><b>TrayUpdate</b></td>
<td>When upgrade is available</td>
<td>When upgrade is available</td>
<td>Yes (pops up periodically)</td>
<td>Detailed view popup</td>
<td>No (Windows, Linux)</td>
<td>Yes</td>
</tr>
<tr>
<td><b>TrayUser</b></td>
<td>Sometimes</td>
<td>Yes</td>
<td>Yes (new account management)</td>
<td>No</td>
<td>Yes</td>
<td>No </td>
</tr>
<tr>
<td><b>TrayUserSeparator</b></td>
<td>No</td>
<td>When multiple users logged in</td>
<td>No</td>
<td>No</td>
<td>Yes</td>
<td>No</td>
</tr>
<tr>
<td><b>TrayVPN</b></td>
<td>No</td>
<td>If VPN configured</td>
<td>Yes (from default view)</td>
<td>No</td>
<td>Yes</td>
<td>Lock screen</td>
</tr>
</table>
