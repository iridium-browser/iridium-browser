---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
- - /developers/design-documents/network-stack
  - Network Stack
page_name: cookiemonster
title: CookieMonster
---

The CookieMonster is the class in Chromium which handles in-browser storage,
management, retrieval, expiration, and eviction of cookies. It does not handle
interaction with the user or cookie policy (i.e. which cookies are accepted and
which are not). The code for the CookieMonster is contained in
net/base/cookie_monster.{h,cc}.

CookieMonster requirements are, in theory, specified by various RFCs. [RFC
6265](http://datatracker.ietf.org/doc/rfc6265/) is currently controlling, and
supersedes [RFC 2965](http://datatracker.ietf.org/doc/rfc2965/). However, most
browsers do not actually follow those RFCs, and Chromium has compatibility with
existing browsers as a higher priority than RFC compliance. An RFC that more
closely describes how browsers normally handles cookies is being considered by
the RFC; it is available at
<http://tools.ietf.org/html/draft-ietf-httpstate-cookie>. The various RFCs
should be examined to understand basic cookie behavior; this document will only
describe variations from the RFCs.

The CookieMonster has the following responsibilities:

*   When a server response is received specifying a cookie, it confirms
            that the cookie is valid, and stores it if so. Tests for validity
            include:
    *   The domain of the cookie must be .local or a subdomain of a
                public suffix (see <http://publicsuffix.org/>--also known as an
                extended Top Level Domain).
    *   The domain of the cookie must be a suffix of the domain from
                which the response was received.

> We do not enforce the RFC path or port restrictions

*   When a client request is being generated, a cookie in the store is
            included in that request if:
    *   The cookie domain is a suffix of the server hostname.
    *   The path of the cookie is a prefix of the request path.
    *   The cookie must be unexpired.
*   It enforces limits (both per-origin and globally) on how many
            cookies will be stored.

**CookieMonster Structure**

The important data structure relationships inside of and including the
CookieMonster are sketched out in the diagram below.

[<img alt="image"
src="/developers/design-documents/network-stack/cookiemonster/CM-inheritance.svg">](/developers/design-documents/network-stack/cookiemonster/CM-inheritance.svg)

In the above diagram, type relationships are represented as follows:

*   Reference counted thread safe: Red outline
*   Abstract base class: Dashed outline
*   Inheritance or typedef: Dotted line with arrow, subclass to
            superclass
*   Member variable contained in type: Line with filled diamond, diamond
            on the containing type end.
*   Pointer to object contained in type: Line with open diamond, diamond
            on the containing type end.

The three most important classes in this diagram are:

*   CookieStore. This defines the interface to anything that stores
            cookies (currently only CookieMonster), which are a set of Set, Get,
            and Delete options.
*   CookieMonster. This adds the capability of specifying a backing
            store (PersistentCookieStore) and Delegate which will be notified of
            cookie changes.
*   SQLitePersistentCookieStore. This implements the
            PersistentCookieStore interface, and provides the persistence for
            non-session cookies.

The central data structure of a CookieMonster is the cookies_ member, which is a
multimap (multiple values allowed for a single key) from a domain to some set of
cookies. Each cookie is represented by a CanonicalCookie(), which contains all
of the information that can be specified in a cookie (see diagram and RFC 2695).
When set, cookies are placed into this data structure, and retrieval involves
searching this data structure. The key to this data structure is the most
inclusive domain (shortest dot delimited suffix) of the cookie domain that does
not name a domain registrar (i.e. "google.com" or "bbc.co.uk", but not "co.uk"
or "com"). This is also known as the Effective Top Level Domain plus one, or
eTLD+1, for short.

Persistence is implemented by the SQLitePersistentCookieStore, which mediates
access to an on-disk SQLite database. On startup, all cookies are read from this
database into the cookie monster, and this database is kept updated as the
CookieMonster is modified. The CookieMonster notifies its associated
PersistentCookieStore of all changes, and the SQLitePersistentCookieStore
batches those notifications and updates the database every thirty seconds (when
it has something to update it about) or if the queue gets too large. It does
this by queuing an operation to an internal queue when notified of a change by
the CookieMonster, and posting a task to the database thread to drain that
queue. The backing database uses the cookie creation time as its primary key,
which imposes a requirement on the cookie monster not to allow cookies with
duplicate creation times.

The internal code of the cookie monster falls into four paths: The setter path,
the getter path, the deletion path, and the garbage collection path.

The setter path validates its input, creates a canonical cookie, deletes any
cookies in the store that are identical to the newly created one, and (if the
cookie is not immediately expired) inserts it into the store. The getter path
computes the relevant most inclusive domain for the incoming request, and
searches that section of the multimap for cookies that match. The deletion path
is similar to the getter path, except the matching cookies are deleted rather
than returned.

Garbage collection occurs when a cookie is set (this is expected to happen much
less often than retrieving cookies). Garbage collection makes sure that the
number of cookies per-eTLD+1 does not exceed some maximum, and similarly for the
total number of cookies. The algorithm is both in flux and subtle; see the
comments in cookie_monster.h for details.

This class is currently used on many threads, so it is reference counted, and
does all of its operations under a per-instance lock. It is initialized from the
backing store (if such exists) on first use.

**Non-Obvious Uses of CookieMonster**

In this writeup, the CookieMonster has been spoken of as if it were a singleton
class within Chrome. In reality, CookieMonster is \*not\* a singleton (and this
has produced some bugs). Separate CookieMonster instances are created for:

*   Use in standard browsing
*   Incognito mode (this version has no backing store)
*   Extensions (this version has its own backing store; map keys are
            extension ids)
*   Incognito for Extensions.

To decide in each case what kinds of URLs it is appropriate to store cookies
for, a CookieMonster has a notion of "cookieable schemes"--the schemes for which
cookies will be stored in that monster. That list defaults to "http" and
"https". For extensions it is set to "chrome-extension".

When file cookies are enabled via --enable-file-cookies, the default list will
include "file", and file cookies will be stored in the main CookieMonster. For
file cookies, the key will be either null, or (for UNC names, e.g.
//host.on.my.network/path/on/that/host) the hostname in the file cookie.
Currently, the store does not distinguish carefully between file cookies and
network cookies.

**Implementation Details; Subject to Change Without Notice**

The call graph of the routines used in the CookieMonster is included below. It
is correct as of revision 59070, but may not apply to later versions.

[<img alt="image"
src="/developers/design-documents/network-stack/cookiemonster/CM-method-calls-new.svg">](/developers/design-documents/network-stack/cookiemonster/CM-method-calls-new.svg)

Key:

*   Green fill: CookieMonster/CookieStore Public method.
*   Grey fill: CookieMonster Private method.
*   Red ring: Method take instance lock.
*   Dark blue fill: CookieMonster static method.
*   Light blue fill: cookie_monster.cc static function
*   Yellow fill: CanonicalCookie method.

The CookieMonster is referenced from many areas of the code, including

but not limited to:

*   URLRequestHttpJob: Main path for HTTP request/response.
*   WebSocketJob
*   Automation Provider
*   BrowsingDataRemover: For deleting browsing data.
*   ExtensionDataDeleter
*   CookieTreeModel: Allows the user to examine and delete specific
            cookies.
*   Plugins
*   HttpBridge: For synchronization
