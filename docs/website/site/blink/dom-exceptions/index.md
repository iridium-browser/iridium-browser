---
breadcrumbs:
- - /blink
  - Blink (Rendering Engine)
page_name: dom-exceptions
title: DOM Exceptions
---

**Useful error messages with ExceptionMessages**

When throwing a DOM exception, please ensure that you take the time to write an
error message that helps developers understand what's gone wrong, and what they
can do to fix things. Something like
exceptionState.throwDOMException(InvalidStateError, "This object's 'readyState'
is not 'open'.") can make the difference between a 5 minute fix, and an all-day
facepalming session. The
[ExceptionMessages](http://cs.chromium.org/ExceptionMessages.h) helper class has
a variety of methods that help you construct a more detailed and specific
message in a consistent way. Perhaps:

es.throwDOMException(TypeMismatchError,
ExceptionMessages::argumentNullOrIncorrectType(1, "Node"));

That will give the user an exception object whose message property reads "Failed
to execute "method" on "Interface": The 1st argument is null, or an invalid Node
object.", which has been scientifically measured to be 83 times more useful than
an exception lacking that detail. In general, developers will be much happier if
we give them enough data to resolve an issue right away, so please don't be
afraid of adding detail.

Just add #include "bindings/v8/ExceptionMessages.h" and go wild.

**Security considerations**

Exception messages are exposed to the web via JavaScript. Please be very careful
when throwing exceptions that might expose interesting data cross-origin. In
particular, SecurityError messages are prone to this sort of accidental leakage.

1.  Always throw SecurityError exceptions via
            [ExceptionState::throwSecurityError](http://cs.chromium.org/ExceptionState::throwSecurityError).
2.  Ensure that the sanitizedMessage (first parameter) can safely be
            exposed to JavaScript.
3.  Add additional detail via an unsanitizedMessage (second parameter)
            if relevant.

Practical examples of this include
DOMWindow::sanitizedCrossDomainAccessErrorMessage and
DOMWindow::crossDomainAccessErrorMessage. We call both of these methods from
inside
[V8Initializer::failedAccessCheckCallbackInMainThread](http://cs.chromium.org/V8Initializer%20failedAccessCheckCallbackInMainThread)
to ensure that JavaScript can't access details about cross-origin frames, but
that those details appear in the console if the exceptions aren't caught.
