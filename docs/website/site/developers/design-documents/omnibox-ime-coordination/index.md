---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: omnibox-ime-coordination
title: Omnibox/IME Coordination
---

## Implementation Status Update

*   Implemented in M17 on Win
*   Implemented in M18 on Chrome OS
*   Implemented in M29 on Win (Views implementation, IMM32)

## Objective

We aim to fix the poor interaction between Chrome’s omnibox and the IME (Input
Method Editor). The main problem is that IME windows overlap with the omnibox
autocomplete results, as pictured below.

[<img alt="image"
src="/developers/design-documents/omnibox-ime-coordination/imeomni.png">](/developers/design-documents/omnibox-ime-coordination/imeomni.png)

This looks unpolished and may be confusing and frustrating to the user.

## Proposed Solution and Plan

Our proposal is to close the omnibox autocomplete popup window whenever the IME
candidate window is open.
We can easily detect the candidate window on Windows and Chrome OS, so we will
implement this design on those platforms. It is likely not possible on Mac and
Linux.
After this design is implemented, we can consider a more sophisticated one. We
would most likely experiment on Chrome OS, as we have the most control on that
platform. If we implement and enjoy very good reception, we can consider further
effort to implement on other platforms.

## Background

### The Omnibox

Based on its current input, the omnibox pops up autocomplete results,
suggestions that may come from browsing history, bookmarks, or Instant Suggest.
There is also inline autocomplete, which is an autocomplete result inserted into
the text edit area itself. Inline autocomplete seems to be already disabled
during IME composition as it would otherwise interfere with composition (see
<http://crbug.com/64983> and <http://crbug.com/75651>).

### The IME

An IME is used to input text in a more complex manner than the typical direct
mapping of keystrokes to characters. For instance, Japanese has too many
characters to fit on a keyboard. Using a Japanese IME, the user inputs a reading
(“fujisan”) then converts them to the desired characters (“富士山”). The IME
presents conversion candidates in a window and in general can present arbitrary
UI elements that the application process has little control over.
Text that has been generated through the IME but not yet committed is called
composition text (or pre-edit text).

### The Problem

In Chrome as of M15, the IME’s UI elements are drawn over the omnibox
autocomplete results, resulting in a poor user experience. Specific issues
include the following:

*   The autocomplete results can’t be seen, but are rendered as if the
            user is expected to make use of them.
*   The autocomplete results appear to be interactive but are not. IME
            handles keystrokes, so up/down arrows navigate through IME windows
            instead of autocomplete results.
*   The user cannot easily uncover a partially obscured autocomplete
            result that looks interesting. The correct method, pressing Enter,
            commits and dismisses the IME, but in practice this is not very
            intuitive, and requires the user to delete and retype the query if
            the suggestion is ultimately rejected. Instead of Enter, the user
            may try to use the mouse (but clicking immediately activates the
            result instead of making it visible) or hit Escape (this closes that
            particular IME window but also alters the composition text, so the
            results change).
*   Navigating through the IME candidates alters the autocomplete
            results, since the composition text changes. The user notices these
            results are updating as if they are useful but cannot use them. This
            feels unpolished.

IME and Omnibox Instant interaction may also be problematic. Instant is
triggered immediately whenever the composition text is updated, which may feel
noisy and sluggish to the user.

## Requirements and Scale

Ideally, we would fix the current poor interaction on all platforms, but our
solution is likely implementable only on Windows and Chrome OS.
We would also prefer the solution to work for all IMEs and all languages.
However, universal coverage is not an absolute requirement, and we can keep a
solution that works for a large enough group of users and doesn’t introduce new
problems to others.
If we attempt a more sophisticated design, we would probably implement on Chrome
OS first, with an eye to implementing on other platforms if it has good
reception. Ideally, we wouldn’t have different behavior on each platform. But it
may be the case that only Chrome OS can be saved, due to the control we have on
that platform.

## Alternatives Considered

We considered several design alternatives. These included the following ideas:

1.  Place the autocomplete popup below the IME window - This solves the
            overlap but can be unsightly as the IME window can be vertically
            large. Also, two focal points on screen (autocomplete results and
            IME candidates) is generally bad UX-wise. There are also questions
            like whether you switch between the two UI elements.
2.  Place IME window inside the autocomplete popup - This is about the
            same as 1.
3.  Remove IME panels - This is invasive and may render some IMEs
            unusable.
4.  Integrate IME suggestions/candidates with autocomplete results -
            This is hard implementation-wise. Some IMEs might not expose their
            suggestions/candidates in a usable way. We'd have to handle the IME
            UI ourselves.
5.  Request IME use compact/horizontal mode - This is more of a wish as
            most IMEs don't have such a mode.
6.  Keep autocomplete results always visible but greyed out during IME
            composition - Some people find this ugly. Also, same issues as 1.
7.  Use inline autocomplete during IME composition but not autocomplete
            popup - This solves overlap but loses multiple autocomplete results.

However, these have both serious implementation difficulties and UX concerns.
Implementation difficulties arise from the limited control over the IME which
varies by platform. Furthermore, altering the IME’s native UX risks confusing
and annoying the user.
Thus the design and implementation costs are high. If we were to choose one of
these solutions, we risk sinking much effort into a solution that might not be
usable. Perhaps it would work only for some IMEs or break on future versions of
IMEs. Or, it may be implemented and ultimately be poorly received by users.
Another idea is to always close the autocomplete popup during IME composition.
However, this is too aggressive. It would prevent users from making use of
autocomplete during the input phase before character conversion (for Japanese,
when inputting kana before converting to kanji/kana). It would also make little
sense for IMEs which usually do not have a composition window open, such as
Korean IMEs.
For reference, Firefox and IE9 seem to always close their suggestions popup
during IME composition, but we haven’t looked into this extensively.

## Detailed Design

On Windows, we can detect the candidate window using composition attributes or
WM_IME_NOTIFY/IMN_OPENCANDIDATE and IMN_CLOSECANDIDATE messages. When the
candidate window is open, we suppress the autocomplete popup. The autocomplete
results should be fetched anyway, to be displayed immediately when the candidate
window closes.
This only detects the candidate window; some IMEs may display windows other than
the candidate window, such as Google Japanese Input, which displays a suggestion
window. But we are OK with allowing such popups, as they are not the common
case, and removing them might be overly aggressive anyway.
We can also choose whether to enable or disable Instant Search during
conversion.

## UMA Metrics

We believe our design is a better UX, but want some way to confirm this. At
least, we must confirm that it is it rare that an autocomplete result is used
while candidate window is open. Unfortunately, getting UMA metrics like the
number of times an autocomplete result was selected is impossible, as Chrome's
UI design is such that an item from the dropdown is always selected. We still
need ideas for measuring the impact of our change.

## Links

*   [Chromium bug
            2924](http://code.google.com/p/chromium/issues/detail?id=2924) -
            Don't let IME candidate window overwrite the autocomplete popup
*   [Chromium bug
            38450](http://code.google.com/p/chromium/issues/detail?id=38450) -
            Omnibox autocomplete behavior for Korean is less than optimal
            (related, but this design doc doesn’t affect this bug)
*   [Chromium bug
            245578](https://code.google.com/p/chromium/issues/detail?id=245578)
            - IME candidate window overlaps Omnibox suggest.
