# Threading

The Open Screen Library is **single-threaded**; all of its code is intended to be
run on a single sequence, with a few exceptions noted below.

A library client **must** invoke all library APIs on the same sequence that is
used to run tasks on the client's
[TaskRunner implementation](https://chromium.googlesource.com/openscreen/+/refs/heads/master/platform/api/task_runner.h).

## Exceptions

* The [trace logging](trace_logging.md) framework is thread-safe.
* The TaskRunner itself is thread-safe.
* The [POSIX platform implementation](https://chromium.googlesource.com/openscreen/+/refs/heads/master/platform/impl/)
  starts a network thread, and handles interactions between that thread and the
  TaskRunner internally.





