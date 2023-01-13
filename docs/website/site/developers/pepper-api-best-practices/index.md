---
breadcrumbs:
- - /developers
  - For Developers
page_name: pepper-api-best-practices
title: Pepper API Best Practices
---

When appropriate, suffix enumeration constants with _PRIVATE.

Two enum constants can't have the same name, so making a separate enum for

Private constants causes trouble. Suffixing the constants with _PRIVATE makes

it clear which values are public and which are private. This also keeps names

from colliding.

Use primitive types in C interfaces when possible.

It's easier for users to deal with primitive types (int, double, etc.) than

PP_Var. This is worth doing, even when trying to make an interface mirror a

JavaScript counterpart. If a string must be valid UTF-8, use PP_Var.

Be extra careful with C++ interfaces.

We don't have any mechanism today to provide multiple versions of a C++

interface to users. This means we must be even more forward-looking about what

is in those APIs, as libraries may come to depend on them.

In C++ implementations, always call the newest reasonable C interface.

Code in ppapi/cpp/ should check for the available C versions and always call

the newest available version that works for the given parameters. This makes it

more feasible to deprecate or remove a version of the C interface at some point

in the future.

Don't pass structs by value.

This can cause API breakage because PNaCl may change calling conventions on

different platforms.

Follow existing reference counting conventions.

When passing PP_Resource or PP_Var as an input parameter to an API, the API

should not expect to be passed a reference. It should explicitly take a

reference if it needs to.

Return values and output parameters must pass a reference back to the caller.

This includes output parameters for calls that return asynchronously via a

CompletionCallback.

Follow existing PP_CompletionCallback conventions.

Functions that take a completion callback should:

- Return int32_t, which must be either a positive value or a value from

ppapi/c/pp_errors.h

- Take only 1 PP_CompletionCallback parameter.

- Run the PP_CompletionCallback exactly once.

Use the Enter\* classes and TrackedCallback to ensure the behavior is

consistent.

Provide all new PPP interfaces as a parameter to a PPB interface.

Legacy PPP interfaces are exposed via the Plugin's GetInterface call and are

associated with an interface string, much like PPB interfaces. This has two

drawbacks:

- It's hard to associate a particular version of a PPB interface with its

corresponding PPP interface.

- We can only ever call the PPP interface on the main thread.

Design your API to be friendly as a porting layer for a more well-known API.

Our FileIO API should make it easy to implement POSIX file operations.

Similarly, our sockets APIs should make it possible to implement BSD/POSIX

sockets.
