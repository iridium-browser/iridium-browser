---
breadcrumbs:
- - /nativeclient
  - Native Client
- - /nativeclient/how-tos
  - '2: How Tos'
page_name: exception-handling-interface
title: Exception Handling Interface
---

## Introduction

Hardware exceptions (invalid memory accesses, division by zero and etc) can not
be caught in the standard C/C++. Different OS provide different mechanisms to
catch hardware exception. POSIX standardizes hardware exception handling via
signals. Native Client doesn't support signals but it provides somewhat similar
interface. The main difference is that Native Client doesn't allow to resume
execution from the failed instruction. Note, that exception handling is disabled
for PNaCl because exception handling interface uses platform-dependent register
context.

## Retrieving Hardware Exception Handling Interface

#include &lt;irt.h&gt;

struct nacl_irt_exception_handling exception_handling_interface;

size_t interface_size = nacl_interface_query(

NACL_IRT_EXCEPTION_HANDLING_v0_1,

&exception_handling_interface,

sizeof(exception_handling_interface));

if (interface_size != sizeof(exception_handling_interface)) {

/\* error \*/

}

## Using nacl_exception Library

An alternative way to access hardware exception handling interface is via
nacl_exception library (it is part of libc). Add -lnacl_exception flag and use
functions from `nacl/nacl_exception.h` header. They have the same parameters and
their names are just prefixed with `nacl_exception`.

## Handling Hardware Exceptions

Hardware exception handlers are declared as

`typedef void (*NaClExceptionHandler)(struct NaClExceptionContext *context);`

in irt.h. They receive platform-dependent hardware exception context which
contains the register state. This function should not return since there is
nowhere to return to. Once exception handler is called, the exception handling
is disabled on this thread. You must call

`int exception_clear_flag(void);`

if you need to reenable it (you want to handle hardware exceptions inside
hardware exception handler for example). This function returns 0 on success and
error code on error (exception handling is disabled). If hardware exception
happens on a thread where exception handling is disabled, the whole NaCl process
is terminated.

The definition of `NaClExceptionContext` can be found in `nacl/nacl_exception.h`
header.

NaCl supports only one exception handler per process. The exception handler is
set with

`int exception_handler(NaClExceptionHandler handler,`

` NaClExceptionHandler *old_handler);`

This function returns 0 on success, or error code on error (exception handling
is disabled, handler is not aligned to the bundle size or outside of code
region, exception handler thread is failed to start). Old exception handler is
returned via old_handler parameter. In order to disable exception handling, pass
`NULL` exception handler.

Exception handler is called on current thread stack by default. If you want to
set alternative stack (to handle stack overflows for example) for exception
handler, use

`int exception_stack(void *stack, size_t size);`

The alternative stack is set per thread. The function returns 0 on success and
error code on error (stack location is outside of NaCl address space). If you
want to disable alternative stack, call this function with `NULL` stack pointer
and zero size.
