---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: printing
title: Printing
---

*   The printing is initiated by the yellow boxes; either by user
            intents or by script.
*   The actual printer spooling is done in the print worker thread.
*   The renderer thread is blocking the whole time, which could lead to
            deadlock where:
    *   The renderer sends a sync message to the browser UI thread
                (which implicitly requires a round trip to the I/O thread)
    *   The browser thread displays a PrintDlgEx() dialog box for
                instance
    *   The tab client area (e.g. the HTML display area) receives a
                invalidation; requiring it to regenerate the display.
    *   (or) A windowed plugin receives a WM_PAINT window message.
    *   The windowed plugin calls NPAPI_ExecuteJavascript()
    *   This calls synchronously the renderer.
    *   But the renderer is not processing messages anymore -&gt;
                **Deadlock**.

Because of this, there is work going on to not block the Renderer Thread by
duplicating the WebViewImpl/WebFrameImpl and use the "inert" duplicate frame for
the printing. This removes all the sync messages requirements.
