---
breadcrumbs:
- - /developers
  - For Developers
page_name: web-idl-interfaces
title: Web IDL interfaces
---

[TOC]

Web interfaces – exposed as JavaScript objects – are generally specified in [Web
IDL](http://heycam.github.io/webidl/) (Interface Definition Language), a
declarative language (sometimes written without the space as WebIDL). This is
the language used in standard specifications, and Blink uses IDL files to
specify the interface and generate JavaScript bindings (formally, C++ code that
the V8 JavaScript virtual machine uses to call Blink itself). [Web IDL in
Blink](/blink/webidl) is close to the standard, and the resulting bindings use
standard conventions to call Blink code, but there are additional features to
specify implementation details, primarily [Blink IDL extended
attributes](https://chromium.googlesource.com/chromium/src/+/HEAD/third_party/blink/renderer/bindings/IDLExtendedAttributes.md).

To implement a new Web IDL interface in Blink:

*   **Interface:** write an IDL file: `foo.idl`
*   **Implementation:** write a C++ file and header: `foo.cc, foo.h`
*   **Build:** add files to the build: edit
            [idl_in_core.gni](https://source.chromium.org/chromium/chromium/src/+/HEAD:third_party/blink/renderer/bindings/idl_in_core.gni)/[idl_in_modules.gni](https://source.chromium.org/chromium/chromium/src/+/HEAD:third_party/blink/renderer/bindings/idl_in_modules.gni)
            and
            [generated_in_core.gni](https://source.chromium.org/chromium/chromium/src/+/HEAD:third_party/blink/renderer/bindings/generated_in_core.gni)/[generated_in_modules.gni](https://source.chromium.org/chromium/chromium/src/+/HEAD:third_party/blink/renderer/bindings/generated_in_modules.gni)
    (for the old bindings generator, edit also
    [core_idl_files.gni](https://source.chromium.org/chromium/chromium/src/+/HEAD:third_party/blink/renderer/core/core_idl_files.gni)/[modules_idl_files.gni](https://source.chromium.org/chromium/chromium/src/+/HEAD:third_party/blink/renderer/modules/modules_idl_files.gni)
    and
    [third_party/blink/renderer/bindings/core/v8/generated.gni](https://source.chromium.org/chromium/chromium/src/+/HEAD:third_party/blink/renderer/bindings/core/v8/generated.gni)/[third_party/blink/renderer/bindings/modules/v8/generated.gni](https://source.chromium.org/chromium/chromium/src/+/HEAD:third_party/blink/renderer/bindings/modules/v8/generated.gni)
    if necessary.)
*   **Tests:** write unit tests ([web
            tests](https://chromium.googlesource.com/chromium/src/+/HEAD/docs/testing/web_tests.md))
            in
            [web_tests](https://source.chromium.org/chromium/chromium/src/+/HEAD:third_party/blink/web_tests/)

The bulk of the work is the implementation, secondarily tests. Writing an IDL
file should require minimal work (ideally just copy-and-paste the spec),
assuming nothing unusual is being done, and the build can be forgotten about
once you've set it up. Details follow.

If you do not intend to expose the IDL to mobile platforms e.g. android you
can exclude the source files and the idl files by checking for the `target_os` buildflag.
Example [idl_in_modules.gni](https://source.chromium.org/chromium/chromium/src/+/main:third_party/blink/renderer/bindings/idl_in_modules.gni;drc=105716ef02f9e386c61848f649664d2d0f59ad52;l=1171).

If excluding Android, also update [not-webview-exposed.txt](https://source.chromium.org/chromium/chromium/src/+/main:android_webview/tools/system_webview_shell/test/data/webexposed/not-webview-exposed.txt) with the excluded IDLs.

## Web IDL

*   Find spec
*   Create a new file called foo.idl in the same directory as you will
            implement the interface, generally
            third_party/blink/renderer/core/\* or
            third_party/blink/renderer/modules/\*

The initial IDL file should contain:

*   License header
*   Link to the spec
*   Link to tracking bug for implementing the interface
*   IDL "fragment" copied from the spec

See [Blink IDL: Style](/blink/webidl#TOC-Style) for style guide.

IDL files contain two types of data:

*   Blink *interface* – behavior; ideally should agree with spec, but
            differences from spec should be reflected in the IDL itself
*   Blink *implementation* – internal-use data, like function names or
            implementation-specific flags, like memory management.

Note that if Blink behavior differs from the spec, the Blink IDL file should
reflect *Blink* behavior. This makes interface differences visible, rather than
hiding them in the C++ implementation or bindings generator.

Also as a rule, nop data should *not* be included: if Blink (bindings generator)
ignores an IDL keyword or extended attribute, do not include it, as it suggests
a difference in behavior when there is none. If this results in a difference
from the spec, this is *good*, as it makes the difference in behavior visible.

Initially you likely want to comment out all attributes and operations,
uncommenting them as you implement them.

### Nulls and non-finite numbers

Two points to be careful of, and which are often incorrect in specs,
particularly older specs, are *nullability* and *non-finite* values (infinities
and NaN). These are both to ensure correct type checking. If these are incorrect
in the spec – for example, a prose specifying behavior on non-finite values, but
the IDL not reflecting this – please file a spec bug upstream, and link to it in
the IDL file.

If null values are valid (for attributes, argument types, or method return
values), the type MUST be marked with a ? to indicate nullable, as in `attribute
Foo? foo;`

Similarly, IEEE floating point allows non-finite numbers (infinities and NaN);
if these are valid, the floating point type – `float` or `double` – MUST be
marked as `unrestricted` as in `unrestricted float` or `unrestricted double` –
the bare `float` or `double` means *finite* floating point.

Ref: [double](http://heycam.github.io/webidl/#idl-double), [unrestricted
double](http://heycam.github.io/webidl/#idl-unrestricted-double), [Type mapping:
double](http://heycam.github.io/webidl/#es-double), [Type mapping: unrestricted
double](http://heycam.github.io/webidl/#es-unrestricted-double)

### Union types

Many older specs use overloading when a union type argument would be clearer.
Please match spec, but file a spec bug for these and link to it. For example:

```none
// FIXME: should be void bar((long or Foo) foo);　https://www.w3.org/Bugs/Public/show_bug.cgi?id=123
void bar(long foo);
void bar(Foo foo);
```

Also, beware that you can't have multiple nullable arguments in the
distinguishing position in an overload, as these are not distinguishing (what
does `null` resolve to?). This is best resolved by using a union type if
possible; otherwise make sure to mark only one overload as having a nullable
argument in that position.

Don't do this:

```none
void zork(Foo? x);
void zork(Bar? x); // What does zork(null) resolve to?
```

Instead do this:

```none
void zork(Foo? x);
void zork(Bar x);
```

...but preferably this:

```none
void zork((Foo or Bar)? x);
```

### Extended attributes

You will often need to add Blink-specific extended attributes to specify
implementation details.

**Please comment extended attributes – *why* do you need special behavior?**

### Bindings

See [Web IDL in Blink](/blink/webidl).

## C++

Bindings code assumes that a C++ class exists for each interface, with methods
for each attribute or operation (with some exceptions). Attributes are
implemented as
[properties](http://en.wikipedia.org/wiki/Property_(programming)), meaning that
while in the JavaScript interface these are read and written as attributes, in
C++ these are read and written by getter and setter methods.

### Names

The class and methods have default names, which can be overridden by the
`[ImplementedAs]` extended attribute; this is **strongly discouraged,** and
method names should align with the spec unless there is very good reason for
them to differ (this is sometimes necessary when there is a conflict, say when
inheriting another interface).

Given an IDL file foo.idl:

```none
interface Foo {
    attribute long a;
    attribute DOMString cssText;
    void f();
    void f(long arg);
    void g(optional long arg);
};
```

...a minimal header file foo.h illustrating this is:

```none
class Foo {
 public:
  int32_t a();
  void setA(int32_t value);
  String cssText();
  void setCSSText(const String&);
  void f();
  void f(int32_t arg);
  void g();
  void g(int32_t arg);
};
```

*   IDL interfaces assume a class of the same name: `class Foo`.
*   IDL attributes call a getter of the same name, and setter with set
            prepended and capitalization fixed: `a()` and `setA()`. This
            correctly capitalizes acronyms, so the setter for cssText is
            `setCSSText()`.
*   IDL operations call a C++ method of the same name: `f()`.
*   Web IDL overloading and IDL optional arguments *without* default
            values map directly to C++ overloading (optional arguments *without*
            default values correspond to an overload either including or
            excluding the argument).
*   IDL optional arguments *with* default values map to C++ calls with
            these values filled in, and thus do not require C++ overloading.
    *   C++ default values SHOULD NOT be used unless necessary.
    *   There are various complicated corner cases, like non-trailing
                optional arguments without defaults, like
        *   `foo(optional long x, optional long y = 0);`

### Type information ("ScriptWrappable")

Blink objects that are visible in JavaScript need type information,
fundamentally because JavaScript is [dynamically
typed](http://en.wikipedia.org/wiki/Dynamic_typing) (so *values* have type),
concretely because the bindings code uses [type
introspection](http://en.wikipedia.org/wiki/Type_introspection) for [dynamic
dispatch](http://en.wikipedia.org/wiki/Dynamic_dispatch) (function resolution of
bindings functions): given a C++ object (representing the implementation of a
JavaScript object), accessing it from V8 requires calling the correct C++
binding methods, which requires knowing its JavaScript type (i.e., the IDL
interface type).

Blink does not use C++ [run-time type
information](http://en.wikipedia.org/wiki/Run-time_type_information) (RTTI), and
thus the type information must be stored separately.

There are various ways this is done, most simply (for Blink developers) by the
C++ class inheriting `ScriptWrappable` and placing `DEFINE_WRAPPERTYPEINFO` in
the class declaration. Stylistically `ScriptWrappable` or its inheritance MUST
be the first class.

Explicitly:

foo.h:

```none
#ifndef third_party_blink_renderer_core_foo_foo_h
#define third_party_blink_renderer_core_foo_foo_h
#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"
namespace blink {
class Foo : public ScriptWrappable /* maybe others */ {
  DEFINE_WRAPPERTYPEINFO();
 public:
  // ...
};
} // namespace blink
#endif third_party_blink_renderer_core_foo_foo_h
```

### Garbage Collection

## See [Blink GC API
reference](https://chromium.googlesource.com/chromium/src/+/HEAD/third_party/blink/renderer/platform/heap/BlinkGCAPIReference.md)

## Build

You need to list the `.idl` file and `.h/.cc` files in the correct GN variable
so that they will be built (bindings generated, Blink code compiled.)

**Now we are replacing bindings code generators from version 1 to 2, and thus
you have to work for both generators.**

There are 3 dichotomies in these `.idl` files, which affect where you list them
in the build:

*   core vs. modules – which component they are in
*   main interface vs. dependency – partial interfaces and interface
            mixins do not have individual bindings (`.h/.cc`) generated
*   testing or not – testing interfaces do not appear in the aggregate
            bindings

For the code generator v1, see comments in
[core_idl_files.gni](https://source.chromium.org/chromium/chromium/src/+/HEAD:third_party/blink/renderer/core/core_idl_files.gni)
and/or
[modules_idl_files.gni](https://source.chromium.org/chromium/chromium/src/+/HEAD:third_party/blink/renderer/modules/modules_idl_files.gni)
in which GN variable your IDL file should be listed. And if your IDL file
defines a callback function or a new union type, see [generated.gni for core
component](https://source.chromium.org/chromium/chromium/src/+/HEAD:third_party/blink/renderer/bindings/core/v8/generated.gni)
or [generated.gni for modules
component](https://source.chromium.org/chromium/chromium/src/+/HEAD:third_party/blink/renderer/bindings/modules/v8/generated.gni)
in which GN variable generated files should be listed.

For the code generator v2, see
[idl_in_core.gni](https://source.chromium.org/chromium/chromium/src/+/HEAD:third_party/blink/renderer/bindings/idl_in_core.gni)
or
[idl_in_modules.gni](https://source.chromium.org/chromium/chromium/src/+/HEAD:third_party/blink/renderer/bindings/idl_in_modules.gni)
for IDL files, and
[generated_in_core.gni](https://source.chromium.org/chromium/chromium/src/+/HEAD:third_party/blink/renderer/bindings/generated_in_core.gni)
or
[generated_in_modules.gni](https://source.chromium.org/chromium/chromium/src/+/HEAD:third_party/blink/renderer/bindings/generated_in_modules.gni)
for generated files.

## Tests

Make sure to test:

*   default objects – create a new object and pass as argument or assign
            to an attribute
*   `undefined/null` – if passed to nullable arguments or set to
            nullable attributes, should not throw; if passed to non-nullable
            arguments or set to non-nullable attributes, should throw but not
            crash

## Subtyping

There are three mechanisms for subtyping in IDL:

*   inheritance: `interface A : B { ... };`
*   [includes
            statements](https://heycam.github.io/webidl/#includes-statement): `A
            includes B;`
*   partial interface: `partial interface A { ... };`

The corresponding C++ implementations are as follows, here illustrated for
`attribute T foo;`

*   inheritance: handled by JavaScript, but often have corresponding C++
            inheritance; one of:
    *   `class A { ... };`
    *   `class A : B { ... };`
*   includes: C++ class must implement methods, either itself or via
            inheritance; one of:
    *   `class A { public: T foo(); void setFoo(...); ... };`
    *   `class A : B { ... };`
*   partial interface: implemented as static member functions in an
            unrelated class:
    *   `class B { static T foo(A& a); static void setFoo(A& a, ...);
                ... };`

IDL files SHOULD agree with spec, and almost always MUST do so. It is not ok to
change the kind of subtyping or move members between interfaces, and violations
SHOULD or MUST be fixed:

*   Inheritance is visible to JavaScript (in the prototype chain), so it
            MUST be correct (it is NOT ok to have non-spec inheritance
            relationships).
*   The distinction between "member of (main) interface definition,
            member of included interface mixin, member of partial interface" is
            not visible in JavaScript (these are all just properties of the
            prototype object), so while this SHOULD agree with spec (so Blink
            IDL agrees with IDL in the spec), this is not strictly required.
*   The distinction between "member of (child) interface" and "member of
            parent interface" is visible in JavaScript (as property on prototype
            object corresponding to (child) interface vs. property on prototype
            object corresponding to parent interface), and thus MUST be correct
            (it is NOT ok to move members between an interface and a parent if
            this disagrees with spec).

#### Technical details

While members of an interface definition, members of interface mixin, and
members of partial interfaces are identical for JavaScript, partial interface
members – and members of certain interface mixin, namely those with the
`[TreatAsPartial]` extended attribute – are treated differently internally in
Blink (see below).

Inheritance and implements are both *interface inheritance*. JavaScript has
single inheritance, and IDL inheritance corresponds to JavaScript inheritance,
while IDL `includes` provides multiple inheritance in IDL, which does not
correspond to inheritance in JavaScript.

In both cases, by spec, members of the inherited interface or interface mixin
must be implemented on the JavaScript object implementing the interface.
Concretely, members of inherited interfaces are implemented as properties on the
prototype object of the parent interface, while members of interface mixins are
implemented as properties of the implementing interface.

In C++, members of an interface definition and members of implemented interfaces
are implemented on the C++ object (referred to as the parameter or variable
`impl`) implementing the JavaScript object. Specifically this is done in the
Blink class corresponding to the IDL interface *or a base class* – the C++
hierarchy is invisible to both JavaScript and the bindings.

Implementation-wise, inheritance and implements differ in two ways:

*   Inheritance sets the prototype object (this is visible in JavaScript
            via `getPrototypeOf`); `implements` does not.
*   Bindings are not generated for inherited members (JavaScript
            dispatches these to the parent prototype), but *are* generated for
            implemented members.

For simplicity, in the wrapper (used by V8 to call Blink) the bindings just
treat members of implemented interfaces and partial interfaces as if they were
part of the main interface: there is no multiple inheritance in the bindings
implementation.

If (IDL) interface A inherits from interface B, then usually (C++) class A
inherits from class B, meaning that:

```none
interface A : B { /* ... */ };
```

is *usually* implemented as:

```none
class A : B { /* ... */ };
```

...or perhaps:

```none
class A : C { /* ... */ };
class C : B { /* ... */ };
```

However, the bindings are agnostic about this, and simply set the prototype in
the wrapper object to be the inherited interface (concretely, sets the
parentClass attribute in the WrapperTypeInfo of the class's bindings). Dispatch
is thus done in JavaScript.

"A includes B;" should mean that members declared in (IDL) interface mixin B are
members of (C++) classes implementing A.

Partial interfaces formally are type extension (*external* type extension, since
specified in a separate place from the original definition), and in principle
are simply part of the interface, just defined separately, as a convenience for
spec authors. However in practice, members of partial interfaces are *not*
assumed to be implemented on the C++ object (`impl`), and are not defined in the
Blink class implementing the interface. Instead, they are implemented as static
members of a separate class, which take `impl` as their first argument. This is
done because in practice, partial interfaces are type extensions, which often
only used in subtypes or are deactivated (via conditionals or as [runtime
enabled features](/blink/runtime-enabled-features)), and we do not want to bloat
the main Blink class to include these.

Further, in some cases we must use type extension (static methods) for
implemented interfaces as well. This is due to componentization in Blink (see
[Browser
Components](http://www.chromium.org/developers/design-documents/browser-components)),
currently `core` versus `modules.` Code in `core` cannot inherit from code in
`modules,` and thus if an interface in `core` implements an interface in
`modules,` this must be implemented via type extension (static methods in
`modules`). This is an exceptional case, and indicates that Blink's internal
layering (componentization) disagrees with the layering implied by the IDL
specs, and formally should be resolved by moving the relevant interface from
`modules` to `core.` This is not always possible or desirable (for internal
implementation reasons), and thus static methods can be specified via the
`[TreatAsPartial]` extended attribute on the implemented interface.

## Inheritance and code reuse

IDL has single inheritance, which maps directly to JavaScript inheritance
(prototype chain). C++ has multiple inheritance, and the two hierarchies need
not be related.

IDL has 3 mechanisms for combining interfaces:

*   (Single) inheritance
*   implements
*   partial interface

## See also

Other Blink interfaces, not standard Web IDL interfaces:

*   [Public C++ API](/blink/public-c-api): C++ API used by C++ programs
            embedding Blink (not JavaScript), including the (C++) "web API"
*   [Implementing a new extension
            API](/developers/design-documents/extensions/proposed-changes/creating-new-apis):
            Chrome extensions (JavaScript interfaces used by extensions), also
            use a dialect of Web IDL for describing interfaces

## External links

For reference, documentation by other projects.

*   Mozilla Developer Network (MDN)
    *   [Web API
                reference](https://developer.mozilla.org/en-US/docs/Web/Reference/API)
    *   [WebIDL
                bindings](https://developer.mozilla.org/en-US/docs/Mozilla/WebIDL_bindings)
    *   [IDL interface
                rules](https://developer.mozilla.org/en-US/docs/Developer_Guide/Interface_development_guide/IDL_interface_rules)
*   W3C Wiki: [Web IDL](http://www.w3.org/wiki/Web_IDL)
