---
breadcrumbs:
- - /user-experience
  - User Experience
page_name: touch-access
title: 'Accessibility: Touch Access'
---

Now that there are Chromebooks with touch screens, it is important that we have
an interface for users with disabilities to be able to interact with a touch
screen. It's based on the same ideas used on [iOS
VoiceOver](http://axslab.com/articles/ios-voiceover-gestures-and-keyboard-commands.php)
and [Android
TalkBack](https://support.google.com/accessibility/android/answer/6006598?hl=en),
but with a few Chrome-specific differences.

Currently iOS and Android provide mobile touch accessibility by allowing users
to touch the screen to explore the content currently showing. The user can hear
spoken feedback about whatever is under their finger. They may then perform
certain gestures to send a click action to the last explored target. There are
other gestures to adjust settings such as volume or reading speed, to navigate
between items on the page, to enter passthrough mode (where touch events are
sent through directly as if accessibility was off), and to perform many other
actions.

Touch accessibility on ChromeOS essentially works like this:

Users may use a finger to explore the screen by touch, hearing spoken feedback
about the item under their finger. They may then perform certain gestures such
as tapping and swiping to send a click to the last explored target, entering
passthrough mode (where touch events are sent through directly rather than being
intercepted by the accessibility system), adjusting volume, changing tabs, or
scrolling.

The feature turns on when spoken feedback is enabled. Currently there's no way
to turn touch accessibility on or off independently; for now we're assuming that
any users who want spoken feedback on probably want this functionality too, but
we could make it a preference later.

The majority of the code exists in the [touch exploration
controller](https://code.google.com/p/chromium/codesearch#chromium/src/ui/chromeos/touch_exploration_controller.h).
When spoken feedback is enabled, an instance of this class is created and all
touch events inputted by the user are rewritten through RewriteEvent. Sometimes
these events are discarded, sometimes they are released with a delay, and
sometimes they change the user's current state (e.g. from single finger pressed
state to touch exploration state).

[TOC]

**## Navigating the Screen**

### **Touch Exploration**

*   When the user is in the touch exploration state, simulated mouse
            moves are dispatched whenever the finger moves. These mouse moves
            causes the screen reader
            ([ChromeVox](http://www.chromevox.com/index.html)) to read the name
            of the object the user is touching.
*   Touch exploration state is entered by holding a finger down for more
            than 300 ms, or by moving a finger on the screen for longer (grace
            period ends) or slower (leave the grace area at a low velocity) than
            it takes to perform a gesture.
*   Once in touch exploration mode, all movements are translated into
            synthesized mouse move events. This causes hover states and events
            to be triggered. (e.g. a menu that opens on a mouse hover would open
            with touch exploration)
    *   The last touch exploration location is stored and updated after
                every artificial mouse move.
*   Touch exploration mode is left when the touch exploration finger is
            lifted or when other fingers are added.

### **Clicking Touch Explored Items**

*   Currently, there are three different ways of clicking on the item
            that was most recently touched through touch exploration.

#### Single Tap

    *   If the user is in touch exploration mode, they can click on the
                last touch exploration location by lifting their finger and
                quickly placing it on the screen again within the double tap
                timeout (300 ms).

#### Split Tap

    *   If the user is in touch exploration mode, they can click without
                lifting their touch exploration finger by tapping anywhere else
                on the screen with a second finger.

    *   Details:
        *   After a successful split tap has been completed, the user
                    will be back in touch exploration.
        *   If either finger moves too far from its original location,
                    no click goes through.
        *   If the touch exploration finger is lifted first, a click
                    still goes through when the split tap finger (the second
                    finger) is lifted.

#### Double Tap

    *   If the user double-taps, the second tap is translated as a click
                to the location of the last successful touch exploration - that
                allows the user to explore anywhere on the screen, hear its
                description, then double-tap anywhere to activate it.

    *   Details:
        *   The double tap must be completed 300 ms after the touch
                    exploration is released or else it is considered to be a
                    single tap.
        *   In terms of timing, the second tap must also be completed
                    within 300 ms of the first tap.

### **Gestures**

*   Gestures can be used to send high-level accessibility commands.

> #### Swipe Gestures

    *   The user can perform swipe gestures in one of the four cardinal
                directions which are then interpreted and used to control the
                UI.
    *   A gesture will be registered if:
        *   the user places one or more fingers on the screen and moves
                    them all in one of the cardinal directions
        *   the distance moved is far enough (outside the "slop region")
                    to determine that the movement was deliberate
        *   all fingers are released after the gesture is performed
        *   the entire gesture is performed within the grace period
                    (300ms)
    *   If the grace period expires before all fingers are lifted:
        *   If only a single finger was placed down, then touch
                    exploration mode is entered.
        *   Otherwise, the system will wait for all fingers to be lifted
                    before processing any more touch events.
    *   Details:

        *   One finger swipes are used for navigation on the screen. For
                    example, a swipe right would correspond to the keyboard
                    shortcut **Shift+Search+Right** (**ChromeVox+Right**).
            *   The default for this is **up/down:** go to next/previous
                        group, and **left/right**: go to next/previous object
                        (for instance, bullet points), but this can be changed
                        in ChromeVox settings.
        *   Two finger swipes are mapped as:
            *   **up**: go to top
            *   **down**: read from here
            *   **left**: browser back
            *   **right**: browser forward
        *   Three finger swipes are mapped as:
            *   **up**: page down (scroll by a set amount)
                *   note that up is page down because with touch it's
                            like you're literally dragging the page up - which
                            lets you see down the page
            *   **down**: page up
            *   **left**/**right**: scrolls through the tabs
        *   Four finger swipes are mapped as:
            *   **up**: home page
            *   **down**: refresh
            *   **left**/**right**: decrease/increase brightness (for
                        low vision users)
        *   Note that all of these mappings are not final and will
                    probably be shifted to best help users. Ideally there will
                    eventually be a menu for users to pick their own mappings

> #### Side Slide Gestures

    *   Slide gestures performed on the edge of the screen can change
                settings continuously.
    *   Currently, sliding a finger along the right side of the will
                change the volume.
        *   Volume control along the edge of the screen is directly
                    proportional to where the user's finger is located on the
                    screen.
        *   The top right corner of the screen automatically sets the
                    volume to 100% and the bottom right corner of the screen
                    automatically sets the volume to 0% once the user has moved
                    past slop.
    *   This could be applied to other settings in the future
                (brightness, granularity, navigation of headings).

    *   Details:
        *   If the user places a finger on the edge of the screen and
                    moves their finger past slop, a slide gesture is performed.
        *   The user can then slide one finger along an edge of the
                    screen and continuously control a setting.
        *   Once the user enters this state, the boundaries that define
                    an edge expand so that the user can now adjust the setting
                    within a slightly bigger width along the side of the screen.
        *   If the user exits this area without lifting their finger,
                    they will not be able to perform any actions, however if
                    they keep their finger down and return to the "hot edge,"
                    then they can still adjust the setting.
        *   In order to perform other touch accessibility movements, the
                    user must lift their finger.

### **Passthrough**

*   Passing finger events as if accessibility is off is useful for
            smooth scrolling, drag and drop, and pinching. There are currently
            two implementations for passthrough.
*   Passing finger events through is also useful for anything which
            implements custom gestures, as mentioned in the "Why A New Set Of
            Features" section.
*   An earcon will always sound when the user enters passthrough mode,
            allowing the user to know that passthrough has begun.

#### Relative Passthrough

    *   If the user double taps and holds for the passthrough time-out,
                all following events from that finger is passed through until
                the finger is released.
    *   This can be useful if a certain element requires a gesture to
                operate, allowing the user to touch explore to that element,
                then perform the gesture without needing to precisely locate the
                element again.
    *   Details

        *   These events are passed through with an offset such that the
                    first touch is offset to be at the location of the last
                    touch exploration location, and every following event is
                    offset by the same amount.
        *   If any other fingers are added or removed, they are ignored.
        *   Once the passthrough finger is released, passthrough stops
                    and the user is reset to no fingers down state.

#### Corner Passthrough

    *   When a user holds a finger in a bottom corner of the screen, any
                other fingers placed on the screen are passed through.

    *   Details:
        *   The user needs to place their finger on the corner of the
                    screen for 700 ms (passthrough time-out) before passthrough
                    will begin.
        *   An earcon will play to indicate that passthrough has been
                    activated.
        *   Once activated, all the events from any subsequent fingers
                    placed on the screen are passed through as long as the
                    finger in the corner stays in the corner and is not
                    released.
        *   Once the finger in the corner has been released, all the
                    following events are ignored until all the fingers have been
                    released.

**Other Details:**

*   If the user taps and releases their finger, after 300 ms from the
            initial touch, a single mouse move is fired. This can be used to
            explore an item on the screen without entering touch exploration
            state.
*   Earcons (short sound notifications) are fired whenever the user
            comes close to moving off the screen or moves onto the screen from
            the edge. Many touch devices have a smooth bezel around the edge of
            the screen, making it practically impossible to tell where the touch
            screen starts by only touch. This earcon helps the user be more
            aware of whether or not their finger is on the touch part of the
            screen.

## Why A New Set of Features?

To determine what would be the best direction for new ChromeOS touch
accessibility features, there were a few things we needed to consider. First of
all, people who were already accustomed to using touch accessibility on mobile
had opinions on what functions they believed to be the most useful. Second of
all, ChromeOS accessibility was new and only had a few features implemented when
we arrived. We surveyed Google employees that used/were interested in
accessibility features on mobile, and we asked for their preferences and ideas
for the design for ChromeOS.

What we discovered:

*   Most commonly used gestures: read next/previous item, read from
            here, jump to top, scroll by page, go back
*   When scrolling, users tended to prefer to go by a page then to
            scroll smoothly
*   Being able to use multiple fingers for gestures was considered to be
            much simpler and more intuitive than more complicated single finger
            gestures - especially on a bigger laptop screen
*   Passthrough mode was rarely used and users were frustrated by
            Android's emphasis on it, but it was felt to be necessary -
            especially when encountering “web apps in the wild” on a Chromebook
            (apps could be developed needing fancy swipes or multi finger
            touches, and there is no "internet police" to stop them)

note: the sample size was fairly small, and we hope that we'll be able to
collect more user feedback when the first version is released.

Therefore, we have decided to implement the most useful features borrowed from
Android and iOS as well as features completely unique to ChromeOS that took
advantage of its own capabilities. For example, corner passthrough and side
slide gestures wouldn't have worked as well on a tablet or mobile device, but
take advantage of the Chromebook's bigger screen to give the user better
accessibility.

## Implementation**: State Machine**

This is a graph of the possible states the user can be in, and all of the user
input and timer time-outs that causes the [touch exploration
controller](https://code.google.com/p/chromium/codesearch#chromium/src/ui/chromeos/touch_exploration_controller.h)
to change the user's state. Occasionally touch presses and releases are
dispatched to simulate clicks, and these dispatched events are also shown in
this chart.

[<img alt="image"
src="/user-experience/touch-access/chromeos_touch_accessibility_user_flow.png">](/user-experience/touch-access/chromeos_touch_accessibility_user_flow.png)

This chart was generated using https://www.gliffy.com

## Future of Chrome Touch Accessibility

Here are some ideas that didn't make it into version one, but would be awesome
to implement soon:

*   Menu for accessibility settings (detail of navigation by
            word/line/paragraph, rate of speech, pitch of speech, etc.)
*   Drag and drop gesture to move an object or copy/paste while being
            able to navigate the screen between locations to find the drop point
*   Pinch/zoom - for low vision users (currently possible through corner
            passthrough, but perhaps a three finger hold could zoom in)
*   Enabling ChromeVox from anywhere - shortcut gesture recognized with
            ChromeVox turned off
*   Custom gestures - user chooses gesture mappings
