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
page_name: i18n
title: i18n for extensions
---

[TOC]

## Introduction

Developers tend to hard-code messages in the code, in their native language, or
more often in English, and we need to provide simple enough message replacement
system for them to prevent that behavior.

There are two approaches we can use, one widely known Firefox ENTITY
replacement, and the other Google Gadgets are using.

### Google Gadgets approach

Google gadgets are based on XML spec, which carries some metadata and html/js of
a gadget.

Each spec lists supported locales, and links to locale files. Any item can be
replaced within the gadget spec (urls, messages...). Substitution is done in
container code (iGoogle, Orkut...), where we control the whole process (fallback
order for example).

Message catalogs are in XML format (to better support our translation pipeline).

Public API, sample gadget spec and message bundles could be found at
<http://code.google.com/intl/sr-RS/apis/gadgets/docs/i18n.html>.

### Firefox approach

Firefox is using XUL files for their extensions (XML format). XML parser
automatically replaces DTD ENTITYs within a XML document given DTD file(s). For
details see [how to localize firefox
extensions.](http://www.rietta.com/firefox/Tutorial/locale.html)

Problem with this approach is that we don't actually have XML/XHTML files tied
to an extension. Also, we may want to implement more flexible fallback
algorithm.

## Proposed solution

We use HTML/JS to develop extensions, and to keep metadata about extensions
(manifest) vs. XUL files for Firefox.

We should use modified Google Gadget approach since they are too HTML/JS
entities:

*   Messages are stored in **JSON** objects (key-value) - one object per
            catalog.
*   Message placeholders are in **__MSG_message_name__** format.
*   Provide formatted messages, **getMessage(**formatted_msg_name,
            \[arg1, arg2,...\]**)**. See
            <http://code.google.com/chrome/extensions/i18n.html> for actual
            implementation.
*   **Single catalog** per locale. This makes fallback and conflict
            resolution algorithms much simpler, and it avoids need for
            namespaces. Multiple catalogs could help with pre-made translations
            (like city names, country names, common phrases like Open, Close,
            Extension version...) but this could be achieved by unifying all
            subcatalogs into a single catalog and resolving ID conflicts.
*   Catalogs are placed at the **fixed location**, to avoid storing that
            information in the source files (like links to catalogs in gadgets).

See details below.

### Locale fallback

Only some locales will have all of the messages translated, or resources
generated. Some locales may be completely missing. In both cases Chrome should
gracefully fall back to what's available.

To do that we need to order locales in tree like structure based on locale
identifiers.

*   Use full locale identifier as locale name -
            **language**_**script**_**country@locale_extensions**. For full
            description of locale identifier look at [Unicode Locale
            Description.](http://www.unicode.org/reports/tr35/#Unicode_Language_and_Locale_Identifiers)
            Script and locale extensions are optional. Extended parameters can
            be sorting order (dictionary vs. phonebook) or anything locale
            specific.
*   Fallback goes from specialized to generic, i.e. : sr_latn_rs -&gt;
            sr_latn -&gt; sr -&gt; default_locale.
*   default_locale is a root locale.

#### Supported locales

We support larger set of locales than Chrome UI. Current list is (as of 35300):
am, ar, bg, bn, ca, cs, da, de, el, en, en_GB, en_US, es, es_419, et, fi, fil,
fr, gu, he, hi, hr, hu, id, it, ja, kn, ko, lt,

lv, ml, mr, nb, nl, or, pl, pt, pt_BR, pt_PT, ro, ru, sk, sl, sr, sv, sw, ta,
te, th, tr, uk, vi, zh, zh_CN, zh_TW

### Replacement policy

To avoid hard-coding strings, developer should use message placeholders in the
code/static files.

*   Use message placeholders, like __MSG_some_message__.
*   For formatted messages (positional arguments) use $1 - $9.

Message concatenation is usually a bad thing, and should be avoided, but it's
possible with __MSG_msg_1__ + __MSG_msg_2__.

### Message container

Message placeholders and message bodies have simple key-value structure, which
can be implemented as:

*   JSON - native to everything we do.
*   Message bundles should be UTF-8 encoded.

**Proposed JSON format**:

```
{
  "name": {
  "message": "message text - short sentence or even a paragraph with a optional placeholder(s)",
  "description": "Description of a message that should give context to a translator",
  "placeholders": {
    "ph_1": {
      "content": "Actual string that's placed within a message.",
      "example": "Example shown to a translator."
    },
    ...
  },
  ...
  }
}
```

**Example**:
```
{
  "hello": {
    "message": "Hello $YOUR_NAME$",
    "description": "Peer greeting",
    "placeholders": {
      "your_name": {
        "content": "$1",
        "example": "Cira"
      },
      "bye": {
        "message": "Bye from $CHROME$ to $YOUR_NAME$",
        "description": "Going away greeting",
        "placeholders": {
          "chrome": {
            "content": "Chrome",
          },
          "your_name": {
            "content": "$1",
            "example": "Cira"
          }
        }
      }
    }
  }
}
```

*   *name* is the name of the message used in message substitution
            (__MSG_**name**__, or getMessage(**name**)). It's a key portion to
            the value that holds message text and description. Name has to be
            unique per catalog, but message body and description don't have to.
            Message name is case insensitive.
*   *message* is actual translated message. Required attribute.
*   *description* is there to help a translator by giving context of the
            message (or better description). Optional attribute.
*   *$NAME$* section defines a placeholder, and it's an element
            indicating a placeholder in your message. Placeholders provide
            immutable English-language text to show to translators in place of
            parts of your messages that you don't want them to edit, such as
            HTML, Trademarked name, formatting specifiers, etc. A placeholder
            should have a name attribute (ph_1) and preferably an example
            element showing how the content will appear to the end user (to give
            the translator some context). All A-Z, 0 - 9 and _ are allowed.
            Placeholder section is optional if message has no $NAME$ sections.
            Placeholder name is case insensitive.
*   *content* is *is the actual message text placeholder is replacing.
            It's required within a placeholder section.*
*   *example* is an example of what the contents of a placeholder will
            look like when actually shown to a customer. These are used by the
            translators to help understand what the placeholder will look like.
            Examples are optional but highly recommended; there should be one
            for each placeholder. For example, a "$1" formatting string that
            will be populated with a dollar amount should have an example like
            "$23.45". For HTML tags or reserved words, the content of the
            example is the content of the message string itself. It's optional
            within a placeholder section.

### Message format

There are couple of possible forms message can take:

*   simple text - "Hello world!".
*   formatted message with positional arguments - "Hello $PERSON_2$ and
            $PERSON_1$" - argument 2 will be printed after Hello, and argument 1
            after and. Translator can re-order positional parameters depending
            on the language.

### Conflict resolution

Same message ID should exist only once per catalog. If there are duplicates -
detected when packing extension - we should ask developer to remove them.

### Plural form

Dealing with plural forms is hard. Each language has different rules and special
cases. To avoid complexity we are going to use plural neutral form.

Instead of saying "11 file were moved" we could say "Files moved: 11".

This is a valid solution in most cases.

### Chrome API

Chrome will automatically replace all message placeholders when loading static
files (html, js, manifest...) given the current browser UI language.

Scripts may want to use messages from different locales, or to fetch resources
and replace message placeholders in them dynamically.

For that we may need:

*   chrome.i18n.getMessage("message") would return message translation
            for current locale

### Structure on disk

There would be a _**locales** subdirectory under main extension directory.

It would contain N subdirectories named as locale identifiers (sr, en_US, en,
en_GB, ...).

top_extension_directory/_**locales**/**locale_identifier**/

Each locale_identifier subdirectory can contain only one **messages.json** file.

Extension manifest has an optional "default_locale": "language_country" field
that points to default language. Some edge cases:

*   There is only one locale in the package - fail if it doesn't match
            default_locale.
*   default_locale is missing, and we have more than one locale in the
            package - fail. This case shouldn't happen - we wouldn't create
            package like that.
*   default_locale points to a locale not present in the package - fail.

Default locale is used as final fallback option if message couldn't be found for
current locale.

Use cases

### Manifest

Manifest file contains metadata about extension in JSON format.

When loading manifest file, Chrome should replace all __MSG_msg_name__
identifiers with messages from the catalog and then process the final object.

### HTML in general

New tab page and possibly some other static content. We currently use google2
template system? which is somewhat an overkill for couple of pages.

We could deliver message catalogs for each locale as part of installation
package, and use message placeholders in new tab source.

### External resources

All absolute urls (like href, src...) should be pointed to __MSG_some_url__, and
each locale could provide separate implementation (image, script...).

On loading extension files - html, js - Chrome would replace all
__MSG_some_url__ with actual, locale specific, url.

### Local Resources

Local resources, like &lt;img src="foo/bar.png"&gt; should be auto resolved to
_locale/current_locale/foo/bar.png or if that resource is missing to fallback
location.
