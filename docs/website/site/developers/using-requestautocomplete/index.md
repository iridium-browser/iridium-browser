---
breadcrumbs:
- - /developers
  - For Developers
page_name: using-requestautocomplete
title: Using requestAutocomplete()
---

The FAQ has moved
[here](https://developer.chrome.com/multidevice/requestautocomplete-faq).

See also the
[tutorial](http://www.html5rocks.com/en/tutorials/forms/requestautocomplete/)
and [HTML
standard](http://www.whatwg.org/specs/web-apps/current-work/multipage/association-of-controls-and-forms.html#dom-form-requestautocomplete).

Tips and tricks for developing

*   When testing, you should tell Chrome to use Wallet's sandbox servers
            so that issued card numbers will not be chargeable. Go to
            **about:flags** and enable the "**Use Wallet Sandbox servers**"
            flag.
*   Developing with Chrome's **early release channels** (like
            [dev](https://www.google.com/chrome/eula.html?extra=devchannel) and
            [beta](https://www.google.com/chrome/eula.html?extra=betachannel))
            will give you earlier access to bug fixes and improvements such as
            helpful debugging messages and better i18n support. However, the
            majority of your site's visitors are likely to be on the stable
            channel.
*   If rAc is not working for you, check the **developer tools console**
            to see if an error message was logged.
*   If you want to develop/test on an http:// site (**no SSL cert**),
            you can launch Chromium with the command line flag
            **--reduce-security-for-testing**
*   You can limit which address countries rAc shows by making a
            **&lt;select&gt;** with one **&lt;option&gt;** per supported
            country.
*   When a user chooses **Wallet**, they'll get a [Virtual OneTime
            Card](https://support.google.com/wallet/answer/2740044?hl=en).
            That's the card your site will see.