---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
- - /developers/design-documents/network-stack
  - Network Stack
page_name: proxy-settings-fallback
title: Proxy settings and fallback
---

On Windows, Chromium uses WinInet's proxy settings.

Consequently, it is important that Chromium interpret and apply these proxy
settings in the same manner as WinInet. Otherwise the same proxy settings may
give different results in Chromium than in other WinInet-based applications
(like Internet Explorer).

In Firefox, the proxy settings are divided into four different modes using radio
buttons. This modal approach makes it pretty easy to understand which proxy
settings will be used, since there is only one set of choices.

<img alt="image"
src="/developers/design-documents/network-stack/proxy-settings-fallback/fox-proxy-settings.png">

However in Internet Explorer, the settings are more complex.

All of the various settings are presented in the UI as optional checkboxes.

This makes it unclear what is supposed to happen when conflicting choices are
given.

Screenshot of IE's settings dialog:

<table>
<tr>
<td> <img alt="image" src="/developers/design-documents/network-stack/proxy-settings-fallback/ie-proxy-settings.png"> </td>
<td> + </td>
<td> <img alt="image" src="/developers/design-documents/network-stack/proxy-settings-fallback/ie-proxy-server-settings.png"> </td>
</tr>
</table>

## How WinInet resolves the ambiguity

\[The following was determined experimentally using Internet Explorer 8 on
Windows XP. (Couldn't find an official explanation of the steps to link to).\]

The way Internet Explorer applies these settings is using a fallback scheme
during initialization:

<img alt="image"
src="/developers/design-documents/network-stack/proxy-settings-fallback/ie-fallback.png">

*   Fallback between the automatic settings is represented with a **blue
            arrow**, and occurs whenever:
    *   The setting is not specified.
    *   The underlying PAC script failed to be downloaded.
    *   The underlying PAC script failed to be parsed.
*   Fallback between the manual settings is represented by a **black
            arrow**, and occurs whenever:
    *   The setting is not specified.
*   The bypass list is applied ONLY within the manual settings.

TODO(eroman): haven't verified fallback for SOCKS.

There is a secondary fallback mechanism at runtime:

<img alt="image"
src="/developers/design-documents/network-stack/proxy-settings-fallback/ie-auto-fallback.png">
<img alt="image"
src="/developers/design-documents/network-stack/proxy-settings-fallback/ie-manual-fallback.png">

So for example if auto-detect was chosen during the initialization sequence, but
the PAC script is failing during execution of FindProxyForURL(), it will
fallback to direct (regardless of whether there are manual proxy settings).
