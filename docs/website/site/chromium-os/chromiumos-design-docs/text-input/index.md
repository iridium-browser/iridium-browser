---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/chromiumos-design-docs
  - Design Documents
page_name: text-input
title: Text Input
---

[TOC]

## OBSOLETE: The contents need to be updated

## Abstract

*   Chromium OS will be internationalized and will support many
            languages other than English
*   Chromium OS will support a variety of text input methods to make
            text entry as easy as possible for non-English languages
*   End user language and input-method settings are synchronized with
            the user's profile in the cloud to enable a consistent user
            experience from any device

## Objective

Build text input support for Chromium OS to make it well internationalized.

## Background

Chromium OS will be supporting multiple languages other than English. Regardless
of the UI language (the language shown in menu items and any other strings in
Chromium OS), Chromium OS should allow you to type in many languages. We will
build text input support for Chromium OS on top of the existing text input
technologies available for Linux.

## Requirements

Chromium OS will provide users several ways to type in text:

*   **Simple Keyboard:** Users type in characters directly from the
            keyboard. Examples: English, German
*   **Switchable Keyboard:** Users can switch keyboard mappings (for
            example, Latin/Cyrillic) and directly type in different character
            sets based on the current mapping. For example, Russian users can
            switch the keyboard mapping between Cyrillic and Latin to compose
            bilingual text.
*   **Simple IME:** Users type in most characters directly using a
            simple Input Method Editor (IME) that does some minor tasks such as
            validation, auto-correction, and composition. Examples: Korean, Thai
*   **Transliteration IME:** Users type in English characters that the
            Transliteration IME directly converts to native characters. Example:
            Indic
*   **Candidate IME:** Users type in all characters indirectly using an
            IME. For example, Japanese has too many characters to directly map
            on a keyboard. Using a Japanese IME, users type in the "sound" (for
            example, "nihon") to the IME, which provides a set of candidates
            (for example, "日本") that the user selects from.

There are also Chromium OS-specific requirements, such as supporting both x86
and ARM processors and following Chromium OS UI design guidelines. Meeting all
of these requirements is not trivial. We'll reuse existing input technologies as
much as possible.

We will support the following languages by any of the ways listed above.

<table>
<tr>
<td>Arabic</td>
<td> Bulgarian</td>
<td> Catalan</td>
<td> Croatian</td>
<td> Czech</td>
<td> Danish</td>
<td> Dutch</td>
<td> English (GB)</td>
<td> English (US)</td>
<td>Estonian</td>
<td>Filipino</td>
<td>Finnish</td>
<td>French</td>
<td> German</td>
<td> Greek</td>
<td> Hebrew</td>
<td> Hindi</td>
<td> Hungarian</td>
<td> Indonesian</td>
<td> Italian</td>
<td> Japanese</td>
<td> Korean</td>
<td>Latvian</td>
<td> Lithuanian </td>
<td> Norwegian</td>
<td> Polish</td>
<td> Portuguese (Brazil)</td>
<td> Portuguese (Portugal)</td>
<td> Romanian</td>
<td> Russian</td>
<td> Serbian</td>
<td>Simplified Chinese</td>
<td>Slovak</td>
<td>Slovenian</td>
<td> Spanish</td>
<td>Spanish in Latin America</td>
<td> Swedish</td>
<td> Thai</td>
<td> Traditional Chinese</td>
<td> Turkish</td>
<td> Ukrainian</td>
<td> Vietnamese</td>
</tr>
</table>

## Design

### Basic architecture

For languages that can be supported by Simple Keyboard or Switchable Keyboard,
we'll use [XKB](http://en.wikipedia.org/wiki/X_keyboard_extension) (X Keyboard
Extension). For languages in other categories, we'll use
[IBus](http://en.wikipedia.org/wiki/Intelligent_Input_Bus) (Intelligent Input
Bus). The following diagram illustrates the basic architecture of text input
support for Chromium OS.

> [<img alt="image"
> src="/chromium-os/chromiumos-design-docs/text-input/basic-architecture2.png">](/chromium-os/chromiumos-design-docs/text-input/basic-architecture2.png)

### Keyboard layout: XKB

We will create the XKB definition file for each keyboard layout.

### Input method framework: IBus

We will use IBus as the input method framework (IM framework). IBus is an
open-source input method framework that works as a middle layer between
applications and language-specific backend conversion engines.

We decided to use IBus for the following reasons.

*   It's the default IM framework for Fedora 11 and Ubuntu 9.10.
*   No rebooting is necessary (unlike older IM frameworks) when major
            configurations are changed, such as when adding a new language.
*   Apps should be more stable. Unlike older IM frameworks, crashes in
            conversion engines won't affect applications.
*   IBus is implemented on top of D-Bus, which is also used in other
            components of Chromium OS.

### Conversion engines

