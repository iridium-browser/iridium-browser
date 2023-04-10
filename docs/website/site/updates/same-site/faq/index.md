---
breadcrumbs:
- - /updates
  - updates
- - /updates/same-site
  - SameSite Updates
page_name: faq
title: SameSite Frequently Asked Questions (FAQ)
---

### Q: What are the new SameSite changes?

Chrome is changing the default behavior for how cookies will be sent in first
and third party contexts.

*   Cookies that do not specify a `SameSite` attribute will be treated
            as if they specified `SameSite=Lax`, i.e. they will be restricted to
            first-party or same-site contexts by default.
*   Cookies that are intended for third-party or cross-site contexts
            must specify `SameSite=None` and `Secure`.

Note: this also means cross-site or third-party cookies are restricted to secure
/ HTTPS connections only.

### Q: When do the new SameSite changes roll live?

This behavior will become the default during the Chrome 80 rollout.

### Q: How can I test the new SameSite defaults?

In the location bar, enter `chrome://flags` to access the flag configuration.
Set the following flags to enabled:

*   `chrome://flags/#same-site-by-default-cookies`
*   `chrome://flags/#cookies-without-same-site-must-be-secure`

### Q: How can I tell if my cookies are affected?

Chrome is displaying warnings in the Console in DevTools which highlight each
cross-site request where cookies would be affected by the new `SameSite`
defaults. The cookies and their respective `SameSite` and Secure attributes are
also visible in DevTools within the Application tab under Storage → Cookies.
This same information is also available in the Network Tab for each request.

### Q: What do I need to do to my cookies?

*   For cookies that are only required in a first-party context, you
            should ideally set an appropriate `SameSite` value of either `Lax`
            or `Strict` and set `Secure` if your site is only accessed via
            HTTPS.
*   For cookies that are required in a third-party context, you must set
            the `SameSite=None` and `Secure` attributes.

### Q: How do I handle older or incompatible browsers?

Refer to following guidance:

*   <https://web.dev/samesite-cookie-recipes/#handling-incompatible-clients>
*   <https://www.chromium.org/updates/same-site/incompatible-clients>

### Q: Are the new defaults applied to Chrome on iOS?

No. Chrome on iOS (as with all other browsers) uses the underlying WebKit engine
and does not currently enforce the new defaults.

### Q: How can I tell if my browser is applying the new SameSite defaults?

The test site: <https://samesite-sandbox.glitch.me/> will show the presence of a
variety of cookies in a same-site and cross-site context along with whether
that’s correct for the new defaults. If all rows show with a green check mark:
✔️ then the browser is enforcing the new defaults.

### Q: What if I have an HTTP page and need third-party cookies?

Ideally, sites should be upgrading to HTTPS and cross-site cookies will not be
sent over a plain HTTP connection. However, on an HTTP page with HTTPS resources
then those secure requests will include cookies that have been marked with
`SameSite=None; Secure`.
Sites that rely on services making use of third-party cookies should ensure they
are including those resources (scripts, iframes, pixels, etc.) via an
appropriate HTTPS URL. You can see this in action on this test site:
<http://crosssite-sandbox.glitch.me/>

A reasonable rule is if the connection is an upgrade, i.e. HTTP page with HTTPS
resources then they can have cookies. However, a downgrade, i.e. an HTTPS page
with HTTP resources will not get cookies on cross-site HTTP resources and users
will most probably see mixed content warnings in the browser UI.

In general, make all requests over HTTPS where possible.

### Q: What is the Lax + POST mitigation?

This is a specific exception made to account for existing cookie usage on some
Single Sign-On implementations where a CSRF token is expected on a cross-site
POST request. This is purely a temporary solution and will be removed in the
future. It does not add any new behavior, but instead is just not applying the
new `SameSite=Lax` default in certain scenarios.

Specifically, a cookie that is at most 2 minutes old will be sent on a top-level
cross-site POST request. However, if you rely on this behavior, you should
update these cookies with the `SameSite=None; Secure` attributes to ensure they
continue to function in the future.

### Q: Why doesn’t my extension work anymore under the new SameSite rules?

Chrome extensions use the `chrome-extension://` URL scheme, which appears as
cross-site to anything `https://` or `http://`. The fix, which is to
[treat](https://cs.chromium.org/chromium/src/chrome/renderer/extensions/chrome_extensions_renderer_client.cc?l=327-328&rcl=93f8b74447f261ada0224ae54176fbecdf03a294)
extension-initiated requests as same-site**\***, is available in Chrome 79 and
later. Some use cases involving requests made from web frames on extension pages
may also behave differently in Chrome 80. If you test on newer (80+) versions of
Chrome and find that your extension is still broken, please file a bug on
crbug.com using [this template](https://bit.ly/2lJMd5c).

**\***
[If](https://cs.chromium.org/chromium/src/chrome/renderer/extensions/chrome_extensions_renderer_client.cc?l=86-90&rcl=9235d01ebbb6f18a74b0b99f7922175b4f11e68a)
the extension has host permissions access to the page.

### Q: Are cookies in WebViews affected by the new default behavior?

\[**UPDATE Jan 8, 2021**: The modern SameSite behavior ([SameSite=Lax by
default, SameSite=None requires
Secure](https://web.dev/samesite-cookies-explained/), and [Schemeful
Same-Site](https://web.dev/schemeful-samesite/)) will be enabled by default for
Android WebView on apps targeting Android S and newer. Existing apps will not be
affected until they choose to update to target Android S. Android S has not yet
been released. Existing apps can be tested with the new modern SameSite behavior
by toggling the flag webview-enable-modern-cookie-same-site in the [developer
UI](https://chromium.googlesource.com/chromium/src/+/HEAD/android_webview/docs/developer-ui.md#Flag-UI).\]

~~The new `SameSite` behavior will not be enforced on Android Webview until
later,~~ though app developers are advised to declare the appropriate `SameSite`
cookie settings for Android WebViews based on versions of Chrome that are
compatible with the None value, both for cookies accessed via HTTP(S) headers
and via Android WebView's [CookieManager
API](https://developer.android.com/reference/android/webkit/CookieManager). This
does not apply to Chrome browser on Android, which will begin to enforce the new
`SameSite` rules at the same time as the desktop versions of Chrome.
