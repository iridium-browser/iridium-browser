---
breadcrumbs: []
page_name: testing-chrome-ad-filtering
title: Testing Chrome Ad Filtering
---

## Context

On February 15, 2018, Chrome will [stop showing
ads](https://blog.chromium.org/2017/06/improving-advertising-on-web.html) on
websites that are not compliant with the [Better Ads
Standards](http://betterads.org/standards). In order for site owners to
understand the impact that this would have on their site, they can test the
functionality by forcing this filtering to be active while they have Chrome's
DevTools open. However, this behavior would normally only take effect on sites
found to be noncompliant with the Standards for more than 30 days.

These instructions apply for Chrome on Windows, Mac, Linux, and Chrome OS.

## Instructions

    Open Chrome's DevTools: click the three dots <img alt="image"
    src="https://lh5.googleusercontent.com/S9E6omee_U2PEMfYgQvqjF1t4sTnX8zzqO2BoCJnW1CWR3GUFPKo2l5F-W9DpTzHXJwfgKAgQlpYV8_ZjQI0-IOxfpxZ3FgBaMEgHEFgrxg-MnK_maDuC16P-x2vQcbj1Xr9yKeL"
    height=26 width=20>menu in the upper right corner of Chrome, and select More
    tools -&gt; Developer tools. Alternatively, you can open DevTools by
    pressing Command+Option+I (Mac) or Control+Shift+I (Windows, Linux).

    In the new panel that opens, click the three dots <img alt="image"
    src="https://lh5.googleusercontent.com/S9E6omee_U2PEMfYgQvqjF1t4sTnX8zzqO2BoCJnW1CWR3GUFPKo2l5F-W9DpTzHXJwfgKAgQlpYV8_ZjQI0-IOxfpxZ3FgBaMEgHEFgrxg-MnK_maDuC16P-x2vQcbj1Xr9yKeL"
    height=26 width=20>menu and select Settings

    Under Network, check Force ad blocking on this site

    Try loading pages, leaving the DevTools panel open. If the DevTools panel is
    closed, ad blocking will cease to function.
