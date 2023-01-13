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
page_name: settings_pages
title: Settings pages
---

**Overview**

We want to enable extensions to provide custom settings UI to extend the (more
and more) minimal settings UI that is build into Chrome.

**Use cases**

Similarly to e.g. browser actions, an extension can define a HTML file in its
manifest that it wants to be rendered as part of Chrome's settings UI. We
provide a set of widgets (CSS and JavaScript) that an extension can use to
render its UI using the same theme as Chrome's settings UI, however, extensions
are free to display arbitrary UI.

**Could this API be part of the web platform?**

no

**Do you expect this API to be fairly stable?**

The API itself will be stable. We might change/extend the JavaScript/CSS
provided to render the settings UI in the future, however, I expect that it will
be possible to do this in a non-breaking way.

**What UI does this API expose?**

An extension using this API can display arbitrary HTML as part of Chrome's
settings UI.

**How could this API be abused?**

An extension using this API can pretend to e.g. manage your passwords, but
instead send all passwords you enter to a third-party site. A possible way to
mitigate this risk would be to disallow all communication with other extension
pages or network resources from settings page. An extension could also mess up
your settings, but it already can do so from the background page.

In order to avoid an extension accessing the APIs available to Chrome's settings
UI, the extension settings page would be rendered in an sandboxed iframe.

**How would you implement your desired features if this API didn't exist?**

An extension could define a browser action or an options page and allow the user
to configure settings from there. Option pages, however, are meant for settings
for the extension, not settings for the browser. Browser actions aren't where
the user expects browser settings either.

**Are you willing and able to develop and maintain this API?**

Yes.
