---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/chromiumos-design-docs
  - Design Documents
- - /chromium-os/chromiumos-design-docs/text-input
  - Text Input
page_name: syncing-input-methods
title: Syncing Languages and Input Methods
---

## Background

In M41, Chromium OS [started syncing
preferences](https://codereview.chromium.org/312023002) for languages, keyboard
layouts, and extension IMEs. Because devices may have different physical
keyboard layouts, as well as different available locales and input methods,
changes on one device should not automatically propagate to another device.
However, we still wanted to provide a convenient out-of-box experience for users
who go beyond their device's default input method when they Powerwash their
system or sign in to a new device.

## Overview

Instead of syncing input methods both ways, we send changes to input methods
from the device to the sync server. This lets us "remember" the most recent
settings without overwriting existing preferences on other machines. When a user
signs in to a device for the first time, we combine the input method used to
sign in with the information from the sync server, adding the user's saved input
method preferences to the local preferences without overwriting any input
methods already set up.

For instance, a user should only have to manually add the Dvorak layout to a
device once. Then, we automatically add this layout to new devices for this
user, without affecting devices that have already been set up. If the layout
isn't available on a device, it's simply not added. In the worst case, we may
add an input method that is incompatible with the hardware keyboard, but the
user would still retain any input methods that they enabled before the first
sync.

## Design

### Preferences

We kept the original unsyncable preferences, which describe the local settings:

*   **settings.language.preferred_languages:** enabled language IDs
            (e.g., "en-US,fr,ko")
*   **settings.language.preload_engines:** preloaded (active) component
            input method IDs (e.g., "pinyin,mozc")
*   **settings.language.enabled_extension_imes:** enabled extension IME
            IDs

We added three syncable preferences:

*   **settings.language.preferred_languages_syncable**
*   **settings.language.preload_engines_syncable**
*   **settings.language.enabled_extension_imes_syncable**

During the first sync, the values of these preferences are merged with the local
preferences above. After that, these preferences are only used to keep track of
the user's latest settings. They never affect the settings on a device that has
already synced. In other words, they can be thought of as one-way, syncing from
the device to the sync server -- except during the first sync.

A fourth preference was added to indicate whether the initial merging should
happen: **settings.language.merge_input_methods**. This is set to true during
OOBE. After the first sync, this is always false. This ensures that devices set
up before M41 won't have their input settings changed.

### First Sync

[chromeos::input_method::InputMethodSyncer](https://cs.chromium.org/chromium/src/chrome/browser/chromeos/input_method/input_method_syncer.h)
was created to handle sync logic for input methods. After the first ever sync,
MergeSyncedPrefs adds the IME settings from the syncable prefs to the local
prefs using the following algorithm for each pref:

1.  Append unique tokens from the syncable pref to the local pref.
2.  Set the syncable pref equal to the merged list.\*
3.  Remove tokens corresponding to values not available on this system.
4.  Set the local pref equal to this validated list.

MergeSyncedPrefs also sets merge_input_methods to false, ensuring that it won't
be called again.

Notice that this solution doesn't assume the first sync happens right after
OOBE. For cases where users have set encryption passphrases, they could easily
add languages and input methods before enabling sync, so we support arbitrary
lists instead of single values during the merge.

### Updates

When a local input method pref changes, InputMethodSyncer updates the syncable
prefs to correspond to the local pref values.\* We update all three prefs at the
same time to ensure the settings are consistent -- since these will be the
values used the next time the user completes OOBE, the languages should
correspond to the input methods.

\* For preload_engines_syncable (input method IDs), we transform the list to use
legacy component IDs so the preference can sync between Chrome OS and Chromium
OS. During first sync, we convert this back to locally valid component IDs.
