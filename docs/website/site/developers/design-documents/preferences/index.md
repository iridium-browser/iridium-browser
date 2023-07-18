---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: preferences
title: Preferences
---

**Abstract:** Preferences are used to store configuration parameters that can be
modified at run-time rather than compile-time. This document describes the
architecture of Chromium’s preferences system and explains how to use it in the
context of extensions and policies.

**Authors:** battre, pamg

**Last modification:** March 16, 2013 (updated code search links)

[TOC]

## Introduction

Preferences are a means to **store configuration options** in Chromium. Each
**preference** is **identified by a key** (a string like “proxy.mode”) and
points to a value that we call the **preference value**. These values can be
represented by any data-type that is defined in “values.h”, i.e. booleans,
integers, strings, lists, dictionaries, et cetera.
Preferences can be **associated** with a **browser profile** or with **local
state**. Local state contains everything that is not directly associated to a
specific profile but rather represents values that are associated with the user
account on the host computer (e.g. “Was the browser shut down correctly last
time?”, “When was the ‘GPU block list’ downloaded the last time?”, etc.). Due to
its nature, local state is never synced across machines.
The **preference values that are effective at runtime can be set from various
sources like default values, files of persisted settings, extensions** and
**policies**.
The **preference system is responsible for** and allows for

*   registering preferences with default values
*   setting and getting of preference values
    *   while performing basic type validation
    *   while respecting precedences (priority) of preference value
                sources
    *   while providing thread safety (allow access only on UI thread,
                provide some helpers for subscribing to preference values on
                other threads)
*   notification of preference value changes

These aspects are addressed in the following.

## Basics

### Registering preferences

**Each preference needs to be registered** before being used as in the following
example:

```none
PrefService* prefs = …;
prefs->RegisterBooleanPref(prefs::kSavingBrowserHistoryDisabled, false, PrefService::UNSYNCABLE_PREF);
```

where `prefs::kSavingBrowserHistoryDisabled` is a string defined in
`pref_names.{h,cc}`.
Here we register a boolean preference with the **key**
`“history.saving_disabled”` **and a default value** of `false`. The default
value determines the type of value that can be stored under the key (in this
case: boolean). Prefs associated with a profile may be labeled as sync-able and
un-sync-able (see [sync](/developers/design-documents/sync)).
Registering preferences is important to define their type and guarantee valid
default values but also because only registered preference keys can be queried
for their current value. Preferences are persisted to disk in the “Preferences”
file as JSON with nested dictionaries. The “.” in the preference key is used as
the nesting operator:

```none
{
  “history”: {
    “saving_disabled”: false
  }
}
```

This hierarchical namespace is not exposed within Chromium. You must not query
`“history”` and expect a dictionary containing `“saving_disabled”`. Always
access registered preferences by their full key.
To register new preferences, add your `RegisterPrefs` call to
`RegisterLocalState` or `RegisterUserPrefs` in `browser_prefs.cc`.

### Accessing preference values

The **preferences system lives entirely on the UI thread**. In the following we
assume that we want to access it from there.
The preferences associated with the profile can be accessed using the
`PrefService` provided by `Profile::GetPrefs()`. The `PrefService` responsible
for local_state is accessible by calling `g_browser_process->local_state()`.
The **PrefService provides getter and setter methods** like
`bool GetBoolean(const char* path) const;`
` void SetBoolean(const char* path, bool value);`
As we want to be sure that modifications trigger notifications (see below),
`GetDictionary()` and `GetList()` return const-pointers. To modify the values,
there are two choices:

1.  Use `void PrefService::Set(const char* path, const Value& value)` to
            set a new dictionary value.
2.  Use a `ScopedUserPrefUpdate` (see `scoped_user_perf_update.h`) to
            modify a dictionary or list.
    For **updating a dictionary**, the code would look as follows:
    `DictionaryPrefUpdate update(pref_service, prefs::kPrefName);`
    `update->SetBoolean(“member_key”, true);`
    `update->Remove(“other_key”, NULL);`
    At the time when “update” leaves its scope, a “value has changed”
    notification is automatically triggered.

