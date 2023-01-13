---
breadcrumbs:
- - /blink
  - Blink (Rendering Engine)
page_name: activedomobject
title: ActiveDOMObject
---

**DRAFT. NEEDS REVIEW**

The ActiveDOMObject is used to implement DOM objects that can involve
asynchronous operations such as loading data from network (e.g. XMLHttpRequest,
WebSocket) to

*   keep them alive while async op is active
*   hold back actions resulting from async op while the document is
            suspended

The ActiveDOMObject classes can override its hasPendingActivity() method to keep
the object alive while some async operation is in progress. As long as
hasPendingActivity() returns true, V8 prolongs its life so that it doesn't get
garbage collected even when it becomes unreachable. Note that there must be a V8
wrapper for the object in order to this to work.

While hasPendingActivity() returns true, the life of the parent Document object
is also prolonged (explanation TBA). You rarely need to override
contextDestroyed() for ActiveDOMObject subclasses.

ActiveDOMObjects are notified of detach of the parent Document object (or
shutdown of the parent WorkerThread) as the stop() method called on it.
Commonly, they start shutdown of the asynchronous operation in stop() if any.
This detach occurs regardless of hasPendingActivity().

suspend() is called when dialogs such as alert(), prompt(), etc. are going to be
shown or the WebInspector's pause is going to be active. resume() is called when
the dialog is closed or the WebInspector resumes script execution. Between
suspend() and resume(), it's recommended that operations on the ActiveDOMObjects
are suspended. Since resource loading is automatically suspended by Chrome's
resource dispatched code, you may not need to manually hold back async method
calls.

When implementing, it is critical that you also add ActiveDOMObject to the
interface flags in your IDL file, or GC will pay no attention to
hasPendingActivity()!

As of Nov 12 2013 by tyoshino@. Updated July 17, 2014 by kouhei@

Thanks haraken@, abarth@
