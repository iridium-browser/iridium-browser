---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/chromiumos-design-docs
  - Design Documents
page_name: system-notifications
title: system notifications
---

## Summary

ChromeOS has several types of system status and some of them should be notified
to the user. This document summarizes which types of notifications are currently
used and what kind of UI flow is necessary here.

The notifications are usually generated through the internal C++ API for the
[desktop
notifications](/developers/design-documents/extensions/proposed-changes/apis-under-development/desktop-notification-api),
which will appear as the [rich
notification](http://blog.chromium.org/2013/05/rich-notifications-in-chrome.html)
as well.

## Blocking and system notifications

Sometimes normal notifications should be blocked due to the current system
status. For example, all notifications should be queued during the lock screen,
and they should appear when unlocked. It also should be blocked when fullscreen
state; notifications should be annoying when the user watches YouTube videos, or
has a presentation (note: the notifications **should** appear in [immersive
fullscreen](/developers/design-documents/immersive-fullscreen) mode instead,
because the shelf / message center UI is visible in such case).

However, the system status could be so important that they should appear in such
cases. This does not mean all system notifications should appear; some could be
misleading. For example, the notification for taking screenshot would behave as
same as the normal notifications, so it should not appear in the lock screen. In
addition, screenshot notification has the message to 'click to view' but click
should not work in lock screen. On the other hand, connecting display would be
okay to be notified even in the lock screen, it just notifies the new system
status.

See also the [notification blocking code
update](https://docs.google.com/document/d/1Ox0Gb659lE2eusk-Gwm-a_JkARva7LydQc3hZNJvDn0/edit?usp=sharing)
design doc.

## List of the system notifications

<table>
<tr>
<td>message center?</td>
<td>Component</td>
<td>Source</td>
<td>Message</td>
<td>Show Always? ("System")</td>
<td>Timeout</td>
<td>Secure? (Show on Lock Screen)</td>
<td>Customize?</td>
<td>(can be disabled)</td>
<td>Click Action</td>
<td>Button</td>
<td>Triggers</td>
</tr>
<tr>
<td>Yes</td>
<td>display</td>
<td>DisplayErrorObserver</td>
<td>Could not mirror displays...</td>
<td>No</td>
<td>Yes</td>
<td>Yes</td>
<td>No</td>
<td>None</td>
<td>display connection</td>
</tr>
<tr>
<td>Yes</td>
<td>display</td>
<td>DisplayErrorObserver</td>
<td>That monitor is not supported</td>
<td>No</td>
<td>Yes</td>
<td>Yes</td>
<td>No</td>
<td>None</td>
<td>display connection</td>
</tr>
<tr>
<td>Yes</td>
<td>display</td>
<td>ResolutionNotificationController</td>
<td>..resolution was changed to...</td>
<td>Yes</td>
<td>Both</td>
<td>No</td>
<td>No</td>
<td>Accept / Revert change</td>
<td>Accept / Revert change</td>
<td>settings change from chrome://settings/display</td>
</tr>
<tr>
<td>Yes</td>
<td>display</td>
<td>ScreenCaptureTrayItem</td>
<td>... is sharing your screen</td>
<td>Yes</td>
<td>No</td>
<td>No</td>
<td>None</td>
<td>Stop Capture</td>
<td>apps</td>
</tr>
<tr>
<td>Yes</td>
<td>display</td>
<td>ScreenShareTrayItem</td>
<td>Sharing control of your screen...</td>
<td>Yes</td>
<td>No</td>
<td>No</td>
<td>None</td>
<td>Stop Share</td>
<td>apps</td>
</tr>
<tr>
<td>Yes</td>
<td>display</td>
<td>ScreenshotTaker</td>
<td>Screenshot taken</td>
<td>No</td>
<td>Yes</td>
<td>No</td>
<td>Yes</td>
<td>Show FIles App</td>
<td>keyboard shortcut</td>
</tr>
<tr>
<td>Yes</td>
<td>display</td>
<td>TrayDisplay</td>
<td>Mirroring to...</td>
<td>No</td>
<td>Yes</td>
<td>Yes</td>
<td>No</td>
<td>Show Display Settings</td>
<td>settings change from chrome://settings/display, and keyboard shortcut</td>
</tr>
<tr>
<td>Yes</td>
<td>display</td>
<td>TrayDisplay</td>
<td>Extending screen to...</td>
<td>No</td>
<td>Yes</td>
<td>Yes</td>
<td>No</td>
<td>Show Display Settings</td>
<td>settings change from chrome://settings/display, and keyboard shortcut</td>
</tr>
<tr>
<td>Yes</td>
<td>display</td>
<td>TrayDisplay</td>
<td>Docked mode</td>
<td>No</td>
<td>Yes</td>
<td>Yes</td>
<td>No</td>
<td>Show Display Settings</td>
<td>close lid</td>
</tr>
<tr>
<td>Yes</td>
<td>language</td>
<td>LocaleNotificationController</td>
<td>The language has changed from...</td>
<td>No</td>
<td>No</td>
<td>No</td>
<td>No</td>
<td>Accept change</td>
<td>Revert change</td>
<td>login</td>
</tr>
<tr>
<td>No</td>
<td>language</td>
<td>TrayAccessability</td>
<td>Spoken feedback is enabled.</td>
<td>\*</td>
<td>Yes</td>
<td>No</td>
<td>None</td>
<td>keyboard shortcut</td>
</tr>
<tr>
<td>No</td>
<td>language</td>
<td>TrayCapsLock</td>
<td>CAPS LOCK is on</td>
<td>\*</td>
<td>Yes</td>
<td>No</td>
<td>None</td>
<td>keyboard shortcut</td>
</tr>
<tr>
<td>Yes</td>
<td>language</td>
<td>TrayIME</td>
<td>Your input method has changed...</td>
<td>No</td>
<td>Yes</td>
<td>No</td>
<td>No</td>
<td>Show IME detailed view</td>
<td>keyboard shortcut</td>
</tr>
<tr>
<td>Yes</td>
<td>network</td>
<td>DataPromoNotification</td>
<td>Google Chrome will use mobile data...</td>
<td>Yes</td>
<td>No</td>
<td>?</td>
<td>No</td>
<td>Promo URL or settings</td>
</tr>
<tr>
<td>Yes</td>
<td>network</td>
<td>network_connect</td>
<td>Activation of... requires a network connection</td>
<td>Yes</td>
<td>No</td>
<td>?</td>
<td>No</td>
<td>Show Settings</td>
</tr>
<tr>
<td>Yes</td>
<td>network</td>
<td>NetworkStateNotifier</td>
<td>...Your .. service has been activated</td>
<td>Yes</td>
<td>No</td>
<td>?</td>
<td>No</td>
<td>Show Settings</td>
</tr>
<tr>
<td>Yes</td>
<td>network</td>
<td>NetworkStateNotifier</td>
<td>You may have used up your mobile data...</td>
<td>Yes</td>
<td>No</td>
<td>?</td>
<td>No</td>
<td>Configure Network</td>
</tr>
<tr>
<td>Yes</td>
<td>network</td>
<td>NetworkStateNotifier</td>
<td>Failed to connect to network...</td>
<td>Yes</td>
<td>No</td>
<td>?</td>
<td>No</td>
<td>Show Settings</td>
</tr>
<tr>
<td>No</td>
<td>network</td>
<td>TraySms</td>
<td>SMS from ...</td>
<td>\*</td>
<td>No</td>
<td>No</td>
<td>Show SMS detailed view</td>
</tr>
<tr>
<td>No</td>
<td>power</td>
<td>TrayPower</td>
<td>Battery full OR %&lt;X&gt; remaining</td>
<td>\*</td>
<td>No / Never</td>
<td>Yes</td>
<td>No</td>
<td>None</td>
<td>low battery</td>
</tr>
<tr>
<td>Yes</td>
<td>power</td>
<td>TrayPower</td>
<td>Your Chromebook may not charge...</td>
<td>No</td>
<td>Yes</td>
<td>Yes</td>
<td>No</td>
<td>None</td>
<td>usb charger connected</td>
</tr>
<tr>
<td>Yes</td>
<td>user</td>
<td>TrayLocallyManagedUser</td>
<td>This user is supervised by...</td>
<td>Yes</td>
<td>No</td>
<td>No</td>
<td>No</td>
<td>None</td>
<td>login</td>
</tr>
<tr>
<td>Yes</td>
<td>user</td>
<td>TraySessionLengthLimit</td>
<td>This session will end in...</td>
<td>Yes</td>
<td>No</td>
<td>No</td>
<td>No</td>
<td>None</td>
<td>timeout set by policy / sync / remaining time change</td>
</tr>
<tr>
<td>No</td>
<td>user</td>
<td>TrayUser</td>
<td>You can only have up to three accounts in multiple sign-in</td>
<td>\*</td>
<td>\*</td>
<td>Yes</td>
<td>No</td>
<td>None</td>
</tr>
</table>
