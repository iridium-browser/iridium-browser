---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/accessibility
  - Accessibility for Chromium Developers
page_name: html-accessibility
title: HTML Accessibility
---

Significant parts of the user interface on Chrome and Chrome OS are built using
the web platform: HTML, JavaScript, and CSS. These parts of Chrome include
"WebUI" (e.g. the New Tab Page, Options pages, Bookmark Manager, and the Chrome
OS Out-of-box-experience and Sign-in screens), plus many extensions and apps
provided by Google. This also applies to help pages and tutorials, such as the
Chrome OS welcome page and the Web Store.

Remember that all of these pages, sites, and apps will be accessed by users with
disabilities, including blind users with screen readers, low-vision users who
prefer large fonts, people who can't use a mouse, and more. You should use all
of the same guidelines you'd use when writing any webpage to make sure it's
maximally accessible to all users - there are thousands of resources online,
just do a Google search for "Accessible HTML".

## Accessibility Developer Tools

You can use the Accessibility Developer Tools extension on the Chrome Web Store
as one way to help test! This extension is developed by the Chromium team and
will help catch lots of common errors and hopefully make it much easier to catch
accessibility errors early in the design process.

*   [Accessibility Developer Tools
            Web](https://chrome.google.com/webstore/detail/accessibility-developer-t/fpkknkljclfencbdbgkenhalefipecmb)
            - for auditing any web page other than an app or built-in page
*   A[ccessibility Developer Tools
            Apps](https://chrome.google.com/webstore/a/google.com/detail/accessibility-developer-t/lfcjaoacndhilkpdhgnfjnienfoibnaa)
            - for auditing packaged apps (link is Google-only)
*   [Accessibility Developer Tools
            WebUI](https://chrome.google.com/webstore/a/google.com/detail/accessibility-developer-t/eacmnlimniaidhecpinhhfjjilfdaccm)
            - for Chromium WebUI like chrome://settings/ (link is Google-only)
*   [Build it from
            source](https://github.com/GoogleChrome/accessibility-developer-tools)

## Auditing and Testing

If you add WebUI tests, an [accessibility
audit](/developers/accessibility/webui-accessibility-audit), based on the same
code as the Accessibility Developer Tools, above, is automatically run on any
pages that get tests.

## Guidelines

Here are some specific things to think about in particular for Chrome:

**Focus**: Make sure that your page is usable without the mouse. You should be
able to press Tab to cycle through all focusable links and controls on the page.
It should be very visually apparent when an element has focus, and this
indication should be as consistent as possible within a page. It should be
impossible for focus to get "trapped" in a single control. When the user
activates a button or control that opens a dialog or pop-up, place focus inside
the new pop-up. When the user closes this pop-up, restore focus to the previous
place.

**Hover**: Don't ever use Hover as the exclusive way to activate a feature. Make
sure you can achieve the same thing by focusing a control with the keyboard.

**Visibility and CSS Animation**: If you use **opacity:0** or **height:0** to
make something disappear (especially for a nice animation), that item will still
be part of the render tree, and screen readers will still read it! You must add
some code to wait until the animation finishes and then set the hidden items to
**hidden**, **visibility:hidden**, or **display:none**.

**Color**: Don't ever use color as the only distinction between one state and
another, unless there are two states with very different perceived brightness
levels and an accessible annotation for screen reader users. Make sure that
there's a lot of contrast between text and the background color.

**Support 300% zoom**: Make sure everything is still usable at 300% zoom.

**Prefer native elements**: Don't use a clickable &lt;div&gt; if you can style a
&lt;a&gt; or &lt;button&gt; to look like what you want. If you create a custom
control out of a &lt;div&gt;, you must either read up on ARIA and test with at
least one screen reader, or send your changes to an accessibility expert for
review.

**Label Images**: All &lt;img&gt; tags must have an alt attribute with an
accessible name. If an image is purely decorative or if the accessible name is
already present in an adjacent element in the DOM, you can use alt="". Note that
alt="" is not the same as leaving out the attribute entirely; if you leave it
out, many screen readers will read the filename of the image instead. If you're
using a background image that's not purely decorative, use the **title**
attribute to provide an accessible name.

**&lt;!-- The alt text describes the meaning of the image. --&gt;**

&lt;**img** **src**="green_dot.png" **alt**="Online"&gt;

**&lt;!-- Purely decorative, so use an empty alt tag. --&gt;**

&lt;**img** **src**="separator.png" **alt**=""&gt;

**&lt;!-- Use a title attribute for a div with a background image. --&gt;**

&lt;**div** **style**="background-image: **url**('john_doe.png')"
**title**="John Doe"&gt;...&lt;**/div**&gt;

**Label Controls**: Controls should have a text label. If the control is a
button whose title is text already, nothing else is needed. Every other control
should usually have a &lt;label&gt; element pointing to it or wrapping it. If
that's impossible, give it a **title** or **placeholder** attribute.

**&lt;!-- Label separate from control. --&gt;**

&lt;**input id**="developer_mode" **type**="checkbox"&gt;

&lt;**label for**="developer_mode"&gt;Developer Mode&lt;**/label**&gt;

**&lt;!-- Label wraps control (also valid). --&gt;**

&lt;**label**&gt;

&lt;**input** **type**="checkbox"&gt;

Developer Mode

&lt;**/label**&gt;

**&lt;!-- Has placeholder. --&gt;**

&lt;**input** **placeholder**="First name"&gt;

**&lt;!-- These need titles because the labels are implied for users who can see
the screen.**

**(Maybe a bad example, but hopefully you get the idea.) --&gt;**

&lt;**input** **size="3" title**="Area code"&gt;

&lt;**input** **size="3" title**="Phone number (first 3 digits)"&gt; -

&lt;**input** **size="4" title**="Phone number (last 4 digits)"&gt;

**Where necessary, override visible text with accessible text:**

Occasionally the text that's on the screen would be confusing to a blind user
with a screen reader because the text is used symboically - for example an 'X'
for a close button. In that case, using the title attribute won't work because
the screen reader user will still also hear the 'X'. For this case, use
aria-label.

Try to avoid using aria-label when title or placeholder would work equally well.

**&lt;!-- Uses aria-label to override the 'X' text and make sure Close is spoken
instead. --&gt;**

&lt;**button aria-label**="Close"&gt;X&lt;**/button**&gt;

Note that when it's an interactive control (like a button, checkbox, etc.) the
aria-label attribute should go on the element that has focus.
