---
breadcrumbs:
- - /administrators
  - Documentation for Administrators
page_name: policy-templates
title: Policy Templates
---

To ease your policy setup, several policy templates can guide you easily through
the configurable options.

Templates can also be generated locally by building the policy_templates
Chromium project.

**Windows**

There are two types of templates available, an ADM and an ADMX template. You
will want to verify what template type you can use on your network.

[ZIP file of ADM/ADMX/JSON templates and
documentation](https://dl.google.com/dl/edgedl/chrome/policy/policy_templates.zip).
[Beta](https://dl.google.com/chrome/policy/beta_policy_templates.zip) and
[Dev](https://dl.google.com/chrome/policy/dev_policy_templates.zip)
administrative templates are also available for testing them before a stable
release.

Google Update (auto-update) has its own templates as well, in
[ADM](https://dl.google.com/update2/enterprise/GoogleUpdate.adm) and
[ADMX](http://dl.google.com/dl/update2/enterprise/googleupdateadmx.zip) forms.

The recommended way to configure policy on Windows is Group Policy Object (GPO),
however on machines that are joined to an Active Directory domain, policy
settings may also be stored in the registry under HKEY_LOCAL_MACHINE or
HKEY_CURRENT_USER in the following paths:

*   Google Chrome: Software\\Policies\\Google\\Chrome\\
*   Chromium: Software\\Policies\\Chromium\\

**Mac**

Policies are defined on a Mac in a plist (property list) file. The latest plist
template is included in the Google Chrome/Chromium installer package. To find
the plist:

1.  [Download Google Chrome](https://www.google.com/chrome/)
2.  Open the bundle and find the Configuration folder
3.  Open a file called com.google.Chrome.manifest. This is the latest
            plist template.
4.  Review the plist template.
5.  Use the template to create your own plist file.
6.  Create your own plist file with appropriate policies based on the
            plist template. Use the property list editor of your choice.
7.  Use your system management tool to push the configuration file to
            client Macs.

To see an example of how to load this file into Profile Manager, see the [Mac
Quickstart guide](/administrators/mac-quick-start).

**Note:** Any other system management tool can be used instead of Profile
Manager such as Jamf or Puppet.

**Linux**

For Linux, consult the HTML documentation contained in the [ZIP file of ADM/ADMX
templates and
documentation](https://dl.google.com/dl/edgedl/chrome/policy/policy_templates.zip)
for the JSON keys that correspond to each policy.

To see where to put this file, see the [Linux Quickstart
guide](/administrators/linux-quick-start).

**Google Chrome OS**

Google Chrome OS devices that are [joined to an Active Directory
domain](https://support.google.com/chrome/a/answer/7497916) can be managed via
Group Policy.

The ADMX templates containing all applicable Chrome OS user and device policies
can be found in the

[ZIP file of ADM/ADMX/JSON templates and
documentation](https://dl.google.com/dl/edgedl/chrome/policy/policy_templates.zip).
[Beta](https://dl.google.com/chrome/policy/beta_policy_templates.zip) and
[Dev](https://dl.google.com/chrome/policy/dev_policy_templates.zip)
administrative templates are also available for testing them before a stable
release.
