---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/testing
  - Testing Home
- - /chromium-os/testing/moblab
  - MobLab
page_name: releasenotes
title: MobLab Release Notes
---

# The latest recovery images for Moblab:

*   [Wukong (fizz) OS Recovery
            Image](https://storage.googleapis.com/moblab-recovery-images/chromeos_14526.55.0_fizz-moblab_recovery_stable-channel_mp.bin.zip)
*   [Wyvern (puff) OS Recovery
            Image](https://storage.googleapis.com/moblab-recovery-images/chromeos_14526.55.0_puff-moblab_recovery_stable-channel_mp-v3.bin.zip)

# June 15th 2023: Moblab Release

**You should reboot Moblab when tests are not running to get this latest version of the software.**
After the update you will be running Moblab version R-2.23.3 

**Highlights**
- Surface the ChromeOS builds recommended by the SPERA team for Performance_CUJ test suite

# March 15th 2023: Moblab Release

**You should reboot Moblab when tests are not running to get this latest version of the software.**
After the update you will be running Moblab version R-2.22.3 

**Highlights**
- Moblab is ready for the upcoming change in the RSA key for ChromeOS builds. ChromeOS builds will be switching to using a new private RSA key that is accessible to only Google and Partners. (Only ChromiumOS builds will have the public RSA key).
- New FWUPD test suite introduced in the experimental section. This test suite would be used by peripheral OEMs for testing peripheral firmware upgrade process.

# January 28th 2023: Moblab Release

**You should reboot Moblab when tests are not running to get this latest version of the software.**
After the update you will be running Moblab version R-2.21.1

**Highlights**
1. New sub-tabs in the "View Jobs" section to filter the jobs based on job status
2. Better display of any authentication issues encountered while accessing a DUT
3. Removing unused test suites (PVS)

# November 28th 2022: Moblab Release
**You should reboot Moblab when tests are not running to get this latest version of the software.**
After the update you will be running Moblab version R-2.20.2

**Highlights**
- Fix to run suite on model specified instead of just the board specified. 
- Improvements to usability of DUT management page

# November 15th 2022: Moblab Patch Release
**You should reboot your Moblab when next not running tests to get this latest version of the software.**

After the update you will be running Moblab version R-2.19.2. 

**Highlights**
- Resolves the issue of: DUTs ending in repair_failed state

# September 30th 2022: Moblab Patch Release
**You should reboot your Moblab when next not running tests to get this latest version of the software.**

After the update you will be running Moblab version R-2.19.1. 

**Highlights**
- Removable storage qual suite now only requires 1 DUT to run

# September 23rd 2022: Moblab Release
**You should reboot your Moblab when next not running tests to get this latest version of the software.**

After the update you will be running Moblab version R-2.19.0. 

**Highlights**
- Added a CUJ tab to the Run Suite tab
- Added back removable storage qual suite
- Allow Moblab to check and save valid OS Override versions

# September 2nd 2022: Moblab Release
**You should reboot your Moblab when next not running tests to get this latest version of the software.**

After the update you will be running Moblab version R-2.18.5. 

**Highlights**
- Removing legacy Storage Qual from the Run Suite tab
- Adding functionality to run FAFT test (instructions: [here](https://docs.google.com/document/d/e/2PACX-1vR1lFFkzgWmx7gMV9o5bffs_WgNWx4N_ceAx-d-np30TG-KkbM9UYXrBZlA0c6HfGhtb2QgudHN0P3r/pub?urp=gmail_link))


# July 21st 2022: Moblab Patch Release
**You should reboot your Moblab when next not running tests to get this latest version of the software.**

After the update you will be running Moblab version R-2.17.3 (from previous R-2.17.2)

**Highlights**
This patch fixes an issue with provisioning by upgrading the devserver component and resolving a CTS test failure. 

# June 24th 2022: Moblab Update
**You should reboot your Moblab when next not running tests to get this latest version of the software.**

After the update you will be running Moblab version R-2.17.2

**Highlights**
This release mainly focussed on UI improvements: 
- Fix column sorting on Manage DUTs page
- Display "Moblab Uptime" on top-right corner for quick access to this data
- Add "Moblab Start Time" and some Useful links to the About page

# May 10th 2022: Moblab Update
**You should reboot your Moblab when next not running tests to get this latest version of the software.**

You should reboot your Moblab to get the latest version of the software. Make
sure no tests are running or scheduled. This update only includes an update to
the Moblab software; after the update you will be running Moblab version
R-2.16.4.


**Highlights** 
* Support for provisioning a locally built custom OS image on a Moblab DUT (instructions: [here](https://docs.google.com/document/u/3/d/e/2PACX-1vRxmVGH43FnQx_2lbyUssQZfmxjsRn6-X7RCvI-BYzNjE9-CcO0ZSLICRRGylzHyk2Cg-Vup2VNcv0p/pub?urp=gmail_link))
* Storage Qual test improvements for pre-qualified components
* Robustifying the moblab start up

# Apr 22nd 2022: Moblab Update
**You should reboot your Moblab when next not running tests to get this latest version of the software.**

You should reboot your Moblab to get the latest version of the software. Make
sure no tests are running or scheduled. This update only includes an update to
the Moblab software; after the update you will be running Moblab version
R-2.15.2.

**Highlights**

*   Fix for running tests on builds later than R102-14667.0.0


# Mar 14th 2022: Moblab Update
**You should reboot your Moblab when next not running tests to get this latest version of the software.**

You should reboot your Moblab to get the latest version of the software. Make
sure no tests are running or scheduled. This update includes an update to
the Moblab software and an update to the underlying ChromeOS version
(ChromeOS version update will roll out gradually starting the week of March 14th);
after the update you will be running Moblab version R-2.15.0.

**Highlights**

*   Fix for running tests based on Python 3

*   New functionality to copy a build to the partner bucket in the Configuration page

*   Other bug fixes and improvements

**UI - Manage DUTs**

*   Now trimming leading/trailing spaces from the key, value, pool, and label inputs

*   Renamed Name column to DUT

**UI - DUT Detail**

*   Removed the obsolete Name field

**UI - Update modal**

*   Fixed wording to point to the About page for updates

**UI - MobMonitor**

*   Fixed the link shown when sending logs

**UI - Configuration**

* Copy build button on the Configuration page will allow you to copy a build to the partner bucket

**Other**

*   Better handling of aborted jobs on Moblab restart

*   Fix for test migration to Python 3


# Dec 13th 2021: Moblab Update
**You should reboot your Moblab when next not running tests to get this latest version of the software.**

**You should reboot your Moblab to get the latest version of the software. Make
sure no tests are running or scheduled. This update only includes an update to
the Moblab software; after the update you will be running Moblab version
R-2.14.0.**

**Highlights**

*   Added new tab for running PVS test-suites

**UI - DUT Detail**

*   Made the IP Address field visible on DUT detail page

**UI - Run Suite**

*   Added new tab for PVS test-suites

**UI - Job Detail**

*   Make the ID field visible

**Other**

*   lucifer: update dockerfile to use go modules

# Nov 4th 2021: Moblab Update

**You should reboot your Moblab when next not running tests to get this latest version of the software.**

You should reboot your Moblab to get the latest version of the software.
*Make sure no tests are running or scheduled*. This update only includes an
update to the Moblab software; after the update you will be running Moblab
version R-2.13.0.

**Highlights**

*   Added ability to access test images from the Moblab UI;

**UI - Manage DUTs**

*   When connected DUT does not have test image the user will be given a link to copy a test image into the partner's bucket.

**Other**

*   Various bug fixes and improvements.

# Sep 30th 2021: Moblab Update

**You should reboot your Moblab when next not running tests to get this latest version of the software.**

This update includes an update to the underlying moblab ChromeOS version;
after the update you will be running Chrome OS version R93-14092.55.0 and Moblab
version R-2.12.3.

[OS Recovery Image](https://storage.googleapis.com/moblab-recovery-images/chromeos_14150.88.0_fizz-moblab_recovery_stable-channel_mp.bin.zip), in the case you need to recover your Moblab.

**Highlights**

*   User notified of missing permissions for a board when new DUTs added;
*   New chrome OS version R93-14092.71.0;
*   Various improvements;

**UI - Manage DUTs**

*   Banner will indicate if the account used on the Moblab is missing permissions to access builds for any newly enrolled DUT.

**UI - Run Suite**

*   Storage qualification is now required to be run inside a pool.

*   A UI widget was added to Build drop down to give more info about not available builds.

**UI - View Jobs**

*   Result logs link will point to the cloud bucket location after the deletion from the Moblab.

**UI - Configuration**

*   Information fields have moved to the newly added About page.

*   The Serial Number field was renamed to Moblab Install Id to be consistent with CPCon.

*   The page now shows the current stable version per build targets and allows setting a global override for all build targets:

<img alt="image"
src="https://lh6.googleusercontent.com/0QQfJtBn_oRsZdpvi85wzscyATuICRgnNhrYgyFh6_kcomH_MUyqPGndjLCiwnEBifUyGyDcSIrjeqvtPuOtE1m0gb751LvCfQ-O4B3PXj5pZ7WCA3Drw5miuVARp6msw38prx7BEw=s0"
height=385 width=624>

**UI - About**

*   The MAC address of the external network interface can be found on the configuration page.

*   CPU temperature is now displayed on the About page. Also Moblab sends the temperature back to CPCon if the remote console feature is enabled.

**Other**

Various bug fixes and improvements.

# Aug 17th 2021: Moblab Update

**You should reboot your Moblab when next not running tests to get this latest version of the software.**

After the update you will be running Moblab version R-2.11.4.

**Highlights**

*   Beta support for servo and FAFT - will be testing with selected
            partners until full general release.

*   CTS suite names changed when running CTS suite. Rather than cts_N
            and cts_P there is just cts and cts_hardware.
    *   Most users can just select cts, there should be no difference in
                functionality.
    *   cts_hardware currently runs no tests, when the Google CTS test
                team roll out that test suite they will contact the affected
                users directly.
*   Split the configuration page into Configuration / About. The version
            and update buttons are now on the About navigation tab.
*   Added an advanced section to configuration navigation, in general
            you should only make changes in Advanced if Google asks you to.

# Aug 10th 2021: Moblab Update

**You should reboot your Moblab when next not running tests to get this latest version of the software.**

After the update you will be running Moblab version R-2.10.4.

This is for a small bug fix that only affects users who are running CTS on
builds newer than R93. If you are not

running CTS on R93 or newer you do not need to update.

The CL that was added is
https://chromium-review.googlesource.com/c/chromiumos/platform/moblab/+/3086906

To fix an issue where test CtsSecurityHostTestCases did not pass with an error message:
bash: ./sepolicy-analyze: Permission denied

# July 20th 2021: Moblab Update

**You should reboot your Moblab when next not running tests to get this latest version of the software.**

This update includes an update to the underlying moblab ChromeOS version; after
the update you will be running Chrome OS version R91-13904.81.0 and Moblab
version R-2.10.3.

[OS Recovery Image](https://storage.googleapis.com/moblab-recovery-images/chromeos_13904.81.0_fizz-moblab_recovery_stable-channel_mp.bin.zip), in the case you need to recover your Moblab.

**Highlights**

*   User controlled result retention period

*   Moblab update notification on UI;

**UI - Configuration**

User controlled result retention period. New configuration parameter is
added to allow users control the retention period of the results locally on
the Moblab. User can set value between 1 hour and 7 days. If results are
fully uploaded to the GC bucket, after the retention period the results are
deleted from the Moblab.

Other

*   Icon added on the toolbar to let the user know that there's an
            update available.
*   Also warnings added on the run suite card alerting the user of pending
            updates.

*   Various bug fixes and improvements.

# June 21th 2021: Moblab Update

**You should reboot your Moblab when next not running tests to get this latest version of the software.**

After the update you will be running Moblab version R-2.9.0.

**Highlights**

*   Auto refresh feature on view jobs page;

*   Added CUJ and MTBF suites;

**UI - View jobs**

*   Add the ability to sort the jobs table.

*   Added auto refresh feature on view jobs page. Which is usef

*   Add jobs status filter for FAILED/ABORTED/SUCCEEDED in the jobs table.

**UI - Run Suite**

*   Added additional performance CUJ and MTBF suite names to the drop down list of 'other' suites.

**Other**

*   Fixed the bug when a new error message will auto dismiss the previous one.

*   Fix the container crash without a service account.

*   Various bug fixes and improvements.

# May 17th 2021: Moblab Update

**You should reboot your Moblab when next not running tests to get this latest version of the software.**

This update includes an update to the underlying moblab ChromeOS version; after
the update you will be running Chrome OS version R90-13816.75.0 and Moblab
version R-2.8.2.

[OS Recovery Image](https://storage.googleapis.com/moblab-recovery-images/chromeos_13816.53.0_fizz-moblab_recovery_stable-channel_mp.bin.zip), in the case you need to recover your Moblab.

**Highlights**

*   Moblab loading page on startup is added;

*   Cloud configuration setup is required;

*   Bad builds are disabled in the build version picker drop-down on Run Suite page;

*   New chrome OS version R90-13816.75.0;

**Startup page**

*   Moblab now has a startup page. It is helpful in cases when loading takes a
            significant amount of time,for example the first boot after powerwash.

            The page will show the loading bar while the first component loads:
            <img alt="image"
            src="https://lh6.googleusercontent.com/UbCrZzPRK6aZjTEEaGF6wYph80rfQ8ETbRFncuOuiD3euy29f2xfuTB4ii78tgk4W4fu12Lu4XG09xYz8AKri8jkFfuPsiQ9alqvfjogdAmPnLtp47XRkyEgJ5MicjZzU5nQkuzu9AaBWJNF6qxo_GceRAyKVcIHpkbL6vnvxuSt33NF"
            height=205 width=624>

            And the status of all the components after that:<img alt="image"
            src="https://lh4.googleusercontent.com/ts0kVxdAp8rrmblexWHW5XmaOo1cJQGXuxLMZ4Pkhr9bLE3AqZUOLf0A2yUoNb0F2xZwGsM04Pd5_jq7Mxx0F1vhbIXOWSa5RuMSaalNxLZ_y5QCUN8Mi9ldkYdG7Oxd0vu7OQEkVRMzRv-kq68lIOMcbuBSesg-ZQktUj6hEbntI3Bk"
            height=432 width=624>

            When all the components are up and running the page will automatically
            redirect to the main page;

**UI - Manage DUTs**

Fix the page not refreshing when labels or attributes were adjusted. Now
whenever label is added or removed the DUTs grid will reflect the change
immediately;

**UI - Run Suite**

On the Run Suite page when selecting a build, the drop-down will show grayed
out build versions for builds that are known to be bad. User won’t be able
to select the build;

**UI - View Jobs**

Filter by name on view jobs page is now auto trimming white space around the
input keyword and performs partial name search;

**UI - Configuration**

The user will be permanently redirected to the Config page until the cloud
configuration is set.

Input validation added to the user override in Known Good OS Version
section;

Fix the WiFi credentials not updating

Removed warning icon on config page. Users were mislead by it assuming
something is wrong with the setup;

Internet connection status indicator fixed;

**Other**

Various bug fixes and improvements.

# April 20th 2021: Moblab Update

**You should reboot your Moblab when next not running tests to get this latest version of the software.**

This update includes an update to the underlying Moblab Chrome OS version; after
the update you will be running Chrome OS version R90-13816.53.0 and Moblab
version R-2.7.3.

[OS Recovery Image](https://storage.googleapis.com/moblab-recovery-images/chromeos_13816.53.0_fizz-moblab_recovery_stable-channel_mp.bin.zip), in the case you need to recover your Moblab.

**Highlights**

*   Moblab software version exposed;

*   Contact email become required for feedback filing;
            [b/1186115](https://bugs.chromium.org/p/chromium/issues/detail?id=1186115)

*   Enrolling older devices issue addressed;

*   New chrome OS version R90-13816.53.0;

**UI - Configuration**

*   Configuration page along with the Chrome OS version now shows the version of
            Moblab software:

            <img alt="image"
            src="https://lh6.googleusercontent.com/4wjAkZpay4pKyKCyjpab2Z17N4tLnz5DfnmihZtw_WpkreNMAfcAc7z9nGBgSOo98gSmil4FhrKpaTfheI8sbSypZamOT_qnqK6Mo5pkUAr3kMHEiyYkmGQ07nYvaxmFXLoUH4D-3w"
            height=121 width=624>

*   Update/Force Update button also checks and applies OS updates if available.

*   Removed the exclamation icon from the copy of the Cloud Configuration section.
            It was confusing users.

**Feedback**

*   Contact email is now a required field to file a feedback report from the Moblab.

**Other**

*   New Chrome OS release version R90-13816.53.0.

*   The issue with enrolling older devices is fixed.

*   Various bug fixes and improvements.

# Mar 16th 2021: Moblab Update

**You should reboot your Moblab when next not running tests to get this latest version of the software.**

To check you have the latest Moblab software you can just look at this text in
the configuration page, if it says No updates found you have the latest.

<img alt="image"
src="https://lh3.googleusercontent.com/PgHoT7JwjD9ZMnH_OWE-0QXTanlQNj2M1qghOzY67a6wgvckRDY7K1kDdWQiC15uJSTy7yikA2PeKSRfKDZbwWnPvbA3-JK7mUDO9AfeWpd8r6ioWMQEv8YD_Q3AEnVczVQbQbTXVKd3qYfjQ9EPsp7Rg0LX9hIfHfN3K0L153E8OZVm"
height=141 width=486>

Moblab checks for new versions every 1 hour.

**Highlights**

*   The old UI has been removed.

*   Result Uploads use less bandwidth particularly for CTS/GTS.

*   StorageQual setup check has been moved to the UI.

*   New button to provision DUTs.

**UI - Configuration Page**

Stable Chrome OS version configuration

Set the version of Chrome OS used during the DUT repair process.

Powerwash is required to update google account configuration.

If a Moblab is moved from one account to another, it must be powerwashed or the
data will not show correctly on CPCon. This has been enforced in the Moblab UI.

**UI - Manage DUT Page**

*   Added DUT labels to summary page

*   Added MAC Address to summary page

*   Similar to the old UI the DUT labels show up on the manage DUT page.

*   Added “Reverify Selected” button

*   Similar to the old UI you can select a set of DUTs and force a reverify. If
            verify fails then a repair will be performed. There is a quick select option for
            all "Repair Failed" DUT's

*   Added Provision DUT’s button

*   Based on user feedback we have added a button that will allow you to provision
            some or all of the DUT’s attached to the Moblab to a particular version for
            testing. This is useful for manual CTS-V testing.

**UI - DUT Detail**

*   Added link to logs in DUT Actions History

**UI - Run Suite**

*   Added suites hardware_thermalqual_fast / hardware_thermalqual_full

*   Added DUTs configuration check for Storage Qualification suite to UI.

*   Previously you would run a test called “check_setup_storage_qual” now the same
            logic is built into the UI so you can not run a storage qual suite without the
            correct labels/number of DUT’s

**UI - View Jobs**

*   Upload status is now shown as an icon; a cloud icon means it has been uploaded to CPCon.

**Offloader**

*   Faster uploads using less bandwidth

*   Files are now compressed before upload. CTS/GTS will not upload debug files for PASSED tests.

**Other**

*   Various bug fixes and improvements.

# Jan 30th 2021: Moblab Update**

**You should reboot your Moblab when next not running test to get this latest version of the software.**

There has been an bug fix update to moblab to fix one important ( but rare ) bug
and two smaller issues.

Note the OS version of moblab will not change with this update, if you want to
make sure you have the latest version of moblab reboot, and check the configuration
page, and make sure it says "No updates found"

**Results Offloader:**

Fix issue where in rare circumstances the database can get out of sync and the

results get sent to CPCon before the test has actually completed: https://crbug.com/1171428

**UI**

Show more builds in the dropdown to select a build number:
https://crbug.com/1157114

**Remote Agent**

Fix issue where disconnected devices were not showing up correctly.
https://crbug.com/1167377

# Jan 21th 2021: Moblab Update

**You should reboot your Moblab to get this latest version of the software.**

This update includes an update to the underlying moblab ChromeOS version; after
the update you will be running version R87-13505.101.0

[OS Recovery Image](https://storage.googleapis.com/moblab-recovery-images/chromeos_13505.101.0_fizz-moblab_recovery_stable-channel_mp.bin.zip), in the case you need to recover your Moblab.

To check you have the latest Moblab software you can just look at this text in
the configuration page, if it says No updated found you have the latest.

[<img alt="image"
src="/chromium-os/testing/moblab/releasenotes/7tG9QTai4PUH2ue.png">](/chromium-os/testing/moblab/releasenotes/7tG9QTai4PUH2ue.png)

Moblab checks for new versions every 1 hour.

**Moblab "Pause":**

This is a new feature, it allow users with particularly long running suites,
normally storage qualification, to pause their Moblab so they can update the
software

or stop generating data if they have a particularly slow uplink to Google.

It should be noted - this feature does **NOT** stop the current running tests.
It prevents new tests from being scheduled, you can then have to wait until all
your DUT's

are in "ready" state. After then you can safely update/reboot your Moblab
without losing data.

Moblab will automatically turn on Pause feature if your disk space is getting
low. This typically happens when your Moblab is generating data faster than your

internet connection can copy it to Google. The Pause will allow time for your
Moblab to catch up and you can unpause when there is enough disk space
available.

**Remote Console**

Bug fixes and some changes to how often data is sent back to CPCon about failed
DUT's.

**Uploader**

Improved performance when there are problems or slow connections, uploader will
now only retry files that have not already been copied to the cloud rather than

re-trying the copy of all the files in the in the test result.

**UI**

Run Suite: The "Other" Tab now has a drop down list of suites you can run. You
can still select an option to enter a suite name that is not on the list.

Other non user visible bug fixes.

This likely will be the last release that includes the Old UI - we have got
feedback on features that need to be included in the new UI and are working on
those

for the next release. We will address all feedback before the remove the Old UI.

# December 7th 2020: Moblab Update**

**You should reboot your Moblab to get this latest version of the software.**

This is a regularly scheduled release of Moblab.

To check you have the latest Moblab software you can just look at this text in
the configuration page, if it says No updated found you have the latest.

[<img alt="image"
src="/chromium-os/testing/moblab/releasenotes/7tG9QTai4PUH2ue.png">](/chromium-os/testing/moblab/releasenotes/7tG9QTai4PUH2ue.png)

Moblab checks for new versions every 1 hour.

Manage DUT’s:

    Launch beta of firmware inspect/update.

        You can now view the current firmware version of all the current
        attached DUT’s. The version in the updater package is also shown and you
        can choose to update the firmware to that version if you wish. This is
        still a beta feature please file bugs / ask questions if you have
        issues.

[<img alt="image"
src="/chromium-os/testing/moblab/releasenotes/944NKgXvoL9ubsp.png">](/chromium-os/testing/moblab/releasenotes/944NKgXvoL9ubsp.png)

**Configuration:**

*   Added checkbox for remote command. If you are not a CTS 3PL you can ignore
            this setting, It is only useful to a few CTS 3PL labs.

**Run Suite:**

*   Added Storage Qualification V2 - Do not run unless requested to do so by Google.

*   Added CTS R - Do not run unless requested to do so by Google.

**General:**

*   There is a warning message on the old UI, we are going to remove that UI in
            January. Please file bugs if you still need to use it.

*   Force all web traffic from browser to Moblab to go through port 80, if you
            are sending the "New UI" over SSH this will make things simpler.

*   Add logs links to the View Jobs table.

**MobMonitor:**

*   Show an unhealthy state if the service account is missing, allows you to fix
            this error.

*   Show warning if any Moblab process is not running. Example this can show if
            offloader has failed.

**Remote Agent:**

*   Fix for issues with network speeds being incorrectly reported.

There are many other bug fixes and small improvements.

# October 21st 2020 Moblab Update: ChromeOS Update, Remote Console

**To intake this update, please find the update button on the new UI
[here](https://docs.google.com/document/d/e/2PACX-1vQ050-Fo1n2m0bYLE7r6LPxrnwEet7QtqGNcoqWJ4TQL5TURmdy9DforpOpteIovznUDyl0NrmSIQv5/pub#h.531gwuswnjb4).
This change will require a reboot to be applied. Please wait until all tests are
complete before applying this change. This change includes revisions to the new
UI which may require a [cache
clear](https://docs.google.com/document/u/1/d/e/2PACX-1vQ050-Fo1n2m0bYLE7r6LPxrnwEet7QtqGNcoqWJ4TQL5TURmdy9DforpOpteIovznUDyl0NrmSIQv5/pub#h.bd548te0mfe6)
to be seen.**

**This update includes an update to the underlying moblab ChromeOS version; after the update you will be running version R86-13421.72.0**

**This update also introduces [remote console](https://docs.google.com/document/d/1FqL1VzOmbGb_0ACQmVjev2fqSGOoFA7YCWIiDX8t7fc/view) functionality, which allows more Moblab/DUT information to be uploaded and summarized in CPCon ( additional information in the link above ). Instructions on how to enable the remote console can be found [here](https://docs.google.com/document/d/1FqL1VzOmbGb_0ACQmVjev2fqSGOoFA7YCWIiDX8t7fc/edit#heading=h.lypza0y6rfc8).**

**Bugfixes:**

Fix to run-suite page occasionally not starting suite + not showing error. -
<https://crbug.com/1137063>

Fix to run-suite page to only show compatible build target or model selection. -
<https://crbug.com/1137063>

Fix so that OAuth credentials are refreshed on configuration changes. -
<https://crbug.com/1136552>

Fix for issue with remote commands not firing. **- <http://crbug.com/1140613>**

## [OS Recovery Image](https://storage.googleapis.com/moblab-recovery-images/chromeos_13421.72.0_fizz-moblab_recovery_stable-channel_mp.bin.zip)
