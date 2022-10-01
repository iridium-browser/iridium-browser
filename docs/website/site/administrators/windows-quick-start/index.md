---
breadcrumbs:
- - /administrators
  - Documentation for Administrators
page_name: windows-quick-start
title: Windows Quick Start
---

This tutorial is meant as an expedited way to get Chromium-based browsers
deployed and configured on a Windows network, by deploying an MSI and applying
group policy. For further configuration, please reference the other
documentation at <http://www.chromium.org/administrators>.
**1. Download the MSI.**

Download the [Google Chrome
MSI](http://www.google.com/chrome/eula.html?msi=true).
**2. Download the ADM / ADMX template.**
You can download the ADM template or ADMX template
[here](/administrators/policy-templates).
**3. Configure the settings for your network.**
Open up the ADM or the ADMX template you downloaded with these steps:

ADM file import:

*   Navigate to Start &gt; Run: gpedit.msc
*   Navigate to Local Computer Policy &gt; Computer Configuration &gt;
            Administrative Templates
*   Right-click Administrative Templates, and select Add/Remove
            Templates
*   Add the **chrome.**adm template via the dialog.
*   Once complete, a Google / Chrome folder will appear under
            'Administrative Templates' if it's not there already.

ADMX file import:

*   Copy the **chrome.admx** file to **%SystemRoot%\\PolicyDefinitions**
*   Copy one or more of the language directories, for example **en-US**,
            to **%SystemRoot%\\PolicyDefinitions** as well. You should copy all
            language folders which already exist under the PolicyDefinitions
            folder.
*   Navigate to Start &gt; Run: gpedit.msc
*   Navigate to Local Computer Policy &gt; Computer Configuration &gt;
            Administrative Templates
*   A Google / Chrome folder will appear under 'Administrative
            Templates' if it's not there already.

Change the configuration settings for the target group of users. The policies
you will be most interested in are:

*   Home page - This is the URL that users see when they first open the
            browser or click the “home” button.
*   Send anonymous usage statistics and crash information. - To turn off
            sending any crash information or anonymous statistics to Google,
            change this setting to be False.
*   **Turning off auto-updates** - steps to turn off auto-update are
            [here](/administrators/turning-off-auto-updates).

A full list of supported policies is [here](/administrators/policy-list-3).
You should then apply the policies to the target users / machines. Depending on
your network’s configuration, this may require time for the policy to propogate,
or you may need to propogate those policies manually via administrator tools.
**4. Push the MSI out to your network.**
If you are using SMS or other tools, use these to push out the Google Chrome MSI
as you would any other installation bundle.
Otherwise, you can run the MSI on the target machines directly, and silently,
with this command:

Msiexec /q /I GoogleChrome.msi

**5. Test your installation.**
On a target machine, launch Google Chrome. The settings you applied in step 3
should be noticeable on the test machine. Congratulations!
If the policies have not propagated to the test machine / user, you may be able
to run “gpupdate” to refresh policy settings.
