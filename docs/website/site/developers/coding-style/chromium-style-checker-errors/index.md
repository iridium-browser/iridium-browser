---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/coding-style
  - Coding Style
page_name: chromium-style-checker-errors
title: Chromium Style Checker Errors
---

[TOC]

## Introduction

Due to constant over inlining, we now have plugins to the clang compiler which
check for certain patterns that can increase code bloat. This document lists
these errors, explains the motivation behind adding it as an error and shows how
to fix them. The [chromium/clang wiki
page](https://chromium.googlesource.com/chromium/src/+/HEAD/docs/updating_clang_format_binaries.md)
explains how you can run the plugin yourself, or how you can run your CLs
through the clang trybots which run this plugin as well.

All diagnostics produced by the style checker are marked with
\[chromium-style\].

## Constructor/Destructor Errors

Constructors and destructors are often much larger and more complex than
programmers think. They are also implicitly defined in each translation unit if
you don't declare them. This can have a cascading effect if a class has member
variables that have inlined/implicitly declared constructors/destructors, or are
templates.

(Internally, the plugin determines whether a class is complex using a point
system, adding 9 points for each templated base class, 10 for each templated
member variable, 3 for each non-POD type member variable, and (only for the
constructor score) 1 for each integral data type. If a class's score is greater
than or equal to 10, it will trip these errors.)

If you get any of these compiler errors:

*   Complex class/struct needs an explicit out-of-line constructor
*   Complex class/struct needs an explicit out-of-line destructor

It's because you've written something like the following in a header:

```c++
class ComplexStuff {
 public:
  void NoDeclaredConstructor();
 private:
  // Enough data members to trip the detector
  BigObject big_object_;
  std::map<std::string, OtherObj> complex_stl_stuff_;
};
```

This is fixed by adding declared constructors:
```c++
class ComplexStuff {
 public:
  // Defined in the .cc file:
  ComplexStuff();
  ~ComplexStuff();
 private:
  BigObject big_object_;
  std::map<std::string, OtherObj> complex_stl_stuff_;
};
```

Likewise, if you get these compiler errors:

*   Complex constructor has an inlined body.
*   Complex destructor has an inline body.

It's because you've written something like the following in a header:

```c++
class MoreComplexStuff {
 public:
  MoreComplexStuff() = default;
  ~MoreComplexStuff() = default;
 private:
  BigObject big_object_;
  std::map<std::string, OtherObj> complex_stl_stuff_;
};
```

The solution is the same; put the definitions for the constructor/destructor in
the implementation file.

Sometimes, you might need to inline a constructor/destructor for performance
reasons. The style checker makes an exception for explicitly inlined
constructors/destructors:

In the .h file:
```c++
class ComplexFastStuff {
 public:
  ComplexFastStuff();
  ~ComplexFastStuff();
 private:
  /* complicated members */
};
```

In the .cc file:
```c++
inline ComplexFastStuff::ComplexFastStuff() = default;

inline ComplexFastStuff::~ComplexFastStuff() = default;
```

If this class is being copied somewhere in the code, then the compiler will
generate an implicit copy constructor for the class. As a result, you might also
see the following error:

*   Complex class/struct needs an explicit out-of-line copy constructor.

This can be fixed by adding an explicit copy constructor to your class:

In the .h file:
```c++
class ComplexCopyableStuff {
  public:
   ComplexCopyableStuff();
   ComplexCopyableStuff(const ComplexCopyableStuff& other);
   ~ComplexCopyableStuff();
  private:
   /* complicated members */
};
```

In the .cc file:
```c++
ComplexCopyableStuff::ComplexCopyableStuff(const ComplexCopyableStuff& other) = default;
```

Note that by defaulting the implementation, you ensure that you don't forget to
copy a member and this code will not need to be updated as the members of the
class change.

Also note that copying a complex class can be an expensive operation. The
preferred solution is almost always is to avoid the copy if possible. Please
consider changing the code and eliminating unnecessary copies instead of simply
adding an out of line copy constructor to silence the error. Additionally, in a
large number of cases, you can utilize move semantics to improve the efficiency
of the code.

Usually a compiler would generate a move constructor for your class, making
moving objects efficient. This is particularly true for STL containers such as
std::map. However, specifying either a copy constructor or a destructor prevents
the compiler from generating a move constructor. All in all, you should prefer
specifying a move constructor instead of a copy constructor. In cases where your
object is used as an rvalue, this will also prevent the error since the compiler
will not generate a copy constructor:

In the .h file:
```c++
class ComplexMovableStuff {
 public:
  ComplexMovableStuff();
  ComplexMovableStuff(ComplexMovableStuff&& other);
  ~ComplexMovableStuff();
 private:
  /* complicated members */
};
```
In the .cc file:
```c++
ComplexMovableStuff::ComplexMovableStuff(ComplexMovableStuff&& other) = default;
```
For more information on move semantics, please refer to
<https://www.chromium.org/rvalue-references>

## Virtual Method Out-of-lining

Virtual methods are almost never inlined in practice. Because of this, methods
on a base class will be emitted in each translation unit that allocates the
object or any subclasses that don't override that method. This is usually not a
major gain, unless you have a wide class hierarchy in which case it can be a big
win (<http://codereview.chromium.org/5741001/>). Since virtual methods will
almost never be inlined anyway (because they'll need to go through vtable
dispatch), there are almost no downsides.

If you get the error:

*   virtual methods with non-empty bodies shouldn't be declared inline

It's because you wrote something like this:
```c++
class BaseClass {
 public:
  virtual void ThisOneIsOK() = default;
  virtual bool ButWeDrawTheLineAtMethodsWithRealImplementation() { return false; }
};
```
And can be fixed by out of lining the method that does work:
```c++
class BaseClass {
 public:
  virtual void ThisIsHandledByTheCompiler() {}
  virtual bool AndThisOneIsDefinedInTheImplementation();
};
```

## Virtual methods and override/final

If you get the error:

*   Overriding method must be marked with 'override' or 'final'.

It is because you are overriding a base class method, yet not annotating that
override with override or final. For example:
```c++
class BaseClass {
 public:
  virtual ~BaseClass() {}
  virtual void SomeMethod() = 0;
};

class DerivedClass : public BaseClass {
 public:
  virtual void SomeMethod();
};
```

The solution? Just tell the compiler what you intend:
```c++
// ...
class DerivedClass : public BaseClass {
 public:
  void SomeMethod() override;
};
```

This allows the compiler to warn you when your intentions stop matching parsed
reality.

## Redundant Virtual specifiers

If you get one of the following errors:

*   'virtual' is redundant; 'override' implies 'virtual'
*   'virtual' is redundant; 'final' implies 'virtual'
*   'override' is redundant; 'final' implies 'override'

Just remove the redundant specifier. Google style is to use only one of virtual,
override, or final per method. For example:
```c++
class BaseClass {
 public:
  virtual void SomeMethod() = 0;
};

class DerivedClass : public BaseClass {
 public:
  virtual void SomeMethod() override final;
};
```
Can be simply written:
```c++
// ...
class DerivedClass : public BaseClass {
 public:
  void SomeMethod() final;
};
```
## Virtual final base class methods

If you get the error:

The virtual method does not override anything and is final; consider making it
non-virtual

That means you have a final method that's not overriding anything. In that case,
consider removing the virtual keyword, since no one can override the method
anyway. For example:
```c++
class BaseClass {
 public:
  virtual void SomeMethod() final;
};
```
Should be written:
```c++
class BaseClass {
 public:
  void SomeMethod();
};
```
## "Using namespace" in a header

You should never have using namespace directives in a header because this can
change the lookup rules in unrelated implementation files.

This is enforced by clang's -Wheader-hygiene warning. This isn't checked by the
style plugin.

If you get the error:

*   using namespace directive in global context in header
            \[-Wheader-hygiene\]

It's because you did something like this in a header:
```c++
using namespace WebKit;
class OurDerivedClass : public WebKitType {
};
```
You can fix this by removing the "using namespace" line and explicitly stating
the namespace:
```c++
class OurDerivedClass : public WebKit::WebKitType {
};
```
## RefCounted types with public destructors

When you declare a type to be referenced counted by making it derive from
base::RefCounted or base::RefCountedThreadSafe, you're indicating that it should
not be destructed until the last reference is Released, either explicitly or by
destructing a scoped_refptr&lt;&gt;. Having a public destructor allows this
contract to be violated, by allowing callers to stack allocate the object, store
it in a scoped_ptr&lt;&gt; instead of a scoped_refptr&lt;&gt;, or to explicitly
delete the object before all references have gone away. Because this can lead to
subtle security bugs, the style checker will warn if class that is derived from
a RefCounted type has either an explicit public destructor or an implicit
destructor, which is automatically public.

If you get the error:

*   Classes that are ref-counted should not have public destructors.

It's because you did something like this:
```c++
class Foo : public base::RefCounted<Foo> {
 public:
  Foo() {}
  ~Foo() {}
};
```
You can fix this by ensuring that the destructor is either protected (if
expecting classes to subclass this base) or private. You'll also need to ensure
that the RefCounted type is friended, as appropriate.
```c++
class Foo : public base::RefCounted<Foo> {
 public:
  Foo() {}
 private:
  friend class base::RefCounted<Foo>;
  ~Foo() {}
};
```
If you get the error:

*   Classes that are ref-counted should have explicit destructors that
            are protected or private.

It's because you did something like this:
```c++
class Foo : public base::RefCounted<Foo> {
// Compiler generated constructors, destructor, and copy operator are public.
};
```
You can fix it by ensuring you explicitly declare a protected or private
destructor:
```c++
class Foo : public base::RefCounted<Foo> {
  // Compiler generated constructors and copy operator are public.
 private:
  friend class base::RefCounted<Foo>;
  ~Foo() {} // Explicit destructor is private.
};
```
## Enumerator max values

If you have an enumerator value that needs to be the max value (useful for
histograms or for IPC), declare your enum as an enum class and name the max
value kMaxValue. The plugin will automatically warn if kMaxValue is no longer
the max value.

For example:
```c++
enum class Foo {
  kOne,
  kTwo,
  kMaxValue = kOne,
};
```
Will produce the error:

*   kMaxValue enumerator does not match max value 0 of other enumerators

This can be fixed by assigning the correct value to kMaxValue:
```c++
enum class Foo {
  kOne,
  kTwo,
  kMaxValue = kTwo,
};
```
