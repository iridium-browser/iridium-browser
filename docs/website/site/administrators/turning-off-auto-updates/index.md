---
breadcrumbs:
- - /administrators
  - Documentation for Administrators
page_name: turning-off-auto-updates
title: Turning Off Auto Updates in Google Chrome
---

Google Chrome on Windows and Mac auto-updates itself on a regular basis. The
auto-updating procedure is performed by Google Update, which is based on the
open-source [Omaha](https://github.com/google/omaha) project. Auto-updated
provide fixes to sometimes critical issues, limiting exposure.

<table>
<tr>
<td><b>Warning: Turning off auto-updates should be done with caution. You may not receive the latest security updates if you do not auto-update.</b></td>
</tr>
</table>

**Turning off Auto-Updates on Windows**

To turn off auto-updates of Google Chrome on Windows, you need to instruct
Google Update to not update it. To do this, you can either:

1.  Use the **Google Update** ADM templates provided [on this
            page](/administrators/policy-templates) or as described in [this
            article](https://support.google.com/chrome/a/answer/6350036).
2.  Set the value of
            HKEY_LOCAL_MACHINE\\SOFTWARE\\Policies\\Google\\Update\\AutoUpdateCheckPeriodMinutes
            to the REG_DWORD value of "0".

Warning: To prevent abuse of this policy, if a device is not joined to an Active
Directory domain, and if this policy has been set to 0 or to a value greater
than 77 hours, this setting will not be honored and replaced by 77 hours after
August 2014. If you are affected by this, and still want to disable Chrome
updates (NOT RECOMMENDED), you may do so by using 'Update policy override' as
described [here](https://support.google.com/chrome/a/answer/6350036#Policies).

More information about Google Update's group policy support is
[here](https://support.google.com/chrome/a/answer/6350036).

**Turning off Auto-Updates on Mac**

More information about turning off auto-updates on a Mac network is
[here](http://www.google.com/support/installer/bin/answer.py?hl=en&answer=147176&ctx=go).

**Turning off Auto-Updates on Linux**

Google Chrome and Chromium are not auto-updated automatically on Linux; your
package manager handles this.

**Frequently Asked questions**

**Q: Does Chrome on Linux auto-update too?**

A: Google Chrome on Linux does not auto-update itself; it relies on your package
manager to update it.

**Q: Does the open-source Chromium browser auto-update like Chrome?**
No. Chromium does not have its own auto-update process, so if you are deploying
Chromium, do you not need to worry about turning off auto-updates.

**Q: How do I know if there is an auto-update happening soon?**

A: You can subscribe to the blog at <https://chromereleases.googleblog.com>,
which lists every dev, beta, and stable release of Google Chrome. Chromium does
not auto-update.

**Q: How often do auto-updates happen? How many can I expect this year?**

A: Major version updates to the stable channel of Google Chrome tend to happen
about every six weeks, although security fixes can come at any time. See
[Release Early, Release
Often](http://blog.chromium.org/2010/07/release-early-release-often.html) for
more information.

**Q: Do you have release notes with each version?**

A: Yes, see the Chrome Enterprise Release Notes. Also, for a granular list of
what's changed and links to all the fixes made, see
<https://chromereleases.googleblog.com>.

**Q: Why would I not want to turn off auto-updates?**

A: Turning off auto-updates means you may miss an update that includes security
fixes, leaving your users at risk.

**Q: Can I turn auto-updates back on?**

A: Yes. Just set the value of the registry key you changed back up to a
reasonable number of minutes between update checks (greater than "0").

**Q: How would I update my users without turning auto-update back on?**

A: You can deploy the latest MSI, which is available
[here](http://www.google.com/chrome/eula.html?msi=true).

**Q: I need auto-updates off so I can test new versions of Google Chrome /
Chromium before everyone else gets them. What do you suggest I do?**

A: Turn off auto-updates via the steps above, and push the group policy to your
network. Then download the latest MSI
[here](http://www.google.com/chrome/eula.html?msi=true). Deploy it on your test
machines, and do your verification. Once it is certified, deploy that same MSI
on the rest of your network. And watch for updates on
<https://chromereleases.googleblog.com> for new versions of the MSI to test and
deploy. Enterprise customers can find additional downloads, such as ADMX
templates to manage Chrome policies
[here](https://cloud.google.com/chrome-enterprise/browser/download/).
