---
breadcrumbs:
- - /Home
  - Chromium
- - /Home/chromium-security
  - Chromium Security
- - /Home/chromium-security/education
  - Education
page_name: security-tips-for-crx-and-apps
title: Security Tips for Developing CRX
---

*(Note: Content adapted from an [earlier blog post on this
topic](http://blog.chromium.org/2011/07/writing-extensions-more-securely.html).)*

Chrome extensions (CRX) are powerful pieces of software in modern browsers, and
as such, you should help ensure that your extensions are not susceptible to
security exploits. If an attacker manages to exploit a vulnerability in a CRX,
they may gain access to the same privileges that the extension has. The Chrome
extensions system has a number of [built-in
protections](http://blog.chromium.org/2009/12/security-in-depth-extension-system.html)
to make it more difficult to introduce exploitable code, but certain coding
patterns can still introduce common web vulnerabilities like cross-site
scripting (XSS).

## **Minimize your permissions**

The most important thing to consider is whether you’re declaring the minimal set
of permissions that you need to function. That way, if you do have a security
bug in your code, the amount of permissions you’re exposing to the attacker is
minimal as well. Avoid requesting (\*/\*) permissions for hosts if you only need
to access a couple, and don’t copy and paste your manifest from example code
blindly. Review your manifest to make sure you’re only declaring what you need.
This applies to permissions like tabs, history, cookies, etc. in addition to
host permissions. For example, if all you’re using is
[chrome.tabs.create](http://code.google.com/chrome/extensions/tabs.html#method-create),
you don’t actually need the tabs permission.

Another way of minimizing install time permissions is to use [optional
permissions](https://developer.chrome.com/extensions/permissions). Using
chrome.permissions API, you can request permissions only when you need them
during runtime.

## **Use content_security_policy in your manifest**

[Content Security
Policy](http://dvcs.w3.org/hg/content-security-policy/raw-file/tip/csp-specification.dev.html)
is supported in extensions via the
[content_security_policy](http://code.google.com/chrome/extensions/trunk/manifest.html#content_security_policy)
manifest field. This allows you to control where scripts can be executed, and it
can be used to help reduce your exposure to XSS. For example, to specify that
your extension loads resources only from its own package, use the following
policy:
"content_security_policy": "default-src 'self'"
If you need to include scripts from specific hosts, you can add those hosts to
this property.

## **Don’t use &lt;script src&gt; with an HTTP URL**

When you include javascript into your pages using an HTTP URL, you’re opening
your extension up to man-in-the-middle (MITM) attacks. When you do so in a
content script, you have a similar effect on the pages you’re injected into. An
attacker on the same network as one of your users could replace the contents of
the script with content that they control. If that happens, they could do
anything that your page can do.
If you need to fetch a remote script, always use HTTPS from a trusted source.

## **Don’t use eval()**

The eval() function is very (too!) powerful, and you should avoid using it
unless absolutely necessary. Where did the code come from that you passed into
eval()? If it came from an HTTP URL, you’re vulnerable to a MITM attack. If any
of the content that you passed into eval() is based on content from a random web
page the user visits, you’re vulnerable to content escaping bugs. For example,
let’s say that you have some code that looks like this:
function displayAddress(address) { // address was detected and grabbed from the
current page
eval("alert('" + address + "')");
}
If it turned out that you had a bug in your parsing code, the address might wind
up looking something like this:
'); dosomethingevil();
There’s almost always a better alternative to using eval(). For example, you can
use JSON.parse if you want to parse JSON (with the added benefit that it runs
faster).

## **Don’t use innerHTML or document.write()**

It’s really tempting to use innerHTML because it’s much simpler to generate
markup dynamically than to create DOM nodes one at a time. However, this sets
you up for XSS bugs. For example:
function displayAddress(address) { // address was detected and grabbed from the
current page
myDiv.innerHTML = "&lt;b&gt;" + address + "&lt;/b&gt;");
}
This would allow an attacker to make an address like the following and once
again run some script in your page:
&lt;script&gt;dosomethingevil();&lt;/script&gt;
Instead of innerHTML, you can manually create DOM nodes and use innerText to
insert dynamic content.

## **Beware of external content**

In general, if you’re generating dynamic content based on data from outside of
your extension (such as something you fetched from the network, something you
parsed from a page, or a message you received from another extension, etc.), you
should be extremely careful about how you use and display it. If you use this
data to generate content within your extension, you might be opening your users
up to increased risk.

## **Additional Resources**

*   Additional security tips and examples for Content Scripts in our
            [Extension Developer
            Docs](https://developer.chrome.com/extensions/content_scripts.html#security-considerations)
*   [Advanced Chrome Extension Exploitation Leveraging API powers for
            Better
            Evil](http://kyleosborn.com/bh2012/advanced-chrome-extension-exploitation-WHITEPAPER.pdf)
            (by krzysztof@kotowicz.net and kos@kos.io)
*   [Security guidelines for Chrome Extension & App API
            developers](https://docs.google.com/document/d/1RamP4-HJ7GAJY3yv2ju2cK50K9GhOsydJN6KIO81das/pub)
