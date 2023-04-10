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
page_name: history-api
title: History API
---

history.

void search(HistoryQuery query, void callback(HistoryItem\[\] results))

void clear(HistoryQuery query) // defaults to clearing all. Should we prompt?

// NOTE: these could be done in a v2 -- I don't see huge use cases for them,
except perhaps for synchronization.

void create({int date, string url, string title, \[string favIconUrl\], \[int
fromId\]}, \[void callback(HistoryItem result)\])

event onHistoryItemCreated(HistoryItem new)

event onHistoryItemRemoved(HistoryItem removed)

struct HistoryItem {

int id

int date

string url

string title

string favIconUrl

int fromId

int totalVisitCount

int totalTypedCount

}

struct HistoryQuery {

// limited to 100, defaults to current day.

optional int\[\] ids

optional string search

optional Date startDate

optional Date endDate

}
