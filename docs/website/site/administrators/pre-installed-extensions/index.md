---
breadcrumbs:
- - /administrators
  - Documentation for Administrators
page_name: pre-installed-extensions
title: Pre-installed Extensions
---

Pre-installing an extension can be done in one of three ways: via Group Policy,
via the Registry, or via master_preferences.

**Pre-installing via Group Policy**

Using policy to deploy an Extension or Chrome Web App is by far the easiest and
scalable method. This is the recommended method for pushing extensions, as it
does not require the CRX file to be on the machine, it must simply be available
at a given URL.

To use this method, just set the policy to 'force install' the extension, as
documented [here](/administrators/policy-list-3#ExtensionInstallForcelist).

**Pre-installing via the Registry**

Using this method, a special registry key indicates what extensions Google
Chrome should load. This means that the extension .crx file (the file downloaded
from the gallery, or the file you package yourself) needs to already be on the
machine. This method is not to be confused with setting policy -- this method
sets a completely different registry key and requires the CRX to be on the
machine in question.

Once you have the extension .crx file you'd like to pre-install, follow these
steps:

1. Copy the .crx file to a location such as: C:\\path\\to\\your\\extension.crx

2. Create the registry key:

32-bit Windows: HKEY_LOCAL_MACHINE\\SOFTWARE\\Google\\Chrome\\Extensions\\\[id
of your extension crx\] 64-bit Windows:
HKEY_LOCAL_MACHINE\\SOFTWARE\\**Wow6432Node**\\Google\\Chrome\\Extensions\\\[id
of your extension crx\]

3. Create the following registry key values:

32-bit Windows: HKEY_LOCAL_MACHINE\\SOFTWARE\\Google\\Chrome\\Extensions\\\[id
of your extension crx\]\\path 64-bit Windows:
HKEY_LOCAL_MACHINE\\SOFTWARE\\**Wow6432Node**\\Google\\Chrome\\Extensions\\\[id
of your extension crx\]\\path TYPE: REG_SZ VALUE:
"C:\\path\\to\\your\\extension.crx" 32-bit Windows:
HKEY_LOCAL_MACHINE\\SOFTWARE\\Google\\Chrome\\Extensions\\\[id of your extension
crx\]\\version 64-bit Windows:
HKEY_LOCAL_MACHINE\\SOFTWARE\\**Wow6432Node**\\Google\\Chrome\\Extensions\\\[id
of your extension crx\]\\version TYPE: REG_SZ VALUE: \[version of your .crx as
specified in the manifest\]

This method requires the CRX is on the machine; this method may not be flexible
enough for all deployments. However, it does mean that users can have an
extension pre-installed behind a corporate firewall that restricts downloading
of files.

**Pre-installing via master_preferences**

You should already be familiar with configuring preferences. If you are not,
read the documentation on [master_preferences and other methods of
pre-configuring preferences](/administrators/configuring-other-preferences).

Pre-installed extensions are added to the master_preferences file that lives
next to chrome.exe. This means that the extension CRX file can live anywhere,
and the bits do not need to live on the target user's machine and don't need to
be packaged into any installation script. If you are not familiar with the
master_preferences file or how it works, you will need to [read this
documentation first](/administrators/configuring-other-preferences).

There are some requirements to using this method:

*   This method only works if the user has access to the public
            extension gallery or another URL where the CRX file is kept; this
            method may not work if the user is behind a corporate firewall or
            proxy that restrict access to the gallery.
*   This method generally only works on new installs. Making it work
            with an existing install is cumbersome and requires a lot of
            clean-up steps.

To pre-install an extension with just master_preferences changes,

1. Find the CRX file you want to install. Download it from the gallery, etc.

2. Open up the CRX with a zip program and find the manifest.json file (it's just
a text file). This contains many values you will need.

3. Set up your master_preferences with values from the manifest.json file.

Here is an example master_preferences, which pre-installs the Google Reader
extension:

{ "homepage" : "http://www.chromium.org", "homepage_is_newtabpage" : true,
"extensions": { "settings": { "apflmjolhbonpkbkooiamcnenbmbjcbf": { "location":
1, "manifest": { "key":
"MIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQC5cK3ybDkh173plsjDXoqUzvsjFRMtbc5+a8HR6dxYBETeXQ7GOR5/xYnsY2R4smo5ly4yUK69iF7rnPNH+X97K7e7JFbuH5W/ZRc8YaIG66oJ9JwKZagSOZasSJPWNz4f1GdaHD1Z4KRucvOYxsaPVdwS2W3nbG6i3oQFaio+JQIDAQAB",
"name": "Google Reader Notifier (Installing...)", "permissions": \[ "tabs",
"http://www.google.com/" \], "update_url":
"http://clients2.google.com/service/update2/crx", "version": "0.0",
"manifest_version": 2 }, "path": "apflmjolhbonpkbkooiamcnenbmbjcbf\\\\0.0",
"state": 1 } } } }

Breaking down the lines in this master_preferences file,

*   Under settings, the first value is the hash of the extension
            ("apflmjolhbonpkbkooiamcnenbmbjcbf"). You get this by packaging up
            the CRX file, and is also the extension's identifier in the gallery.
*   "location" must always be 1.
*   The "manifest" section must contain "key", "name", "permissions",
            "update_url", "version", and "manifest_version". These can come from
            the extension's manifest.
*   The "key" value comes from the packaged extension, just like the
            hash. If you look at an unzipped CRX file, you'll find the "key" in
            manifest.json.
*   "name" can be anything, although having a temporary tag (i.e.
            "(Installing...)") will help users understand why an extension is
            taking an extra bit of time to load.
*   "permissions" **must be the same as the permissions in the extension
            CRX file** at "update_url", or the user will see lots of warnings
            and it won't load. So, you can't specify and empty permissions
            array, and the real extension requires lots of permissions -- that
            would hide escalating privilege.
*   "update_url" is the URL where the CRX lives. Again, this is in the
            manifest.json file.
*   "version" should always be "0.0"
*   "manifest_version" should be the same as in the extension CRX file
            manifest.json (current manifest_version 2)
*   "path" should always be the extension's hash followed by "\\\\0.0".
*   "state" should always be 1.

If any of these rules are broken, the extension may not load or the user may see
a warning.

**Important:** If the extension contains content scripts that need permissions,
they must be listed in the master_preferences as well. For example,

{ "extensions": { "settings": { "apflmjolhbonpkbkooiamcnenbmbjcbf": {
"location": 1, "manifest": { "content_scripts": \[ { "all_frames": true, "js":
\[ "script.js" \], "matches": \[ "http://\*/\*", "https://\*/\*" \], "run_at":
"document_start" } \], "key":
"MIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQC5cK3ybDkh173plsjDXoqUzvsjFRMtbc5+a8HR6dxYBETeXQ7GOR5/xYnsY2R4smo5ly4yUK69iF7rnPNH+X97K7e7JFbuH5W/ZRc8YaIG66oJ9JwKZagSOZasSJPWNz4f1GdaHD1Z4KRucvOYxsaPVdwS2W3nbG6i3oQFaio+JQIDAQAB",
"name": "Google Reader Notifier (Installing...)", ...

If the extension has content scripts that need permissions / access, and you do
not specify it here, the extension will not load!

You can pre-load multiple extensions. Adding an additional extension is as easy
as adding another block under "settings":

{ "extensions": { "settings": { "apflmjolhbonpkbkooiamcnenbmbjcbf": { &lt;--
extension one "location": 1, "manifest": { ... } },
"oaiwevnmzvoinziufeuibyfnzwevmiiw": { &lt;-- extension two "location": 1,
"manifest": { ... }, ... } } }

If the extension has any permissions that require user approval, you also need
to include a granted_permissions section:

{ "extensions": { "settings": { "mihcahmgecmbnbcchbopgniflfhgnkff": {
"location": 1, "manifest": { ... }, "granted_permissions": { "api": \[ "tabs"
\], "explicit_host": \[ "http://\*.google.com/\*", "https://\*.google.com/" \],
"scriptable_host": \[ "http://example.com/" \] }, ... }, ... } } }

If you do not include a granted_permissions section, Chrome immediately disables
the extension. The granted_permissions field has the following subfields:

*   "api" contains API permissions in the "permissions" key of the
            manifest
*   "explicit_host" contains any host permissions in the "permissions"
            key of the manifest
*   "scriptable_host" should be set to any hosts in the extension's
            content scripts

The easiest way to generate the granted_permissions field is to install the
extension locally and then copy the extension's granted_permissions from the
Preferences file of your [Chrome profile](/user-experience/user-data-directory).
