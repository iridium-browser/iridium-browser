---
breadcrumbs:
- - /administrators
  - Documentation for Administrators
page_name: complex-policies-on-windows
title: Complex policies on Windows
---

[TOC]

## Background

Chrome represents policies as JSON values internally. Up until version 37 all
Chrome policies were of simple types:

*   **Booleans** (example:
            [SafeBrowsingEnabled](https://chromeenterprise.google/policies/#SafeBrowsingEnabled))
*   **Integers** (example:
            [DefaultCookiesSetting](https://chromeenterprise.google/policies/#DefaultCookiesSetting))
*   **Strings** (example:
            [ProxyPacUrl](https://chromeenterprise.google/policies/#ProxyPacUrl))

Additionally, Chrome has supported some policies as **Lists of Strings**
(example: [URLBlocklist](https://chromeenterprise.google/policies/#URLBlocklist)).

These policy types match what GPO can represent natively on Windows using
Administrative Template files (ADM or ADMX).

Some policies need more complex values that don't fit well in any of these
types. For example,
[ExtensionInstallForcelist](https://chromeenterprise.google/policies/#ExtensionInstallForcelist)
defines a list of extensions to install automatically. Each extension is defined
by its extension ID and a remote update URL (to support extensions from private
stores). This can be easily represented in JSON as a list of objects, where each
object has two strings:

```
[
  {
    "id": "public-ext-id-1",
    "update_url": "https://clients2.google.com/service/update2/crx"
  },
  {
    "id": "private-ext-id-2",
    "update_url": "http://www.local/chrome/updates.xml"
  }
]
```

Unfortunately there is no native GPO type to represent this and the policy needs
to be configured as a List of Strings, where each string contains both the ID
and the update URL separated by a semicolon:

```
[
  "public-ext-id-1;https://clients2.google.com/service/update2/crx",
  "private-ext-id-2;http://www.local/chrome/updates.xml"
]
```

## New in Chrome 37: complex policies

Chrome 37 introduced new policies with complex values that don't fit in any of
the native GPO or Registry types:
[RegisteredProtocolHandlers](https://chromeenterprise.google/policies/#RegisteredProtocolHandlers)
and [Managed
Bookmarks](https://chromeenterprise.google/policies/#ManagedBookmarks).

There are 3 ways to configure the values for these policies: as a JSON string in
the GPO editor, as a JSON string in the registry, or as an expanded JSON object
in the registry.

## JSON values as strings

All of these policies can be configured as strings that contain a JSON value.
Any JSON editor should able to edit and validate the JSON string, including some
[online editors](http://google.com/search?q=json%20editor).

JSON can contain simple values:

*   Strings are wrapped in quotes: "example string value"
*   Integers are just spelled out: 123
*   Boolean values can be specified as true or false

JSON supports lists of values. The values should be enclosed between \[ and \]
and separated by commas: \[ "this", "is", "a", "list", "of", "strings" \].

JSON also supports objects (sometimes called dictionaries), which contain a
string key mapped to any other valid JSON value (which may be a list or another
object/dictionary): { "key": "string value", "key that maps to integer": 123,
"key that maps to list": \[ 1, 2, 3 \] }

The examples below will configure the [Managed
Bookmarks](https://chromeenterprise.google/policies/#ManagedBookmarks)
policy to build this bookmark structure:

*   Google (google.com)
*   YouTube (youtube.com)
*   Chrome links
    *   Chromium (chromium.org)
    *   List of Policies
                (https://chromeenterprise.google/policies/)

Each bookmark is a JSON object with a "name" key indicating its name, and a
"url" key indicating the URL or a "children" key that maps to another list of
bookmarks, to create folders.

The JSON string for the structure listed above is:

```
[
  {
    "name":"Google",
    "url":"google.com"
  },
  {
    "name":"YouTube",
    "url":"youtube.com"
  },
  {
    "name":"Chrome links",
    "children":[
      {
        "name":"Chromium",
        "url":"chromium.org"
      },
      {
        "name":"List of Policies",
        "url":"https://chromeenterprise.google/policies/"
      }
    ]
  }
]
```

[<img alt="image"
src="/administrators/complex-policies-on-windows/bookmarks4.png">](/administrators/complex-policies-on-windows/bookmarks4.png)

## Option 1: JSON strings in the GPO editor

If you use the ADM or ADMX templates you can just locate the policy to configure
(in this case, "Managed Bookmarks"), enable it, and set the JSON string in the
string field for that policy:

[<img alt="image"
src="/administrators/complex-policies-on-windows/bookmarks5.png">](/administrators/complex-policies-on-windows/bookmarks5.png)

## Option 2: JSON strings in the registry editor

**Note**: Chrome only loads policies directly from the registry on AD enrolled
machines.

Chrome policies can be configured under Software\\Policies\\Google\\Chrome (or
Software\\Policies\\Chromium for Chromium) in HKCU or HKLM. For complex
policies, just create a new String value with the policy name and set the JSON
string in the Value field.

[<img alt="image"
src="/administrators/complex-policies-on-windows/bookmarks6.png">](/administrators/complex-policies-on-windows/bookmarks6.png)

## Option 3: expanded JSON in the registry

**Note**: Chrome only loads policies directly from the registry on AD enrolled
machines.

Chrome will try to load JSON lists and JSON objects/dictionaries directly from
the registry too, for the new complex policies. This format makes it easier to
directly edit the policy values in the registry but requires understanding how a
JSON list and a JSON dictionary can be represented in the registry.

The basic rules are:

*   Strings are stored as String Values (right-click on the right side,
            choose New -&gt; String Value)
*   Integers are stored as Dword Values (New -&gt; DWORD (32-bits))
*   Booleans are also stored as Dword Values. Use 0 for false and 1 for
            true.

Objects/dictionaries can contain any of these simple types. Just add the value
as described above, and use the "Value Name" field to set the name of the entry
in the dictionary.

If the entry in the dictionary is a list or another dictionary then you can
create a new Key on the left side of the registry editor. Its name will be used
for the dictionary key name, and its contents will be interpreted as another
dictionary.

Lists are represented just like dictionaries, but they key names must be "1",
"2", "3" and so on. Note that counting starts at 1 and not at 0 (this is what
the GPO editor does for lists too). Chrome knows the expected policy format
internally, and will load each entry as a list or object as appropriate.

Here's an example for the Managed Bookmarks configuration listed above. Start by
creating a new registry Key (these are shown as folders on the left side) named
"ManagedBookmarks":

[<img alt="image"
src="/administrators/complex-policies-on-windows/reg1.png">](/administrators/complex-policies-on-windows/reg1.png)

The value of this policy is a list of objects, so this folder will contain other
subfolders named "1", "2", etc. Start by creating a subfolder named "1":

[<img alt="image"
src="/administrators/complex-policies-on-windows/reg2.png">](/administrators/complex-policies-on-windows/reg2.png)

The contents of this folder are the keys for the first object inside the list of
bookmarks. You can now add the 2 strings that declare a bookmark, the "name" and
the "url". This configuration should be enough to show one bookmark in Chrome.

To add bookmark folders, start by adding a new entry in the main list (see
folder "3" below) and give the folder a "name" too. But instead of specifying a
"url", add a new subfolder named "children" that contains the bookmarks for that
folder. The "children" folder is a list of bookmarks, so it should contain
subfolders named "1", "2", etc again.

[<img alt="image"
src="/administrators/complex-policies-on-windows/reg3.png">](/administrators/complex-policies-on-windows/reg3.png)

Each child bookmark of the folder needs to have its "name" and "url" defined
again:

[<img alt="image"
src="/administrators/complex-policies-on-windows/reg4.png">](/administrators/complex-policies-on-windows/reg4.png)

## Troubleshooting

The first step to diagnose problems is the internal chrome://policy page. You
should see your policies listed in that page and their corresponding values.

*   If the policy is present and its value is shown too but it doesn't
            seem to work then this might be a new bug. Please file a new report
            at [crbug.com](http://crbug.com) with the Enterprise template.
*   If the policy is present but the value is invalid then there is a
            problem in the JSON configuration.
*   If the policy is not present then it wasn't found by Chrome.

Policies configured via GPO have some delay until they are applied; run the
gpupdate command to flush them. If you use the registry then note that Chrome
only loads those policies on machines that are enrolled to an AD domain.

It's generally a good idea to validate your JSON string to make sure it doesn't
have any invalid constructs; a common source of errors is trailing commas at the
end of a list or a dictionary, which is not supported in JSON. Use an [online
JSON validator](http://google.com/search?q=json%20validator) (like
[JSONLint](http://jsonlint.com/)) to make sure your JSON string is valid.
