---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/testing
  - Testing and infrastructure
page_name: no-compile-tests
title: No-compile Tests
---

No Compile Tests are used to verify that certain coding constructs will fail to
compile. These were developed to test the base/functional/callback.h and
base/functional/bind.h constructs which need to ban certain types of assignments
in order to preserve type safety. The tests however can be used to enforce any
sort of compile-time check, such as static_assert.

**Why do we want to assert failure in compilation?**

Most of the time you don't want to do this. However, with template code, you are
playing with the type system. In particular with callbacks, it's possibly to
accidentally write code that allows unsafe type conversion leading to a code
execution. Here's a specific example:

int func(double f) {

printf("woohoo! Called with %lf!\\n", f);

}

Callback&lt;int(void)&gt; c = Bind(&func, 1);

If you look carefully at the types, Callback&lt;int(void)&gt; has no concept of
how many arguments func originally needed, or what types they were. Somewhere
between the call to Bind() and the assignment, the type of func:int(int), and
the type of 1:int was captured and erased so that it could be assigned into a
Callback that only knew the resulting function was int(void). To do this, there
is some casting happening that can break type safety. In the Bind()
implementation, the code has been constructed carefully to fail at compile time
if you were to try and do something like:

Callback&lt;void(void)&gt; c = Bind(&func, 1);

However, the system is complicated and regressions can occur. In fact, they
already snuck in once. To ensure we do not regress in behavior and allow a
dangerous type safety violation, we need a set of "no compile tests" that assert
that known dangerous constructs will not silently compile. This makes it much
safer to modify the Bind() implementation.

Though this uses the Callback/Bind system as an example, any code that ends up
needing to ensure a certain set of compile time constraints aren't violated can
benefit from these kinds of tests.

**How to write:**

1.  Create a ".nc" source file.
2.  Import build/nocompile.gni
3.  Include the ".nc" source file into a sources section of a
            nocompile_test target (eg., base_nocompile_tests).

The nocompile.gni import adds a template target that will process the .nc files
and produce a .cc file that represents the test results. On a failed no-compile
test (aka success compilation), the generated .cc file will consist of a set of
#error lines holding the compiler output. This will cause the unittest to fail
to build. If all tests succeed, then the .cc file will contain a set of
unittests representing each no-compile test which trivially succeed. This lets
you know that the tests were at least run.

The comment at the top of build/nocompile.gni has most of the documentation for
how to use this system.
