---
breadcrumbs:
- - /Home
  - Chromium
- - /Home/chromium-security
  - Chromium Security
page_name: deprecating-powerful-features-on-insecure-origins
title: Deprecating Powerful Features on Insecure Origins
---

[TOC]

## Proposal

As browser users manage more and more of their day-to-day lives online, we
(Chrome Security) believe they should reasonably expect that browsing and
interacting with the web is secure and protects their sensitive information
across their entire browsing experience. Protecting users’ privacy and security
requires connecting to secure origins wherever possible. Practically speaking,
this means restricting features to HTTPS in lieu of HTTP, especially for
powerful web platform features.

While [strong progress](https://transparencyreport.google.com/https/overview)
has been made in increasing HTTPS adoption on the web over the past several
years, there are still millions of sites that do not support secure connections
over HTTPS, which means that support for HTTP isn’t going away in the
foreseeable future. As long as a significant portion of browsing the web can
only happen over HTTP, it’s important that we take steps to protect and inform
users whenever we cannot guarantee that their connections are secure.

Continuing from our past efforts to [restrict new features to secure
origins](/Home/chromium-security/prefer-secure-origins-for-powerful-new-features),
we are taking further steps on our path of deprecating powerful features on
insecure origins in order to mitigate the most privacy- and security-sensitive
risks of using HTTP in Chrome. To guide our efforts going forward, we have
created the following principles, which we will use to prioritize future work in
this area:

- **Better inform users when making trust decisions about sites over
  insecure connections** Over time, Chrome users are making an
  increasingly large amount of trust decisions when interacting with
  websites, whether by granting permissions for powerful web features,
  submitting personal information to a website, or downloading
  executable code. However, when the connection to a website is
  unauthenticated and unencrypted, these trust decisions aren’t bound
  to the intended site in a meaningful way. When Chrome presents users
  with such a trust decision, we plan on ramping up warnings to users
  when these features are being used over connections to insecure
  origins.

- **Limit the ability for sites to circumvent security policies over
  insecure connections** The [same-origin-policy
  (SOP)](https://developer.mozilla.org/en-US/docs/Web/Security/Same-origin_policy)
  is an important security policy that limits a website’s ability to
  interact with resources from other origins. Several powerful web
  platform features (such as
  [postMessage](https://developer.mozilla.org/en-US/docs/Web/API/Window/postMessage)
  and [CORS](https://developer.mozilla.org/en-US/docs/Web/HTTP/CORS))
  allow for websites to exempt domains from this policy to provide a
  more feature-rich experience. Our goal for future versions of Chrome
  to gradually limit the ability for insecure origins to be expressed
  in policy exceptions like these.

- **Change how, and for how long, Chrome stores site content provided
  over insecure connections** When browsing to a site over HTTP today,
  Chrome will both [cache site
  content](/developers/design-documents/network-stack/http-cache) for
  future use as well as providing access to persistent [local
  storage](https://developer.mozilla.org/en-US/docs/Web/API/Window/localStorage).
  When this content is provided over insecure and unauthenticated
  connections, the value of this persistent storage is significantly
  reduced and we are exploring ways to limit the duration this content
  will persist on user devices between visits to insecure origins.

## Powerful Features restricted to Secure Origins

A (non-exhaustive) list of powerful features we have already restricted to
secure origins includes:

- Geolocation
- Device motion / orientation
- EME
- getUserMedia
- AppCache
- Notifications

## Testing Powerful Features

If you are a developer that needs to keep testing a site using a powerful
feature that has been restricted to secure origins, [this
article](https://web.dev/how-to-use-local-https/) provides helpful instructions
for a variety of options for local development, including testing on
<http://localhost> where possible, creating and using self-signed certificates,
as well as creating, installing, and managing self-signed CAs to issue testing
and development certificates.

In addition to this guidance, there are some Chrome and Android-specific tips
that can help when developing and testing features restricted to secure origins:

1.  You can use `chrome://flags/#unsafely-treat-insecure-origin-as-secure` to run
    Chrome, or use the
    `--unsafely-treat-insecure-origin-as-secure="http://example.com"` flag
    (replacing "example.com" with the origin you actually want to test), which
    will treat that origin as secure for this session. Note that on Android and
    ChromeOS the command-line flag requires having a device with root access/dev
    mode.

2.  On a local network, you can test on your Android device using [port
    forwarding](https://developers.google.com/web/tools/chrome-devtools/remote-debugging/local-server)
    to access a remote host as localhost.

We continue to invest in improved methods for testing powerful features on
insecure origins, and we'll update this page once we've developed them. Feel
free to contribute ideas to
[security-dev@chromium.org](https://groups.google.com/a/chromium.org/forum/#!forum/security-dev).
