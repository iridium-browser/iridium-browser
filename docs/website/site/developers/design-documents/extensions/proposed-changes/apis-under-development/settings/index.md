---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
- - /developers/design-documents/extensions
  - Extensions
- - /developers/design-documents/extensions/proposed-changes
  - Proposed & Proposing New Changes
- - /developers/design-documents/extensions/proposed-changes/apis-under-development
  - API Proposals (New APIs Start Here)
page_name: settings
title: Settings API
---

**Overview**

This page proposes adding methods/events to chrome.settings to let extensions
store settings data (assumed to be small pieces of string-like data) that is
synced across a user's browser instances. Initially, the module would start in
the chrome.experimental.settings namespace.

**Use cases**

Ever since extensions/apps sync was implemented, extension settings sync has
been an oft-requested and
[anticipated](http://googlesystem.blogspot.com/2010/12/predictions-for-googles-2011.html)
feature. Syncing an extension's settings greatly enhances the usefulness of
extension sync.

**Could this API be part of the web platform?**

No.

**Do you expect this API to be fairly stable?**

Yes. This API is based on the HTML5 storage spec which is unlikely to change
much.

**What UI does this API expose?**

No additional UI is exposed. However, we may need to add UI to
chrome://extensions for turning off settings sync for particular extensions. On
the other hand, extension developers could do this themselves.

**How could this API be abused?**

An ill-behaved extension could create lots of data to sync, bogging down both
the client and the server. This may also cause the user to be throttled,
possibly affecting sync for other data types (e.g., bookmarks, preferences).

**How would you implement your desired features if this API didn't exist?**

Currently, extensions sync their settings by stuffing data in bookmarks. This is obviously very hacky and causes problems for sync.

**Are you willing and able to develop and maintain this API?**

The sync team can develop and maintain this API.

**Draft API spec**

It might be helpful to refer to docs for the HTML5 storage spec: see [this
overview](http://diveintohtml5.org/storage.html) and [the spec
itself](http://dev.w3.org/html5/webstorage/).

> **API Reference: chrome.experimental.settings (later
> chrome.extension.settings)**

> **Methods**

> **getSettings**

> chrome.experimental.settings.getSettings(function callback)

> **Parameters**

> ***callback ( function )***

> **Callback function**

> The callback parameter should specify a function that looks like this:

> function(Storage settings) {...}

> **settings ( Storage )**

> **The settings Storage object. It is guaranteed that nothing else will access this (including sync) for the lifetime of the callback.**

> **Events**

> **onSettingsChanged**

> chrome.experimental.settings.onSettingChanged.addListener(function(StorageEvent
> e) {...});

> Fires when a setting has been changed, either locally or from another browser
> instance.

> **Parameters**

> **e ( StorageEvent )**

> Details of the setting that was changed.

*   The Storage object passed to the getSettings callback behaves very
            similarly to the localStorage object; it is persisted across browser
            restarts and is per-profile. However, any changes made to it may
            eventually be synced to other browser instances with synced turned
            on. Also, the onSettingsChanged event may be fired when changes from
            other browser instances are synced to the local one.
*   No guarantees are made as to how long it takes for changes to
            propagate, or even if they're propagated at all. Sync may be turned
            off, clients may be throttled, and other reasons may prevent or
            delay propagation.
*   Each top-level key-value pair is guaranteed to be synced atomically.
            That is, while settings\["a"\] and settings\["b"\] may be synced
            independently, two clients writing separate values to the same key
            will cause the final state to be either one or the other. However,
            the settings Storage object at worst behaves like another
            localStorage.
*   The storage limits for the settings Storage object are more
            stringent than localStorage; tentatively, it is planned that each
            key/value pair can have a maximum of 16kb, and the total storage for
            an extension cannot exceed 100kb. It is expected that the most
            common use case for this are small pieces of string-like data, e.g.
            settings and preferences.
*   Extension developers must take care not to modify settings too often
            or else they risk being throttled (if they do on the order of
            hundreds or thousands of changes in a 24 hour period).
*   Sync will only kick in after a call to getSettings to propagate any
            changed settings and/or apply any new settings. This means that an
            extension developer has some control on how often sync fires.
*   Uninstalling an extension currently clears all settings data for it.
            (This may be surprising in some cases; we may need to revisit this
            in the future.)
*   This API will have its own manifest permission ("settings") but
            won't trigger a warning.

> Example usage:

> // Imagine an extension that retrieves data from a particular URL.

> chrome.experimental.settings.getSettings(function(settings) {

> // Do initial frobnigation, setting a default value if necessary.

> var dataUrl = settings\["dataUrl"\];

> if (!dataUrl) {

> // This will be propagated to other browser instances after this

> // callback.

> dataUrl = settings\["dataUrl"\] = "http://www.foo.com/mydata";

> }

> doInitialFrobnigate(dataUrl);

> });

> // Do an update every time the dataUrl setting is changed (either

> // locally or remotely).

> chrome.experimental.settings.onSettingChanged.addListener(function(e) {

> if (e.key == "dataUrl") {

> console.info("Changing data URL from " + e.oldValue + " to " + e.newValue);

> doUpdateFrobnigate(e.newValue);

> }

> });

> Example code demonstrating when it's okay to access the settings object:

> chrome.extension.settings.getSettings(function(settings) {
> // While this callback is being invoked, the extension has exclusive access to
> the settings.
> // At other times the extension does not have access. We do this to avoid
> raciness between
> // multiple processes manipulating the data concurrently.
> var foo = settings.foo;
> settings.foo = foo + 1; // using settings object here is OK
> window.setTimeout(function() {
> // This is invalid! The extension no longer has access to |settings|. We need
> to make it
> // so that trying to read or write from the settings object after the callback
> is complete
> // throws an error.
> alert(settings.foo);
> }, 1000);
> // Access to the extension's settings goes away at the end of this callback.
> });

**Open questions**

*   Is there a need for syncing other types of data, e.g. large pieces
            of data or structured data?
*   Do we need anything for the enterprise use case? It may be useful to
            be able to provide a default settings object and maybe even to mark
            some settings non-editable.
