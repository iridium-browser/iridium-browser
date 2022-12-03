---
breadcrumbs:
- - /user-experience
  - User Experience
page_name: resolution-independence
title: Resolution Independence
---

**UI under development. Designs are subject to change.**

We are focusing on two primary resolutions (at 1280@125dpi and 1366@150dpi), but
should behave well on standard shipping netbooks.

[<img alt="image"
src="/user-experience/resolution-independence/Screen%20shot%202009-10-14%20at%208.47.17%20AM.png">](/user-experience/resolution-independence/Screen%20shot%202009-10-14%20at%208.47.17%20AM.png)

**Prototype: (Requires a webkit browser)**

****[Chrome dynamic UI
prototype](http://ux.chromium.org/demos/dynamicui/index.html#).****

<img alt="image"
src="/user-experience/resolution-independence/Screen%20shot%202010-02-02%20at%2010.51.45%20AM.PNG"
height=133 width=200>

For different DPIs we would support different multipliers:

*   **&lt;150** dpi
    *   Use existing assets
    *   Tabs are 24px tall with 16px icons
    *   Examples
        *   1024x600@10.1 (117dpi)
        *   1280x800@12.1 (125dpi)
        *   1366x768@11.6 (135dpi)
        *   Any screens with height &lt;600
*   **150+** dpi
    *   Use **1.25** Multiplier assets
    *   Tabs are 30px tall with 24px icons
    *   Examples:
        *   1366x768@10.1 (150dpi)

*   **190+** dpi
    *   Use **1.5** Multiplier assets
    *   Tabs are 36px tall with 32px icons
    *   Examples
        *   1920x1080@11.6 (190dpi Theoretical)

*   **&gt;220** dpi (or devices magnified by the user)
    *   Use vector based rendering
    *   Tabs are **1/5** inches tall

Exceptions:

*   Devices seen at greater distances use higher multipliers
