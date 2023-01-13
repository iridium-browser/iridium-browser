# Trace Logging Library Usage

The idea behind Trace Logging is that we can choose to trace a function and
then, with minimal additional code, determine how long function execution ran
for, whether the function was successful, and connect all of this information to
any existing informational logs. The below provides information about how this
can be achieved with OSP's TraceLogging Infrastructure.

## Compilation

By default, TraceLogging is enabled as part of the build.
To disable TraceLogging, include flag `--args="enable_trace_logging=false"`
when calling `gn gen` as part of building this library.

## Imports

To use TraceLogging, import the following header file:
  * *util/trace_logging.h*
This file will import all other Trace Logging dependencies.

## Trace IDs

When a Trace Log is created, a new unsigned integer is associated with this
specific trace, which will going forward be referred to as a Trace ID.
Associating a Trace ID has the following benefits:

* The same Trace ID can be associated with all Informational Logs generated
  during this traced method’s execution. This will allow for all informational
  logs to easily be associated with a method execution, so that if an unexpected
  failure occurs or edge case pops up we will have additional information useful
  in debugging this issue and can more easily determine the state of the system
  leading to this issue.

* If another Trace Log is created during the execution of this first traced
  method, we will have a hierarchy of Trace IDs. This will allow us to easily
  create an execution tree (through trivial scripting, preexisting tool, or
  database operations) to get a better view of how execution is proceeding.
  Together, the IDs associated with these calls form a Trace ID Hierarchy. The
  Hierarchy has the following format:
  * *Current Trace ID*: The ID of the function currently being traced.
  * *Parent Trace ID*: The ID of the function which was being traced when this
    Trace was initiated.
  * *Root Trace ID*: The ID of the first traced function under which this the
    current Trace was called.

As an example:
``` cpp
    public void DoThings() {
      TRACE_SCOPED(category, "outside");
      {
        TRACE_SCOPED(category, "middle");
        {
          TRACE_SCOPED(category, "inside");
        }
      }
    }
```

This could result in the following Trace ID Information:

| NAME    | ROOT ID  | PARENT ID  | TRACE ID  | RESULT  |
|---------|----------|------------|-----------|---------|
| outside | 100      | 0          | 100       | success |
| middle  | 100      | 100        | 101       | success |
| inside  | 100      | 101        | 102       | success |

Note that, prior to any trace calls, the Trace ID is considered to be 0x0 by
convention.

## Trace Results

The "result" of a trace is meant to indicate whether the traced function
completed successfully or not. Results are handled differently for synchronous
and asynchronous traces.

For scoped traces, the trace is by default assumed to be successful. If an error
state is desired, this should be set using `TRACE_SET_RESULT(result)` where
result is some Error::Code enum value.

For asynchronous calls, the result must be set as part of the `TRACE_ASYNC_END`
call. As with scoped traces, the result must be some Error::Code enum value.

## Tracing Functions
All of the below functions rely on the Platform Layer's IsTraceLoggingEnabled()
function. When logging is disabled, either for the specific category of trace
logging which the Macro specifies or for TraceCategory::Any in all other cases,
the below functions will be treated as a NoOp.

### Synchronous Tracing
  ```c++
  TRACE_SCOPED(category, name)
  TRACE_SCOPED(category, name, traceId, parentId, rootId)
  TRACE_SCOPED(category, name, traceIdHierarchy)
  ```
  If logging is enabled for the provided |category|, trace the current scope. The scope
  should encompass the operation described by |name|. The latter two uses of this macro are
  for manually providing the trace ID hierarchy; the first auto-generates a new trace ID for
  this scope and sets its parent trace ID to that of the encompassing scope (if any).

  ```c++
  TRACE_DEFAULT_SCOPED(category)
  TRACE_DEFAULT_SCOPED(category, traceId, parentId, rootId)
  TRACE_DEFAULT_SCOPED(category, traceIdHierarchy)
  ```
  Same as TRACE_SCOPED(), but use the current function/method signature as the operation
  name.

