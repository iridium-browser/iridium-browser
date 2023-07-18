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
page_name: preference-api
title: Preferences API
---

### Overview

The preferences API allows you to manipulate the browser preferences, such as
the accept languages, URLs to restore on startup etc. For the full list of
preferences, please refer to chrome/common/pref_names.cc. (Note: since the first
version will only implement the access of accept languages, this document will
focus on accept languages below.)

### Use cases

It allows extensions to read and write the browser preferences. Given accept
languages as an example, page translation extension and dictionary extension
will need to get the accept languages from the browser and use them as the
targeted languages for page or word translation.

### Could this API be part of the web platform?

Given accept languages as an example, read accept languages could be part of the
web platform, it could be exposed by window.navigator.acceptLanguages while UI
language is exposed through widow.navigator.language. But we would also like to
be able to modify accept languages preferences as well by extension, for
example, it'd be nice if we could "learn" the accept-languages through
translate, such as if you decline to translate a French page, that would be a
good signal that you want it added to the accept-languages.

### Do you expect this API to be fairly stable?

Yes*.*

### What UI does this API expose?

None*.*

### How could this API be abused?

Read accept languages should be OK.

### Are you willing and able to develop and maintain this API?

Yes*.*

### Draft API spec

## chrome.preferences.

## void getAcceptLanguages(void callback(String acceptLanguages))

## void setAcceptLanguages(Value newAcceptLanguages)

## void appendAcceptLanguage(Value acceptLanguage)

### Notes

The first version will only implement the access of accept languages.

### Issues

*   Where should the accept-languages API live:
            chrome.preferences.acceptLanguages? or chrome.i18n.acceptLanguages?
*   Any issues with modifying the browser's accept languages through
            extension API?
