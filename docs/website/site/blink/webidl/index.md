---
breadcrumbs:
- - /blink
  - Blink (Rendering Engine)
page_name: webidl
title: Web IDL in Blink
---

*Blink developers (non-bindings development): for general IDL use, see [Web IDL
interfaces](/developers/web-idl-interfaces); for configuring bindings, see
[Blink IDL Extended Attributes](/blink/webidl/blink-idl-extended-attributes);
for IDL dictionaries use, see [IDL dictionaries in
Blink](https://docs.google.com/document/d/1mRB5zbfHd0lX2Y_Hr7g6grzP_L4Xc6_gBRjL-AE7sY8/edit?usp=sharing).*

[TOC]

## Overview

[​Web IDL](https://heycam.github.io/webidl/) is a language that defines how
Blink interfaces are bound to V8. You need to write IDL files (e.g.
xml_http_request.idl, element.idl, etc) to expose Blink interfaces to those
external languages. When Blink is built, the IDL files are parsed, and the code
to bind Blink implementations to V8 interfaces automatically generated.

This document describes practical information about how the IDL bindings work
and how you can write IDL files in Blink. The syntax of IDL files is fairly well
documented in the [​Web IDL spec](https://heycam.github.io/webidl/), but it is
too formal to read :-) and there are several differences between the Web IDL
spec and the Blink IDL due to implementation issues. For design docs on bindings
generation, see [IDL build](/developers/design-documents/idl-build) and [IDL
compiler](https://chromium.googlesource.com/chromium/src/+/HEAD/third_party/blink/renderer/bindings/IDLCompiler.md).

For Blink developers, the main details of IDLs are the extended attributes,
which control implementation-specific behavior: see [Blink IDL Extended
Attributes](https://chromium.googlesource.com/chromium/src/+/HEAD/third_party/blink/renderer/bindings/IDLExtendedAttributes.md)
for extensive details.

Our goal is to converge Blink's IDL and Web IDL. The grammar is almost
identical; see below.

## Basics of IDL

Here is an example of IDL files:

```none
[CustomToV8]
interface Node {
    const unsigned short ELEMENT_NODE = 1;
    attribute Node parentNode;
    [TreatReturnedNullStringAs=Null] attribute DOMString nodeName;
    [Custom] Node appendChild(Node newChild);
    void addEventListener(DOMString type, EventListener listener, optional boolean useCapture);
};
```

Let us introduce some terms:

*   The above IDL file describes the `Node` **interface**.
*   `ELEMENT_NODE` is a **constant** of the `Node` interface.
*   `parentNode` and `nodeName` are **attributes** of the `Node`
            interface.
*   `appendChild(...)` and `addEventListener(...)` are **operations** of
            the `Node` interface.
*   `type`, `listener` and `useCapture` are **arguments** of the
            `addEventListener` operation.
*   `[CustomToV8]`, `[TreatReturnedNullStringAs=Null]` and `[Custom]`
            are **extended attributes**.

The key points are as follows:

*   An IDL file controls how the bindings code between JavaScript engine
            and the Blink implementation is generated.
*   Extended attributes enable you to control the bindings code more in
            detail.
*   There are ~60 extended attributes, explained in [a separate
            page](https://chromium.googlesource.com/chromium/src/+/HEAD/third_party/blink/renderer/bindings/IDLExtendedAttributes.md).
*   Extended attributes can be specified on interfaces, methods,
            attributes, arguments, and types (but not constants, enums, etc.).

The valid extended attributes depend on where they attach: interfaces and
methods have different extended attributes.

A simple IDL file template looks like:

```none
interface INTERFACE_NAME {
    const unsigned long value = 12345;
    attribute Node node;
    void func(long argument, ...);
};
```

With extended attributes, this looks like:

```none
[
    EXTATTR,
    EXTATTR,
    ...,
] interface INTERFACE_NAME {
    const unsigned long value = 12345;
    [EXTATTR, EXTATTR, ...] attribute Node node;
    [EXTATTR, EXTATTR, ...] void func([EXTATTR, EXTATTR, ...] optional [EXTATTR] long argument, ...);
};
```

## Syntax

Blink IDL is a dialect of Web IDL. The lexical syntax is identical, but the
phrase syntax is slightly different.

Implementation-wise, the lexer and parser are written in
[PLY](http://www.dabeaz.com/ply/) (Python lex-yacc), an implementation of lex
and yacc for Python. A standard-compliant lexer is used (Chromium
[tools/idl_parser/idl_lexer.py](https://code.google.com/p/chromium/codesearch#chromium/src/tools/idl_parser/idl_lexer.py)).
The parser (Blink
[bindings/scripts/blink_idl_parser.py](https://cs.chromium.org/chromium/src/third_party/blink/renderer/bindings/scripts/blink_idl_parser.py))
derives from a standard-compliant parser (Chromium
[tools/idl_parser/idl_parser.py](https://code.google.com/p/chromium/codesearch#chromium/src/tools/idl_parser/idl_parser.py)).

Blink deviations from the Web IDL standard can be seen as the BNF production
rules in the derived parser.

**Style**

Style guidelines are to generally follow [Blink style](/blink/coding-style) for
C++, with a few points highlighted, addenda, and exceptions. These are not
enforced by a pre-submit test, but do assist legibility:

*   Include the [current Blink license
            header](http://www.chromium.org/blink/coding-style#TOC-License) in
            new files
*   For IDL based on standards/specifications:
    *   Include a comment with the URL of the spec (and specific
                section, if possible) where the IDL is defined.
    *   Follow any IDL samples given in specs.
    *   Keep the order of interface and dictionary members the same as
                in the spec.
    *   Document any deviations from the spec with `// TODO(name or bug
                link):` comments
*   4-space indent.
*   Avoid line breaks, notably:
    *   Keeping extended attributes of members (attributes, constants,
                and operations) on the same line as the member.
    *   Generally keep argument lists of methods on the same line as the
                definition. Ok to break if it's v. long or for overloaded
                methods.
    *   For overloaded methods, it is ok to use line breaks to group
                arguments. E.g., if one method has arguments (a, b, c) and the
                other has arguments (a, b, c, d, e, f), it is ok to include a
                line break between c and d, to clarify the grouping.
*   Alphabetize lists of extended attributes.
*   For extended attributes on interface, put each on a separate line
            with a trailing comma, except for the last one. Note that this is
            *not* style used in the standard, which uses a horizontal list on
            the line before the interface. Please omit the `[]` list if it's
            empty. Examples of Blink style:

```none
[
    A,
    B  /* No trailing commas on the last extended attribute */
] interface Foo {
    ...
};
interface Bar {
    ...
};
```

*   No spacing for horizontal alignment, except for lists of constants.
*   For anonymous special operations, leave a space between the type and
            the parenthesize argument list; if you don't, the type looks like a
            function name!

```none
getter DOMString (unsigned long index); // Not: DOMString(unsigned long index) 
```

*   For special operations, the (first) argument to indexed property
            operations should be called `index`, and the (first) argument to
            named property operations should be called `name`; the second
            argument in property setters should be called `value`. For example:

```none
// Indexed property operations 
getter DOMString (unsigned long index); 
setter DOMString (unsigned long index, DOMString value); 
deleter boolean (unsigned long index); 
 
// Named property operations 
getter DOMString (DOMString name); 
setter DOMString (DOMString name, DOMString value); 
deleter boolean (DOMString name);
```

## Semantics

Web IDL exposes an interface to JavaScript, which is implemented in C++. Thus
its semantics bridge these two languages, though it is not identical to either.
Web IDL's semantics are much closer to C++ than to JavaScript – in practice, it
is a relatively thin abstraction layer over C++. Thus C++ implementations are
quite close to the IDL spec, though the resulting interface is somewhat
unnatural from a JavaScript perspective: it behaves differently from normal
JavaScript libraries.

### Types

*See: [Web IDL types](http://heycam.github.io/webidl/#idl-types).*

[Primitive types](http://heycam.github.io/webidl/#dfn-primitive-type) in Web IDL
are very close to fundamental types in C++ (booleans, characters, integers, and
floats), though note that there is no `int` type in Web IDL (specs usually use
`long` instead).

### undefined and null

JavaScript has two special values, `undefined` and `null`, which are often
confusing and do not fit easily into C++. Indeed, precise behavior of
`undefined` in Web IDL has varied over time and is under discussion (see W3C Bug
[23532](https://www.w3.org/Bugs/Public/show_bug.cgi?id=23532) - Dealing with
undefined).

Behavior on `undefined` and `null` **MUST** be tested in web tests, as these can
be passed and are easy to get wrong. If these tests are omitted, there may be
crashes (which will be caught by ClusterFuzz) or behavioral bugs (which will
show up as web pages or JavaScript libraries breaking).

For the purposes of Blink, behavior can be summarized as follows:

*   `undefined` and `null` are valid values for **basic types**, and are
            automatically converted.
    *   Conversion follows [ECMAScript type
                mapping](http://heycam.github.io/webidl/#dfn-convert-ecmascript-to-idl-value),
                which generally implements JavaScript [Type
                Conversion](https://tc39.github.io/ecma262/#sec-type-conversion),
                e.g. [ToBoolean](https://tc39.github.io/ecma262/#sec-toboolean),
                [ToNumber](https://tc39.github.io/ecma262/#sec-tonumber),
                [ToString](https://tc39.github.io/ecma262/#sec-tostring).
    *   They may be converted to different values, notably `"undefined"`
                and `"null"` for `DOMString`.
    *   For numeric types, this can be affected by the extended
                attributes `[Clamp]` and `[EnforceRange]`.
        *   `[Clamp]` changes the value so that it is valid.
        *   `[EnforceRange]` throws a `TypeError` on these invalid
                    values.
*   for **interface types**, `undefined` and `null` are both treated as
            `null`, which maps to `nullptr`.
    *   for nullable interface types, like `Node?`, `null` is a valid
                value, and thus `nullptr` is passed to the C++ implementation
    *   for non-nullable interface types, like `Node` (no `?`), `null`
                is *not* a valid value, and a `TypeError` is thrown, as in
                JavaScript
                [ToObject](http://people.mozilla.org/~jorendorff/es6-draft.html#sec-toobject).
        *   *However,* this nullability check is *not* done by default:
                    it is only done if `[LegacyInterfaceTypeChecking]` is
                    specified on the interface or member (see Bug
                    [249598](https://code.google.com/p/chromium/issues/detail?id=249598):
                    Throw TypeError when null is specified to non-nullable
                    interface parameter)
        *   Thus if `[LegacyInterfaceTypeChecking]` is specified in the
                    IDL, you do *not* need to have a null check in the Blink
                    implementation, as the bindings code already does this, but
                    if `[LegacyInterfaceTypeChecking]` is not specified, you
                    *do* need to have a null check in the Blink implementation.
*   for **dictionary types**, `undefined` and `null` both correspond to
            an empty dictionary
*   for **union types**, `undefined` and `null` are assigned to a type
            that can accept them, if possible: null, empty dictionary, or
            conversion to basic type
*   **function resolution**
    *   `undefined` affects function resolution, both as an optional
                argument and for overloaded operations, basically being omitted
                if trailing (but some exceptions apply). This is complicated
                (see W3C Bug
                [23532](https://www.w3.org/Bugs/Public/show_bug.cgi?id=23532) -
                Dealing with undefined) and not currently implemented.
        Further, note that in some cases one wants different behavior for `f()`
        and `f(undefined)`, which requires an explicit overload, not an optional
        argument; a good example is `Window.alert()`, namely `alert()` vs.
        `alert(undefined)` (see W3C Bug
        [25686](https://www.w3.org/Bugs/Public/show_bug.cgi?id=25686)).
    *   `null` affects function resolution for overloaded operations,
                due to preferring nullable types, but this is the only effect.

### Function resolution

Web IDL has *required* arguments and *optional* arguments. JavaScript does not:
omitted arguments have `undefined` value. In Web IDL, omitting optional
arguments is *not* the same as explicitly passing `undefined`: they call have
different behavior (defined in the spec prose), and internally call different
C++ functions implementing the operation.

Thus if you have the following Web IDL function declaration:

```none
interface A {
    void foo(long x);
 }; 
```

...the JavaScript `a = new A(); a.foo()` will throw a `TypeError`. This is
specified in Web IDL, and thus done by the binding code.

However, in JavaScript the corresponding function can be called without
arguments:

```none
function foo(x) { return x }
 foo() // undefined
```

Note that `foo()` and `foo(undefined)` are almost identical calls (and for this
function have identical behavior): it only changes the value of
`arguments.length`.

To get *similar* behavior in Web IDL, the argument can be explicitly specified
as `optional` (or more precisely, `optional` with `undefined` as a default
value). However, these do *not* need to have the same behavior, and do *not*
generate the same code: the spec may define different behavior for these calls,
and the bindings call the implementing C++ functions with a different number of
arguments, which is resolved by C++ overloading, and these may be implemented by
different functions.

For example, given an optional argument such as:

```none
interface A {
    void foo(optional long x);
 };
```

This results in a = new A(); a.foo() being legal, and calling the underlying
Blink C++ function implementing `foo` with no arguments, while
`a.foo(undefined)` calls the underlying Blink function with one argument.

For *overloaded* operations, the situation is more complicated, and not
currently implemented in Blink (Bug
[293561](https://code.google.com/p/chromium/issues/detail?id=293561)). See the
[overload resolution
algorithm](http://heycam.github.io/webidl/#dfn-overload-resolution-algorithm) in
the spec for details.

Pragmatically, passing `undefined` for an optional argument is necessary if you
wish to specify a value for a later argument, but not earlier ones, but does not
necessarily mean that you mean to pass in `undefined` explicitly; these instead
get the special value "missing".

Passing `undefined` to the last optional argument has unclear behavior for the
value of the argument, but does mean that it resolves it to the operation with
the optional argument, rather than others. (It then prioritizes nullable types
and dictionary types, or unions thereof.) For example:

```none
interface A {
    void foo(optional long x);
    void foo(Node x);
 };
```

This results in a = new A(); a.foo(undefined) resolving to the first `foo`, it
is not clear if the resulting call is `a.foo()`, to `a.foo` with "missing", or
(most likely) `a.foo(undefined)` (here using the first overloaded function): it
affect overload resolution, but perhaps not argument values. Note that
`undefined` is also a legitimate value for the argument of `Node` type, so it
would not be illegal, but the overload resolution algorithm first picks optional
arguments in this case.

Note that Blink code implementing a function can also check arguments, and
similarly, JavaScript functions can check arguments, and access the number of
arguments via `arguments.length`, but these are not specified by the *language*
or checked by bindings.

***Warning:*** `undefined` is a *valid value* for required arguments, and many
interfaces *depend* on this behavior particularly booleans, numbers, and
dictionaries. Explicitly passing `undefined`, as in `a.foo(undefined)`, does
*not* cause a type error (assuming `foo` is unary). It is clearer if the
parameter is marked as `optional` (this changes semantics: the argument can now
also be omitted, not just passed explicitly as `undefined`), but this is not
always done in the spec or in Blink's IDL files.

## File organization

The Web IDL spec treats the Web API as a single API, spread across various IDL
fragments. In practice these fragments are `.idl` files, stored in the codebase
alongside their implementation, with basename equal to the interface name. Thus
for example the fragment defining the `Node` interface is written in n`ode.idl`,
which is stored in the `third_party/blink/renderer/core/dom` directory, and is
accompanied by `node.h` and `node.cc` in the same directory. In some cases the
implementation has a different name, in which case there must be an
`[ImplementedAs=...]` extended attribute in the IDL file, and the `.h/.cc` files
have basename equal to the value of the `[ImplementedAs=...]`.

For simplicity, each IDL file contains a *single* interface or dictionary, and
contains all information needed for that definition, except for dependencies
(below), notably any enumerations, implements statements, typedefs, and callback
functions.

### Dependencies

In principle (as a matter of the Web IDL spec) any IDL file can depend on any
other IDL file, and thus changing one file can require rebuilding all the
dependents. In practice there are 4 kinds of dependencies (since other required
definitions, like enumerations and typedefs, are contained in the IDL file for
the interface):

*   `partial interface` – a single interface can be spread across a
            single main `interface` statement (in one file) and multiple other
            `partial interface` statements, each in a separate file (each
            `partial interface` statement is associated with a single main
            `interface` statement). In this case the IDL file containing the
            partial interface has some other name, often the actual interface
            name plus some suffix, and is generally named after the implementing
            class for the members it contains. From the point of view of spec
            authors and compilation, the members are just treated as if they
            appeared in the main definition. From the point of view of the
            build, these are awkward to implement, since these are incoming
            dependencies, and cannot be determined from looking at the main
            interface IDL file itself, thus requiring a global dependency
            resolution step.
*   `implements` – this is essentially multiple inheritance: an
            interface can implement multiple other interfaces, and a given
            interface can be implemented by multiple other interfaces. This is
            specified by implements statements in the implementing file (these
            are outgoing dependencies), though from the perspective of the build
            the interface → .idl filename of that interface data is required,
            and is global information (because the .idl files are spread across
            the source tree).
*   **Ancestors** – an interface may have a parent, which in turn may
            have a parent. The immediate parent can be determined from looking
            at a single IDL file, but the more distant ancestors require
            dependency resolution (computing an ancestor chain).
*   **Used interfaces (cross dependencies)** – a given interface may
            *use* other interfaces as *types* in its definitions; the contents
            of the used interfaces *may* affect the bindings generated for the
            using interface, though this is often a *shallow dependency* (see
            below).

In practice, what happens is that, when compiling a given interfaces, its
partial interfaces and the other interfaces it implements are merged into a
single data structure, and that is compiled. There is a small amount of data
recording where exactly a member came from (so the correct C++ class can be
called), and a few other extended attributes for switching the
partial/implemented interface on or off, but otherwise it is as if all members
were specified in a single `interface` statement. This is a **deep dependency**
relationship: *any* change in the partial/implemented interface changes the
bindings for the overall (merged) interface, since *all* the data is in fact
used.

Bindings for interfaces in general do *not* depend on their ancestors, beyond
the name of their immediate parent. This is because the bindings just generate a
class, which refers to the parent class, but otherwise is subject to information
hiding. However, in a few cases bindings depend on whether the interface
inherits from some other interface (notably EventHandler or Node), and in a few
cases bindings depend on the extended attributes of ancestors (these extended
attributes are "inherited"; the list is
[compute_dependencies.INHERITED_EXTENDED_ATTRIBUTES](https://cs.chromium.org/chromium/src/third_party/blink/renderer/bindings/scripts/compute_interfaces_info_overall.py?q=INHERITED_EXTENDED_ATTRIBUTES&l=59),
and consists of extended attributes that affect memory management). There is
thus a **shallow dependency** on ancestors, specifically only on the ancestor
chain and on inherited extended attributes, not on the other contents of
ancestors.

On the other hand, the dependencies on used interfaces – so-called **cross
dependencies** – are generally **shallow dependency** relationships: the using
interface does not need to know much about the used interface (currently just
the name of the implementing class, and whether the interface is a callback
interface or not). Thus *almost all changes* in the used interface do not change
the bindings for the using interface: the public information used by other
bindings is minimal. There is one exception, namely the `[PutForwards]` extended
attribute (in standard Web IDL), where the using interface needs to know the
type of an attribute in the used interface. This "generally shallow"
relationship may change in future, however, as being able to inspect the used
interface can simplify the code generator. This would require the using
interface to depend on used interfaces, either rebuilding all using interfaces
whenever a used interface is changed, or clearly specifying or computing the
public information (used by code generator of other interfaces) and depending
only on that.

## IDL extended attribute validator

To avoid bugs caused by typos in extended attributes in IDL files, the extended
attribute validator was introduced to the Blink build flow to check if all the
extended attributes used in IDL files are implemented in the code generator. If
you use an extended attribute not implemented in code generators, the extended
attribute validator fails, and the Blink build fails.

A list of IDL attributes implemented in code generators is described in
[IDLExtendedAttributes.txt](https://cs.chromium.org/chromium/src/third_party/blink/renderer/bindings/IDLExtendedAttributes.txt).
If you want to add a new IDL attribute, you need to

1.  add the extended attribute to
            Source/bindings/IDLExtendedAttributes.txt.
2.  add the explanation to the extended attributes document.
3.  add test cases to run-bindings-tests (explained below).

Note that the validator checks for known extended attributes and their arguments
(if any), but does not enforce correct use of the the attributes. A warning will
not be issued if, for example, `[Clamp]` is specified on an interface.

## Tests

### Reference tests (run-bindings-tests)

[third_party/blink/tools/run_bindings_tests.py](https://cs.chromium.org/chromium/src/third_party/blink/tools/run_bindings_tests.py)
tests the code generator, including its treatment of extended attributes.
Specifically, run_bindings_tests.py compiles the IDL files in
[bindings/tests/idls](https://cs.chromium.org/chromium/src/third_party/blink/renderer/bindings/tests/idls/),
and then compares the results against reference files in
[bindings/tests/results](https://cs.chromium.org/chromium/src/third_party/blink/renderer/bindings/tests/results/).
For example, run_bindings_tests.py reads test_object.idl, and then compares the
generated results against v8_test_object.h and v8_test_object.cc, reporting any
differences.

If you change the behavior of the code generator or add a new extended
attribute, please add suitable test cases, preferably *reusing* existing IDL
files (this is to minimize size of diffs when making changes to overall
generated bindings). You can reset the run-bindings-tests results using the
--reset-results option:

```none
third_party/blink/tools/run_bindings_tests.py --reset-results
```

run_bindings_tests.py is run in a presubmit script for any changes to
Source/bindings: this requires you to update test results when you change the
behavior of the code generator, and thus if test results get out of date, the
presubmit test will fail: you won't be able to upload your patch via git-cl, and
the CQ will refuse to process the patch.

The objective of run-bindings-tests is to show you and reviewers how the code
generation is changed by your patch. **If you change the behavior of code
generators, you need to update the results of run-bindings-tests.**

Despite these checks, sometimes the test results can get out of date; this is
primarily due to dcommitting or changes in real IDL files (not in
Source/bindings) that are used in test cases. If the results are out of date
*prior* to your CL, please rebaseline them separately, before committing your
CL, as otherwise it will be difficult to distinguish which changes are due to
your CL and which are due to rebaselining due to older CLs.

Note that using real interfaces in test IDL files means changes to real IDL
files can break run-bindings-tests (e.g., Blink
[r174804](https://src.chromium.org/viewvc/blink?revision=174804&view=revision)/CL
[292503006](https://codereview.chromium.org/292503006/): Oilpan: add
\[WillBeGarbageCollected\] for Node., since Node is inherited by test files).
This is ok (we're not going to run run_bindings_tests.py on every IDL edit, and
it's easy to fix), but something to be aware of.

It is also possible for run_bindings_tests.py to break for other reasons, since
it use the developer's local tree: it thus may pass locally but fail remotely,
or conversely. For example, renaming Python files can result in outdated
bytecode (.pyc files) being used locally and succeeding, even if
run_bindings_tests.py is incompatible with current Python source (.py), as
discussed and fixed in CL
[301743008](https://codereview.chromium.org/301743008/).

### Behavior tests

To test behavior, use [web
tests](https://chromium.googlesource.com/chromium/src/+/HEAD/docs/testing/web_tests.md),
most simply actual interfaces that use the behavior you're implementing. If
adding new behavior, it's preferable to make code generator changes and the
first actual use case in the same CL, so that it is properly tested, and the
changes appear in the context of an actual change. If this makes the CL too
large, these can be split into a CG-only CL and an actual use CL, committed in
sequence, but unused features should not be added to the CG.

For general behavior, like type conversions, there are some internal tests, like
[bindings/webidl-type-mapping.html](https://cs.chromium.org/chromium/src/third_party/WebKit/LayoutTests/bindings/webidl-type-mapping.html),
which uses
[testing/type_conversions.idl](https://cs.chromium.org/chromium/src/third_party/blink/renderer/core/testing/type_conversions.idl).
There are also some other IDL files in
[testing](https://cs.chromium.org/chromium/src/third_party/blink/renderer/core/testing/),
like
[testing/internals.idl](https://cs.chromium.org/chromium/src/third_party/blink/renderer/core/testing/internals.idl).

## Where is the bindings code generated?

By reading this document you can learn how extended attributes work. However,
the best practice to understand extended attributes is to try to use some and
watch what kind of bindings code is generated.

If you change an IDL file and rebuild (e.g., with ninja or Make), the bindings
for that IDL file (and possibly others, if there are dependents) will be
rebuilt. If the bindings have changed (in ninja), or even if they haven't (in
other build systems), it will also recompile the bindings code. Regenerating
bindings for a single IDL file is very fast, but regenerating all of them takes
several minutes of CPU time.

In case of xxx.idl in the Release build, the bindings code is generated in the
following files ("Release" becomes "Debug" in the Debug build).

```none
out/Release/gen/third_party/blink/renderer/bindings/{core,modules}/v8_xxx.{h,cc}
```

## Limitations and improvements

A few parts of the Web IDL spec are not implemented; features are implemented on
an as-needed basis. See
[component:Blink&gt;Bindings](https://bugs.chromium.org/p/chromium/issues/list?q=component:Blink%3EBindings)
for open bugs; please feel free to file bugs or contact bindings developers
([members of
blink-reviews-bindings](https://groups.google.com/a/chromium.org/forum/#!members/blink-reviews-bindings),
or
[bindings/OWNERS](https://cs.chromium.org/chromium/src/third_party/blink/renderer/bindings/OWNERS))
if you have any questions, problems, or requests.

Bindings generation can be controlled in many ways, generally by adding an
extended attribute to specify the behavior, sometimes by special-casing a
specific type, interface, or member. If the existing [extended
attributes](https://chromium.googlesource.com/chromium/src/+/HEAD/third_party/blink/renderer/bindings/IDLExtendedAttributes.md)
are not sufficient (or buggy), please file a bug and contact bindings
developers!

Some commonly encountered limitations and suitable workarounds are listed below.
Generally limitations can be worked around by using custom bindings, but these
should be avoided if possible. If you need to work around a limitation, please
put a `TODO` with the bug number (as demonstrated below) in the IDL so that we
can remove the hack when the feature is implemented.

### Syntax error causes infinite loop

Some syntax errors cause the IDL parser to enter an infinite loop (Bug
[363830](https://code.google.com/p/chromium/issues/detail?id=363830)). Until
this is fixed, if the compiler hangs, please terminate the compiler and check
your syntax.

### Type checking

The bindings do not do full type checking (Bug
[321518](https://code.google.com/p/chromium/issues/detail?id=321518)). They do
some type checking, but not all. Notably nullability is not strictly enforced.
See `[TypeChecking]` under **undefined and null** above to see how to turn on
more standard type checking behavior for interfaces and members.

## Bindings development

### Mailing List

## If working on bindings, you likely wish to join the [blink-reviews-bindings](https://groups.google.com/a/chromium.org/forum/#!forum/blink-reviews-bindings) mailing list.

### See also

*   [Web IDL interfaces](/developers/web-idl-interfaces) – overview
            how-to for Blink developers
*   [Blink IDL Extended
            Attributes](https://chromium.googlesource.com/chromium/src/+/HEAD/third_party/blink/renderer/bindings/IDLExtendedAttributes.md)
            – reference for Blink developers and bindings developers
*   [IDL build](/developers/design-documents/idl-build) – design doc
*   [IDL compiler](/developers/design-documents/idl-compiler) – design
            doc

---

Derived from: <http://trac.webkit.org/wiki/WebKitIDL> *Licensed under
[BSD](http://www.webkit.org/coding/bsd-license.html):*

BSD License

Copyright (C) 2009 Apple Inc. All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS “AS IS” AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
