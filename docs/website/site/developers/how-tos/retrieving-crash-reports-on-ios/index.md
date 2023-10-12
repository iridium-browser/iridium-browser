---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/how-tos
  - How-Tos
page_name: retrieving-crash-reports-on-ios
title: Retrieving Crash Reports on iOS
---

[TOC]

There are multiple ways to retrieve crash reports from an iOS device. They are
listed below from (roughly) easiest to hardest. Please use whichever method
works best for you.

## Syncing with iTunes

This method involves syncing your device with iTunes. After syncing, crash
reports will be copied to a specific location on your hard drive. This method is
also documented at
<https://developer.apple.com/library/ios/#qa/qa1747/_index.html>.

1.  Sync your device with iTunes on your desktop.
2.  After syncing, look for crash logs in the correct directory. See
            below for a list of directories for each operating system.
3.  In this directory, look for files starting with "Chrome_".

(NOTE FOR MAC USERS: ~/Library is hidden by default on Mac OS X. To easily get
this folder, open the Finder application, then hold the "option" key while
clicking on the "Go" menu. You should see a menu item for "Library." Click on
that, then continue navigating to Logs, CrashReporter, etc.

<table>
<tr>

Operating System

Location

</tr>
<tr>

<td>Mac OS X:</td>

<td>~/Library/Logs/CrashReporter/MobileDevice/&lt;DEVICE_NAME&gt;</td>

</tr>
<tr>

<td>Windows XP</td>

<td>C:\\Documents and Settings\\&lt;USERNAME&gt;\\Application Data\\Apple Computer\\Logs\\CrashReporter\\MobileDevice\\&lt;DEVICE_NAME&gt;</td>

</tr>
<tr>

<td>Windows Vista or 7</td>

<td>C:\\Users\\&lt;USERNAME&gt;\\AppData\\Roaming\\Apple Computer\\Logs\\CrashReporter\\MobileDevice\\&lt;DEVICE_NAME&gt;</td>

</tr>
</table>

## Emailing from a device

1.  Start by opening up the Settings app.
2.  Navigate to General -&gt; About -&gt; Diagnostics & Usage -&gt;
            Diagnostic & Usage Data.
3.  Select a Chrome crash from the list. This will start with “Chrome_”
            and contain the timestamp of the crash.
4.  Tap on the crash and you will see a text field with a crash log.
            Long press to Select All and then Copy the crash text.
5.  Paste it into something you can get off of your device (for example,
            an email to yourself).

## Using the Xcode Organizer

NOTE: This method will only work for iOS developers. If you are not a developer,
you will not have Xcode installed on your machine.

1.  Launch Xcode on your desktop machine.
2.  Open the Xcode Organizer window. (Window menu -&gt; Organizer, or
            Cmd-Shift-2.)
3.  Find your device in the left sidebar, then select “device logs”.
4.  Choose a Chrome crash (or multiple crashes) and select “Export” at
            the bottom of the Organizer window. This will copy the crash reports
            to your hard drive.
