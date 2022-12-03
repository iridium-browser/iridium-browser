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
page_name: omnibox-api
title: Omnibox API
---

### Status

Dream phase. The underlying omnibox API is fairly complex. This is a work in
progress, and likely incomplete / missing key bits.

### Overview

There are two goals for the omnibox extension API:

1.  Register a keyword that would allow the extension author to provide
            simple command hooks.
2.  Be a full omnibox provider, with autocomplete matches.

### API

omnibox.

// The simple way:

// keyword-based (tab to search?) API. Perhaps usable to implement a basic
command-line interface.

// Register a keyword for handling in this extension.

void registerKeyword(string keyword)

// Called when the user enters text after using this extension's registered
keywords.

event onKeywordInput(string input)

// The hard way:

// Be a full-on omnibox provider.

// Notifies Omnibox that matches are ready, and provides them for browser-side

// use when the Omnibox wants them.

void matchesAvailable(Match\[\] matches)

// Called by the Omnibox when the text in the box has been changed.

// TODO: AutocompleteInput has a lot more info than this

event onInputChanged(string input)

// The user has requested that this match never show up in results again.

event onMatchDeleted(Match match)

struct Match {

int relevance

bool deletable

string fill_into_edit

int inline_autocomplete_offset

string destination_url

string contents

// missing content class

string description

// missing description class

}
