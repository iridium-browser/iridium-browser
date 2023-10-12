---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/how-tos
  - How-Tos
- - /developers/how-tos/trace-event-profiling-tool
  - The Trace Event Profiling Tool (about:tracing)
page_name: tracing-event-instrumentation
title: Adding Traces to Chromium/WebKit/Javascript
---

See [Trace Event Profiling Tool
(about:tracing)](/developers/how-tos/trace-event-profiling-tool) for more
information on about:tracing

Sometimes, about:tracing wont show you all the detail you want. Don't worry, you
can add more instrumentation yourself.

****Trace macros are very low overhead. When tracing is not turned on, trace
macros cost at most a few dozen clocks. When running, trace macros cost a few
thousand clocks at most. Arguments to the trace macro are evaluated only when
tracing is on --- if tracing is off, the value of the arguments dont get
computed.****

**[trace_event_common.h](https://code.google.com/p/chromium/codesearch#chromium/src/base/trace_event/common/trace_event_common.h&q=f:trace_event_common.h&sq=package:chromium&type=cs&l=1)
defines the below behavior.**

## Function Tracing

The basic trace macros are TRACE_EVENTx, in 0, 1, and 2 argument flavors. These
time the enclosing scope. In Chrome code, just do this:

> #include "base/trace_event/trace_event.h"

> // This traces the start and end of the function automatically:

> void doSomethingCostly() {

> TRACE_EVENT0("MY_SUBSYSTEM", "doSomethingCostly");

> ...

> }

In WebKit/Source/WebCore/, you can trace this way:

> #include "platform/TraceEvent.h"

> void doSomethingCostly() {

> TRACE_EVENT0("MY_SUBSYSTEM", "doSomethingCostly");

> ...

> }

****The "MY_SUBSYSTEM" argument is a logical category for the trace event. This
category can be used by the UI to hide certain kinds events, or even skip them
from being collected. A common practice is to use the chromium module in which
the code lies, so for example, code in content/renderer gets the "renderer"
category.****

**TRACE_EVENT1 and 2 allow you to capture 1 and 2 pieces of data along with the
event. So, for example, this records the paint dimensions number along with the
paint:**

**void RenderWidget::paint(const Rect& r) {**

**TRACE_EVENT2("renderer", "RenderWidget::paint", "width", r.width(), "height",
r.height());**

## Javascript Tracing

You

**********can instrument javascript code as well:**********

function Foo() {

console.time(“Foo”);
var now = Date.now();

while (Date.now() &lt; now + 1000);

console.timeEnd("Foo");

}

**Note that currently, console.time/timeEnd spam the inspector console when it
is open --- when inspector is closed, there is little or no performance penalty
to the API.**

## Counters
## To track a value as it evolves over time, use TRACE_COUNTERx or TRACE_COUNTER_IDx. For example:

> ## #include "base/trace_event/trace_event.h"

> ## shmAlloc(...) {
> ## ...
> ## TRACE_COUNTER2("shm", "BytesAllocated", "allocated", g_shmBytesAllocated,
> "remaining", g_shmBytesFree);
> ## }

> ## shmFree(...) {
> ## ...
> ## TRACE_COUNTER2("shm", "BytesAllocated", "allocated", g_shmBytesAllocated,
> "remaining", g_shmBytesFree);
> ## }

## Sometimes a counter is specific to a class instance. For that, use the _ID variant:

> ## Document::didModifySubtree(...) {
> ## TRACE_COUNTER2("dom", "numDOMNodes", calcNumNodes());

> ## }

## This will create a different counter for each ID.

## Asynchronous Events

Sometimes, you will have an asynchronous event that you want to track. For this,
use TRACE_EVENT_ASYNC_BEGINx and TRACE_EVENT_ASYNC_ENDx. For example:

> #include "base/trace_event/trace_event.h"

> AsyncFileOperation::AsyncFileOperation(...) {
> TRACE_EVENT_ASYNC_BEGIN("io", "AsyncFileOperation", this);

> }

> void AsyncFileOperation::OnReady() {

> TRACE_EVENT_ASYNC_END("io', "AsyncFileOperation", this);
> ...
> }

Async operations can start and finish on different threads, or even different
proceses. Association is done by the concatenation of the name string and the
third "id" argument. The nearest match in time establishes the relationship.

## Resources

*   [trace_event_common.h](https://code.google.com/p/chromium/codesearch#chromium/src/base/trace_event/common/trace_event_common.h&q=f:trace_event_common.h&sq=package:chromium&type=cs&l=1),
            the basic readme for trace_event instrumentation
*   [Tracing Ecosystem
            Explainer](https://docs.google.com/document/d/1QADiFe0ss7Ydq-LUNOPpIf6z4KXGuWs_ygxiJxoMZKo/edit?pli=1#heading=h.dytg6ymhhy0b)
*   [Trace Event
            Format](https://docs.google.com/document/d/1CvAClvFfyA5R-PhYUmn5OOQtYMH4h6I0nSsKchNAySU/edit?pli=1)
