# Contextual Search Component

The Contextual Search component implements some platform agnostic services that
mediate between the main CS implementation in the Browser and some other parts
of the system.  These include the JS API service to allow communication from
the Overlay to CS, and user-interaction aggregation.

## JS API Service

The JS API Service allows JavaScript in the Overlay Panel to call into
Contextual Search.  This is done by providing an entry point at
chrome.contextualSearch when the Contextual Search Overlay Panel is active.

Enabling this API is somewhat complicated in order to make sure that the API
only exists for a Renderer created by CS for the Overlay:

1. Whenever a Renderer is created, an OverlayJsRenderFrameObserver is created.
   This class is at the heart of the connection between the Renderer, CS, JS
   and the JavaScript API.
2. An OverlayJsRenderFrameObserver is created for every renderer to check if
   it's a CS Overlay Panel.  When the renderer starts up it asks Contextual
   Search if the current URL belongs to its overlay panel, and if it does then
   the JS API is enabled for that renderer to allow it to accept JS messages
   from the page.
3. When the OverlayJsRenderFrameObserver gets the response in #2 it creates a
   ContextualSearchWrapper object that allows injecting JavaScript into the
   WebFrame.
4. When some JS in a page wants to call into Chrome it simply accesses methods
   in the chrome.contextualSearch API.  If the page is being presented in the
   Overlay Panel for CS then that object will exist with native method
   implementations.  The renderer forwards the API request to the browser for
   processing.

The above interaction is used by the Translate onebox to allow showing a
translation in the caption of the Contextual Search Bar, and to provide control
of the position of the Overlay.

## User Interaction Aggregation

The CtrAggregator and WeeklyActivityStorage classes are used aggregate user
actions on a weekly basis for use in machine learning for improved triggering.

