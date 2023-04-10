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
page_name: language-detection
title: Language Detection for chunks of text
---

## Overview

## Add a new chrome.i18n.detectLanguage that enables CLD language detection for a chunk of user-supplied text.

## Use cases

## This API would be useful for extensions that want to translate parts of a page that are in a different language than the page itself. This is also useful in implementing an extension that translates user-supplied text (in a gadget, for example) into a language of the user’s choice, without needing to rely on an external server for language detection of the text. The existing chrome.tabs.detectLanguage API only provides language detection on a page-level granularity.

## Could this API be part of the web platform?

## No, this is intended to fill in a functionality gap with an existing chrome extension API, and not to introduce new web standards.

## Do you expect this API to be fairly stable?

## The set of languages that can be detected by the CLD may change over time, which affects the results of this function in a similar way as chrome.tabs.detectLanguage.

## What UI does this API expose?

## None.

## How could this API be abused?

## There’s nothing obvious that could be abused here.

## How would you implement your desired features if this API didn't exist?

## The only way to detect the language of a chunk of text from a page right now would be to transmit the text to a third party server and wait for a reply. This is not desirable as it introduces privacy concerns about sending text scraped from parts of a page to a server before a user explicitly requests they be translated.

## Are you willing and able to develop and maintain this API?

## Yes.

## Draft API spec

## Methods

## detectLanguage

## chrome.i18n.detectLanguage(string text, function callback)

## Detects the primary language of a supplied chunk of text. Language detection is more reliable for larger chunks of text (preferably 100 characters or more), but may be able to make a determination with small chunks for languages that use unique characters, such as Arabic or Hebrew.

## Parameters

## text ( string )

## The text to analyze.

## callback ( function )

## The callback parameter should specify a function that looks like this:

## function(array of DetectedLanguage languages) {...};

## result ( LanguageDetectionResult )

## Will contain up to three languages that were detected in text.

## Types

## DetectedLanguage

## ( object )

## language ( string )

## An ISO language code such as en or fr. For a complete list of languages that could be returned here, see [kLanguageInfoTable](http://src.chromium.org/viewvc/chrome/trunk/src/third_party/cld/languages/internal/languages.cc). The 2nd to 4th columns will be checked and the first non-NULL value will be returned except for Simplified Chinese for which zh-CN will be returned.

## confidence ( integer )

## A value between 0 and 100, inclusive, that indicates Chrome’s percentage confidence that the supplied text is in this language. High values indicate high confidence.

## LanguageDetectionResult

## ( Object)

## reliable ( boolean )

## True if language detection was considered reliable. This is the case when a single language emerges with a confidence value that is significantly higher than the next most likely language.

## languages ( array of DetectedLanguage )

## The possible languages that were detected for the supplied text, if any.

## Open questions

## Is it useful to return the raw confidence numbers for each language? These may change with each new implementation of the CLD.
