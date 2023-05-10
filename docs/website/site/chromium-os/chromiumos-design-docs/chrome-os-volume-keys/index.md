---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/chromiumos-design-docs
  - Design Documents
page_name: chrome-os-volume-keys
title: Chrome OS Volume Keys
---

**# **## Current, intended behavior (since mid-2011)****

**When the user presses one of the three volume-related keys (Mute, Volume Down,
or Volume Up), a bubble appears in the lower-right corner of the screen
displaying the current volume level.**

**<img alt="image" src="https://lh5.googleusercontent.com/Nm9_2BQJ5T3Sqt3rD6ZYejyox3OcpxsoA61mc8NM9Aug_19_i4bnXDmZtO3BA0uFIXMXfBkCoMVvpdheGIIxHjwZpUWz1wAp811TBxJ_Ry79bd1X6hQ" height=116px; width=330px;>**

**Non-muted and ~60% of maximum level**

**At the left, an icon shows whether the system is muted or not, and if not
muted, a coarse approximation of the current volume level (via zero, one, or two
sound waves emanating from a speaker). To the right of the icon, a toggle button
displays the current muting status.**

**<img alt="image" src="https://lh3.googleusercontent.com/aFZMhLBlyas-l9XSOCg8W-KMonwPhnf4J9pt0iK9MbVaBnlRANM0_gck1ZsDH9vLHZkoZ5xu842ZviGA84WVzhj5BuGU__9LWai_R9g5KekYmB6sNDs" height=116px; width=330px;>**

**Muted with ~60% level saved**

**Either the icon or the toggle button can be clicked to toggle the muting status. At the right side of the bubble, a horizontal slider displays the current volume level. If the system is muted, the slider is displayed in an inactive state; note that the previously-set volume level is still visible, though.**
**The same controls are present in the larger bubble that is displayed when the
user clicks the system / status tray in the lower-right corner of the screen:**

**<img alt="image" src="https://lh4.googleusercontent.com/ibZ7w_Ljo-MURsOQhV0Dx-XZPpdDU5fLEKNKSOSLkh6BlyyiTvsk94XiNWQfLpVSpYLemRluuqywKIFvTKJDB_uUbLyl381r2Zby4kUUkkkpzSrdRJ8" height=362px; width=324px;>**
**The volume keys’ current behavior was initially proposed in [chromium-os bug 13618](https://code.google.com/p/chromium-os/issues/detail?id=13618). Briefly,**

**### When not muted:**

**Mute: mutes instantly**
**Volume Down: decreases the volume**
**Volume Up: increases the volume**

**### When muted:**

**Mute: does nothing**
**Volume Down: sets the saved volume to zero, keeping the system muted**
**Volume Up: unmutes and restores saved volume (increasing it slightly if zero)**

**## Objections**

**### Many other operating systems, A/V equipment, etc. provide a “toggle mute” button instead.**

**When in an environment where sound is undesirable (e.g. library, shared office, etc.), the user is guaranteed that pressing the Mute key will instantly mute the system; there is no risk of toggling sound back on if the system was already muted. If the user wants to set the volume to the lowest audible level, only three key-presses are needed (Mute, Volume Down, and then Volume Up).**

**### But the user can just hold the Volume Down key if they want to make sure the volume is at 0%.**

**This is a poor substitute; decreasing the volume from 100% to 0% takes more than a second and the user is unable to easily restore the original volume level later.**

**### The toggle button between the icon and the slider is confusing (see e.g. [this bug](https://code.google.com/p/chromium/issues/detail?id=170935)).**

**The button was originally labeled, but the label’s visual appearance was sub-optimal ([chromium bug 143426](https://code.google.com/p/chromium/issues/detail?id=143426)). Per [chromium bug 152070](https://code.google.com/p/chromium/issues/detail?id=152070), the unlabeled button will be merged into a more-meaningful icon (as tracked in [chromium bug 137947](https://code.google.com/p/chromium/issues/detail?id=137947)).**
