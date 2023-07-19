---
breadcrumbs:
- - /blink
  - Blink (Rendering Engine)
page_name: unittesting
title: Unit Testing in Blink
---

**WARNING: This document is a work in progress!**

[TOC]

## Unit Testing Tools

GTest and GMock are both imported into Blink and can be used in unit tests. Most
existing tests are purely GTest based, but GMock is slowly being used more.

### GTest - Google Unit Testing Framework

> *"Google's framework for writing C++ tests on a variety of platforms (Linux,
> Mac OS X, Windows, Cygwin, Windows CE, and Symbian). Based on the xUnit
> architecture. Supports automatic test discovery, a rich set of assertions,
> user-defined assertions, death tests, fatal and non-fatal failures, value- and
> type-parameterized tests, various options for running the tests, and XML test
> report generation."*

#### Further Documentation

*   GTest Project Website - <https://code.google.com/p/googletest/>
*   Primer - <https://code.google.com/p/googletest/wiki/Primer>
*   FAQ - <https://code.google.com/p/googletest/wiki/FAQ>
*   Advanced Guide -
            <https://code.google.com/p/googletest/wiki/AdvancedGuide>

#### Tip: GTest and regular expressions (regexp)

If you are using gtest "[Death
Tests](https://code.google.com/p/googletest/wiki/AdvancedGuide#How_to_Write_a_Death_Test)"
or GMock's EXPECT_THAT with MatchesRegex or ContainsRegex, you have to **be very
careful with your regex**.

There is no common syntax between all operating system for character class
matches;

*   Character class short cuts are NOT part of the POSIX regex standard
            and **DO NOT** work on Mac OS X. It also **wont** give you a warning
            saying the regex is invalid.

```none
EXPECT_THAT("abc", MatchesRegex("\w*")) # Does NOT work on Mac OS X.
```

*   Character classes (IE the square bracketed kind) **DO NOT** work
            with the gtest internal regex engine, and thus on Windows. At least
            it will warn you that the regex is invalid.

```none
 EXPECT_THAT("abc", MatchesRegex("[a-c]*")) # Does NOT work on Windows.
```

*(CL <https://codereview.chromium.org/55983002/> proposes making chromium only
use the gtest internal regex engine which would fix this problem.)*

#### Tip: Using GMock matchers with GTest

You can use GMock EXPECT_THAT and the GMock matchers inside a GTest test for
more powerful matching.

Quick example;

```none
EXPECT_THAT(Foo(), testing::StartsWith("Hello"));EXPECT_THAT(Bar(), testing::MatchesRegex("Line \\d+"));ASSERT_THAT(Baz(), testing::AllOf(testing::Ge(5), testing::Le(10)));Value of: Foo()  Actual: "Hi, world!"Expected: starts with "Hello"
```

More information at;

*   [TotT: Making a Perfect
            Matcher](http://googletesting.blogspot.com.au/2009/10/tott-making-perfect-matcher.html)
*   [GMock Cookbook - Using Matchers in Google Test
            Assertions](https://code.google.com/p/googlemock/wiki/CookBook#Using_Matchers_in_Google_Test_Assertions)

#### Error: Has the "template&lt;typename T&gt; operator T\*()" private.

More information at <https://code.google.com/p/googletest/issues/detail?id=442>

Workaround:

<pre><code><b>namespace</b> testing {
<b>namespace</b> internal {
// gtest tests won't compile with clang when trying to EXPECT_EQ a class that
// has the "template&lt;typename T&gt; operator T*()" private.
// (See <https://code.google.com/p/googletest/issues/detail?id=442)>
//
// Work around is to define this custom IsNullLiteralHelper.
<b>char</b>(&IsNullLiteralHelper(<b>const</b> WebCore::CSSValue&))[2];
}
}
</code></pre>

### GMock - Google C++ Mocking Framework

<https://code.google.com/p/googlemock/>

> *Inspired by jMock, EasyMock, and Hamcrest, and designed with C++'s specifics
> in mind, Google C++ Mocking Framework (or GMock for short) is a library for
> writing and using C++ mock classes.*

#### Further Documentation

*   \[TODO\]

#### Tip: GMock and regular expressions (regexp)

GMock uses the gtest for doing the regexs, [see the section under gtest
above](/blink/unittesting#TOC-Tip:-GTest-and-regular-expressions-regexp-).

#### Tip: Mocking non-virtual functions

For speed reasons, a majority of Blink's functions are non-virtual. This can
make them quite hard to mock. Here are some tips for working around these
problems;

#### Tip: Mock Injection (Dependency Injection)

Useful documentation:

*   TotT: Testing Against Interfaces -
            <http://googletesting.blogspot.com.au/2008/07/tott-testing-against-interfaces.html>
*   TotT: Defeat "Static Cling" -
            <http://googletesting.blogspot.com.au/2008/06/defeat-static-cling.html>

Using a proxy interface internally in your class;

<pre><code>// MyClass.h
// ------------------------------------------------------------
<b>class</b> MyClass {
<b>private</b>:
    <b>class</b> iExternal {
    <b>public</b>:
        <b>virtual</b> <b>void</b> function1(<b>int</b> a, <b>int</b> b);
        <b>virtual</b> <b>bool</b> function2();
        <b>static</b> iExternal* instance() { <b>return</b> pInstance(); }
        <b>static</b> <b>void</b> setInstanceForTesting(iExternal* newInstance) { pInstance(true, newInstance); }
        <b>static</b> <b>void</b> clearInstanceForTesting() { pInstance(true, 0); }
    <b>protected</b>:
        iExternal() { }
    <b>private</b>:
        <b>inline</b> <b>static</b> iExternal* pInstance(<b>bool</b> set = false, iExternal* newInstance = 0)
        {
            <b>static</b> iExternal* defaultInstance = <b>new</b> iExternal();
            <b>static</b> iExternal* instance = defaultInstance;
            <b>if</b> (set) {
                <b>if</b> (!newInstance) {
                    newInstance = defaultInstance;
                }
                instance = newInstance;
            }
            <b>return</b> instance;
        }
    };
<b>public</b>:
    <b>void</b> aFunction() {
        <b>if</b> (iExternal::instance()-&gt;function2()) {
            iExternal::instance()-&gt;function1(1, 2);
        }
    }
}
</code></pre>

<pre><code>// MyClassTest.cpp
// ------------------------------------------------------------
<b>class</b> MyClassTest : <b>public</b> ::testing::Test {
    <b>class</b> iExternalMock : <b>public</b> MyClass::iExternal {
    <b>public</b>:
        MOCK_METHOD0(function1, <b>void</b>(<b>int</b>, <b>int</b>));
        MOCK_METHOD0(function2, <b>bool</b>());
    };
    <b>void</b> setInstanceForTesting(MyClass::iExternal& mock) {
        MyClass::iExternal::setInstanceForTesting(&mock);
    }
};
TEST_F(MyClassTest, aFunctionTest)
{
    iExternalMock externalMock;
    EXPECT_CALL(externalMock, function2())
        .WillOnce(Return(true))
        .WillOnce(Return(false));
    EXPECT_CALL(externalMock, function1(1, 2));
    setInstanceForTesting(externalMock);
    MyClass c;
    c.aFunction();
}
</code></pre>

#### Tip: Mocks and OwnPtr (PassOwnPtr)

OwnPtr and mocking objects can be tricky to get right, here is some important
information.

***The Problem***

As normal, once you return an object via a PassOwnPtr you no longer control the
life cycle of the object. This means that you **must not** use the object as an
expectation (EXPECT_CALL) for another function call because;

*   On each call, GMock checks if any of the expectations match.
*   On termination, if something went wrong GMock might try to print the
            expectation (for both matched and unmatched expectations).

Here is some example code which is **WRONG**:

*   At **(1)** myA1 has been deleted, but GMock will check **both** the
            mockb EXPECT_CALLs.
*   At **(2)** both myA1 and myA2 have been deleted, but if EXPECT_CALL
            is not matched GMock may try to print myA1 and myA2.

<pre><code>// Actual implementation
<b>class</b> A {};
<b>class</b> B {
   <b>virtual</b> use(A& a) {} {}
};
<b>class</b> C {
   B* m_b;
   C(B* b): m_b(b) {}
   <b>void</b> doIt(PassOwnPtr&lt;A&gt; myA) {
     m_b-&gt;use(*myA);
     // As we own myA it gets deleted here.
   }
};
// Mocks
<b>class</b> MockB : <b>public</b> B {
    MOCK_METHOD0(use, <b>void</b>(A&));
};
// Test
TEST(MyTest, CDoItTest)
{
  OwnPtr&lt;A&gt; myA1 = adoptPtr(<b>new</b> A());
  OwnPtr&lt;A&gt; myA2 = adoptPtr(<b>new</b> A());
  MockB mockb;
  EXPECT_CALL(mockb, use(Ref(*myA1.get()))); // Ref() means "is a reference to"
  EXPECT_CALL(mockb, use(Ref(*myA2.get())));
  C c(&mockb);
  c.doIt(myA1.release());
  c.doIt(myA2.release()); // <b>(1)</b>
  // <b>(2)</b>
}
</code></pre>

***Solutions that don't work***

Creating a specialization of OwnedPtrDeleter

> `template <> struct OwnedPtrDeleter<MyClass> {}`

> **Why?**

> > The OwnedPtrDeleter specialization must be visible at the location that the
> > OwnPtr/PassOwnPtr is created.

## Test Helpers

Test helpers are an important part of making Blink easier to test for everyone.
The more test helpers that exist, the easier it is to write new unit tests as
you have to write less boilerplate code and find it easier to debug failing
tests.

Test helpers include;

*   Pretty printing functions for types.
*   Shared fake implementations of complex types.
*   Quick creation functions for types.
*   `operator==` definitions to allow `EXPECT_EQ` and comparisons in
            `EXPECT_CALL` mocks to work.

Test helpers **should;**

*   be define in a "XXXTestHelper.h" file, where XXX is the type (or
            type group) that it helps (there might also be a XXXTestHelper.cpp
            in rare cases).
*   have some basic tests to make sure they work. **Nobody wants to
            debug test helpers while writing their own tests!**
    *   This is specially important for PrintTo functions to make sure
                they actually print what you expect. You can use the
                `EXPECT_THAT` with Regex from GMock to make these tests easier
                to write.
    *   These should be in a XXXTestHelperTest.cpp file (shouldn't need
                a header file).

### Operator==

Both the `EXPECT_EQ` and the `EXPECT_CALL` methods use `a == b` to compare if
two objects are equal. However for many reasons you don't want this operator to
be generally available in Blink. You can define the operator in the test helper
instead and then it will only be available during tests.

Unlike PrintTo, operator== is not a template so the normal type hierarchy
applies.

<pre><code><b>bool</b> <b>operator</b>==(<b>const</b> TYPE& a, <b>const</b> TYPE& b)
{
    <b>return</b> SOMETHING
}
</code></pre>

#### **Operator== Gotchas - Namespacing**

The **operator==** MUST be define in the same namespace as the type for
`EXPECT_EQ` to work. For example, if type is `::WebCore::AnimatableValue` the
operator must be in the `::WebCore` namespace.

### Pretty Printers

Pretty printers make it much easier to see what is going on in your test and why
a match isn't working. They should be created for any simple type which has a
useful string representation.

```none
void PrintTo(const A& a, ::std::ostream* os)
{
    *os << "A@" << &a;
}
```

More Information on creating pretty printers can be found at [GTest Advanced
Guide: Teaching Google Test How to Print Your
Values](https://code.google.com/p/googletest/wiki/AdvancedGuide#Teaching_Google_Test_How_to_Print_Your_Values).

#### **Pretty Printers Gotchas - Namespace**

Pretty Printers **must** be define in the same namespace as the class which it
is printing.

<pre><code>
<b>namespace</b> A {
  <b>class</b> A{};
}
<b>namespace</b> {
  <b>void</b> PrintTo(<b>const</b> A& a, ::std::ostream* os) {} // Never called
}
</code></pre>

#### **Pretty Printers Gotchas - Type matching**

Pretty Printers only work on **exact** and **known** type match. This means
that;

*   A PrintTo for a base class will not apply to children classes.
*   A PrintTo for a specific class will not apply when that class is
            referenced/pointed to as a base class.

Further information at the gtest bug tracker -
<https://code.google.com/p/googletest/issues/detail?id=443>

This is hard to understand, but shown by the following example (also attached as
printto-confusing.cpp).

<pre><code>
#include &lt;iostream&gt;
#include &lt;gtest/gtest.h&gt;
<b>using</b> std::cout;
<b>using</b> testing::PrintToString;
<b>class</b> A {};
<b>class</b> B : <b>public</b> A {};
<b>class</b> C : <b>public</b> B {};
<b>void</b> PrintTo(<b>const</b> A& a, ::std::ostream* os)
{
    *os &lt;&lt; "A@" &lt;&lt; &a;
}
<b>void</b> PrintTo(<b>const</b> B& b, ::std::ostream* os)
{
    *os &lt;&lt; "B@" &lt;&lt; &b;
}
<b>int</b> main() {
    A a;
    B b;
    C c;
    A* a_ptr1 = &a;
    B* b_ptr = &b;
    A* a_ptr2 = &b;
    C* c_ptr = &c;
    A* a_ptr3 = &c;
    cout &lt;&lt; PrintToString(a) &lt;&lt; "\n";       // A@0xXXXXXXXX
    cout &lt;&lt; PrintToString(b) &lt;&lt; "\n";       // B@0xYYYYYYYY
    cout &lt;&lt; PrintToString(c) &lt;&lt; "\n";       // 1-byte object &lt;60&gt;
    cout &lt;&lt; PrintToString(*a_ptr1) &lt;&lt; "\n"; // A@0xXXXXXXXX
    cout &lt;&lt; PrintToString(*b_ptr) &lt;&lt; "\n";  // B@0xYYYYYYYY
    cout &lt;&lt; PrintToString(*a_ptr2) &lt;&lt; "\n"; // A@0xYYYYYYYY
}
</code></pre>

You can work around this problem by also defining a couple of extra PrintTo
methods (also attached as printto-workaround.cpp).

<pre><code>#include &lt;iostream&gt;
#include &lt;gtest/gtest.h&gt;
<b>using</b> std::cout;
<b>using</b> testing::PrintToString;
#define OVERRIDE override
// MyClass.h
// ---------------------------------------------------------------
<b>class</b> MyClassA {
// As WebKit is compiled without RTTI, the following idiom is used to
// emulate RTTI type information.
<b>protected</b>:
   <b>enum</b> MyClassType {
     BType,
     CType
   };
   <b>virtual</b> MyClassType type() <b>const</b> = 0;
<b>public</b>:
    <b>bool</b> isB() <b>const</b> { <b>return</b> type() == BType; }
    <b>bool</b> isC() <b>const</b> { <b>return</b> type() == CType; }
};
<b>class</b> MyClassB : <b>public</b> MyClassA {
    <b>virtual</b> MyClassType type() <b>const</b> OVERRIDE { <b>return</b> BType; }
};
<b>class</b> MyClassC : <b>public</b> MyClassB {
    <b>virtual</b> MyClassType type() <b>const</b> OVERRIDE { <b>return</b> CType; }
};
// MyClassTestHelper.h
// ---------------------------------------------------------------
<b>void</b> PrintTo(<b>const</b> MyClassB& b, ::std::ostream* os)
{
    *os &lt;&lt; "B@" &lt;&lt; &b;
}
// Make C use B's PrintTo
<b>void</b> PrintTo(<b>const</b> MyClassC& c, ::std::ostream* os)
{
    PrintTo(*<b>static_cast</b>&lt;<b>const</b> MyClassB*&gt;(&c), os);
}
// Call the more specific subclass PrintTo method
// *WARNING*: The base class PrintTo must be defined *after* the other PrintTo
// methods otherwise it'll just call itself.
<b>void</b> PrintTo(<b>const</b> MyClassA& a, ::std::ostream* os)
{
    <b>if</b> (a.isB()) {
        PrintTo(*<b>static_cast</b>&lt;<b>const</b> MyClassB*&gt;(&a), os);
    } <b>else</b> <b>if</b> (a.isC()) {
        PrintTo(*<b>static_cast</b>&lt;<b>const</b> MyClassC*&gt;(&a), os);
    } <b>else</b> {
        //ASSERT_NOT_REACHED();
    }
}
<b>int</b> main() {
    MyClassB b;
    MyClassC c;
    MyClassB* b_ptr = &b;
    MyClassA* a_ptr1 = &b;
    MyClassC* c_ptr = &c;
    MyClassA* a_ptr2 = &c;
    cout &lt;&lt; PrintToString(b) &lt;&lt; "\n";       // B@0xYYYYYYYY
    cout &lt;&lt; PrintToString(*b_ptr) &lt;&lt; "\n";  // B@0xYYYYYYYY
    cout &lt;&lt; PrintToString(*a_ptr1) &lt;&lt; "\n"; // B@0xYYYYYYYY
    cout &lt;&lt; PrintToString(c) &lt;&lt; "\n";       // B@0xAAAAAAAA
    cout &lt;&lt; PrintToString(*c_ptr) &lt;&lt; "\n";  // B@0xAAAAAAAA
    cout &lt;&lt; PrintToString(*a_ptr2) &lt;&lt; "\n"; // B@0xAAAAAAAA
}
</code></pre>

## Future Proposals

All these issues are up for discussion and have yet to be decided on;

*   Creation of high quality fake objects for core blink components.
            Each fake should be created and maintained by the OWNERS that owns
            the real implementation. See [Testing on the Toilet: Know Your Test
            Doubles](http://googletesting.blogspot.com.au/2013/06/testing-on-toilet-fake-your-way-to.html),
            TotT: Fake Your Way to Better Tests -
            <http://googletesting.blogspot.com.au/2013/06/testing-on-toilet-fake-your-way-to.html>
*   Creation of test helpers for all objects in Blink.
*   Split unit tests into their own build unit rather then one big
            "webkit_unittest" (faster test building and linking, ability to use
            mocks via inclusion)
