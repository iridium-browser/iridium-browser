---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: cookies-and-prerender
title: Cookies and Prerender
---

Shared local storage such as DOM storage, IndexedDB, and HTTP cookies present
challenges for prerendering. Ideally mutations made by the prerendered page
should not be visible to other tabs until after the user has activated the page.
Mutations made by other pages should be reflected in the prerendered page, which
may have already read from local storage before the other pages made the
changes.
There is some thought about adding a transactional cookie store which would be
memory only for the prerendered page. Unlike some of the HTML5 storage
mechanisms, cookies primarily exist in HTTP and setter's may not have access to
the Visibility API to change behavior. Basically, the transactional store would
exist in memory only. If the user never uses a prerendered page, any mutations
will just disappear. If the user tries to navigate to a page, mutations in the
transactional cookie store are compared against mutations in the profile's main
cookie store. If there are any conflicts, the prerendered page is cancelled and
the mutations disappear. Otherwise, the mutations are merged into the profile's
main cookie store and the page is swapped in.

Prerendered requests will use a ChromeURLRequestContext which has a new
CookieStore interface, but is otherwise the same as the current profile’s
ChromeURLRequestContext. If the PrerenderContents are discarded without being
used, the changes made to the CookieStore interface go away. Otherwise, the
deltas will be committed to the main CookieStore for the profile. If there is a
merge problem, the prerendered page is discarded and a fresh request is issued.
There are a couple of possible approaches to implementing this:
a) Do a complete copy of the canonical cookie store for the prerendered page.
Both the canonical cookie store and the prerendered cookie store make mutations
locally, and both maintain a log of mutations. Abandoning is simple: just delete
the new cookie store. Conflict detection happens by examining the logs of the
canonical store and the prerender store, and merges are simply applying the logs
of the prerendered store to the canonical store if no detection detected. The
main advantage is that this does not change the current "get all cookies for
this request" path at all. The downsides are memory cost plus memcpy
(back-of-the-envelope estimate is 600KB for store) and some overhead for
Set-Cookie in non prerender case due to log.
b) Add ability for CookieMonster's to be chained together. The prerendered
version starts out with 0 entries, but points to the canonical backing store.
Set-Cookie in the prerendered page only adds entries to the prerendered version.
"Get all cookies" in the prerendered page will need to merge together cookies
from the prerendered cookiemonster and the backing cookiemonster. Abandoning is
easy - just remove the prerendered cookie monster. Merge will look at last
modified times for entries and any newer ones in the backing store will cause
conflict detection. The main concern is how to detect a clearing of Set-Cookie
in the backing store at the time of merge. The main advantage of this case is
low overhead at startup and fairly minimial logic changes. The concerns are the
complexity of "get all cookies" for the prerendered case, as well as detecting
deleted cookies during the merge.
c) Add a new CookieStore implementation which stores diffs and has a
CookieMonster backing store. This is similar to b), but could handle the deleted
cookie case better. The downside is the potential for lots of duplicated logic,
and very different lookup paths. Also, a fair bit of code does downcasting of
CookieStore to CookieMonster, so the CookieStore interface will need to change
for this to work.
A few other concerns:
- Are cookies ever cached in the render process? For example, does
document.cookies do an IPC over to the CookieMonster to retrieve the values each
time, or is there a cache that we will have to deal with?
- How big are the cookiestore's typically? Could we get away with the
copy-and-log approach?
