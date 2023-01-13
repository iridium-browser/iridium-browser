---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/base-newsletters
  - //base Newsletters
page_name: 2022q3
title: //base Newsletter, 2022Q3 Edition
---

[TOC]

## What is this?

The Chromium
[//base](https://chromium.googlesource.com/chromium/src/+/9e001703a2/base/) code
contains foundational building blocks for the project. It evolves over time to
meet the project's changing needs. This periodic newsletter highlights some of
the changes Chromium developers should be aware of.

## New and shiny

### base::expected<typename T, typename E>

A new vocabulary type based on the
[C++23 proposal](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2021/p0323r11.html)
that allows functions to return either an expected value of type T on success or
an error value of type E on failure, e.g. instead of writing:

```
// Returns a base::Value::Dict on success, or absl::nullopt on failure. `error`
// will be populated with a diagnostic message on failure.
absl::optional<base::Value::Dict> ParseMyFormat(base::StringPiece input,
                                                std::string& error);
```

Simply write this instead:

```
// Returns the parsed base::Value::Dict on success, or a string with a
// diagnostic message on failure.
base::expected<base::Value::Dict, std::string> ParseMyFormat(
    base::StringPiece input);

```

Returning a success value is simple:

```
if (auto* dict = parsed_value.GetIfDict()) {
  return std::move(*dict);
}
```

While returning an error value required using the `base::unexpected` hint:

```
return base::unexpected(error_message);
```

Callers can use `has_value()` to check for success, and then use `value()`,
`operator*`, or `operator->` to access the success value. On error, callers can
use `error()` instead to access the error value.

One notable divergence from the proposed C++23 library feature is the lack of a
bool operator (explicit or otherwise).

*   Prefer `base::expected<T, E>` over `absl::optional<T>` when returning
    auxiliary information for failures.
*   Helper types like `base::FileErrorOr<T>` should be migrated to
    `base::expected<T, base::FileError>`.
*   Mojo does not yet support this as a first-class primitive; see the
    [open request](https://crbug.com/1327821).

### New helpers in base/test/metrics/histogram\_tester.h

Testing is an important part of Chromium. Even code that records metrics should
have test coverage to help ensure the correctness of the metrics. To help
support metrics testing, [crrev.com/998514](https://crrev.com/998514) and
[crrev.com/1002653](https://rrev.com/1002653) changed
[struct base::Bucket](https://chromium.googlesource.com/chromium/src/+/9e001703a2/base/test/metrics/histogram_tester.h#200)
to accept enums, and added 4 new gMock matchers:

*   [base::BucketsAreArray](https://chromium.googlesource.com/chromium/src/+/9e001703a2/base/test/metrics/histogram_tester.h#241)
*   [base::BucketsAre](https://chromium.googlesource.com/chromium/src/+/9e001703a2/base/test/metrics/histogram_tester.h#248)
*   [base::BucketsIncludeArray](https://chromium.googlesource.com/chromium/src/+/9e001703a2/base/test/metrics/histogram_tester.h#265)
*   [base::BucketsInclude](https://chromium.googlesource.com/chromium/src/+/9e001703a2/base/test/metrics/histogram_tester.h#289)

Now it is possible to write
[more concise test expectations](https://chromium.googlesource.com/chromium/src/+/9e001703a2/components/password_manager/core/browser/store_metrics_reporter_unittest.cc#1058)
when testing metrics.

### base::ScopedBoostPriority

Chromium's architecture involves multiple processes and multiple threads. Giving
threads the appropriate priority can greatly improve performance, but changing a
thread's priority in a safe manner in cross-platform code can be tricky. To
avoid [subtle bugs](https://crbug.com/1335489),
[crrev.com/1013320](https://crrev.com/1013320) added
[base::ScopedBoostPriority](https://chromium.googlesource.com/chromium/src/+/9e001703a2/base/threading/scoped_thread_priority.h)
to make it easier to boost a thread's priority in a controlled manner, without
having to worry about the complications involved in restoring the original
priority. Please consider using this scoper instead of calling
[base::PlatformThread::SetCurrentThreadType()](https://chromium.googlesource.com/chromium/src/+/9e001703a2/base/threading/platform_thread.h#240)
directly.

Moreover, //base contains numerous scoper classes to make life easier for
Chromium developers. Check out
[these](https://chromium.googlesource.com/chromium/src/+/9e001703a2/base/files/scoped_temp_dir.h#8)
[examples](https://chromium.googlesource.com/chromium/src/+/9e001703a2/base/test/scoped_mock_clock_override.h#15).

## Project updates

### Standalone PartitionAlloc

The Chrome Memory Team is preparing to bring the benefits of
[PartitionAlloc](https://chromium.googlesource.com/chromium/src/+/9e001703a2/base/allocator/partition_allocator/PartitionAlloc.md)
to Chrome-external projects! Our first targets are PDFium (which uses an older
copy of PA today), ARCVM, and d8. The goal here is to allow external clients to
transparently use PartitionAlloc as their allocator of choice with a simple DEPS
roll.

We've already completed most of the work of breaking dependencies on //base,
which almost enables us to lift-and-shift the codebase as a single unit. More
work is yet to come on doing the same for //build and other Chrome-isms. The
team is currently working on assuming control of the shim in //base/allocator,
which would allow us to provide the functional equivalent of
PartitionAlloc-Everywhere for external clients.

### base::Value API refactoring

The commonly used base::Value class has been
[refactoring its APIs](https://docs.google.com/document/d/1CwYuMXnVQsRsghwVzEkWj9GZzfERputSLQaKx5xLhjQ/edit)
for the last 6 years to better take advantage of modern C++ features. The
original v1 proposal had some pain points that made adoption difficult. The v2
proposal addressed those problems and migration to the v2 APIs has been making
lots of progress, with many [deprecated](https://crbug.com/1187045)
[APIs](https://crbug.com/1187066) getting [removed](https://crbug.com/1187100)
in the past year. Please consider helping with migration and take one of the
bugs blocking the [meta bug](https://crbug.com/1187001).

Issues with the v1 proposal that lead to the v2 proposal:

*   The original plan was to entirely replace dictionaries with
    `base::flat_map<std::string, base::Value>` and lists with
    `std::vector<base::Value>`.
*   This turned out to be less practical than initially hoped:
    `base::DictionaryValue` and `base::ListValue` contained useful helpers that
    were all flattened into `base::Value`. Code that wants to use the helpers
    ends up losing type-safety and just using a generic `base::Value`
    everywhere.
*   In addition, directly exposing the underlying type makes it harder to make
    future improvements. e.g. switching `base::Value` to use
    `absl::flat_hash_map` internally.
*   The v2 proposal implemented `base::Value::Dict` and `base::Value::List` to
    address the above issues.

Open issues with the refactoring:

*   Migration can still be difficult to incrementally land. `base::Value::Dict`
    / `base::Value::List` are not subclasses of `base::Value` and do not really
    interoperate unless the caller is OK destructively passing the dict or list
    to function accepting a `base::Value`.
    *   `base::ValueView` is a read-only way for accepting values of all types
        but is not really intended for general usage (it's intentionally
        difficult to extract the exact subtype contained).
    *   `base::DictAdapterForMigration` is a non-owning wrapper around
        `base::DictionaryValue` or `base::Value::Dict`. It exposes the
        `base::Value::Dict` API and allows callers of a function that takes this
        adapter type to be incrementally migrated.
    *   TBD: Implement a corresponding adapter type for `base::Value::List`.

## Reminders

### How to use raw\_ptr appropriately

The MiraclePtr project seeks to improve memory safety in Chromium. As part of
this work, Chromium now uses the
[raw\_ptr](https://chromium.googlesource.com/chromium/src/+/9e001703a2/base/memory/raw_ptr.h)
smart pointer class in many places. Being relatively new, here is a quick
summary on how to use it appropriately:

*   `raw_ptr<T>` use for class/struct fields in place of a raw C++ pointers is
    highly encouraged. Use whenever possible with a few caveats.
*   `raw_ptr<T>` should *not* be used for function parameters or return values.
*   `raw_ptr<T>` should be used in non-renderer code as class/struct members.
    *   Renderer code uses the
        [Oilpan](https://chromium.googlesource.com/chromium/src/+/9e001703a2/third_party/blink/renderer/platform/heap/BlinkGCAPIReference.md)
        garbage collector instead.
*   A closely related smart pointer is
    [raw\_ref](https://chromium.googlesource.com/chromium/src/+/9e001703a2/base/memory/raw_ref.h),
    for cases where a pointer cannot be null.

See
[base/memory/raw\_ptr.md](https://chromium.googlesource.com/chromium/src/+/9e001703a2/base/memory/raw_ptr.md)
for more details.

### Use base::BindLambdaForTesting() to simplify tests

When dealing with callbacks in tests, oftentimes it can be much easier to use
[base::BindLambdaForTesting()](https://chromium.googlesource.com/chromium/src/+/9e001703a2/base/test/bind.h#48)
instead of
[base::BindRepeating()/BindOnce()](https://chromium.googlesource.com/chromium/src/+/9e001703a2/base/bind.h#22).
[Example CL](https://crrev.com/1035144).

## Bye bye

### base::PostTask() and friends in base/task/post\_task.h

`base::PostTask()` was the original way of posting to the “thread pool” (then
named `base::TaskScheduler`). It was then enhanced to allow `base::TaskTraits`
to specify `BrowserThreads` as a destination. `base::ThreadPool()` was later
added as a trait to force explicitly mentioning the destination. That was deemed
overly complicated and was moved to API-as-a-destination in
[Task APIs v3](https://docs.google.com/document/d/1tssusPykvx3g0gvbvU4HxGyn3MjJlIylnsH13-Tv6s4/edit).
Namely, `base::ThreadPool::*` and `content::Get(UI|IO)ThreadTaskRunner()`.

`base/task/post_task.h` is gone as of
[crrev.com/998343](https://crrev.com/998343). However, on Android, the Java side
of `PostTask()` still needs to go through this migration and is thus the only
(internal) caller remaining of the old traits-as-destination format which allows
//base to post to //content. //content Java browser thread APIs will need to be
added to fully finalize this API-as-a-destination migration and cleanup the
internal hooks for it.

### base::CharTraits in base/strings/char\_traits.h

This was a constexpr version of `std::char_traits` from the `<string>` header.
`std::char_traits` became constexpr in C++17, so existing uses were migrated to
`std::char_traits` and the //base version was removed in
[crrev.com/993996](https://crrev.com/993996).
