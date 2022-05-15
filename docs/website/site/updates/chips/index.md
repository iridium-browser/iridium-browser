---
breadcrumbs:
- - /updates
  - updates
page_name: chips
title: Cookies Having Independent Partitioned State (CHIPS)
---

## Motivation

From the [CHIPS explainer](https://github.com/WICG/CHIPS):

In order to increase privacy on the web, browser vendors are either planning or already shipping restrictions on cross-site tracking. This includes [phasing out support for third-party cookies](https://blog.chromium.org/2020/01/building-more-private-web-path-towards.html), cookies sent in requests to sites other than the top-level document's site, since such cookies enable servers to track users' behavior across different top-level sites.

Although third-party cookies can enable third-party sites to track user behavior across different top-level sites, there are some cookie use cases on the web today where cross-domain subresources require some notion of session or persistent state that is scoped to a user's activity on a single top-level site.

## The `Partitioned` Attribute

Partitioned cookies are cross-site cookies which are only available on the top-level site they were created.

Third parties which want to opt into receiving partitioned cookies should include the `Partitioned` attribute in their `Set-Cookie` header:

`Set-Cookie: __Host-name=value; Secure; Path=/; SameSite=None; `**`Partitioned`**

Partitioned cookies must include the [`__Host-` prefix](https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Set-Cookie#cookie_prefixes) and cannot have the [`SameParty` attribute](https://developer.chrome.com/blog/first-party-sets-sameparty/).
Chrome will enforce these rules for cookies set with the `Partitioned` attribute even when cookie partitioning is disabled, but if the feature is disabled the resulting cookie will still be sent to requests to its host on different top-level sites than where it was set.

## Origin Trial

If you are interested in participating in the CHIPS origin trial, then you need to include the `Origin-Trial` header in each HTTP response with a valid token.
You must also send the `Accept-CH: Sec-CH-Partitioned-Cookies` header in each HTTP response as well.

If you have successfully opted into the origin trial, subsequent requests from the Chrome client will include the `Sec-CH-Partitioned-Cookies: ?1` request header until the current session is ended.
If you store persistent partitioned cookies then you will receive the `Sec-CH-Partitioned-Cookies: ?0` request header for the first request to the cookies' origin.

**Not all Chrome clients using versions 100-102 will be in the trial.**
**If the client does not send the Sec-CH-Partitioned-Cookies header, then partitioned cookies are not enabled.**

If you do not respond with a valid token in the `Origin-Trial` header and `Accept-CH: Partitioned-Cookies`, then the partitioned cookies on the machine will be converted to unpartitioned cookies.

You can register your site for the origin trial [here](https://developer.chrome.com/origintrials/#/view_trial/1239615797433729025).

**The origin trial is only available in Chrome versions 100-102.**

### Example usage

When a site wishes to participate in the origin trial, they should include the following headers in their response:

```text
Origin-Trial: *valid Origin Trial token*
Accept-CH: Sec-CH-Partitioned-Cookies
Set-Cookie: __Host-name=value; Secure; Path=/; SameSite=None; Partitioned;
```

Remember, in order to keep participating in the trial, you must include these headers in each HTTP response.

If the opt in is successful, Chrome will include the following headers in future requests:

```text
Sec-CH-Partitioned-Cookies: ?1
Cookie: __Host-name=value
```

If your site receives the cookie without this client hint, then this means your site did not opt into the origin trial correctly and the cookie you are receiving is not partitioned.

If the site sets persistent partitioned cookies (e.g. a max age of 1 day):

```text
Origin-Trial: *valid Origin Trial token*
Accept-CH: Sec-CH-Partitioned-Cookies
Set-Cookie: __Host-name=value; Secure; Path=/; SameSite=None; Partitioned; Max-Age=86400;
```

if the user visits the site after the current session has ended, the first request to the site will include the following request headers:

```text
Sec-CH-Partitioned-Cookies: ?0
Cookie: __Host-name=value
```

If the site responds with the `Accept-CH` and `Origin-Trial` headers, Chrome will continue to send partitioned cookies and the `Sec-CH-Partitioned-Cookies: ?1` request header.

If the site does not opt back into the trial, the `__Host-name` cookie will be converted into an unpartitioned cookie.
This will allow the site to roll back its usage of partitioned cookies in case it causes server breakage.

You can also persist your participation in the origin trial between sessions using the `Critical-CH: Sec-CH-Partitioned-Cookies` response header:

```text
Origin-Trial: *valid Origin Trial token*
Accept-CH: Sec-CH-Partitioned-Cookies
Critical-CH: Sec-CH-Partitioned-Cookies
```

This will cause Chrome to restart the request and send the `Sec-CH-Partitioned-Cookies: ?1` request header.

### JavaScript

Frames that [opt into the Origin Trial](http://googlechrome.github.io/OriginTrials/developer-guide.html) will have access to reading and writing partitioned cookies via JavaScript APIs such as document.cookie and the CookieStore API.
Frames that are not in the trial's scripts will neither be able to read nor write partitioned cookies.

The CHIPS Origin Trial will not be supported in service workers.

### Design doc

You can view the more detailed design document of the CHIPS Origin Trial [here](https://docs.google.com/document/d/1EPHnfHpZHpV09vITXu8cEEIMt1DYiRN_pZfeBal8UQw).

### Local testing

If you want to test your changes locally on your machine, you can enable the CHIPS origin trial bypass feature (chrome://flags/#partitioned-cookies-bypass-origin-trial) on your local device to use partitioned cookies on any site without them needing to opt into the trial.

### A/B testing

Sites can use the origin trial for [A/B testing](https://en.wikipedia.org/wiki/A/B_testing) by sending the `Origin-Trial` and `Accept-CH: Sec-CH-Partitioned-Cookies` headers to users in the experiment group and not send those headers to users in the control group.
However, determining which users should belong to which group is the responsibility of the origin trial participant's servers.

## End-to-End Testing

These instructions describe how a web developer can perform end-to-end testing of Partitioned cookies in Chromium.

Note: these instructions will only work with a Chromium instance M100 or above.

1. Go to chrome://flags/#partitioned-cookies and change the setting to "Enabled".

1. Restart Chromium by clicking the "Relaunch" button in the bottom-right corner, or by navigating to chrome://restart.

## Example Usage

Embeds which want to opt into using partitioned cookies should include the `Partitioned` attribute in their `Set-Cookie` header:

`Set-Cookie: __Host-name=value; Secure; Path=/; SameSite=None; `**`Partitioned`**

Note that the cookie has the `__Host-` prefix and does not include the `SameParty` attribute.

You can also set a partitioned cookie in JavaScript:

```javascript
cookieStore.set({
  name: '__Host-name',
  value: 'value',
  secure: true,
  path: '/',
  sameSite: 'none',
  // Set a partitioned cookie using the attribute below.
  partitioned: true,
});
```

## Demo

One you have followed the instructions in the [End-to-End Testing](#end-to-end-testing) section, you can use the following instructions to see a demonstration of the Partitioned attribute.

1. Go to chrome://settings/cookies and make sure that the radio button is set to "Allow all cookies" or "Block third-party cookies in Incognito".

1. Open a new tab and navigate to https://cr2.kungfoo.net/cookies/index.php.

1. Click "Set cookie (SameSite=None)" to set an **unpartitioned** SameSite=None cookie named "unpartitioned".

1. Click "Set partitioned cookie (SameSite=None; Partitioned)" to set a partitioned cookie, "__Host-1P_partitioned".

1. Open DevTools to Application > Cookies > https://cr2.kungfoo.net and you should see both the UI display both cookies.
  Note that the partitioned cookie has a "Partition Key", https://kungfoo.net, whereas the unpartitioned cookie does not.

1. Navigate to https://lying-river-tablecloth.glitch.me to see a site which embeds a cross-site iframe whose origin is https://cr2.kungfoo.net.
  As you can see, the unpartitioned SameSite=None cookie is available, but the partitioned cookie is not since we are now on a different top-level site.

1. Click "Set partitioned cookie (SameSite=None; Partitioned)" to set a partitioned cookie, "__Host-3P_partitioned", in a third-party context.

1. Open Application > Cookies > https://cr2.kungfoo.net and you should see both cookies. Note that the new partitioned cookie's "Partition Key" is https://lying-river-tablecloth.glitch.me, the top level site (glitch.me is on the [Public Suffix List](https://publicsuffix.org/)).

1. Go back to the first tab which has the chrome://settings/cookies page open. Change the setting to "Block Third-Party Cookies".

1. Navigate back to the tab navigated to https://lying-river-tablecloth.glitch.me and refresh the page.
  You should see that only the partitioned cookie, "__Host-3P_partitioned", is available.

1. Open DevTools to the "Network" tab and refresh the page. Click on the request to "thirdparty.html" and click on the "Cookies" tab.
  You should see that the unpartitioned cookie was blocked in a third-party context, but the partitioned cookie was allowed.

1. Go back to the tab which has chrome://settings/cookies open. Change the setting back to "Allow all cookies" or "Block third-party cookies in Incognito".

1. Open the tab which has https://lying-river-tablecloth.glitch.me open. You should see that both the "unpartitioned" and "__Host-3P_partitioned" cookies are available again.

1. Click the "Clear cookies" button. This will cause cr2.kungfoo.net to send the `Clear-Site-Data: "cookies"` header. You should see that both the "unpartitioned" and "__Host-3P_partitioned" cookies were removed.

1. Navigate the tab back to https://cr2.kungfoo.net/cookies/index.php. You should see that the "__Host-1P_partitioned" cookie was not removed after cr2.kungfoo.net sent the `Clear-Site-Data` header on a different top-level site.

## Resources
- [CHIPS explainer](https://github.com/WICG/CHIPS)
- [CHIPS Chromestatus Page](https://chromestatus.com/feature/5179189105786880)
- [CHIPS Intent to Prototype](https://groups.google.com/a/chromium.org/g/blink-dev/c/hvMJ33kqHRo/m/3diUOI0uAQAJ)
