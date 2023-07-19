---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: ui-localization
title: Add & translate strings (aka 'Localization' or 'Translations')
---

**Help improve our translations:**
* **[File a bug](https://bugs.chromium.org/p/chromium/issues/entry?template=Translation%20Issue)** using the translation template; CC "chromelocalization@google.com"
* **[Provide translations](https://davidplanella.org/chromium-opens-to-community-translations-in-launchpad/)** for the chromium-browser package on Ubuntu

---

[TOC]

Chromium uses [GRIT](/developers/tools-we-use-in-chromium/grit) for managing its
translated strings in grd/grdp files.

Base messages are written in U.S. English (en-US) and are translated into
supported language locales by an internal Google Localization process.
Translations are submitted to the chromium
[.xtb](https://source.chromium.org/search?q=android%20file:%5C.xtb&sq=&ss=chromium)
files after a few weeks.

Strings are included on all platforms by default and will needlessly increase
the download size if they're not used. It's important to judiciously surround
strings with appropriate &lt;if&gt; clauses to ensure that they are only
included on the platforms where they're actually used.

### How strings get translated

1.  Strings get added to a .grd(p) file in en-US.
2.  \[BlackCloudOfMagic\] Translators are provided with the new strings.
3.  &lt;Googler-only link&gt; Further internal detail about the
    translation process
    [here](https://g3doc.corp.google.com/chrome/language/g3doc/localization.md).
    &lt;/Googler-only link&gt;
4.  \[BlackCloudOfMagic\] Translations are created and stored in .xtb
    files
5.  Changes to .xtb files are submitted to the Chromium source tree

## Add a string

If you're developing for Mac, also see notes on [how to add strings for Chrome on Mac](/developers/design-documents/ui-localization/mac-notes).

1.  Check whether the string you need already exists. If you find a
    matching string, read the description to see how it's intended to be
    used, and leverage it if you can. Otherwise, continue.
2.  Add the string to the grd file (`generated_resources.grd`,
    `webkit_strings.grd`, `chromium_strings.grd` or
    `google_chrome_strings.grd`).
    *   See tips on [writing better user-facing strings](/user-experience/ui-strings).
3.  Include
    [screenshots](/developers/design-documents/ui-localization#add-a-screenshot),
    [meanings](/developers/design-documents/ui-localization#use-message-meanings-to-disambiguate-strings),
    and
    [descriptions](/developers/design-documents/ui-localization#write-good-descriptions)
    for all strings. This is crucial for high-quality translations.
4.  Use [ICU message syntax](https://unicode-org.github.io/icu/userguide/format_parse/messages/) to
    accommodate plurals and gender.
5.  As needed, surround the string with an appropriate &lt;if&gt; clause
    to ensure that it's only included it on platforms where it's
    actually used (e.g., Mac and Linux); this avoids needlessly
    increasing the download size. Strings used on all platforms can omit
    the &lt;if&gt; clause.
6.  The next time you build the solution, this will automatically add
    the en-US string.
7.  In your code, include `ui/base/l10n/l10n_util.h` and
    `chrome/grit/generated_resources.h`.
8.  To get the string, use `l10n_util::GetStringUTF16`. Alternately, you
    can use `l10n_util::GetStringFUTF16` which will replace placeholders
    $1 through $4 with the extra arguments of GetStringFUTF16. Note that
    we generally prefer to use UTF-16 encoded strings for user-visible
    text.
9.  Deal with message placeholder content. Example:

    ```
    Hey <ph name="USER">%1$s<ex></ex></ph>, you have <ph name="NUMBER"><ex>10</ex>$2</ph> messages.
    ```

## Add an Android/Java string

1.  Check whether the string you need already exists. If you find a
    matching string, read the description to see how it's intended to be
    used, and leverage it if you can. Otherwise, continue.
2.  Add the string to the grd file in the Java folder (e.g.
    `content/public/android/java/strings/android_content_strings.grd`).
    Strings should be named in all caps with the prefix `IDS_`. For
    example: `IDS_MY_STRING`.
    *   See tips on [writing better user-facing strings](/user-experience/ui-strings).
3.  Include
    [screenshots](/developers/design-documents/ui-localization#add-a-screenshot),
    [meanings](/developers/design-documents/ui-localization#use-message-meanings-to-disambiguate-strings),
    and
    [descriptions](/developers/design-documents/ui-localization#write-good-descriptions)
    for all strings. This is crucial for high-quality translations.
4.  At build time, an Android-style strings.xml file will be generated
    in the out directory.
5.  You can reference these strings in Java code using the standard
    `R.string.my_string` syntax, or in Android layouts using
    `@string/my_string`. Note: the name is lowercase and the `IDS_` prefix
    is stripped.
6.  Deal with [special characters](https://developer.android.com/guide/topics/resources/string-resource#FormattingAndStyling).
7.  Deal with message placeholder content. Example:

    ```
    Hey <ph name="USER">%1$s<ex></ex></ph>, you have <ph name="NUMBER"><ex>10</ex>$2</ph> messages.
    ```

## Add a new grd(p) file

This should be rare. We want to be very careful about expanding the number of
grd(p) files in our source tree.

**General notes:**

*   New grdp files need to be referenced from a parent grd file.
*   If the resources will be referenced from C++ code, make sure to do
    the following actions. This doesn't impact the translation process,
    but your new grd file won't compile without them.
    *   New grd files need to be added to `/src/tools/gritsettings/resource_ids`
    *   New grd files need to have a rule added to
        `/src/chrome/chrome_resources.gyp`. During `gclient sync`, these
        grit rules are run, and your grd's `.h` file will be generated.
        This file should be included by any `.cc` file that references
        your strings.

**Notes for translation:**

*   New grd files need to be added to
    `/src/tools/gritsettings/translation_expectations.pyl` (or
    &lt;message&gt;s won't be translated).
*   If your new grd(p) will result in new XTB files after translation,
    you must commit placeholder .xtb files or else Chrome won't compile.
    The placeholders need to have a basic xml structure. Example for
    creating the XTBs:

    ```
    > for lang in fr de en-GB etc; do echo '<?xml version="1.0" ?><!DOCTYPE translationbundle><translationbundle lang="'$lang'"></translationbundle>' > foo_strings_$lang.xtb; done
    ```

*   If your new grd will NOT be translated (set in
    translation_expectations.pyl and no XTB placeholder files required
    above), there is very minimal XML content required in your grd.
    Example:

    ```
    <?xml version="1.0" encoding="utf-8"?>
    <!--
    This file contains all "about" strings.  It is set to NOT be translated, in translation_expectations.pyl.  en-US only.
    -->
    <grit base_dir="." latest_public_release="0" current_release="1"
        source_lang_id="en" enc_check="möl">
    <outputs>
        <output filename="grit/about_strings.h" type="rc_header">
        <emit emit_type='prepend'></emit>
        </output>
    </outputs>
    <release seq="1" allow_pseudo="false">
        <messages fallback_to_english="true">
        <message name="IDS_NACL_DEBUG_MASK_CHOICE_DEBUG_ALL">
            Debug everything.
        </message>
        </messages>
    </release>
    </grit>
    ```

---

## Give context to translators

### Add a screenshot

Screenshots of Chrome's UI strings can provide more and clearer context for our
translators. Requests for screenshots make up at least 5% of localization bugs.

Follow the steps at
[**https://g.co/chrome/translation**](https://g.co/chrome/translation) to add a
screenshot to correspond with your strings.

### Use message meanings to disambiguate strings

**When?** Existing translations are reused when a new message matches an
existing one. We rely heavily on reused translations to keep our localization
costs manageable. If your string has a specific need to be treated differently
than existing strings with identical text (because of a unique context or new
restrictions like length or capitalization), add a meaning attribute. Messages
with different meaning attributes will be translated separately. Adding a
meaning to an existing string will trigger a retranslation.

The meaning attribute may specify a need for disambiguation, due to:

*   The nature of the message (e.g., action, description, accessibility,
    ID)
*   The nature of the word (e.g., noun, adjective, verb). For example,
    "click" may be translated to "clic" or "clique" depending on
    context.
*   Any constraints (e.g., length)
*   Any homonymy disambiguation
*   Don't include: General context for the message (that should go in
    the description attribute)

**How?** In the example below, we want to apply specific capitalization rules,
so we can't reuse the existing translation. We added a meaning to the string so
that the automated Translation Console tool disambiguates this string. The
meaning's details will help future Chromium contributors understand the
difference among identical strings; however, the translators will not see the
`meaning` attribute:

```
<message name="IDS_BOOKMARK_BUBBLE_PAGE_BOOKMARKED"
  meaning="In Title Case for Apple OS"
  desc="In Title Case: Title of the bubble after bookmarking a page.">
  Bookmark Added!
</message>
```

### Write good descriptions

**Why?** The message description is a critical piece of context our translators
receive when translating UI strings. Translators see each string in isolation
and in a random order: they don't know which feature the string is associated
with, where it might appear on a page, or what action it triggers. Adding enough
context to each string in a project increases the speed, accuracy, and quality
of translations, which ultimately improves user satisfaction. For example,
of the following code:

```
<message name="IDS_BOOKMARK_BUBBLE_PAGE_BOOKMARKED"
  desc="In Title Case: Title of the bubble after bookmarking a page.">
  Bookmark Added!
</message>
```

the translator would only see:
* "In Title Case: Title of the bubble after bookmarking a page."
* "Bookmark Added!"


**How?** Add as much of the info below in the message description as would be
useful:

*   Location: Button, title, link, drop-down menu, etc.
*   Grammar: Is this a noun or a verb? If it's an adjective, what is it
    describing?
*   Audience: Who is the intended audience?
*   Causality: What causes this message to appear? What is the next
    message that will appear?
*   Placeholder: What do the placeholders mean? What are a few values
    that your placeholder may display? If you have multiple
    placeholders, do they need to be arranged in a certain way? Will
    this message replace a placeholder in another message?
*   Length: Does this message have to have fewer than some number of
    characters? (For example, mobile products have limited UI space.)
    How should line breaks be dealt with? Are there character limits per
    line?

Keep in mind that the translators will not know the product as well as you do,
and they work on multiple products and projects. Also, they're not engineers.
So make sure that the description will be understandable by a more general
audience and doesn't contain too much technical detail. Imagine giving this
description out of context to a person who isn't on your project, like your
cousin. Would they still understand it?

Note: changing a message description without changing the message itself or
adding a meaning attribute does not trigger a retranslation of a string.

**Examples of good message descriptions:**

* **Source text**: "US city or zip" <br>
  **Description**: The message is displayed in gray in an empty search box for
  a movie showtimes location. Localize by country to name city and optional
  postal code. <br>
  **Comment**: This description clearly explains where the source text appears
  in the UI, and gives instructions on how the message should be adapted
  for non-US locales. <br>

  <br>

* **Source text**: "Zoom" <br>
  **Description**: Clicking the Zoom menu command launches help on how to zoom.
  Try to limit translations to 10 characters. <br>
  **Comment**: A thorough description tells what the source text plays does,
  what it triggers, and states a character limit as well as a rationale. <br>

  <br>

* **Source text**: "Account budget increased" <br>
  **Description**: This text is the placeholder in the sentence 'PLACEHOLDER
  from X to Y' <br>
  **Comment**: Since the string is only part of a sentence, this description
  provides essential information about context, and alerts the translator
  that it will appear as part of a longer UI message. <br>

  <br>

**Examples of poor message descriptions:**

* **Source text**: "All" <br>
  **Description**: The word "All" in the phrase "Select: All None" <br>
  **Comment**: Good immediate context, but unclear what "all" refers to. <br>
  **Suggested description**: Appears in the phrase "Select: All None" and
  refers to message threads. <br>

  <br>

* **Source text**: "PLACEHOLDER from X to Y" <br>
  **Description**: Describes account budget changes <br>
  **Comment**: It is not clear what the placeholders are used for. <br>
  **Suggested description**: Describes account budget changes; PLACEHOLDER is
  one of "account budget increased" or "account budget decreased"; X and Y
  are both dollar amounts. <br>

  <br>

* **Source text**: "We could not send your message. A space alien ate it. Please
  try again in a few minutes." <br>
  **Description**: A funny error message. <br>
  **Comment**: Translators will translate the message literally, so if they
  should be creative, the message description should let them know that.
  Also, "space alien" may not be culturally appropriate. <br>
  **Better Description**: An error message. Since humor varies across
  cultures, use an appropriate translation for the error. It doesn't have
  to be a direct translation. <br>

  <br>
