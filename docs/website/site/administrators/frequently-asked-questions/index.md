---
breadcrumbs:
- - /administrators
  - Documentation for Administrators
page_name: frequently-asked-questions
title: Frequently Asked Questions
---

****What is Google Chrome?****

> **Google Chrome is a new, fast web browser from Google.**

****Why should we use Google Chrome? What would it give my organization?****

> **Google Chrome offers a number of benefits, including security, speed, stability, and simplicity. Find out about all the features of Google Chrome [here](http://google.com/chrome).**

**What is Google Chrome for Enterprise? Is it a different build than the Google
Chrome I install from google.com?**

> Google Chrome for Enterprise is just Google Chrome. Every Google Chrome has
> the same features, so it's equivalent to the Google Chrome you can download
> from google.com.

**What enterprise features does Chrome offer?**

> Support for (group) policy and centralized configuration (list of supported
> policies [here](/administrators/policy-list-3)), a specialized MSI installer,
> and control over auto-update frequency.

**Does Google Chrome support enterprise features on all platforms?**

> Yes.

**How do I install Google Chrome?**

> You can download Google Chrome from <http://www.google.com/chrome>.

**Do you have an offline installer or an MSI?**

> Yes. You can download the latest MSI
> [here](https://enterprise.google.com/chrome/chrome-browser/).

**Can I roll back Google Chrome to a previous version?**

> No - rollback is not supported.

> To get to a previous version (which would not be supported by Google), you
> would need to uninstall your current version, delete every user's saved
> profile data, and re-install the older version. Users' personal profile data
> is kept in:

> On Windows XP: C:\\Documents and Settings\\&lt;user&gt;\\Local
> Settings\\Application Data\\Google\\Chrome\\User Data

> On Windows Vista / 7:
> C:\\Users\\&lt;user&gt;\\AppData\\Local\\Google\\Chrome\\User Data

> This means users will lose their bookmarks, history, etc., so use this method
> with extreme caution.

**Are there older versions of the MSI available?**

> No, we do not currently make older versions of the MSI available, as they are
> no longer supported.

**How can I install per-system instead of per-user?**

> The MSI will only install at system-level, which means all users on the
> machine will have access to the same instance / version of Google Chrome.

**Does Google Chrome install over itself? What if users already have it
installed?**

> Google Chrome does not install over itself. It first checks if it is already
> installed at the same elevation level.

> However, if Google Chrome is already installed by the user at user-level,
> installing it system-level will remove the user-level installs and replace
> them with a system-level install. User/profile data would remain.

**How do I control Google Chrome's settings?**

> You can control and lock down user preferences through [Group Policy
> settings](/administrators/windows-quick-start) on Windows, [MCX configuration
> on Mac](/administrators/mac-quick-start), and [JSON files in special
> directories on Linux](/administrators/linux-quick-start). For Windows, it is
> recommended to use Group Policy vs. preferences files because only Group
> Policy can be enforced.

**What settings does Google Chrome allow an administrator to configure?**

> A list of supported policies is [here](/administrators/policy-list-3).

**Does Google Chrome allow me to set mandatory preferences and recommended
preferences through policy?**

> Yes. On Windows, you can set policies in HKEY_LOCAL_MACHINE and
> HKEY_CURRENT_USER, respectively. On Linux, set the policy files under
> /etc/opt/chrome/policies/managed and /etc/opt/chrome/policies/recommended,
> respectively.

**Do you have group policy templates, or an example of a policy file?**

> Yes. You can find group policy templates
> [here](/administrators/policy-templates).

**Where can I get a policy template for Google Chrome for Mac?**

> The template is bundled into the application package itself.

**What if I need to pre-configure a setting that is not a supported policy?**

> You can put some preferences into a file called "master_preferences" next to
> the Google Chrome executable, and those will be interpreted as part of the
> user's preferences. You can find out more about this technique
> [here](/administrators/configuring-other-preferences).

**What if users have already installed Chrome themselves? Will those Chromes
respect policies I set?**

> Yes.

**How do I pre-install extensions?**

> Information on pre-installing extensions is
> [here](/administrators/pre-installed-extensions).

****How do I turn off auto-updates?****

> **Although it is not recommended, you can find out how to turn off
> auto-updates [here](/administrators/turning-off-auto-updates).**

**How often does Google Chrome update? How many versions per year should one
expect?**

> Google Chrome's stable channel updates often. You can see how many major and
> minor updates there were to the stable channel on
> <http://googlechromereleases.blogspot.com>

**Where do I go to find out about Google Chrome updates? How do I know a new
update is coming?**

> Follow the updates on <http://googlechromereleases.blogspot.com>. We suggest
> adding this as one of your regular RSS feeds. We post here every time there is
> a new release of any channel.

**Does Google Chrome auto-update, even if the users on a machine do not have
administrative rights?**

> Yes.

**Where can I find release notes for each version?**

> **<http://googlechromereleases.blogspot.com>** has high-level release notes
> and lists security fixes for each release. We suggest you follow this blog.

**How can I get early warning and information about security updates?**

> Subscribing to the blog at <http://googlechromereleases.blogspot.com> will
> give you information about security updates as soon as they are public; this
> is the right list to watch.

**How many versions back do you support?**

> We **only** support the most current stable channel release. Older releases
> are not supported.

**How do I know what the most current version of Google Chrome is for Windows?**

> We've created a utility at <https://omahaproxy.appspot.com/win> to list the
> current stable version on Windows.

**How do I know what the most current version of Google Chrome is for other
platforms?**

> As above, <https://omahaproxy.appspot.com/viewer> provides a list of current
> revisions.

**Can I get phone or email support?**

> Yes, if you are a Google Apps for Business customer. Call the same number, use
> the same email address, or file a ticket via your control panel the same way
> you do for any Google Apps issues. Click
> [here](http://support.google.com/enterprisehelp/bin/answer.py?hl=en&answer=138863)
> for information on how to access the Google Enterprise Support Portal.

**What other support resources are available?**

> Google Chrome has a full [help
> center](http://www.google.com/support/chrome/?hl=en-US), along with a [help
> forum](http://www.google.com/support/forum/p/Chrome?hl=en&utm_source=HC&utm_medium=leftnav&utm_campaign=chrome),
> and [public bug tracker](http://code.google.com/p/chromium/issues/list).

**I found a bug in Chrome that I need fixed. Who should I contact and when can I
expect it fixed?**

> [Please file a bug](http://code.google.com/p/chromium/issues/entry) in our
> public issue tracker. You may want to search for any other similar bugs to
> make sure the issue isn't already being resolved.

> Unfortunately, we cannot give exact timing on when specific issues will be
> fixed. You can follow the public bug to see when changes are made, when it is
> marked as fixed and closed.

> If you have support from Google via Google Apps subscription, please contact
> Google as you would for a Google Apps issue.

**Google Chrome doesn't work with some of our internal applications. Will you
fix this?**

> Most of these issues are not actually bugs in Google Chrome -- the
> applications themselves were written for a specific web browser, and do not
> handle other browsers properly. You may want to contact whoever administrates
> those applications and ask about browser compatibility, as Google Chrome
> cannot fix those issues.

> Of course, if you do find issues that are true bugs in Google Chrome (pages
> crash, it shows web pages differently than Safari, etc.), then [please let us
> know](http://code.google.com/p/chromium/issues/entry) by filing a bug.

**We want to deploy Google Chrome, but we have legacy applications that don't
work in it. What can we do?**

> There are a couple of options in this scenario:

> 1. Update the legacy application to work on modern browsers (if possible)

> 2. Use Chrome's [legacy browser
> support](https://support.google.com/chrome/a/answer/3019558?hl=en) to enable
> loading legacy applications in a legacy browser.
