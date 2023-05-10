---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/how-tos
  - How-Tos
- - /developers/how-tos/trace-event-profiling-tool
  - The Trace Event Profiling Tool (about:tracing)
- - /developers/how-tos/trace-event-profiling-tool/memory
  - OBSOLETE. Memory profiling in chrome://tracing
page_name: startup-tracing-with-memory-profiling
title: Startup tracing with memory profiling
---

**Related wiki pages**

*   See [this
            page](/developers/how-tos/trace-event-profiling-tool/recording-tracing-runs)
            for general information on startup tracing.
*   See [this
            page](https://chromium.googlesource.com/chromium/src/+/HEAD/docs/memory-infra/README.md)
            for general information about memory tracing.

There are two ways to perform memory tracing at startup:

**Simple way with default configuration (one memory dump every 250 ms., one
detailed memory dump every 2 s.)**

$chrome --no-sandbox \\

--trace-startup=-\*,disabled-by-default-memory-infra \\

--trace-startup-file=/tmp/trace.json \\

--trace-startup-duration=7

Then just load /tmp/trace.json in chrome://tracing

**Advanced, memory tracing configuration**

If you need more advanced configuration (e.g., higher granularity dumps, no need
for detailed memory dumps) specify a custom trace config file, as follows:

$ cat &gt; /tmp/trace.config

{

"startup_duration": 4,

"result_file": "/tmp/trace.json",

"trace_config": {

"included_categories": \["disabled-by-default-memory-infra"\],

"excluded_categories": \["\*"\],

"memory_dump_config": {

"triggers": \[

{"mode":"light", "periodic_interval_ms": 50},

{"mode":"detailed", "periodic_interval_ms": 1000}

\]

}

}

}

$chrome --no-sandbox --trace-config-file=/tmp/trace.config