You **do not need to save modified preferences**. All set and update operations
trigger a timer to write the modified preferences to disk. This reduces the
number of incremental write operations. If you really need to be sure that the
preferences are written to disk (during shutdown), use
`PrefService::CommitPendingWrite()`.

The **PrefChangeRegistrar** allows to subscribe to preference change events. A
class that implements the `NotificationObserver` interface
(`notification_observer.h`) can have a member variable

```none
PrefChangeRegistrar registrar_;
```

In its constructor, the class would initialize the registrar as follows

```none
registrar_.Init(pref_service);
registrar_.Add(prefs::kPreference, this);
```

Any changes to `prefs::kPreference` would call the `Observe()` method of `this`.
The **PrefMember** classes (`BooleanPrefMember`, `IntegerPrefMember`, …, see
`pref_member.h`) are helper classes that stay in sync with preference values
beyond the scope of the UI thread. This allows simple reading of preference
values even outside the UI thread.

### PrefStores and Precedences

Chromium knows several locations where preferences can be stored:

<table>
<tr>
<td><b> Store</b></td>
<td><b>Purpose </b></td>
</tr>
<tr>
<td>managed_platform_prefs </td>
<td>contains all managed platform (non-cloud policy) preference values </td>
</tr>
<tr>
<td>managed_cloud_prefs </td>
<td>contains all managed cloud policy preference values </td>
</tr>
<tr>
<td>extension_prefs </td>
<td>contains preference values set by extensions </td>
</tr>
<tr>
<td>command_line_prefs </td>
<td>contains preference values set by command-line switches </td>
</tr>
<tr>
<td>user_prefs </td>
<td>contains all user-set preference values </td>
</tr>
<tr>
<td>recommended_platform_prefs </td>
<td>contains all recommended platform policy preference values </td>
</tr>
<tr>
<td>recommended_cloud_prefs </td>
<td>contains all recommended platform cloud preference values </td>
</tr>
<tr>
<td>default_prefs </td>
<td>contains all default preference values </td>
</tr>
</table>
Source:
[pref_value_store.h](https://code.google.com/p/chromium/codesearch#chromium/src/base/prefs/pref_value_store.h&q=file:pref_value_store.h%20%22decreasing%20order%20of%20precedence%22&sq=package:chromium&type=cs&l=34)

The locations are **sorted in decreasing order of precedence** meaning that a
higher preference store overrides preference values returned by a lower
preference store. This is important because it means for example that an
extension can override command-line configured preferences and that policies can
overrule extension controlled preference values.
The `default_prefs` `PrefStore` contains the values set by
`PrefService::Register...Value` (see above).
The four policy related `PrefStore`s are discussed in a chapter below.
The `user_prefs` `PrefStore` represents a persisted pref store. It contains
preference values configured by Chromium’s “Preferences” dialogs (and also
preferences set by extensions stored under extension-specific preference keys
“`extensions.settings.$extensionid.preferences`”).
The `extension_prefs` `PrefStore` maintains an in-memory view of the currently
effective preference values that are set by extensions. As multiple extensions
can try to override the same preference, this PrefStore respects their relative
preferences (more on this below). It provides a view over the
“`extensions.settings.*.preferences`” paths in the `user_prefs`.
The `command_line_prefs` represent preferences activated by command-line flags.

### Incognito Profile

The incognito profile is designed to overlay the regular profile. Preferences
defined in the regular profile are visible in the incognito profile unless they
are overridden.
The following table illustrates which `PrefStore`s are shared between the
profiles and which are handled differently.
<table>
<tr>
<td><b>Store in regular profile</b></td>
<td><b>Incognito profile</b></td>
</tr>
<tr>
<td>managed_platform_prefs </td>
<td>Same PrefStore as in regular profile</td>
</tr>
<tr>
<td>managed_cloud_prefs </td>
<td>Same PrefStore as in regular profile</td>
</tr>
<tr>
<td>extension_prefs </td>
<td>Special handling for different life-cycles (see below)</td>
</tr>
<tr>
<td>command_line_prefs </td>
<td>Same PrefStore as in regular profile</td>
</tr>
<tr>
<td>user_prefs </td>
<td>In-memory overlay that prevents persistence to disk</td>
</tr>
<tr>
<td>recommended_platform_prefs </td>
<td>Same PrefStore as in regular profile</td>
</tr>
<tr>
<td>recommended_cloud_prefs </td>
<td>Same PrefStore as in regular profile</td>
</tr>
<tr>
<td>default_prefs</td>
<td>Same PrefStore as in regular profile</td>
</tr>
</table>
Extension controlled preferences can be defined for three different scopes:

*   regular (inherited by incognito profile, persisted to disk)
*   incognito_persistent (overrides regular, persisted to disk)
*   incognito_session_only (overrides regular and incognito_persistent,
            not persisted to disk, life-time limited to one incognito session)

The in-memory overlay for user_prefs can be used to ensure that specific changes
to the preferences do not get persisted in the regular user prefs.

## Class Structure

The following figure illustrates the `PrefStores` (the figures use UML syntax:
dashed lines with triangles represent inheritance; solid lines with arrows and
filled diamonds represent composition; solid lines with arrows and empty
diamonds represent aggregation):<img alt="image"
src="https://lh4.googleusercontent.com/1uVGQHkigA9rUfiAyp9u9wpolF425l4wji6YNJne7Ct8RYb8blgyeTAPGbTjrDCCR9D9mCTsXLBzT8prDjeLvVf_n0XmBdXHF98npBjrVnXthNB5oMo5C6go0y1yGSUb">
`PrefStore` is an interface that allows read access and subscription to changed
values.
The `PersistentPrefStore` interface extends this with setter methods as well as
read and write operations to load and persist the preferences to disk. The
`JsonPrefStore` implements this interface to store the “Preferences” file in the
profile on disk. The `IncognitoUserPrefStore` provides means for an in-memory
overlay. This allows storing incognito preferences that are not written to disk
but forgotten when the last incognito window is closed.
The `ConfigurationPolicyPrefStore`s handle policy driven preferences. They are
discussed in a separate chapter.
The `CommandLinePrefStore`, `DefaultPrefStore` and `ExtensionPrefStore` store
the respective types of preference values. The `ExtensionPrefStore` is discussed
in a separate chapter. These three PrefStores are based on the
`ValueMapPrefStore` which provides an in-memory implementation of a `PrefStore`
and relies on the `PrefValueMap` for this.
The `PrefValueStore` is a facade around all `PrefStores` that enforces the
precedence presented above. It maintains ownership of the respective
`PrefStore`s and asks them one by one until the first `PrefStore` contains a
specific key. `PrefValueStore` also answers questions about where a preference
value comes from. For example, client code occasionally wants to know whether a
preference is under control of the user, i.e., not forced by configuration
management or extensions.
The `PrefService` is the global interface to preferences. It supports
registering default values, and simple access to currently effective preference
values. At this abstraction layer, the `PrefValueStore` becomes an
implementation detail of the `PrefService`:

<img alt="image"
src="https://lh3.googleusercontent.com/-3xUJutScqdsTVmiR1QCi0nPow-L2sBZBk17oOU7iWpKdfFqAkoSEpclcQXYRrHK9H3xwe28rHhzRmMtnZeeWHcX1hhHsiYHwUHwQZA2D3iplxqMHl5XVkCCjTIjrCfV">
The `PrefService` serves three purposes:

*   Registering default values for preferences.
*   Retrieving the currently effective preference value.
*   Storing preference values in the persisted user_prefs.

The `ExtensionPrefs` class manages preference values that extensions want to
set. It persists values to disk using the `PrefService` and feeds values to the
`ExtensionPrefStore` so that the `PrefValueStore` can consider extension
controlled preferences for effective preference values. The latter happens
indirectly. The `ExtensionPrefs` instance writes the preference values into the
`ExtensionPrefValueMap` which knows about regular and incognito preferences. The
`ExtensionPrefStore`, which is specific to either a regular profile or an
incognito profile, subscribes to the `ExtensionPrefValueMap` and retrieves the
respective regular or incognito preferences.

### Notification Mechanisms

The following figure indicates how preferences are set (blue arrows) and how
change notifications are propagated to subscribers (red arrows).

<img alt="image"
src="https://lh3.googleusercontent.com/f40v5VbhUvCRk6R0zHSwQ1K9IrRJ2q860-gywMk-VfEbVRwXquL8y8ZnUKaGiHq5tyB4iVUVQgaSB5o1VNy8jDFc5dxgagHfZEqZApACk349KQNhwDfqgQMgbEJ-zgi6">
The blue paths are triggered in three different ways:

*   Registering a default preference value: `PrefService` →
            `DefaultPrefStore`
*   Setting user preference (user modifies a setting in Chromium):
            `PrefService` → `JsonPrefStore`
*   Extension sets a preference: `ExtensionPrefs` →
            `ExtensionPrefValueMap` and for persistence beyond browser sessions:
            `ExtensionPrefs` → `PrefService` → `JsonPrefStore`

Helper classes like the `PrefChangeRegistrar` and `PrefMember` can be used to
subscribe to changes as outlined above.

## Extension Controlled Preferences

Some (selected) preferences can be controlled by extensions (such as the proxy
settings). These preferences need to be made available to extensions explicitly
as we do not want to allow extensions to configure all preferences.
Some particularities of extension controlled preferences need to be addressed:

*   Two or more extensions might write different preference values for
            the same preference. E.g. two extensions might want to set the proxy
            configuration. In this case it is important to ensure that one
            extension overrides the preference values of the other extension and
            to guarantee a **deterministic precedence**. This means that each
            time somebody requests the current value of a preference, he gets
            the same reply. The precedence rule is defined as **“last extension
            installed wins”**. This precedence order needs to be maintained even
            between browser restarts where extensions are loaded asynchronously.
            This is realized by adding a path
            `extensions.settings.$EXTENSION_ID.install_time` to the Preferences
            file.
*   The `ExtensionPrefStore` needs to **provide values even before
            extensions are started**. Imagine that a proxy extension provides
            proxy configuration values that are essential for any kind of
            network communication. Other extensions would not be able to
            communicate until the proxy extension has been started. For this
            reason, the Preferences file is extended by
            `extensions.settings.$EXTENSION_ID.preferences`. At this point we
            store a dictionary, whose keys are un-expanded path names that
            identify the preferences. Maintaining these entries and updating the
            `ExtensionPrefStore` that represents the winning preferences of
            extensions is task of the `ExtensionPrefs` class.
*   We want to allow using **different preference values for incognito
            windows**. We support two different kinds of incognito preferences:
    *   **persistent_incognito** preferences are persisted in the
                Preferences file as
                `extension.settings.$EXTENSION_ID.incognito_preferences`. Their
                life-time spans several browser sessions (i.e. browser restart)
                and can be used for settings about incognito windows that are
                NOT related to the specific browsing history of an incognito
                session. Otherwise we would defeat the purpose of the incognito
                mode (don’t store any browsing behavior related data).
    *   **session_only_incognito** preferences are not persisted. They
                can only be set once at least one incognito window has been
                opened and are cleared when the last incognito window is closed.

### Exposing preferences to extensions

In order to expose a preference to an extension, you need to follow these steps:
Add a property of type ChromeSettings to your extension namespace in the
respective file in `chrome/common/extensions/api`. This is an example from the
proxy extension:

```none
"properties": {
  "settings": { // “settings” is the name of the preference exposed to the extension
    "$ref": "ChromeSetting",
    "description": "Proxy settings to be used. The value of this setting is a ProxyConfig object.",
    "value": [
      "proxy", // this is an internal, unique key referred to as “preference key” below
      {"$ref": "ProxyConfig"} // this is the schema to which preferences need to comply
    ]
  }
}
```

Add an `PrefMappingEntry` to `kPrefMapping[]` in `extension_preference_api.cc`.
You require three entries:

*   Name of the preference as exposed to the extension (referred to as
            “preference key” above)
*   Name of the preference as handled within the browser (a string
            literal in the prefs:: namespace)
*   Name of the permission required to modify the preference

Optionally you can register a `PrefTransformer` to `PrefMapping::PrefMapping()`
in `extension_preference_api.cc`. This allows for a transformation of how
preferences are structured within the browser and in the extension. For example,
the Proxy Settings API exposes preferences differently to extensions than they
are stored internally.
By following these steps, we guarantee that all preferences are made available
in the same way. We provide `get()`, `set()`, and `clear()` functions as well as
an `onChanged` event.

## Configuration Policy

Configuration policy, a.k.a. admin policy, uses preferences as the main way of
exposing policy settings in the Chromium code. There are two levels policy can
come in:

*   **Mandatory policy**: Settings that are forced upon the user by
            their administrator. These policy settings are injected through the
            `managed_platform` and `managed_cloud`
            `ConfigurationPolicyPrefStore`s, which are the highest-precedence
            `PrefStore`s. Any user-visible configuration settings (e.g. in
            chrome://settings) should be disabled and merely show the preference
            value if a policy-configured preference value is present. To
            facilitate implementing this, there is
            `PrefService::Preference::IsManaged()` which will return `true` if
            there's a managed policy setting.
*   **Recommended policy**: At the low-precedence side of the
            `PrefStore` stack, there are two more
            `ConfigurationPolicyPrefStore`s: `recommended_platform` and
            `recommended_cloud`. These contain policy-configured values that are
            not forced on the user, but merely defaults provided by the
            administrator, which the user can still override. From the
            perspective of the Chromium code, recommended policy shouldn't
            require special handling in most cases; the PrefService will just
            return the policy-configured default when appropriate.

The `ConfigurationPolicyPrefStore` objects always keep the latest known policy
configuration. Behind the scenes, the policy settings are read from the
platform's native management APIs through the `ConfigurationPolicyProvider`
abstraction, which monitors the platform's policy configuration an notifies
`ConfigurationPolicyPrefStore` to reexamine policy configuration as provided by
the ConfigurationPolicyProvider on changes. ConfigurationPolicyProvider will
then map the new configuration to preferences and expose them to PrefValueStore,
generating `PREF_CHANGED` notifications as appropriate.

Covering the design of all `ConfigurationPolicyProvider` implementations is out
of scope for this document, but here is a quick overview (see also
<http://www.chromium.org/administrators>):

*   On Windows, `ConfigurationPolicyProviderWin` integrates with Windows
            Group Policy. Policy settings are read from the registry and
            Chromium watches for policy notifications to update policy settings
            when Group Policy changes.
*   Mac has a mechanism called "Managed Preferences" and
            policy-configured settings come as locked application preferences.
            They can be configured and distributed through the Workgroup Manager
            application and Apple's Directory Service.
*   For Linux, `ConfigDirPolicyProvider` reads policy settings from JSON
            files in `/etc`. The directories are monitored for changes so policy
            can be reloaded when files change.
*   On ChromeOS, there is code that can pull down policy settings from a
            device management service in the cloud.

## Command line parameters

In order to map command line parameters to preference values, you can edit
`command_line_pref_store.cc` and follow the examples therein. Note that it may
be advantageous to copy command-line flags into the `CommandLinePrefStore`. This
decouples our code from a static global `CommandLine` instance.