### Asynchronous Tracing
  `TRACE_ASYNC_START(category, name)`
    If logging is enabled for the provided category, this function will initiate
    a new asynchronous function trace with name as provided. It will not end
    until TRACE_ASYNC_END is called with the same Trace ID generated for this
    async trace. When this call is used, the Trace ID Hierarchy will be
    determined automatically and the caller does not need to worry about it and,
    as such, **this call should be used in the majority of asynchronous tracing
    cases**.

  `TRACE_ASYNC_START(category, name, traceId, parentId, rootId)`
    If logging is enabled for the provided category, this function will initiate
    a new asynchronous function trace with name and full Trac ID Hierarchy as
    provided. It will not end until TRACE_ASYNC_END is called with the same
    Trace ID provided for this async trace. Each of trace ID, parent ID, and
    root ID is optional, so providing only a subset of these values is also
    valid if the caller only desires to set specific ones.

  `TRACE_ASYNC_START(category, name, traceIdHierarchy)`
    This call is intended for use in conjunction with the TRACE_HIERARCHY macro
    (as described below). this function will initiate a new asynchronous
    function trace with name and full Trace ID Hierarchy as provided. It will
    not end until TRACE_ASYNC_END is called with the same Trace ID provided for
    this async trace.

  `TRACE_ASYNC_END(category, id, result)`
    This call will end a trace initiated by TRACE_ASYNC_START (as described
    above) if logging is enabled for the associated category. The id is expected
    to match that used by an TRACE_ASYNC_START call, and result is the same as
    TRACE_SET_RESULT's argument.

### Other Tracing Macros
  `TRACE_CURRENT_ID`
    This macro returns the current Trace ID at this point in time.

  `TRACE_ROOT_ID`
    This macro returns the root Trace ID at this point in time.

  `TRACE_HIERARCHY`
    This macro returns an instance of struct Trace ID Hierarchy containing the
    current Trace ID Hierarchy. This is intended to be used with
    `TRACE_SET_HIERARCHY` (described below) so that Trace ID Hierarchy can be
    maintained when crossing thread or machine boundaries.

  `TRACE_SET_HIERARCHY(ids)`
    This macro sets the current Trace Id Hierarchy without initiating a scoped
    trace. The set ids will cease to apply when the current scope ends. This is
    intended to be used with `TRACE_HIERARCHY` (described above) so that Trace
    ID Hierarchy can be maintained when crossing thread or machine boundaries.

  `TRACE_SET_RESULT(result)`
    Sets the current scoped trace's result to be the same as the Error::Code
    argument provided.

### Example Code
Synchronous Tracing:
``` cpp
    public void DoThings() {
      TRACE_SCOPED(category, "DoThings");

      // Do Things.
      // A trace log is generated when the scope containing the above call ends.

      TRACE_SET_RESULT(Error::Code::kNone);
    }
```

Asynchronous tracing with known Trace ID (recommended):
This approach allows for asynchronous tracing when the function being traced can
be associated with a known Trace ID. For instance, a packet ID, a request ID, or
another ID which will live for the duration of the trace but will not need to be
passed around separately.
``` cpp
    public void DoThingsStart() {
      TRACE_ASYNC_START(category, "DoThings", kKnownId);
    }

    public void DoThingsEnd() {
      TRACE_ASYNC_END(category, kKnownId, Error::Code::kNone);
    }
```

Asynchronous tracing with unknown Trace ID (not recommended):
This approach allows for asynchronous tracing even when no existing ID can be
associated with the trace.
``` cpp
    public TraceId DoThingsStart() {
      TRACE_ASYNC_START(category, "DoThings");
      return TRACE_CURRENT_ID;
    }

    public void DoThingsEnd(TraceId trace_id) {
      TRACE_ASYNC_END(category, trace_id, Error::Code::kNone);
    }
```

## File Division
The code for Trace Logging is divided up as follows:
  * *util/trace_logging.h*: the macros Open Screen library code is expected to
      use to log trace events.
  * *platform/base/trace_logging_types.h*: the types/enums used in the platform
      API as well as internal library code.
  * *util/trace_logging/*: the internal infrastructure backing the macros in
      *trace_logging.h*, and connecting it to the platform API.
  * *platform/api/trace_logging_platform.h*: the platform implementation that is
      used as the trace logging destination while tracing is active.
This information is intended to be only eplanatory for embedders - only the one
file mentioned above in Imports must be imported.

## Embedder-Specific Tracing Implementations

For an embedder to create a custom TraceLogging implementation:

1. *Create a TraceLoggingPlatform*
  In platform/api/trace_logging_platform.h, the interface TraceLoggingPlatform
  is defined. An embedder must define a class implementing this interface. The
  methods should be as performance-optimal as possible, since they might be
  called frequently (especially `IsLoggingEnabled(TraceCategory)`) and are often
  in the critical execution path of the library's code.

2. *Call `openscreen::StartTracing()` and `StopTracing()`*
  These activate/deactivate tracing by providing the TraceLoggingPlatform
  instance and later clearing references to it.

**The default implementation of this layer can be seen in
platform/impl/trace_logging_platform.cc.**
