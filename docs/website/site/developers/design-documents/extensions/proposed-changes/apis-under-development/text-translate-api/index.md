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
page_name: text-translate-api
title: Text Translate API
---

## Problem

Extension developers would like to be able to translate the text of pages.
Transforming arbitrary DOM trees into blocks of text of suitable length for
translation is hard, and potentially time consuming if done naively. Noticing
dynamic updates to the page is also difficult to do correctly. Also, modifying
the content of text nodes may confuse some web applications who weren't
expecting mutation events to occur (this is probably a minor problem though).

## Proposal

Expose an API that can feed an extension process blocks of text to translate.
Chromium takes care of the hard work of creating nice blocks of text, removing
html tags, noticing dynamic updates, handling races between the extension and
renderer process, and modifying the text of the page in the most compat-friendly
way.

The extension just has to translate and return the text when requested.

## API

chrome.textTranslate

// Start translating text for the specified tab. The callback gets called

// repeatedly until stop() is called. Each time it is called it will pass

// a block of text that needs translating.

// NOTE: tab.documentId doesn't yet exist and needs to be added.

void start(int tabId, int documentId, void callback(int blockId, string text))

// The extension should call this function when a block of text has been

// translated.

void updateBlock(int blockId, string translatedText))

// Stop translating text for a tab.

void stop(int tabId, int documentId)

## Example Usage

// Start translating

chrome.tabs.getCurrent(null, function(tab) {

chrome.textTranslate.start(tab.id, tab.documentId, translate);

});

function translate(blockId, text) {

var req = new XMLHttpRequest();

req.open("POST", "http://www.google.com/sometranslateservice", false);

req.onreadystatechange = function() {

if (req.readyState == 4) {

chrome.textTranslate.updateBlock(blockId, req.responseText);

}

}

req.send(null);

}

## Implementation Details

TODO: Flesh this area out Here are some ideas:

*   It seems reasonable for a "block" to just be any block-level
            element, such as "div", and "p".
*   Invalidation should happen on a block level. So if a text node
            inside a block changes, the whole block is retranslated. But if a
            parent node of a block changes, the block is not retranslated.
*   There will have to be code in the renderer tracking outstanding
            translate requests so that they can be abandoned if the related
            blocks change.
*   If possible, we should do the text modification at the CSS layer in
            webcore so that it is not visible to the web page (similar to how
            text-transform is implemented today).
