---
breadcrumbs:
- - /developers
  - For Developers
page_name: rtl-in-webkit
title: BiDi support in WebKit
---

[TOC]

## ## RTL Task Force

Objective: Improve RTL handling in webkit editing and layout/rendering.

Mailing list: rtl-editing@google.com

[Issue
tracker](https://spreadsheets.google.com/a/google.com/ccc?key=0AjIpE6jmfbPCdHJlOW9iUUp5WmpuQ1VIUTBFR2VncUE&authkey=CKCj8d4O&hl=en#gid=0)

Members: Alex Komoroske (PM), Xiaomei Ji, Ryosuke Niwa, Jeremy Moskovich, Levi
Weintraub, Uri Bernstein, Aharon Lanin, Roland Steiner, Jason Elbaum, Ahmed
Hassan, Mohamed Elhawary, Michal Levin (UX).

## Standard Proposal: Additional Requirements for Bidi in HTML

*   [Proposal](http://www.w3.org/TR/html-bidi/)

*   [Implementation
            status](https://docs.google.com/a/google.com/document/d/17nb3wlYkIG9MNtL1mlXVPOVe4QwyApiHRYim3nTi5CE/edit?hl=en#)

*   [WebKit rollup bug](https://bugs.webkit.org/show_bug.cgi?id=50910)

## Test Suite

RTL in Rich Text Edit in [browserscope](http://www.browserscope.org/).

HTML+CSS internationalization test suite:[ text
direction](http://www.w3.org/International/tests/tests-html-css/list-text-direction).

## The Challenges

### Lack of one-to-one correspondence between visual and logical character positions.

For right-to-left text, logical order (the order the text is stored) and visual
order (the order the text is displayed) are not the same. They are not just the
reversed order of each other when bi-directional text is involved. They also
don't have a simple relationship (without going through reordering resolved
levels in the [ Unicode Bidi Algorithm](http://unicode.org/reports/tr9/)). For
example, the last logical run of text could be a in the middle visually (not
visually first neither visually last).

&lt;div style="direction: ltr;"&gt;

Just

&lt;span&gt;testing רק&lt;/span&gt;

בודק

&lt;/div&gt;

\[Bidi editing in Mozilla\] The main issue involved in bidirectional editing is
that there is no one-to-one relationship between a logical position in the text,
and a visual position on screen. A single logical position can map to two
different visual positions, and a single visual position might map to two
different logical positions.

*In the following examples, I will use uppercase Latin letters to represent RTL
(e.g. Hebrew) letters, whereas lowercase Latin letters will represent LTR (e.g.
Latin) letters. This is the convention, as it makes it easier for people who can
not read RTL languages to understand the examples.*

Consider, for example, the text with the following logical representation:
**latinHEBREWmore** (this example deliberately omits spaces, in order to avoid
the issues associated with resolving their directionality). This text is
displayed on the screen as **latinWERBEHmore**.

Consider the logical position between **n** and **H**. This is immediately after
**n**, so it maps to the visual position between **n** and **W**. But it is also
immediately before **H**, so it also maps to the visual position between **H**
and **m**.

Now, consider the visual position between **n** and **W**. It is immediately
after **n**, so it can be mapped to the logical position between **n** and
**H**. But it also immediately after **W**, so it can also be mapped to the
logical position between **W** and **m**.

Bidirectional text is stored logically, and (obviously) displayed visually. The
caret, being a graphical element, corresponds to a visual location. The user can
manipulate text and move the caret through a combination of **logical
functions** (such as typing or deleting) and **visual functions** (such as using
the arrow keys). Therefore, the problem of mapping between logical and visual
positions in a way that meets the user's expectations is the central problem of
bidirectional editing.\[Bidi editing in Mozilla\]

IBM has a non-trivial [algorithm
](http://www-01.ibm.com/software/globalization/topics/bidiui/conversion.jsp)for
rendering the caret in correct position in bi-directional text.

### Lack of clear spec on BiDi editing behavior

As mentioned above, mapping between logical and visual positions is the central
problem of bidirectional editing. But, there is no standard on how such mapping
should be done. Different browsers behave differently. For example, a logical
string "123ABC" (where 123 represents Arabic number and ABC represents Arabic
letter) is rendered as "CBA123" visually when the paragraph directionality is
left-to-right. The visual positions in Firefox, IE, and Chrome/WebKit are as
following:

Internet Explorer: (0)(1)(7)C(6)B(5)A(4)1(2)2(3)3
Firefox: (0)(7)C(6)B(5)A(1)(4)1(2)2(3)3
Chrome/WebKit: (0)C(5)B(4)A1(1)2(2)3(3)(6).
Which one meets user's expectation is under
[discussion](https://bugs.webkit.org/show_bug.cgi?id=53696) right now.
Another case of lacking clear spec is that there is no consensus on whether a UI
function should behave logically or visually. Different browsers might behave
differently. For example, IE handles cursor movement (arrow keys) as logical
function, while Firefox (by default) and WebKit \[after a long debate on the
topic\] handle it as visual function. For example, there are always discussions
on whether selection should be treated as visual or logical. Currently, IE,
Firefox (by default) and WebKit all treat selection as a logical operation. But
Firefox also provides options to toggle between visual and logical behavior. For
example, whether HOME & END should put the caret at the visual extreme or they
should be considered as logical operation. Currently, IE and WebKit handle HOME
& END as putting the caret at the visual extreme, while Firefox (by default)
handles it as logical operation.

### Lack of clear spec on behavior of HTML in the face of certain BiDi issues.

For some HTML elements, there is no standard defined on how they should be
rendered in RTL or in a bi-directional context. Different browsers behave
differently. Since RTL users are mostly adjusted to the behavior of IE, they
would expect the same behavior as that of IE, even it might not be the desired.
For example, there is no standard defined on how to render &lt;input&gt;
element, whether it should auto-detects the input text's directionality and uses
that for rendering, or use the &lt;input&gt; element's directionality for
rendering the input text. There are some discussions (comment
[#2](https://bugs.webkit.org/show_bug.cgi?id=27889#c10),
[#7](https://bugs.webkit.org/show_bug.cgi?id=27889#c10),
[#8](https://bugs.webkit.org/show_bug.cgi?id=27889#c10),
[#10](https://bugs.webkit.org/show_bug.cgi?id=27889#c10) ) in related WebKit
[issue](https://bugs.webkit.org/show_bug.cgi?id=27889). For example, there is no
standard defined on how to render &lt;selection&gt;/&lt;option&gt; elements when
either or both the element have 'dir' attributes defined. There is discussion
(comment [#2](https://bugs.webkit.org/show_bug.cgi?id=29612#c5),
[#5](https://bugs.webkit.org/show_bug.cgi?id=29612#c5)) in related Webkit
[issue](https://bugs.webkit.org/show_bug.cgi?id=29612). The same goes for for
script dialog text, &lt;title&gt; attribute etc. Aharon Lanin proposed
additional requirement for BiDi in HTML for HTML5.

The main issue involved in RTL rendering is not only whether to render
bi-directional text correctly for any HTML structure (from example, inline
element inside block element) when 'dir' attribute is set, when unicode bidi
control characters are used, when unicode-bidi property is set. But also to
rendering the whole RTL page correctly. For example, previously, the #1 issue
reported in Middle East North Africa by Chrome users is the [truncation of the
content of RTL page](https://bugs.webkit.org/show_bug.cgi?id=23556)s so only the
right-most part of the content that fit in browser window was displayed. Fixing
it requires a good understanding of the rendering layer (with 67k line of code)
and its relation with the page and platform layers. Beyond that, the fix needed
to work on different platforms (for example, Apple's Mac port has it's own
distinct code path handling scroll view) as well.

## References

"dir" attribute:

<http://www.w3.org/TR/html401/struct/dirlang.html#h-8.2>

"direction" and "unicode-bidi" properties:

<http://www.w3.org/TR/CSS21/visuren.html#direction>

Handling right-to-left scripts:

<http://www.w3.org/TR/i18n-html-tech-bidi/>

Internationalization techniques: handling text direction:

<http://www.w3.org/International/techniques/authoring-html#direction>

What you need to know about the bidi algorithm and inline markup:

<http://www.w3.org/International/articles/inline-bidi-markup/>

Unicode Bidi Algorithm:

<http://unicode.org/reports/tr9/>

BiDi HowTo:

<http://doctype.googlecode.com/svn/trunk/bidihowto/index.html>

Public talk by Aharon on BiDi on the web:

<https://video.google.com/a/?pli=1#/Play/contentId=f041561143265ad5>

Bidi editing in Mozilla:

<https://wiki.mozilla.org/User:Uri/Bidi_editing>

Guidelines of a logical user interface for editing bidirectional text by IBM

<http://www-01.ibm.com/software/globalization/topics/bidiui/index.jsp>

## RTL Release Note

Milestone 9 (r72805):

1.  Solved the #1 issue reported in Middle East and North Africa. Now,
            RTL page with content overflow could be viewed completely with
            horizontal scrollbar instead of page being truncated (r72852).
2.  Unicode bidi control characters are preserved in text input area.
            So, Hebrew, Arabic, and Persian speakers can type or cut/copy/paste
            the text having unicode bidi control characters correctly, and they
            can start enjoy twitters (r71566).

Milestone 10 (r76408):

1.  TAB inside &lt;pre&gt; overflow correctly in RTL context (r72847).
2.  up arrow works correctly with RTL text with word wrapping (r72861).
3.  clicking to position cursor at the end of a line of Arabic text
            position the cursor correctly (r73548).
4.  Default horizontal position of position:fixed block is right in RTL
            context.
5.  move to line boundary visually works correctly in RTL context
            (r75018).
6.  Caret goes to the right direction when pressing an arrow key after
            selection is made (r76312).

Milestone 11 (r80534)

1.  Support dir="auto" (r79024).
2.  ISO-8859-8 Hebrew text displayed correctly (r78491).
3.  Text runs with different directionality inside an embedding inline
            box displayed correctly in RTL context (r77267).
4.  Background image positioned correctly in RTL context (r78396).
5.  &lt;option&gt; supports the dir attribute and be displayed
            accordingly both in the drop-down and after being chosen (r77654).
6.  extend selection by line boundary works correctly in RTL context
            (r78799).
