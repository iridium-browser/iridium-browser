---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: confirm-to-quit-experiment
title: Confirm to Quit Experiment
---

## Overview

The browser is one of the most heavily multi-tasked and long-running programs on
a user's computer. For many users, the uptime of Chromium nearly matches that of
their computer. One aspect of the Chromium user experience that frustrates users
is accidentally quitting due to tapping Cmd+Q. This keyboard shortcut is near
other commonly-used accelerators like Cmd+Tab and Cmd+W. The goal of this
improvement is to find a non-intrusive way to prevent accidental quitting while
still allowing users to quit easily and quickly.

This touches aspects of issues
[147](http://code.google.com/p/chromium/issues/detail?id=147),
[27786](http://code.google.com/p/chromium/issues/detail?id=27786), and
[34215](http://code.google.com/p/chromium/issues/detail?id=34215).

## Proposed UI

The proposed experience, from [issue
27786](http://code.google.com/p/chromium/issues/detail?id=27786), is to display
a floating window when the user taps Cmd+Q that instructs them to continue
holding in order to quit. After a few seconds, the quit is confirmed and Chrome
proceeds to exit. If the user does not continue to hold Cmd+Q, the floating
window will remain on screen for a few seconds before fading out to give the
user instructions if they do actually intend to quit.

We have not yet decided if this UI is the right one, and there are some concerns
that presenting users with such non-standard UI will help them learn habits that
are not applicable to other applications. Results from the experiment will
determine whether we ship this feature.

### A Look at Other Browsers:

Safari asks if you want to quit and has a Remmber Me checkbox. Safari's solution
is adequate (easy to turn off if it gets in your way, but a good default safety
net), but it forces an interruption in the user experience. The proposed UI
allows a user to keep her hand on the same keys and just continue pressing to
confirm a quit.

Firefox asks whether you want to Exit, Cancel, or Save and Exit. It's asking the
wrong question at the wrong time: users shouldn't have to predict whether or not
they want to recreate a session when quitting. Chrome's session restore feature
makes this a non-issue, so the question is not asked. They also have a Remember
Me checkbox.

## Implementation

This experiment currently only exists on Mac. We hook into
-(NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication\*)sender
and execute this UI there. We spin a nested run loop manually in
NSEventTrackingRunLoopMode by waiting for a KeyUp event. If no events occur
within the set threshold duration, the quit is confirmed. Since
-applicationShouldTerminate: method is not normally called as part of Chromium's
unique shutdown process (a necessity of its multi-process architecture), it is
manually called from -(BOOL)tryToTerminateApplication:(NSApplication\*)app.

While prototyping, we discovered that holding down Cmd+Q and simply quitting
after a set duration results in a frustrating behavior: because the system sends
multiple KeyDown events while keeping the keys pressed, after Chromium quits,
the system will start sending Cmd+Q to the previous key application, which then
quits as well. This would obviously be extremely frustrating, so instead, we
mark the quit as confirmed/committed after the set duration, but do not finalize
the quit until the next KeyUp event. In order to get users to release the key
combination without introducing more text-based UI, we animate all of the open
windows' opacity to 0. This change in state convinces users that Chromium has
"quit" and when the KeyUp event is received, the process begins its normal
shutdown procedure (as it was blocked by this nested run loop) by returning
NSTerminateNow.

If the user does not hold down Cmd+Q long enough to meet the threshold duration,
we return NSTerminateCancel which in turn causes -tryToTerminateApplication: to
return NO. This aborts the quit process. The floating panel lingers on-screen
for a second before fading out, which gives users who are unfamiliar with this
procedure a chance to learn how to quit Chromium.

[<img alt="image"
src="/developers/design-documents/confirm-to-quit-experiment/Content.png"
height=256
width=320>](/developers/design-documents/confirm-to-quit-experiment/Content.png)
