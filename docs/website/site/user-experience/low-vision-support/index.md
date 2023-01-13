---
breadcrumbs:
- - /user-experience
  - User Experience
page_name: low-vision-support
title: 'Accessibility: Low-Vision Support'
---

We believe that Chrome should provide a complete and satisfying experience to
users who have low vision. Here are three of the most common needs, and how they
can currently be addressed in Chrome.

[TOC]

### Full Page Zoom

In order to increase or decrease the zoom level of the currently displayed web
page, you can use either the keyboard shortcuts (see table below) or access
‘Zoom-&gt;Larger/Normal/Smaller’ in the Chrome Page menu.

[<img alt="image"
src="/developers/design-documents/accessibility/xoom_menu.png">](/developers/design-documents/accessibility/xoom_menu.png)

Page zoom level will be remembered for *each unique domain*, e.g. if you
increase zoom for Google Search (http://www.google.com), zoom level will also be
increased for Google Calendar (http://www.google.com/calendar). However, since
Gmail (http://mail.google.com) is on a different domain, zoom level will remain
unchanged in this example.

<table>
<tr>
<td> <b>Shortcut </b></td>
<td><b> Action</b> </td>
</tr>
<tr>
<td> ​Ctrl++</td>
<td> ​Increase zoom level.</td>
</tr>
<tr>
<td> ​Ctrl+-</td>
<td> ​Decrease zoom level.</td>
</tr>
<tr>
<td> ​Ctrl+0</td>
<td> ​Set zoom level to default level.</td>
</tr>
</table>

### Adjusting Font Face and Size

There are a number of ways to adjust your font settings. Chrome natively
supports changing of Fonts and Languages, including setting font face and size
for:

*   Serif Font
*   Sans-Serif Font
*   Fixed-width Font

To access these settings, open the Google Chrome Options, select the Under the
Hood tab and click the Fonts and Languages button. The dialog that opens allows
you to adjust your font settings (note: there is an open bug
([1040](http://crbug.com/1040)) to allow these settings to override fonts
specified in web pages).

[<img alt="image"
src="/developers/design-documents/accessibility/chrome_font_and_languages.png">](/developers/design-documents/accessibility/chrome_font_and_languages.png)

In addition, there are a variety of Chrome Extensions that can be used to modify
various font properties:

*   [Change
            Colors](https://chrome.google.com/extensions/detail/jbmkekhehjedonbhoikhhkmlapalklgn)
    *   Optional font family and font size configuration
    *   Ability to add new custom fonts
*   [Minimum
            Font](https://chrome.google.com/extensions/detail/pofdgleodhojjnibdfnlapkadjepdnka)
    *   Allows you to set the minimum font size on all web pages
*   [Zoomy](https://chrome.google.com/extensions/detail/jgfonhdeiaaflpgphemdgfkjimojblie)
    *   Changes zoom level according to resolution & browser size.

### High Contrast and Custom Color Support

There are a number of steps you can take to configure Chrome to run with custom
contrast and colors. Beginning in [Chrome
73](https://developers.google.com/web/updates/2019/03/nic73), Chrome provides
partial support for system-level color schemes. Dark mode for Chrome's UI is now
supported on Mac, and Windows support is on the way.

1.  Install a [Chrome
            Extension](http://www.google.com/support/chrome/bin/answer.py?answer=154007)
            which allows you to specify your own custom color combinations, for
            instance the [Change
            Colors](https://chrome.google.com/extensions/detail/jbmkekhehjedonbhoikhhkmlapalklgn)
            extension.
    *   Quick page action to apply/remove styling overrides on a per
                page, per domain or global basis (overriding Web page colors)
    *   Optional background, text, links and visited links color
                configuration
    *   Option for showing/hiding images
    *   Option for showing/hiding Flash objects
2.  Use a [Chrome
            Theme](https://tools.google.com/chrome/intl/en/themes/index.html)
            for some control of the color scheme of the Chrome user interface.
            As an example, the [BitNova Dark
            theme](https://chrome.google.com/extensions/detail/okaafmdeogblpdihiidddcnclfhpngcm)
            offers white text on a black background. The [Chrome Extensions
            Gallery](https://chrome.google.com/extensions) offers many other
            themes, with a variety of color combinations.

[<img alt="image"
src="/developers/design-documents/accessibility/custom_colors.png">](/developers/design-documents/accessibility/custom_colors.png)

---

#### Other pages on accessibility

*   [Accessibility: Keyboard Access](/user-experience/keyboard-access)
*   [Accessibility: Touch Access](/user-experience/touch-access)
*   [Accessibility: Screen reader
            support](/user-experience/assistive-technology-support)
*   [Accessibility Design
            Document](/developers/design-documents/accessibility) (for
            developers)
