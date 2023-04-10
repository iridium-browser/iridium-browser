---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: mac-occlusion
title: Mac Window Occlusion API Use
---

On Mac, a NSWindow can check to see if it is visible to the user (that is, check
to see if it is not covered by other windows, or on another desktop). This
document discusses how this feature can be used to save power.

**==Visible and Hidden on RenderWidget and RenderWidgetHostView==**

There are two separate pieces of state that are currently conflated (they are
always kept in lock-step), which need to be decoupled for this feature.

**RenderWidget::SetHidden**

This is in the renderer process. It is controlled through calling
RenderWidgetHostImpl::WasHidden/WasShown, which sends a ViewMsg_WasHidden/Shown
IPC down to the renderer. This will make the renderer process stop producing
frames, and free up some resources. This state is entered when a tab is
backgrounded.

This is the state that is queried by the Javascript Page Visibility API. See the
example at
http://ie.microsoft.com/TEStdrive/Performance/PageVisibility/Default.html

Of note is that on Safari, unlike Chrome (for now), the Page Visibility API
indicates that the page is hidden when it is occluded. We should match this
behavior.

Also of note is that when a page has a frame subscriber (for tab capture), the
renderer must always be in the visible state, not the hidden state.

**RenderWidgetHostView::Show/Hide and
RenderWidgetHostViewBase::WasShown/WasHidden**

These functions control if the RenderWidgetHostViewCocoa (the NSView
corresponding to the RenderWidgetHostViewMac) is actually visible on-screen. It
is possible (and does happen) that there are multiple RenderWidgetHostViewCocoas
attached to the same NSWindow, and which one is visible is controlled by this
Show/Hide method.

It is these functions that call RenderWidgetHostImpl::WasHidden/Shown, which, in
turn, tells the renderer what is happening, keeping this state in lock-step with
the RenderWidget.

These functions are called by WebContentsImpl::WasShown/WasHidden.

**Relationship with Tab Capture**

When a WebContentsImpl starts capturing, the active capture will (sometimes,
hopefully) prevent WebContents::WasHidden from being called.

This is done by the WebContents::should_normally_be_visible() flag, which all
callers to WebContents::WasHidden need to check (very brittle).

This is also accomplished by having WebContentsImpl::DecrementCapturerCount
potentially call WasHidden when the last capturer goes away.

**==Problems With This Situation for Occlusion==**

When a Chrome window is occluded, we will want the RenderWidget in the renderer
process to be told that it is hidden (this will cause it to drop its resources
and to indicate this to Javascript), but we do not want the the NSView for the
RenderWidgetHostView to be hidden (because we want it to be immediately visible
when the window is un-covered).

This combination of state, where the RenderWidget is not visible, but the
RenderWidgetHostView is visible, is not currently supported.

**==Proposed Changes==**

We should divorce the ideas of the RenderWidgetHostView being visible from the
RenderWidget being visible. When doing tab capture, it is necessary that the
RenderWidget be visible, but the RenderWidgetHostView should be capable of being
hidden.

At present, the relevant interface between WebContentsImpl and
RenderWidgetHostView is through WebContentsImpl calling
RenderWidgetHostView::Show/Hide.

This interface can be changed in two ways.

**Proposal A (not favored):**

Split this interface into RenderWidgetHostView::CaptureWasStarted/Stopped and
RenderWidgetHostView::Show/**ShowOccluded**/Hide.

From there, it will be up to the various RenderWidgetHostViews implementations
to decide if they want to conflate these, and how it wants to separate them out.

**Proposal B (favored):**

Split this interface into RenderWidgetHostView::ShowWidget/HideWidget and
RenderWidgetHostView::Show/Hide.

In this version, WebContentsImpl takes care of the logic to determine, based on
the view state and the capture state, if the RenderWidget should be visible or
hidden. The RenderWidgetHostView, meanwhile, can add optimizations to
partially-deactivate the view if it knows that the RenderWidget has been hidden
(and therefore won't be updating its content often). It's not clear if a
ShowOccluded function is very useful here (occluded state can be inferred from
the RenderWidget being hidden but the view being visible), but the possibility
shouldn't be excluded.

**New Configurations**

For situations where tab capture is enabled, but the tab being captured is
backgrounded, the RenderWidget will be visible, but the RenderWidgetHostView
will not be.

For situations where the window is occluded, the RenderWidget will not be
visible, but the RenderWidgetHostView will be.
