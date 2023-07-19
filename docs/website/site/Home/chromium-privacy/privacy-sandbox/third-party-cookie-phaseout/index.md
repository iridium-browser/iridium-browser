---
breadcrumbs:
- - /Home
  - Chromium
- - /Home/chromium-privacy
  - Chromium Privacy
- - /Home/chromium-privacy/privacy-sandbox
  - The Privacy Sandbox
page_name: third-party-cookie-deprecation-testing-and-debugging
title: Third-Party Cookie Deprecation Testing and Debugging
---

[TOC]

## Enable Third-Party Cookie Blocking

In Chrome settings, go to **Privacy and Security** -> **Cookies and other site data** or
`chrome://settings/cookies`. Under **General Settings** ensure that **Block third-party
cookies** is enabled.

<img alt="image" src="/Home/chromium-privacy/privacy-sandbox/third-party-cookie-phaseout/3p-cookie-blocking.png" height=420 width=619>

## Testing your site
Go to the actual site you want to test and thoroughly test site functionality, with a
focus on anything involving federated login flows, online payments, 3-D Secure
verification, multiple domains, or cross-site embedded content (iframes, images, videos,
etc.) to see if it’s working properly. If not, use the following tools to find out the
cookies that may be affected.

## Debugging

### Use Chrome DevTools

1. Make sure third-party cookie blocking is enabled in chrome://settings/cookies.
1. Open [DevTools](https://developer.chrome.com/docs/devtools/overview/#open).
1. Reload the webpage to capture all the network requests made by the site.
1. Analyze the requests:
    1. Click the "3rd-party request” checkbox to narrow down the requests that need third-party
    cookies.
    1. Click through the request to look for the cookies that have "SameSite=None; Secure;"
    from the blocked cookies in the “Cookies” tab. These are third-party cookies.
    1. Remember to check the “show filtered out cookies” checkbox, the ones highlighted in yellow
    are the filtered out cookies.
    1. Look for the cookies that have an info icon in the “Name” column. Hover the mouse over the
    icon, if the tooltip says "This cookie is blocked due to user preferences", that means this
    cookie was affected by third-party cookie blocking. (Make sure this cookie is not blocked due
    to [site specific user settings](/Home/chromium-privacy/privacy-sandbox/third-party-cookie-phaseout/cookie-setting-for-site.png), which would show the same reason.)
1. Identify third-party cookie users: see which domains are setting or receiving the cookies by
looking into the “Headers” tab.

The following examples show blocked third-party cookies in the DevTools.

Third-party cookie blocked for a request:

<img alt="Third-party cookie blocked for a request" src="/Home/chromium-privacy/privacy-sandbox/third-party-cookie-phaseout/3p-cookie-blocked-request.png" height=220 width=610>

Third-party cookie blocked for a response:

<img alt="Third-party cookie blocked for a response" src="/Home/chromium-privacy/privacy-sandbox/third-party-cookie-phaseout/3p-cookie-blocked-response.png" height=220 width=610>

Unfortunately, Chrome can only tell you when there are cookies that will behave differently under
the third-party cookie blocking behavior, but it can’t tell you which cookies might be responsible
for site breakage.

### Use NetLog

The NetLog only covers cookies accessed over the network via HTTP(S) and does not include other
methods of cookie access such as document.cookie (JavaScript) or chrome.cookies (extensions).

The [instructions to use NetLog to debug cookie issues caused by SameSite attribute](https://www.chromium.org/updates/same-site/test-debug/#using-a-netlog-dump) is applicable to third-party cookie deprecation,
with some tweeks needed: make sure to look for cookies marked with “EXCLUDE_USER_PREFERENCES”.