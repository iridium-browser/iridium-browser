---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
- - /developers/design-documents/extensions
  - Extensions
- - /developers/design-documents/extensions/how-the-extension-system-works
  - How the Extension System Works
page_name: default-apps
title: Default Apps
---

Branded Chrome builds ship with a few default apps (Gmail, YouTube, etc.) that
are installed for new users. Default apps are implemented using a variant of the
[external
extensions](http://code.google.com/chrome/extensions/external_extensions.html)
mechanism.

**Adding a new default app**

1.  Locate the app in the Chrome Web Store
2.  Select the "Debug" tab
3.  Save its .crx using the "CRX Package Download: Published version"
            link
4.  Copy the .crx to
            [src/chrome/browser/resources/default_apps](http://src.chromium.org/viewvc/chrome/trunk/src/chrome/browser/resources/default_apps/).
5.  Add it to
            [external_extensions.json](http://src.chromium.org/viewvc/chrome/trunk/src/chrome/browser/resources/default_apps/external_extensions.json?view=markup)
            and
            [common.gypi](http://src.chromium.org/viewvc/chrome/trunk/src/build/common.gypi?view=markup)
            (look for default_apps_list and default_apps_list_linux_dest)

Testing your changes by making a branded build is tedious. Instead you can
manually copy the default apps directory in your build output, e.g.:

$ cp -r chrome/browser/resources/default_apps out/Debug/

Then you can start chrome with out/Debug/chrome
--user-data-dir=/tmp/&lt;somenewdir&gt; to simulate the new user experience.

For an example, see [the
changelist](https://chromiumcodereview.appspot.com/10535133) that added Google
Docs as a default app.
