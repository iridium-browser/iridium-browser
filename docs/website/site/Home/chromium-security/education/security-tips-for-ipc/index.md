---
breadcrumbs:
- - /Home
  - Chromium
- - /Home/chromium-security
  - Chromium Security
- - /Home/chromium-security/education
  - Education
page_name: security-tips-for-ipc
title: Security Tips for IPC
---

**Note: This document is for legacy IPC. For Mojo IPC, please refer to
<https://chromium.googlesource.com/chromium/src/+/HEAD/docs/security/mojo.md>**

**The [Integer Semantics section has moved to Markdown
too](https://chromium.googlesource.com/chromium/src/+/HEAD/docs/security/integer-semantics.md).**

Chrome's[inter-process communication
(IPC)](http://www.chromium.org/developers/design-documents/inter-process-communication)
layer is the communication channel supporting our [multi-process
architecture](http://www.chromium.org/developers/design-documents/multi-process-architecture).
Security bugs in IPC can have [nasty
consequences](http://blog.chromium.org/2012/05/tale-of-two-pwnies-part-1.html),
but sticking to these tips should help you avoid most pitfalls. Questions,
feedback, or suggestions to security@chromium.org.

[TOC]

## Trust only the browser process.

**Generally, privileged processes must set all policy. In Chromium, this means
the browser process. "Policy" means: sizes, addresses, object names, filesystem
pathnames, permissions/ACLs, specific implementations of interfaces, etc. In
practice, this is how it should work:**

    ******Unprivileged process asks for capability.******

    ******Privileged process applies policy to find an implementation for the
    capability.******

    ******Unprivileged process receives it and performs constrained operations
    on it.******

    ******Privileged process owns the capability lifecycle.******

## **Do not trust renderer, PPAPI, or GPU processes.**

IPC messages from the renderers must be viewed with the same skepticism as one
would apply to user input. These messages are untrustworthy input.

## Sanitize and validate untrustworthy input.

If you're handling filenames or paths derived from untrustworthy input, make
sure to avoid [directory traversal
attacks](http://en.wikipedia.org/wiki/Directory_traversal_attack) by sanitizing
(e.g. by using a FilePath rather than a string, because FilePath implicitly
checks for ".." traversal sequences and other unanticipated platform behavior)
and ensuring the resulting path is within your base directory.

To construct a valid pathname, apply a function like FilePath::BaseName() to the
untrustworthy pathnames; now it's a basename for sure, and not a full pathname
or a sneaky trick. Then prefix the basename with a static directory name. Also
apply simple, "obviously-correct" lexical checks such as an RE match for
/^\\w{8,16}$/.

## Allowing is better than blocking.

If you know the full set of valid data, then compare against that rather than
checking for occurrences of known-bad data.

## **Safely handle known-bad input.**

When validating untrustworthy input, don't simply use CHECK. We do not want the
input validation mechanism to become an easy way for a malicious renderer to
kill the browser process. It's usually better to ignore the bad input, or for
the privileged process to immediately kill the sender of invalid inputs.

To terminate a malfunctioning IPC sender, see
BrowserChildProcessHostImpl::TerminateOnBadMessageReceived.

## **Use and validate specific, constrained types; let the compiler work for you.**

Use the most specific data type you can to enable the type system to do part of
your data validation. In other words, do you really need to use a string? Using
more constrained types (e.g. primitives, enum, GURL instead of std::string that
is "supposed to be a URL", etc.) lets the compiler do the type checking for you
and leads to faster, often smaller, and safer code that is easier for
maintainers and reviewers to understand. The most common pattern we see
violating this is unnecessary use of strings, which are essentially "blob types"
that are often slow (copying, lexing/parsing, used as keys in maps), and "vague"
in API contracts. The caller has to know how the callee is going to parse the
string, and the callee has to parse and validate it correctly (see the next
section).

Some other specific tips:

*   Use integer types for ids (e.g. audio device IDs:
            <https://codereview.chromium.org/12440027>).
*   Use pre-defined styles instead of allowing arbitrary CSS injection
            with strings, which could lead to XSS (e.g.
            <https://codereview.chromium.org/11413018/diff/8041/chrome/browser/ui/browser_instant_controller.cc#newcode252>).
*   If you must pass filesystem pathnames and path components over IPC,
            use FilePath and check for path traversal using
            FilePath::ReferencesParent.
*   IPC_ENUM_TRAITS() is deprecated (it generates unchecked enums).
            IPC_ENUM_TRAITS_MAX_VALUE and friends in ipc/param_traits_macros.h
            can help you make a constrained enum. Keep in mind that it takes the
            range min..max *inclusive*.

More generally, don't implement your own serialization mechanism
(std::vector&lt;char&gt;, protobufs) on top of the Chrome IPC system. Break up
your structs and use the primitives provided by Chrome IPC.

## **Keep it simple.**

**Send limited capabilities (e.g. file descriptors -but not directory
descriptors-), not open-ended, complex objects (e.g. pathnames). For example, to
write a temporary file, the renderer should ask the browser for a file
descriptor/HANDLE; the browser should create one entirely according to its own
policy; and then the browser should pass the descriptor to the renderer.**

## Be aware of the subtleties of integer types.

First read about the scary security implications of[ integer arithmetic.
](http://en.wikipedia.org/wiki/Integer_overflow)Adhere to these best practices:

*   **Use unsigned types for values that shouldn't be negative or where
            defined overflow behavior is required.**
*   Use explicitly sized integer types, such as int32, int64, or uint32,
            since sender and receiver could potentially use different
            interpretations of implicit types.
*   Use the integer templates and cast templates in
            [base/numerics](https://code.google.com/p/chromium/codesearch#chromium/src/base/numerics/)
            to avoid overflows, *especially when calculating the size or offset
            of memory allocations.*

## **Be aware of the subtleties of integer types across C++ and Java, too.**

When writing code for Chromium on Android, you will often need to marshall
arrays, and their sizes and indices, across the language barrier (and possibly
also across the IPC barrier). The trouble here is that the Java integer types
are well-defined, but the C++ integer types are whimsical. A Java int is a
signed 32-bit integer with well-defined overflow semantics, and a Java long is a
signed 64-bit integer with well-defined overflow semantics. in C++, only the
explicitly-sized types (e.g. int32_t) have guaranteed exact sizes, and only
unsigned integers (of any size) have defined overflow semantics.

Essentially, Java integers *actually are* what people often (incorrectly)
*assume* C++ integers are. Furthermore, Java Arrays are indexed with Java ints,
whereas C++ arrays are indexed with size_t (often implicitly cast, of course).
Note that this also implies a 2 Gig limit on the number of elements in an array
that is coming from or going to Java. That Should Be Enough For Anybody, but
it's good to keep in mind.

You need to make sure that every integer value survives its journey across
languages intact. That generally means explicit casts with range checks; the
easiest way to do this is with the base::checked_cast cast or
base::saturated_cast templates in
[safe_conversions.h](https://code.google.com/p/chromium/codesearch#chromium/src/base/numerics/safe_conversions.h).
Depending on how the integer object is going to be used, and in which direction
the value is flowing, it may make sense to cast the value to jint (an ID or
regular integer), jlong (a regular long integer), size_t (a size or index), or
one of the other more exotic C++ integer types like off_t.

## **Don't leak information, don't pass information that would be risky to use.**

**In particular, don't leak addresses/pointers over the IPC channel, either
explicitly or accidentally. (Don't defeat our
[ASLR](http://en.wikipedia.org/wiki/Address_space_layout_randomization)!) Worse:
sending pointers over the IPC is almost certainly a sign of something very wrong
and could easily lead to memory corruption.**

Do not pass child_id, that is [the ID of child processes as viewed by the
browser](https://code.google.com/p/chromium/codesearch#chromium/src/content/common/child_process_host_impl.h&l=54)
(which are not the same as the OS PIDs), from the browser via IPC. This
construct is risky because it would be tempting to send back this ID to the
browser and mistakenly use it as an authentication token, which it is not.

## **Avoid Unsafe (Common) Coding Patterns**

*   Avoid accessing underlying string or array data via
            std::string::c_str or std::vector::data. If you do, make sure to
            stay in bounds. Note that std::string::operator\[\] and
            std::vector::operator\[\] are not required to do bounds checking.
*   Databases: When storing together both user data and metadata, make
            sure that the renderer can't directly read/write metadata via some
            sort of corrupted access of user data.
*   Databases: Assume that the database might be corrupted. Integers
            might have become large or negative, filesystem paths might have
            changed to "../../../../etc/passwd", etc. Do some validation.
*   Don't validate inputs with DCHECKs (and WebKit ASSERTs) in the
            high-privilege process that fail with invalid input from the
            lower-privilege process. Instead, explicitly validate each input and
            fail fast on any invalid input. Using the macros alone leads to a
            false sense of security since they aren't compiled into release
            builds.
*   Be careful with shared memory mappings (specifically tracking their
            sizes on either side of the IPC channel). Do not store and trust
            sizes on one side of the channel. Avoid specifying the size when
            calling Map.
*   Keep serializers/deserializers within ParamTraits Read and Write
            methods.
*   If your design requires you to serialize a structure into a string
            you are doing it wrong. Look at the IPC_STRUCT_ macros.
*   Use ReadLength wherever possible in deserializers instead of
            ReadInt, et c.
*   Avoid using ReadData within deserializers. If your design requires
            this it is almost certainly wrong.
*   Remember that, when specifying an ID in a message, a compromised
            process on the less privileged end can can specify another valid ID.
            Code should not assume that objects looked up by ID match other
            local state.

## What About Mojo?

The underlying principles are exactly the same whether reviewing a Mojo-based CL
vs. a Chromium IPC CL. A short presentation can be found at
<https://docs.google.com/a/google.com/presentation/d/1uo8WAD6Hgq_gjlODb2ioUNGQs9RGlUYSQywxZOwCzcE/edit?usp=sharing>
