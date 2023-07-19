---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: cookie-prompt-replacement
title: 'Design document: Cookie prompt replacement'
---

DRAFT: April 22, 2010

Jochen Eisinger &lt;jochen@chromium.org&gt;

## Objective

> Provide the functionality of the application modal cookie prompt with
> something non-modal. <http://crbug.com/38199>

## Background

> Since Google Chrome 4.1, the user has the option to be prompted for every
> cookie ("Cookie" refers to HTTP cookies, Web databases, local storage, and
> AppCache. In the future, this might also include things like indexed database)
> being set. This prompt has to be application modal for the following reason:
> If a HTML page contains the following JavaScript code:

> document.cookie = "A=B";
> console.log(document.cookie);

> JavaScript execution has to be blocked until the user decided whether to allow
> the cookie to be set or not. Since WebKit assumes that it always can execute
> JavaScript, blocking JavaScript amounts to blocking the renderer. If the
> renderer is shared between multiple tabs and/or windows, the prompt needs to
> be application modal. To create a consistent user experience, the cookie
> prompt was designed to be always application modal.

## Requirements

> The following use-cases for the cookie prompt exist. A replacement should
> address all of those.

    1.  Compile a list of exception rules for the sites a user visits
                and trusts.
    2.  Learn what sites are setting cookies and other site data.
    3.  Force certain cookies to expire at the end of the browsing
                session.
    4.  Allow access to cookies on a case-by-case basis.

## Design

> The idea is to remove the cookie prompt entirely. Instead, the cookie blocked
> notification bubble is augmented with enough information to address use-cases
> 1-3.

> **1. Compile a list of exception rules for the sites a user visits and
> trusts**

> The cookie blocked notification bubble contains a link or button (mock needed)
> that will open a dialog similar to the content settings exception dialog
> pre-filled with patterns matching all sites requesting cookies on the current
> web page. The user can then grant or deny access to the individual sites. It
> won't be possible to remove or add patterns from this dialog. The dialog will
> also contain any pre-existing exception that applies to the current web page.

> **2. Learn what sites are setting cookies and other site data.**

> The cookies blocked notification bubble will show a text like "The site
> example.com and 4 sites included from it were denied access to cookies" or "3
> sites included from example.com where denied access to cookies" (mock needed).
> A link to details will open a dialog similar to the "Cookies and other site
> data" dialog pre-filled with all cookies set by the sites included from the
> current web page. It will not be possible to modify cookies in this dialog.
> The information per cookie required for this dialog is the same that is
> currently presented to the user in the modal cookie prompt. It can be
> collected on the same code-path that leads to this prompt.

> **3. Force certain cookies to expire at the end of the browsing session.**

> In addition to "Block" and "Allow", content settings exceptions can have the
> action "Allow for session". This will force all HTML cookies matching such a
> rule to become session cookies. Non-HTML cookies, such as web databases, will
> be matched against the set of exceptions during shutdown of Chrome and deleted
> where required.

> In contrast to the current cookie prompt, a website that requires cookies will
> not work on the first visit. The user first has to explicitly add exceptions
> for this site and then reload the page. Furthermore, it is not possible to
> allow and block certain cookies from the same site - content setting
> exceptions apply on a per-site basis. Use-case 4 can be, however, addressed to
> a certain extend using the [cookie extension
> API](/developers/design-documents/extensions/proposed-changes/apis-under-development/proposal-chrome-extensions-cookies-api).

Alternatives Considered

> An alternative would be to allow all cookies and present the user with an UI
> to remove/block some or all cookies. This has several disadvantages:

    *   For all kinds of cookies (e.g. AppCache, local storage), a
                transaction model has to be implemented.
    *   The user needs to decide what to do with the cookies at latest
                when navigating away from the current page.
    *   The UI is more intrusive than just displaying a content blocked
                notification in the omnibox.