Many conversion engines are available for IBus, such as ibus-pinyin for Chinese.
We'll use the following open source conversion engines.

<table>
<tr>
<td><b>IBus engine</b> </td>
<td> ibus-pinyin</td>
<td><b> ibus-chewing</b> </td>
<td><b> ibus-table</b></td>
<td><b> ibus-hangul</b></td>
<td><b>Language</b> </td>
<td> Chinese (Simplified)</td>
<td> Chinese (Traditional - Chewing)</td>
<td> Chinese (Traditional - Bopomofo and Cangjie)</td>
<td> Korean</td>
</tr>
</table>
New: For Japanese, we plan to port [Google Japanese
Input](http://www.google.com/intl/ja/ime/) to Chromium OS.

### User experience

We'll make the text input experience be uniform with the user experience in
Chromium OS. To achieve this goal, we'll rewrite all user interfaces using
Chromium's UI toolkit library called Views.

**Candidate window**

Will be implemented as separate process. Communicates to IBus-daemon via D-Bus.

New: Here's a screenshot of a work-in-progress version of the candidate window.
[A demo](/system/errors/NodeNotFound) is also available.

> [<img alt="image"
> src="/chromium-os/chromiumos-design-docs/text-input/candidate_window.png">](/chromium-os/chromiumos-design-docs/text-input/candidate_window.png)

**XKB configuration dialog**

Will be part of Chromium's options dialog. Communicates to X server via libcros.

**IBus configuration dialog**

Will be part of Chromium's options dialog. Communicates to IBus-daemon via
libcros.

**Conversion engine specific configuration dialogs**

Will be part of Chromium's options dialog. Communicates to conversion engines
via libcros.

**Input language selector**

Will be part of Chromium's status area (near the time and the power indicator).
Communicates to IBus-daemon and X Server via libcros. We'll integrate the input
language selector with the keyboard layout selector, unlike traditional Linux
desktops. Here's a mockup of what it will look like:

> [<img alt="image"
> src="/chromium-os/chromiumos-design-docs/text-input/input.png">](/chromium-os/chromiumos-design-docs/text-input/input.png)

**Input language selector (on login manager)**

Probably not required, because Google accounts (that is, email addresses) used
for login are all in ASCII characters, as are the passwords. We will implement
it once it becomes necessary. Internationalized keyboards can be supported by
XKB configurations.

**Sync with the cloud**

Text input-related data is synced with the cloud. We sync the user's choices of
languages, keyboard layouts, and input method extensions as described in
[Syncing Languages and Input
Methods](/chromium-os/chromiumos-design-docs/text-input/syncing-input-methods).

In the future, we may sync:

*   User settings for XKB, IBus, each IME, and so on.
*   User input history used for better conversions (optional)
*   User-defined dictionaries (optional)

Syncing configuration data is a must, but users can opt out of syncing other
user data.

**Security considerations**

An indirect input layer like IBus can be a security weak point, as it can be
exploited as a key logger. We plan to use
[GRSecurity](http://en.wikipedia.org/wiki/Grsecurity) to restrict resource
access from IBus and conversion engines. We also plan to review the security of
IBus.

**Long-term goals**

Although the following goals aren't targeted for the initial version, these are
some of our long-term goals:

*   Better integration with browsers, such as changing behavior
            depending on the input form.
*   Multi-modal input, such as hand writing and voice recognition.
*   Virtual keyboard (aka software keyboard). Web-based virtual keyboard
            can be used in the initial version, though.
*   Multi-keypress input

## Code location

### IBus

*   chromium-os/src/third_party/ibus/ (Tip of tree of IBus. We'll use
            the ibus-daemon, ibus-pinyin, ibus-chewing, ibus-table, and
            ibus-hangul modules as-is. However, we won't use the ibus-setup or
            ibus-panel modules.)

### Configuration dialog and language selector

*   [chromium/src/chrome/browser/chromeos/options](http://src.chromium.org/viewvc/chrome/trunk/src/chrome/browser/chromeos/options/)
            (configuration page)
*   [chromium-os/src/platform/cros](http://src.chromium.org/cgi-bin/gitweb.cgi?p=cros.git;a=summary)
            (libcros part, replacement for ibus-setup and ibus-panel)
    *   [chromeos_ime.cc](http://src.chromium.org/cgi-bin/gitweb.cgi?p=cros.git;a=blob;f=chromeos_ime.cc;hb=HEAD)
                - APIs used for implementing the candidate window
    *   [chromeos_language.cc](http://src.chromium.org/cgi-bin/gitweb.cgi?p=cros.git;a=blob;f=chromeos_ime.cc;hb=HEAD)
                - APIs used for manipulating input language status.

### Candidate window

*   [chromium/src/chrome/browser/chromeos/text_input](http://src.chromium.org/viewvc/chrome/trunk/src/chrome/browser/chromeos/text_input/)
