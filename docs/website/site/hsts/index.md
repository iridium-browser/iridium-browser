---
breadcrumbs: []
page_name: hsts
title: HTTP Strict Transport Security
---

[HTTP Strict Transport Security](https://tools.ietf.org/html/rfc6797) allows a
site to request that it always be contacted over HTTPS. HSTS is supported in
Google Chrome,
[Firefox](https://blog.mozilla.org/security/2012/11/01/preloading-hsts/),
Safari, Opera, Edge and
[IE](https://web.archive.org/web/20150217020413/https://blogs.msdn.com/b/ie/archive/2015/02/16/http-strict-transport-security-comes-to-internet-explorer.aspx)
(caniuse.com has a [compatibility
matrix](https://caniuse.com/#feat=stricttransportsecurity)).

The issue that HSTS addresses is that users tend to type `http://` at best, and
omit the scheme entirely most of the time. In the latter case, browsers will
insert `http://` for them.

However, HTTP is insecure. An attacker can grab that connection, manipulate it
and only the most eagle eyed users might notice that it redirected to
`https://www.bank0famerica.com` or some such. From then on, the user is under the
control of the attacker, who can intercept passwords, etc at will.

An HSTS enabled server can include the following header in an HTTPS reply:

```
Strict-Transport-Security: max-age=16070400; includeSubDomains
```

When the browser sees this, it will remember, for the given number of seconds,
that the current domain should only be contacted over HTTPS. In the future, if
the user types `http://` or omits the scheme, HTTPS is the default. In fact, all
requests for URLs in the current domain will be redirected to HTTPS. (So you
have to make sure that you can serve them all!).

For more details, see the [specification](https://tools.ietf.org/html/rfc6797).

## Preloaded HSTS sites

There is still a window where a user who has a fresh install, or who wipes out
their local state, is vulnerable. Because of that, Chrome maintains an "HSTS
Preload List" (and other browsers maintain lists based on the Chrome list).
These domains will be configured with HSTS out of the box.

If you own a site that you would like to see included in the preloaded HSTS list
you can submit it at <https://hstspreload.org>.

## Examining the HSTS list within the browser

You can see the current HSTS Rules -- both dynamic (set by a response header)
and static (preloaded) using a tool on the `about://net-internals#hsts` page.

Check the source for the [full
list](https://cs.chromium.org/chromium/src/net/http/transport_security_state_static.json).

(To see the version of the list in a particular version of Chrome, visit [this
URL](https://chromium.googlesource.com/chromium/src/+/__branch_commit__/net/http/transport_security_state_static.json)
with `__branch_commit__` replaced by the hash of the relevant build from
[here](https://omahaproxy.appspot.com/).)
