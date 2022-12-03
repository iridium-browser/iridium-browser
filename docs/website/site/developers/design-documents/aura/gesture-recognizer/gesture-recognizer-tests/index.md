---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
- - /developers/design-documents/aura
  - Aura
- - /developers/design-documents/aura/gesture-recognizer
  - Gesture Recognizer
page_name: gesture-recognizer-tests
title: Gesture Recognizer Tests
---

## Notation:

*   Green text
    *   The name of a test which focuses on the preceding scenario.
*   None
    *   No test exists where the focus is the preceding scenario.
    *   There may be other tests which exercise this scenario, but
                ideally there would be a test in which this is the primary
                focus.

## Gesture Events:

*   Tap Down
    *   Press
        *   GestureSequenceTest.TapDown
*   Tap
    *   Press, Wait, Release
        *   GestureSequenceTest.Tap
*   Double Tap
    *   Press, Wait, Release, Press, Wait, Release
        *   GestureSequenceTest.DoubleTap
*   Long Press
    *   Press, Wait
        *   GestureSequenceTest.LongPress
*   Three Fingered Swipe
    *   Press 1, Press 2, Press 3, Swipe
        *   GestureSequenceTest.ThreeFingeredSwipeLeft
        *   GestureSequenceTest.ThreeFingeredSwipeRight
        *   GestureSequenceTest.ThreeFingeredSwipeUp
        *   GestureSequenceTest.ThreeFingeredSwipeDown
    *   Press 1, Move 1, Press 2, Move 2, Press 3, Swipe
        *   None
*   Scroll
    *   Begin, Update
        *   Press, Move
            *   GestureSequenceTest.NonRailScroll
            *   GestureSequenceTest.HorizontalRailScroll
            *   GestureSequenceTest.VerticalRailScroll
        *   Press 1, Press 2, Move 1, Move 2
            *   GestureSequenceTest.PinchScroll
    *   End
        *   Press, Move, Release
            *   GestureSequenceTest.NonRailFling
            *   GestureSequenceTest.HorizontalRailFling
            *   GestureSequenceTest.VerticalRailFling
*   Pinch
    *   Begin, Update:
        *   Press 1, Press 2
            *   GestureSequenceTest.Pinch
            *   GestureSequenceTest.PinchFromTap
        *   Press 1, Move 1, Press 2
            *   GestureSequenceTest.PinchFromScroll
        *   Press 1, Press 2, Release 1, Press 1
            *   GestureSequenceTest.PinchFromScrollFromPinch
    *   End

## Aborting Gesture Events:

*   Tap Down
    *   Can’t be aborted
*   Tap
    *   Press, Cancel :
        *   GestureSequenceTest.TapCancel
    *   Press, Move, Release:
        *   None
    *   Press, PreventDefault’ed release
        *   GestureRecognizerTest.NoTapWithPreventDefaultedRelease
*   Double Tap
    *   Press, Release, Press different place, Release
        *   GestureSequenceTest.OnlyDoubleTapIfClose
    *   Press, Release, Press, Move, Release
        *   None
*   Long Press
    *   Press, Release, Wait :
        *   None
    *   Press, Move, Wait :
        *   GestureSequenceTest.LongPressCancelledByScroll
    *   Press 1, Press 2, Wait
        *   GestureSequenceTest.LongPressCancelledByPinch
*   Three Fingered Swipe
    *   Press 1, Press 2, Press 3, Swipe Diagonally
        *   GestureSequenceTest.ThreeFingeredSwipeDiagonal
    *   Press 1, Press 2, Press 3, Swipe in opposite directions
        *   GestureSequenceTest.ThreeFingeredSwipeSpread
*   Scroll
    *   Begin, Update
        *   Can’t be aborted
    *   End
        *   Can’t be aborted
*   Pinch
    *   Begin, Update
        *   Can’t be aborted
    *   End
        *   Can’t be aborted

## Miscellaneous:

*   Tap Followed by Scroll:
    *   GestureSequenceTest.TapFollowedByScroll
*   Release without Press:
    *   GestureSequenceTest.IgnoreDisconnectedEvents
*   Unprocessed gesture events generate synthetic mouse events:
    *   GestureRecognizerTest.GestureTapSyntheticMouse
*   Queueing of events:
    *   GestureRecognizerTest.AsynchronousGestureRecognition

## Targeting:

1.  **If a touch is already locked to a target, keep it.**
    *   **GestureRecognizerTest.GestureEventTouchLockSelectsCorrectWindow**
2.  **If a window is capturing touches, target that window.**
    *   **WindowTest.TouchCaptureTests**
    *   **WindowTest.TouchCaptureCancelsOtherTouches**
    *   **WindowTest.TouchCaptureDoesntCancelCapturedTouches**
3.  **If it's outside the root window, target the root window.**
    *   **SystemGestureEventFilterTest.TapOutsideRootWindow**
    *   **GestureRecognizerTest.GestureEventOutsideRootWindowTap**
4.  **If it's near another touch, use the other touch's target.**
    *   **GestureRecognizerTest.GestureEventTouchLockSelectsCorrectWindow**
5.  **Otherwise, use whichever window the touch is above.**
    *   **None**
