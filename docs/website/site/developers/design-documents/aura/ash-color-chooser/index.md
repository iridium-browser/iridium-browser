---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
- - /developers/design-documents/aura
  - Aura
page_name: ash-color-chooser
title: Ash Color Chooser
---

**## Overview**

**This document describes how to achieve &lt;input type=”color”&gt; UI in ChromeOS/Ash.**

**# Motivation**

**&lt;input type=”color”&gt; is a new input type introduced in HTML5. It enables users to pick a color and returns a value of #hhhhhh format. WebKit forms team has decided to use platform’s default color picker rather than implementing a new one. It means that we need to implement our own color picker view for ChromeOS/Ash.**

**# First Goal**

*   **Implement simple color picker dialog w[orking with &lt;input
            type=”color&gt;](https://docs.google.com/a/google.com/document/d/1YIKO-5_m0ZpkvtE24QKKty1ZZk7aYVk-Bw9Xq8zpMEg/edit?hl=en_US).**
*   **Implement with views, not HTML.**
*   **Features**
    *   **Behave as Modal Dialog (see
                [doc](https://docs.google.com/a/google.com/document/d/1YIKO-5_m0ZpkvtE24QKKty1ZZk7aYVk-Bw9Xq8zpMEg/edit?hl=en_US))**
        *   **Appears in the topmost z-order**
    *   **Pick up a color from HSV model**
        *   **Has both a Hue bar and a Saturation-Value plane (see below
                    mock).**
    *   **Pick up a color from text**
        *   **A[llow
                    copy/paste](https://docs.google.com/a/google.com/document/d/1YIKO-5_m0ZpkvtE24QKKty1ZZk7aYVk-Bw9Xq8zpMEg/edit?hl=en_US)**
        *   **Specify the color by …**
            *   **rgb(xx ,yy, zz)**
            *   **#RRGGBB**
            *   **CSS well-known color name**
            *   **… anything else?**
    *   **Work on Chrome OS**

*   **first mock**

    **![](/developers/design-documents/aura/ash-color-chooser/col.png)**

    *   **Confirmed that the upper box isn’t necessary, since the color
                will be shown in the page.**
*   **Current screenshot:**

    **![](/developers/design-documents/aura/ash-color-chooser/Screenshot%202012-06-26%203_50_42%20PM.png)**

**# Next plans (not confirmed yet)**

*   **Global Color Picker: Picking a color from any point on the
            desktop.**
    *   **Even from other tab, window or desktop.**
*   **“Favorite Color” feature**
    *   **Pick up the color easily from each user’s favorite color.**
*   **Works on Windows and Chrome OS**
    *   **Currently, Windows has native OS Color PIcker Dialog. This new
                Color Picker will replace it.**

**# Not Goal**

*   **Alpha value (transparency) support**
    *   **HTML5 spec has not supported alpha value.**

**# References**

**Input type=color spec: <http://www.w3.org/TR/html-markup/input.color.html>**
