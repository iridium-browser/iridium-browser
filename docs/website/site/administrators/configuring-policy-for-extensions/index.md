---
breadcrumbs:
- - /administrators
  - Documentation for Administrators
page_name: configuring-policy-for-extensions
title: Configuring Apps and Extensions by Policy
---

Policies can also be configured for extensions that support policy management
via the [managed storage
API](http://developer.chrome.com/extensions/manifest/storage). The sample
[Managed
Bookmarks](http://developer.chrome.com/extensions/examples/extensions/managed_bookmarks.zip)
extension can be used to configure Chrome bookmarks via a policy, for example.
Extensions that support policy management are listed in **chrome://policy**,
together with the policies configured for them.

This page documents how to configure policies for extensions, using the Managed
Bookmarks extension as an example. Extensions can also be [installed via
policy](http://www.chromium.org/administrators/policy-list-3#ExtensionInstallForcelist);
the examples below assume that the Managed Bookmarks extension has been loaded
as an unpacked extension from **chrome://extensions** and got the extension ID
"gihmafigllmhbppdfjnfecimiohcljba".

This extension supports two policies: "Bookmarks Bar" and "Other Bookmarks".
Each is a list of bookmarks, where each bookmark is a dictionary that contains a
"title" and either a "url" or a list of "children". The examples below configure
a "Chromium" bookmark to "chromium.org" and a "Videos" folder with a bookmark to
"youtube.com".

**Chrome OS**

Policies for Chrome OS must be configured via the admin console at
<https://admin.google.com>.

The policy for the extension can be uploaded in a txt file after the extension
has been selected to be configured. Note that this option only appears for
extensions that support policy configuration.

The txt file should contain a valid JSON object, mapping a policy name to an
object describing the policy. For now only the policy value can be configured;
other options may be added in the future, such as the policy level.

Example txt file for simple policy values:

```
{
  "Server": {
    "Value": "http://my.server/api"
  },
  "CloudSync": {
    "Value": true
  },

  "Allowlist": {
    "Value": [ "foo", "bar", "baz" ]
  }
}
```

The following example txt file is equivalent to the bookmarks configurations
above:

```
{
  "Bookmarks Bar": {
  "Value": [
    {
      "title": "Chromium",
      "url": "chromium.org"
    },
    {
      "title": "Videos",
      "children": [
        {
          "title": "YouTube",
          "url": "youtube.com"
        }
      ]
    }
  ]
}
```

## Windows

Policies for extensions should be written to the registry under
`HKLM\Software\Policies\Google\Chrome\3rdparty\extensions\gihmafigllmhbppdfjnfecimiohcljba\policy`
or under
`HKLM\Software\Policies\Chromium\3rdparty\extensions\gihmafigllmhbppdfjnfecimiohcljba\policy`
for Chromium. It's also possible to use HKCU instead of HKLM. The equivalent
path can be configured via GPO.

Example reg file to configure bookmarks (TODO: this hasn't been verified yet):

```
Windows Registry Editor Version 5.00
[HKEY_LOCAL_MACHINE\Software\Policies\Google\Chrome\3rdparty\extensions\gihmafigllmhbppdfjnfecimiohcljba\policy\Bookmarks Bar\1]
"title"="Chromium"
"url"="chromium.org"
[HKEY_LOCAL_MACHINE\Software\Policies\Google\Chrome\3rdparty\extensions\gihmafigllmhbppdfjnfecimiohcljba\policy\Bookmarks Bar\2]
"title"="Videos"
[HKEY_LOCAL_MACHINE\Software\Policies\Google\Chrome\3rdparty\extensions\gihmafigllmhbppdfjnfecimiohcljba\policy\Bookmarks Bar\2\children\1]
"title"="YouTube"
"url"="youtube.com"
```

## Linux

Policies for Chrome are configured via JSON files placed in
`/etc/opt/chrome/policies/managed/` (for Chrome) or
`/etc/chromium/policies/managed/` (for Chromium). These JSON files should contain
dictionaries that map a policy name to its value. The special 3rdparty key can
be used to configure policies for Chrome components. Under that key, the
extensions key is used to configure extensions, by mapping an extension's ID to
its policies. For example:

```
{
  "ShowHomeButton": true,
  "3rdparty": {
  "extensions": {
  "gihmafigllmhbppdfjnfecimiohcljba": {
    "Bookmarks Bar": [
      {
        "title": "Chromium",
        "url": "chromium.org"
      },
      {
        "title": "Videos",
        "children": [
          {
            "title": "YouTube",
            "url": "youtube.com"
          }
        ]
      }
    ]
  }
}
```

In this configuration, ShowHomeButton is one of the Chrome policies, and the
policies for the extension are listed under the gihmafigllmhbppdfjnfecimiohcljba
key.

## Mac

The policies for the extension can be configured via MCX preferences for the
`com.google.Chrome.extensions.gihmafigllmhbppdfjnfecimiohcljba` bundle, or for the
`org.chromium.Chromium.extensions.gihmafigllmhbppdfjnfecimiohcljba` bundle if
using Chromium. This can be done by creating a plist file with the configuration
and importing it using dscl:

```
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
  <key>com.google.Chrome.extensions.gihmafigllmhbppdfjnfecimiohcljba</key>
  <dict>
    <key>Bookmarks Bar</key>
    <dict>
      <key>state</key>
      <string>always</string>
      <key>value</key>
      <array>
        <dict>
          <key>title</key>
          <string>Chromium</string>
          <key>url</key>
          <string>chromium.org</string>
        </dict>
        <dict>
          <key>title</key>
          <string>Videos</string>
          <key>children</key>
        <array>
          <dict>
            <key>title</key>
            <string>YouTube</string>
            <key>url</key>
            <string>youtube.com</string>
          </dict>
      </array>
      </dict>
      </array>
    </dict>
  </dict>
</dict>
</plist>
```

The first key indicates the bundle ID that is to be configured. Note that each
policy maps first to its metadata, and its value is listed inside the value key.
The state key is used by the MCX preferences to determine how often this policy
should be enforced; setting it to always keeps this policy in place at all
times. This configuration can be imported with dscl using an administrator
account:

```
$ dscl -u admin_username /Local/Default -mcximport /Computers/local_computer configuration.plist
```

Substitute `admin_username` with a valid administrator username, and
`configuration.plist` with the path to the plist configuration listed above. If
dscl complains that the path is invalid then you can create a node for the local
computer with these commands:

```
$ GUID=\`uuidgen\`
$ ETHER=\`ifconfig en0 | awk '/ether/ {print $2}'\`
$ dscl -u admin_username /Local/Default -create /Computers/local_computer
$ dscl -u admin_username /Local/Default -create /Computers/local_computer RealName "Local Computer"
$ dscl -u admin_username /Local/Default -create /Computers/local_computer GeneratedUID $GUID
$ dscl -u admin_username /Local/Default -create /Computers/local_computer ENetAddress $ETHER
```

The preferences system can be told to propagate these changes immediately:

```
$ sudo mcxrefresh -n username</td>
```

If `username` is running Chrome with the Managed Bookmarks extension then Chrome
will load this policy in the next 10 seconds. Pressing "Reload policies" in
**chrome://policy** loads them immediately.
