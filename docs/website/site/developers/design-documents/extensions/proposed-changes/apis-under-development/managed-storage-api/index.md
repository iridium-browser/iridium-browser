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
page_name: managed-storage-api
title: Managed Storage API
---

**Proposal Date**

June 27 2012

Who is the primary contact for this API?
joaodasilva@chromium.org
Who will be responsible for this API?

Chrome Enterprise team: chrome-enterprise-muc@google.com,
chrome-enterprise@google.com

Overview
This is a proposal to extend the storage API
([proposal](/developers/design-documents/extensions/proposed-changes/apis-under-development/settings),
[API](http://code.google.com/chrome/extensions/storage.html)) to add support for
managed values that can be set by an administrator, allowing the administrator
to configure extensions for their domains.

Extensions will have a new 'managed' namespace under 'chrome.storage' that is
read-only and provides the values of the settings configured by the
administrator for that extension.

Use cases

Many features of Chrome can be configured using
[policies](/administrators/policy-list-3). We'd like to give administrators the
capability to further customize Chrome for their deployments by allowing them to
not only [force extensions to be
installed](/administrators/policy-list-3#ExtensionInstallForcelist), but also to
configure them. Some examples:

*   Use an extension to preconfigure bookmarks, and to keep them in sync
            with a shared list retrieved using XHR
*   Use an extension to block URLs, using blocklists downloaded from
            other websites
*   Use an extension to manipulate cookies for internal sites
*   Use an extension to customize the Omnibox results for their domain
*   Use an extension to periodically publish the recent history to an
            internal logging server
*   Use an extension to configure the proxy settings
*   Use an extension to configure the privacy settings

Many of these scenarios can be achieved by creating a custom extension with the
desired features; we'd like to simplify that by allowing generic extensions to
be configured via the policy mechanisms.

Do you know anyone else, internal or external, that is also interested in this
API?
Administrators often post feature requests on the bug tracker, and some of them could be served by extensions. There are also management scenarios on ChromeOS that would benefit from this feature.
Could this API be part of the web platform?

That's not planned at this stage. In the future we might consider another proposal to also manage preferences for a website, possibly through the localStorage API.
Do you expect this API to be fairly stable? How might it be extended or changed
in the future?
We expect these modified APIs to be stable and don't foresee any further
changes. They are also designed with compatibility with existing extensions in
mind.

**If multiple extensions used this API at the same time, could they conflict with each others? If so, how do you propose to mitigate this problem?**
Each extension can only read its own stored settings, so we don't expect this to
introduce conflicts.
List every UI surface belonging to or potentially affected by your API:
This proposal does not introduce new UI.
**Actions taken with extension APIs should be obviously attributable to an
extension. Will users be able to tell when this new API is being used? How?**

The modified APIs don't results in any browser-side actions, as they are purely
informational to the extension. The only user-visible aspect of this API is the
local storage view in the resources inspector. However, managed settings will
not be shown there, since they come from the policy system and are not persisted
in local storage.

How could this API be abused?
This is a read-only API that is only meant to allow extensions to read managed
settings that have been set explicitly for the requesting extension.

Imagine you’re Dr. Evil Extension Writer, list the three worst evil deeds you
could commit with your API (if you’ve got good ones, feel free to add more):
No idea how this can be abused, really.
What security UI or other mitigations do you propose to limit evilness made possible by this new API?
No need for this is foreseen.
Alright Doctor, one last challenge:
Could a consumer of your API cause any permanent change to the user’s system
using your API that would not be reversed when that consumer is removed from the
system?

No.

How would you implement your desired features if this API didn't exist?
Administrators could roll their own extension and push an extension update when
they want to modify the extension's behavior. This isn't as flexible as the
existing policy mechanisms, and would require the developer of the extension to
roll their own policy delivery mechanism. Many administrators also don't have
the technical skills or resources to create their own extensions or to maintain
a fork of a pre-made extension.

Another possibility would be to implement a "native" Chrome policy that supports
the administrator's use case, but it is not reasonable to introduce a new policy
for every corner case. Allowing policy to override the storage settings of some
extensions greatly broadens the customizability of Chrome for administrators.

Draft API spec
This proposal involves a backend modification and an extension to the existing
[storage API](http://code.google.com/chrome/extensions/storage.html).

On the backend, a new storage API implementation will be added that pulls values
from the [policy
service](http://src.chromium.org/viewvc/chrome/trunk/src/chrome/browser/policy/policy_service.h?view=markup)
for the particular extension, when available. These values are exposed in the
'managed' namespace. The 'managed' namespace is similar to 'local' and 'sync',
but is a read-only namespace.

The callback of
[storage.get](http://code.google.com/chrome/extensions/storage.html#method-StorageArea-get)
currently gets an object mapping a setting key to its value. It will be extended
to receive a second object that maps a setting key to an object with metadata
about that setting (existing code can still work with the first argument only).
For now, the only well-known metadata key will be "policyLevel", which takes the
value "mandatory" or "recommended". This indicates whether the administrator
would like the value to be enforced, or if it is just a suggested default that
the user can override. The metadata object can be extended in the future to
include other details (for example, the sync version).

Other remarks:

*   Settings configured via a policy do not count towards the
            extension's quota. getBytesInUse() is 0 for the 'managed' namespace.
*   The extension can still set() a key that is being overridden by a
            policy in another namespace. It is up to the extension to give
            priority to a managed value, if it exists.
