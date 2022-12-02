---
breadcrumbs:
- - /for-testers
  - For Testers
- - /for-testers/installer
  - Installer (test plan)
page_name: installer--basic-testing
title: 'Installer: Basic Testing'
---

[TOC]

## Test cases

### Install

<table>
<tr>
Test case Steps Expected Result </tr>
<tr>
<td>Fresh Install</td>
<td>Install latest build of Google Chrome.</td>

*   <td>On completion of installation, First Run UI dialog will be
            launched with no other alert prompts </td>
*   <td>First Run UI dialog: Shortcuts will be created according to the
            user's choice. Bookmarks, passwords, and other settings will be
            imported/not imported from IE/FF as per user's choice. </td>
*   <td>A new folder will be created:</td>
    <td><b>XP</b>: C:\\Documents and Settings\\username\\Local
    Settings\\Application Data\\Google\\Google Chrome\\Application</td>
    <td><b>Vista</b>: C:\\Users\\username\\AppData\\Local\\Google\\Google
    Chrome\\Application</td>
*   <td>Google Chrome can be launched from shortcuts and the Add/Remove
            panel (XP) / Programs and Features (Vista)</td>

</tr>
<tr>
<td>Recovery </td>

<td>If Google Chrome files are accidentally deleted and Google Chrome cannot be launched, run the installer again.</td>

*   <td>On completion of installation, Google Chrome can be launched
            from shortcuts, Add/Remove panel (XP) / Programs and Features
            (Vista)</td>

</tr>
</table>

### Overinstall

<table>
<tr>
Test case Steps Expected Result </tr>
<tr>
<td>Overinstall</td>
<td>Overinstall latest build of Google Chrome </td>

*   <td>On launching Google Chrome, <b>about:version</b> should show the
            latest build number.</td>
*   <td>Google Chrome can be launched from shortcuts, Add/Remove panel
            (XP) / Programs and Features (Vista)</td>

</tr>
</table>

### First Run UI

<table>
<tr>
Test case Steps Expected Result </tr>
<tr>
<td>First Run UI dialog launch</td>
<td>Do a fresh install of Google Chrome. (Note: First Run UI will not be launched if it is not a fresh install)</td>

*   <td>On completion of installation, First Run UI dialog will be
            launched giving the user choice to customize the settings.</td>
*   <td>A file called <b>First Run</b> will be created under:</td>
    <td><b>XP</b>: C:\\Documents and Settings\\username\\Local
    Settings\\Application Data\\Google\\Google Chrome\\Application</td>
    <td><b>Vista</b>: C:\\Users\\username\\AppData\\Local\\Google\\Google
    Chrome\\Application</td>

</tr>
<tr>
<td>First Run UI file not found </td>
<td>Delete First Run file.</td>

*   <td>On Google Chrome launch, First Run UI dialog will be launched
            </td>

</tr>
</table>

### Uninstall

<table>
<tr>
Test case Steps Expected Result </tr>
<tr>
<td>Uninstall Google Chrome </td>
<td>Click <b>Start > All Programs > Google Chrome</b> and select <b>Uninstall</b></td>
<td>or</td>
<td>Windows XP: From the <b>Add/Remove Programs</b> dialog, uninstall Google Chrome.</td>
<td>Windows Vista: From the <b>Programs and Features</b> window, uninstall Google Chrome.</td>

<td>Verify the following on your local machine:</td>

*   <td>Google Chrome is uninstalled with no issues. </td>
*   <td>The Google Chrome folder is removed from:</td>
    <td><b>XP</b>: C:\\Documents and Settings\\user\\Local Settings\\Application
    Data\\Google\\Google Chrome\\Application\\</td>
    <td><b>Vista</b>: C:\\Users\\username\\AppData\\Local\\Google\\Google
    Chrome\\Application</td>
*   <td>Google Chrome will not appear in the <b>Add/Remove</b> panel on
            XP, or the <b>Programs and Features</b> window in Vista.</td>
*   <td>Shortcuts are deleted.</td>

</tr>
<tr>
<td>Uninstall Google Chrome while Google Chrome is running </td>

1.  <td>Launch Google Chrome.               </td>
2.  <td>Try to uninstall Google Chrome. </td>

<td>The following message will appear:</td>

<td><b>Please close all Google Chrome windows and try again</b></td>

<td>Google Chrome will not be uninstalled. 	</td>

<td>On closing Google Chrome and uninstalling, Google Chrome will uninstall with no issues.</td>

</tr>
</table>
