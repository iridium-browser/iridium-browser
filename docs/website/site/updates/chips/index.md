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

## End-to-End Testing

These instructions describe how a web developer can perform end-to-end testing of Partitioned cookies in Chromium.

Note: these instructions will only work with a Chromium instance M99 or above.

1. Go to chrome://flags/#partitioned-cookies and change the setting to "Enabled".

1. Restart Chromium by clicking the "Relaunch" button in the bottom-right corner, or by navigating to chrome://restart.

## Example Usage

Third parties which want to opt into receiving partitioned cross-site cookies should include the `Partitioned` attribute in their `Set-Cookie` header:

`Set-Cookie: __Host-name=value; Secure; Path=/; SameSite=None; `**`Partitioned`**

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

1. Open a new tab and navigate to https://cr2.kungfoo.net/cookies.

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

## Resources
- [CHIPS explainer](https://github.com/WICG/CHIPS)
- [CHIPS Chromestatus Page](https://chromestatus.com/feature/5179189105786880)
- [CHIPS Intent to Prototype](https://groups.google.com/a/chromium.org/g/blink-dev/c/hvMJ33kqHRo/m/3diUOI0uAQAJ)
