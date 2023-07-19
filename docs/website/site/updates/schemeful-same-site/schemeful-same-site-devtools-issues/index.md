---
breadcrumbs:
- - /updates
  - updates
- - /updates/schemeful-same-site
  - Schemeful Same-Site
page_name: schemeful-same-site-devtools-issues
title: Schemeful Same-Site DevTools Issues
---

Schemeful Same-Site is a change which further protects your website from CSRF
attacks by distinguishing between the insecure and secure versions of it.
Meaning that an attacker which takes control of http://bank.example isn't able
to trick a user into clicking a link and sending `SameSite=Strict` cookies to
https://bank.example, potentially logging in and draining the user's account.
Schemeful Same-Site makes it so that http://bank.example and
https://bank.example are considered cross-site to each other; just like how
https://bank.example and https://shopping.example are considered cross-site to
each other.

To determine which, if any, of your website's cookies are affected you can check
the [DevTools Issue
Tab](https://developers.google.com/web/tools/chrome-devtools/issues) for any
issues that pop up on your site.

If you see any of the following issue titles then that means one of your
website's cookies is going to be/was blocked. See the relevant section for each
issue title below to see what could be causing this and the "How to Resolve"
section how you can resolve it.

## Issue Titles

*   Migrate entirely to HTTPS to continue having cookies sent on
            same-site requests
*   Migrate entirely to HTTPS to have cookies sent on same-site requests

If you see either of these titles then this issue is caused by a navigation. See
the navigation section within "Types of Issues"

*   Migrate entirely to HTTPS to continue having cookies sent to
            same-site subresources
*   Migrate entirely to HTTPS to continue allowing cookies to be set by
            same-site subresources
*   Migrate entirely to HTTPS to have cookies sent to same-site
            subresources
*   Migrate entirely to HTTPS to allow cookies to be set by same-site
            subresources
    *   In a rare case this may also occur while POSTing a form. See the
                POSTing section within "Types of Issues"

If you see any of these titles then this issue is caused by a subresource. See
the subresource section within "Types of Issues"

## Types of Issues

Note: Two urls with the same registrable domain but different schemes are termed
cross-scheme to each other.

### Navigation Issues

## Navigating between cross-scheme versions of a website would previously allow `SameSite=Strict` cookies to be sent. Now this is treated the same as a navigation from an entirely different website which means `SameSite=Strict` cookies will be blocked from being sent.

### Subresource Issues

Loading a cross-scheme subresource on a page would previously allow
`SameSite=Strict` or `SameSite=Lax` cookies to be sent or set. Now this is
treated the same way as any other third-party subresource which means that any
`SameSite=Strict` or `SameSite=Lax` cookies will be blocked.

### POSTing Issue

Posting between cross-scheme versions of a website would previously allow
cookies set with `SameSite=Strict` or `SameSite=Lax `to be sent. Now this is
treated the same as a POST from an entirely different website which means only
`SameSite=None `cookies can be sent.

Because `SameSite=None` require the `Secure` attribute on cookies, this can only
be done from HTTP to HTTPS. The other way, HTTPS to HTTP, will not work as
cookies with the Secure attribute cannot be sent to insecure URLs.

## Debugging Tips

Please see [this
page](/updates/schemeful-same-site/testing-and-debugging-tips-for-schemeful-same-site)
for more information on how to debug your site.

## How to Resolve

### Upgrade to HTTPS

The **best way** to resolve these issues is to make sure your entire site is
upgraded to HTTPS. Once this is the case none of your cookies will be affected
by Schemeful Same-Site.

### Unable to Upgrade to HTTPS

While we strongly recommend that you upgrade your site entirely to HTTP to
protect your users, if you’re unable to do so yourself we suggest speaking with
your hosting provider to see if they can offer that option.

If that’s still not possible then a workaround you can try is relaxing the
`SameSite` protection on affected cookies.

*   In cases where only `SameSite=Strict` cookies are being blocked you
            can lower the protection to `SameSite=Lax`.
*   In cases where both `SameSite=Strict` and `SameSite=Lax` cookies are
            being blocked and your cookies are being sent to (or set from) a
            secure URL you can lower the protections to `SameSite=None`.
    *   This workaround will fail if the URL you’re sending cookies to
                (or setting them from) is insecure. This is because
                `SameSite=None` requires the `Secure` attribute on cookies which
                means those cookies may not be sent or set to a insecure URL. In
                this case you will be unable to access that cookie until your
                site is upgraded to HTTPS.
