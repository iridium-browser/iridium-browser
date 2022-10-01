---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/testing
  - Testing and infrastructure
- - /developers/testing/android-tests
  - Android Tests
page_name: testing-android-code-that-crosses-the-c-java-boundary
title: Testing Android code that crosses the C++/Java boundary
---

## Java unit testing using JUnit

The Junit tests run on the host, in an environment that doesn't include the C++
components of Chromium. This gives two problems:

*   Testing ***@CalledByNative*** members:

> These members are not generally called from other Java code, so are typically
> declared as private. They are, however, part of the class's interface, so it
> makes sense to test them. The solution is simple; change their scope to
> package or public.

*   Calls to ***native*** members.

> Calls to native code won't work in this environment, so the test needs to
> intercept these calls. To solve this:

> 1.  Give your native methods a scope (package or public) that makes
              them visible to the tests, and mark them @VisibleForTesting.
> 2.  Use Mockito.spy to spy on the class you are testing.
> 3.  Stub the native method using Mockito. See the Mockito notes on
              [spying on real
              objects](http://site.mockito.org/mockito/docs/current/org/mockito/Mockito.html#spy).

> The following code shows an example of this:

> ` // Spy on the authenticator so that we can override and intercept the native
> method call.`

> ` HttpNegotiateAuthenticator authenticator =`

> ` spy(HttpNegotiateAuthenticator.create(1234, "Dummy_Account"));`

> ` doNothing().when(authenticator).nativeSetResult(anyLong(), anyBoolean(),
> anyString());`

You can check that the native methods are called correctly using Mockito.verify
etc

## C++ unit testing of classes calling Java

If you are testing a C++ class that calls Java methods there is generally no way
of stubbing or mocking the Java methods in C++ without significant code changes
(e.g. creating a C++ wrapper for the Java methods). C++ unit tests can, however
call Java through the JNI. For many cases calling the real Java classes in the
tests works fine, however if you need to stub or mock a Java class you can do so
by writing an alternative version of it, with the same native interface, and
building your tests against that alternative version.

One problem that can occur is that certain Android SDK methods (e.g. those of
AccountManager) return their results asynchronously through callbacks. These
callbacks are called through the Android event loop, so can only be called on a
thread that runs an Android Looper. At least some of the unit tests (e.g. the
net_unittests) run on a thread that does not call the Android looper within
their message loop's Run() method. One possible solution to this is to:

1.  Create a new Java thread with a Looper.
2.  Use ThreadUtils.setUiThread to tell Chrome that this is the UI
            thread.
3.  Run your test on the original thread, calling its Run method as
            appropriate. The test's Run loop should be terminated through a call
            to Quit once the result has been returned to the C++ code.

base::StartTestUiThreadLooper will start such a dummy UI thread.
