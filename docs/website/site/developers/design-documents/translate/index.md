---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: translate
title: Translate
---

## Objective

The great thing about the web is that it lets people from all over the world
share information and ideas. Unfortunately that information is sometimes in a
language the user does not understand.
The goal of the Translate feature is to provide a way for users to translate a
page's text when it is not in the same language as the one Chrome is configured
with.

## Overview

The Translate feature works in conjunction with the Google Translate server,
which is actually responsible for translating the page.
Here is the basic scenario of a page translation:

*   the user visits a page in a different language
*   once the page has been loaded, the renderer extracts the text from
            the page and detects which language it's in and sends it to the
            browser process
*   the browser compares the page language to the language it is
            configured with. If they are different it shows an infobar with a
            button to translate the page
*   if the user clicks that button, the browser download a script for
            translating the page from the Google Translate server and sends it
            to the renderer
*   the renderer injects the script in the page and instructs it to
            translate the page
*   the render keeps polling the script for when translation is complete
            or if it has failed, and forward that notification to the browser
*   the browser displays an infobar to report that the page has been
            translated or that the translation failed

## Detailed typical scenario

### Page language detection

The renderer already extracts the text from each loaded page for indexing
purpose (so that users can look for pages they visited based on words these
pages contain). The language detection piggy-backs on that process. Once the
text has been extracted for indexing, a third-party library called CLD (Compact
Language Detection) is used to detect the language.
Note that the language detection is fairly fast but may not always be accurate
for certain pages.
The page language is then sent along with the page's text to the browser in the
ViewHostMsg_PageContents IPC message.

### Exception to the translation

Some pages may not want to be translatable. They might provide their own
translation mechanism (for example GMail) or may not be good target for
translation (heavy dynamic sites). To disable translation, they need to include
a specific meta tag (name "google", content should be "notranslate").
Whether the page should be a candidate for translation is also provided with the
ViewHostMsg_PageContents IPC message that is sent by the renderer.

### Prompting the user for translation

Once the ViewHostMsg_PageContents IPC message is received by the browser, the
TabContents sends a TAB_LANGUAGE_DETERMINED notifications. The TranslateManager
receives that notifications and shows the translate infobar to the user if
applicable. There are several transate related infobars (before translation,
during translation, after translation, error when translating). All these
infobars are controlled by the same delegate TranslateInfobarDelegate, which has
a type to determine which type of infobar it is.

### Initiating the translation

If the user requests the page to be translated, the TranslateManager first needs
to download the translate script from the Google server. The download script is
cached in the TranslateManager so it can be reused for future translations. We
do expire and refetch the script once a day, to ensure that newer versions of
that script are downloaded for people with long running browser sessions. The
TranslateManager then sends a ViewMsg_TranslatePage IPC message to the renderer
to start the translation. The translate script is passed along, with also the
original language and language the page should be translated to.

### Translating the page

On receiving the translate script, the renderer injects it in the page and start
the translation (see TranslateHelper::StartTranslation() ).
For security reasons, we do not want to the page contents to be able to notify
the renderer directly. So we poll the state of the translation regularly until
we are notified the translation is complete or has failed (see
TranslateHelper::CheckTranslateStatus()). Once we have been notified of such an
event, TranslateHelper sends a ViewHostMsg_PageTranslated message to the browser
which displays the "after translate" infobar to indicate to the user that the
translation was performed.
