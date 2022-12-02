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
page_name: extension-manifesto
title: Extensions Manifesto
---

### Problem statement

Chromium can't be everything to all people. People use web browsers in a variety
of environments and for a wide variety of jobs. Personal tastes and needs vary
widely from one user to the next. The feature needs of one person often conflict
directly with those of another. Further, one of the design goals of Chromium is
to have a minimal light-weight user interface, which itself conflicts with
adding lots of features.

User-created extensions have been proposed to solve these problems:

*   The addition of features that have specific or limited appeal ("that
            would be great as an extension").
*   Users coming from other browsers who are used to certain extensions
            that they can't live without.
*   Bundling partners who would like to add features to Chromium
            specific to their bundle.

### Goals

An extension system for Chromium should be:

*   Webby
    *   Developing and using extensions should be very similar to
                developing and using web pages.
    *   We should reuse the web platform wherever possible instead of
                creating new proprietary APIs.
    *   Web developers should be able to easily create Chromium
                extensions.
    *   Installing and using an extension should feel lightweight and
                simple, like using a web app.
*   Rich
    *   It should be possible to create extensions as polished as if
                they had been developed by the Chromium team.
    *   Eventually, it should be possible to implement major chunks of
                Chromium itself as extensions.
*   General
    *   There should be only one extension system in Chromium that
                handles all types of extensibility.
    *   Infrastructure like autoupdate, packaging, and security should
                be shared.
    *   Even traditional NPAPI plugins should be deployable as
                extensions.
*   Maintainable
    *   The system should require low ongoing maintenance from the
                Chromium team, and minimize potential for forward compatibility
                issues.
    *   We should not need to disable deployed extensions when we
                release new versions of Chromium.
*   Stable
    *   Extensions should not be able to crash or hang the browser
                process.
    *   Chromium should assign blame to extensions that are overusing
                resources via tools like the task manager and web inspector.
    *   Poorly behaving extensions should be easy to disable.
*   Secure
    *   It must not be possible for third-party code to get access to
                privileged APIs because of the extension system.
    *   Extensions should be given only the privileges they require, not
                everything by default.
    *   Extensions should run in sandboxed processes so that if they are
                compromised, they can't access the local machine.
    *   It should be trivial for authors to support secure autoupdates
                for extensions.
    *   We must be able to block extensions across all Chromium
                installations.
*   Open
    *   Extension development must not require use of any Google
                products or services.
    *   Extensions should work the same in Chromium as in Google Chrome.

### Use Cases

The following lists some types of extensions that we'd like to eventually
support:

*   Bookmarking/navigation tools: Delicious Toolbar, Stumbleupon,
            web-based history, new tab page clipboard accelerators
*   Content enhancements: Skype extension (clickable phone numbers),
            RealPlayer extension (save video), Autolink (generic microformat
            data - addresses, phone numbers, etc.)
*   Content filtering: Adblock, Flashblock, Privacy control, Parental
            control
*   Download helpers: video helpers, download accelerators, DownThemAll,
            FlashGot
*   Features: ForecastFox, FoxyTunes, Web Of Trust, GooglePreview,
            BugMeNot

This list is non-exhaustive, and we expect it to grow as the community expresses
interest in further extension types.

### Proposal

We should start by building the infrastructure for an extension system that can
support different types of extensibility. The system should be able to support
an open-ended list of APIs over time, such as toolbars, sidebars, content
scripts (for Greasemonkey-like functionality), and content filtering (for
parental filters, malware filters, or adblock-like functionality). Some APIs
will require privileges that must be granted, such as "access to the history
database" or "access to mail.google.com".

Extension components will typically be implemented using web technologies like
HTML, JavaScript and CSS with a few extra extension APIs that we design.
Extensions will run in their own origin, separate from any web content, and will
run in their own process (with the exception of content scripts, which must run
in the same process as the web page they are modifying). Some form of native
code components may also be supported, but the goal is to minimize the need for
this in extensions.

### Packaging and Distribution

For performance reasons, extensions will need to be loaded out of a local cache.
Extensions need to be loaded immediately at startup, ideally before pages are
loaded, yet shouldn't affect startup time. Out of date versions are still served
from the cache until a new version has been completely downloaded and validated.
All extensions will have a [manifest](/system/errors/NodeNotFound) that includes
information such as: the name and description of the extension, the URLs to the
various toolbars, workers, and content scripts that compose the extension; and
an autoupdate URL and public key for validating extension updates.
There will eventually be three mechanisms for packaging and distributing
extensions:

1.  A signed package of resources in a single file, served out of a
            local cache
2.  A collection of "live" resource URLs served over SSL
3.  A collection of files served off of a local filesystem out of a
            well-known directory. This is primarily for development purposes,
            but can also be used for pre-bundled extensions.

Initially, we'll implement only 1 and 3. To implement 2, we'll need an
implementation of the HTML5 app cache. This is because typical cache behavior
such as eviction is not acceptable for extensions. Even when the app cache
exists, the resources will still need to be served over SSL to prevent
man-in-the-middle tampering.

The signed package mechanism will be a zip file with the manifest in a specific
name/location. These files would be created with a custom tool that we provide
that handles validation, manifest creation and signing. This tool could be part
of an online hosting / authoring service for extensions (see below).

### Installation

Installation of extensions should be a simple and minimal interface, ideally
consisting of only two clicks. A link to an extension is embedded on a web page
and a user clicks on it. A confirmation dialog is presented that lists the
permissions that the extension requires. This is an all or nothing proposition -
if the user doesn't want to give a particular privilege, they can't selectively
disable one. If they decline, the extension will fail to be installed. An
extension can't request "optional" privileges.

Most extensions should be able to load in place without forcing a browser
restart or even a page reload when they are installed.

An interface will be available which allows users to review the extensions that
they have installed and what privileges these extensions have. From this
interface it will be a simple operation to temporarily disable extensions or
permanently uninstall them.

### Automatic Updates

Similar to Google Chrome, it is important for security that extensions be able
to silently update. This should be a capability that is present for all
extensions by default, not something the author has to plan for.

In the case where an updated extension needs new privileges, we will prompt the
user that the extension needs "to be updated", which will essentially start the
installation phase again, prompting the user for the extra privileges. Ideally
this UI should work in a non-modal, minimally-intrusive way. We need to decide
what happens when the user says no to these new privileges (upgrade simply
fails, or extension is disabled).

Extension updates will be performed over HTTP while Chromium is running. The
downloaded package will be digitally signed to prevent MITM attacks. The initial
signature will be implicitly trusted. Updates will only be applied if the
version number is greater than the installed version number.

We will provide a service designed to reduce burden to developers by reducing
traffic costs and providing a robust, secure mechanism for autoupdates that they
can easily leverage rather than having to handle the logistics on their own
site. It would also provide authors with a way to easily create and verify their
extension packages and manifests. However, developers will always have the
option to package, sign, and host extensions on their own site.

The central service will maintain a blocklist of known malicious or harmful
extensions. This blocklist will be used by the browser to disable these
extensions.

It's likely in the future we may want to provide a consumer front-end which
would allow users to more easily find the most popular, highest quality and
trustworthy extensions.

### API details

We've published a list of [potential
APIs](/developers/design-documents/extensions/proposed-changes/apiwishlist).
Details are published in the [Browser APIs
document](/developers/design-documents/extensions/how-the-extension-system-works/api-pattern-design-doc)
as they are designed.

### Next Steps

Our first milestone will be to implement a functioning extension system that can
support content scripts. The majority of the work will be the infrastructure,
including:

*   packaging and signing
*   installation
*   autoupdate
*   management and removal
*   blocking
*   web service

Once we have content scripts working, we can move on to additional types of
extensibility like toolbars, sidebars, etc.
