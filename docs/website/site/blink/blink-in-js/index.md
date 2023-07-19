---
breadcrumbs:
- - /blink
  - Blink (Rendering Engine)
page_name: blink-in-js
title: Blink-in-JavaScript
---

## NOTE: THIS DOCUMENT IS NO LONGER VALID

## It is left available out of historical interest.

## Overview

***Blink-in-JavaScript*** is a mechanism to enable Blink developers to implement
DOM features in JavaScript (instead of C++). The goal of Blink-in-JS is to
improve web layering by implementing high-level DOM features on top of existing
web-exposed APIs. You can learn the design in [this design
document](https://docs.google.com/a/google.com/document/d/13cT9Klgvt_ciAR3ONGvzKvw6fz9-f6E0FrqYFqfoc8Y/edit#)
and [this
slide](https://docs.google.com/a/google.com/presentation/d/1XvZdAF29Fgn19GCjDhHhlsECJAfOR49tpUFWrbtQAwU/edit#slide=id.g3840fe06e_00).
If you are interested in the security model, you can also look at [this
document](https://docs.google.com/a/google.com/document/d/1AtnKpzQaSY3Mo1qTm68mt_3DkcZzrp_jcGS92a3a1UU/edit).

## Guideline

When you want to implement a feature in Blink-in-JS, you need to follow the
following guideline.

1.  If you plan to implement a feature in Blink-in-JS, you need to send
            an Intent-to-Implement to blink-dev@. The Intent-to-Implement should
            explain the advantages of the Blink-in-JS and have a list of private
            script APIs that will be required for the Blink-in-JS. (\*\*)
2.  You are strongly discouraged to use private script APIs. If you need
            to add a private script API, you need to provide a justification for
            the API and get an LGTM from one API owner (\*\*\*). The API must
            meet either of the following conditions:

    *   The API is a missing part of the current web and going to be
                exposed to the web in the future.
    *   The API is for supporting a will-be-deprecated feature.

(\*) See [this
slide](https://docs.google.com/a/chromium.org/presentation/d/1-5wpqeIltM40DAZdQBhbqnzOZuFNezS4eUfx1T4YV50/edit#slide=id.g437b0e633_00)
for a background of this guideline.

(\*\*) The fact that we’re lowering the priority of Blink-in-JS means that we
are going to be conservative about accepting your Intent-to-Implement. It is
important to consider if your Blink-in-JS work has higher impact than other
projects you could work on alternately. The Intent-to-Implement must provide
clear advantages of why you think it is important for Blink to implement the
feature in Blink-in-JS. For example, the following Intent-to-Implements look
appealing:

*   I need to implement MediaControls for Android. There are two options
            for us: implement it in C++ or implement it in JS. Given that
            MediaControls is a self-contained and high-level feature that can be
            developed on top of existing web-exposed API, I propose to implement
            MediaControls in Blink-in-JS. It is easier to develop and maintain.
*   XMLViewer is already written in JS and it is invoked using direct V8
            APIs. Given that Blink-in-JS has a safer mechanism to run JS in
            Blink, I propose to move XMLViewer to Blink-in-JS.
*   I plan to make a substantial restructuring of the rendering system
            for performance. I noticed that marquee-specific code in the
            rendering system prevents us from making the restructuring. Given
            that marquee is an out-dated feature, I want to just factor out the
            implementation to Blink-in-JS instead of supporting the
            marquee-specific code in the new rendering system.

On the other hand, the following Intent-to-Implement would not be appealing:

*   HTMLFormControls are self-contained and it is not hard to move the
            implementation to Blink-in-JS. The benefit is that we can remove a
            bunch of code from C++. However, I’m not sure if HTMLFormControls is
            an actively developed area and thus it’s not clear at this point how
            helpful it is for on-going Blink projects.

(Note: These are just examples and not intending to imply go or non-go of
MediaControls-in-JS etc)

(\*\*\*) It is a good idea to ask review for dglazkov@ (API owner), jochen@ (API
owner), haraken@ and area experts of the API.

## Basics

The most common usage of Blink-in-JS is to implement DOM features defined in an
IDL file in JavaScript. For example, consider to implement a &lt;marquee&gt; tag
in Blink-in-JS.

In this case, what you need to do is just to:

*   Add \[ImplementedInPrivateScript\] IDL extended attributes to DOM
            attributes/methods in an IDL file.
*   Implement the DOM attributes/methods in JavaScript (This JavaScript
            file is called a ***private script***.)

Specifically, you first need to add \[ImplementedInPrivateScript\] IDL extended
attributes to DOM attributes/methods in
[HTMLMarqueeElement.idl](https://code.google.com/p/chromium/codesearch#chromium/src/third_party/WebKit/Source/core/html/HTMLMarqueeElement.idl&q=htmlmarqueeelement.idl&sq=package:chromium&type=cs).

```none
interface HTMLMarqueeElement : HTMLElement {
  [ImplementedInPrivateScript] void start();
  [ImplementedInPrivateScript] void stop();
  [ImplementedInPrivateScript] attribute long loop;
  [ImplementedInPrivateScript] attribute long scrollAmount;
  ...;
};
```

Second, you need to implement the DOM attributes/methods in
[HTMLMarqueeElement.js](https://code.google.com/p/chromium/codesearch#chromium/src/third_party/WebKit/Source/core/html/HTMLMarqueeElement.js&sq=package:chromium&type=cs).

```none
installClass("HTMLMarqueeElement", function(HTMLMarqueeElementPrototype)) {
  HTMLMarqueeElementPrototype.start = function() {
    // Implement the start() method.
    // |this| object is equal to the wrapper of the <marquee> element.
  }
  Object.defineProperty(HTMLMarqueeElementPrototype, 'loop', {
      get: function() {
        // Implement the getter of the loop attribute.
        // |this| object is equal to the wrapper of the <marquee> element.
      },
      set: function(value) {
        // Implement the setter of the loop attribute.
        // |this| object is equal to the wrapper of the <marquee> element.
      },
  });
  ...; // Implement other DOM attributes/methods.
};
```

That's it. Then the IDL compiler auto-generates the binding code that connects
user's script with the JavaScript functions defined in the private script.

The important points are as follows:

*   A private script runs in a dedicated isolated world. For security
            reasons, no JavaScript objects nor DOM wrappers are shared between
            the private script and user's script. This restriction is needed to
            prevent the private script from leaking confidential information to
            user's script.
*   To force the restriction, the type of arguments you can pass from
            user's script to the private script is limited to JavaScript
            primitive types (e.g., int, double, string, boolean etc) and DOM
            wrappers. Similarly, the type you can return from the private script
            back to user's script is limited to JavaScript primitive types and
            DOM wrappers. You cannot pass JavaScript functions, JavaScript
            objects, Promises etc.
*   |global| is a window object of the private script. You can use the
            |global| object as you like, but note that the |global| object is
            shared among all private scripts. For example, HTMLMarqueeElement.js
            and XSLT.js share the same |global| object. Thus you should not
            cache data specific to your private script onto the |global| object.
*   |HTMLMarqueeElementPrototype| is a prototype object of the private
            script. Your main work is to define DOM attributes/methods on the
            prototype object.
*   |this| object is equal to the wrapper of the &lt;marquee&gt; element
            in the isolated world for private scripts. Due to the security
            isolation, |this| object is a different wrapper from the wrapper of
            the &lt;marquee&gt; element in the main world. You can cache
            whatever you want onto the |this| object. It is guaranteed that the
            |this| object is not accessible from user's script.

    By default, the IDL compiler assumes that you have HTMLMarqueeElement.h in
    Blink. If you don't want to have HTMLMarqueeElement.h, you need to add
    \[NoImplHeader\] IDL attribute on the interface.

*   |this| object has the following prototype chain: |this| --&gt;
            HTMLMarqueeElementPrototype --&gt; HTMLMarqueeElement.prototype
            --&gt; HTMLElement.prototype --&gt; Element.prototype --&gt;
            Node.prototype --&gt; ....

For more details, you can look at how
[HTMLMarqueeElement.js](https://code.google.com/p/chromium/codesearch#chromium/src/third_party/WebKit/Source/core/html/HTMLMarqueeElement.js&sq=package:chromium&type=cs)
and
[PrivateScriptTest.js](https://code.google.com/p/chromium/codesearch#chromium/src/third_party/WebKit/Source/core/testing/PrivateScriptTest.js&q=privatescripttest.js&sq=package:chromium&type=cs)
are implemented.

## Details

By using the \[ImplementedInPrivateScript\] IDL extended attribute, you can
implement DOM features exposed to the web in a private script. However, this is
sometimes not sufficient to implement real-world DOM features in a private
script. Sometimes you will need to invoke a private script from C++. Sometimes
you will need to implement internal DOM attributes/methods that are only exposed
to private scripts.

In general, Blink-in-JS supports the following four kinds of APIs.

*   \[user's script & private script =&gt; C++\]: This is a normal DOM
            attribute/method (where Blink-in-JS is not involved). The DOM
            attribute/method is implemented in C++, and the DOM attribute/method
            is exposed to both user's script and private scripts.
*   \[user's script & private script =&gt; private script\]: This is the
            most common usage of Blink-in-JS explained above. The DOM
            attribute/method is implemented in a private script, and the DOM
            attribute/method is exposed to both user's script and private
            scripts.
*   \[private script =&gt; C++\]: This is an "internal" DOM
            attribute/method for private scripts. The DOM attribute/method is
            implemented in C++, and the DOM attribute/method is exposed only to
            private scripts (not exposed to user's script).
*   \[C++ =&gt; private script\]: This is a way to invoke a private
            script from C++. The DOM attribute/method is implemented in a
            private script, and a C++ static function is provided to invoke the
            DOM attribute/method so that Blink can use it wherever it wants.

You can control the kind of each API by combining the
\[ImplementedInPrivateScript\] IDL extended attribute and
\[OnlyExposedToPrivateScript\] IDL attribute.

*   \[user's script & private script =&gt; C++\]: Use no IDL extended
            attributes.
*   \[user's script & private script =&gt; private script\]: Use
            \[ImplementedInPrivateScript\].
*   \[private script =&gt; C++\]: Use \[OnlyExposedToPrivateScript\].
*   \[C++ =&gt; private script\]: Use \[ImplementedInPrivateScript,
            OnlyExposedToPrivateScript\].

Here is an example:

```none
interface XXX {
  void f1();  // Normal DOM method implemented in C++; exposed to user's script and private scripts.
  [ImplementedInPrivateScript] void f2();  // DOM method implemented in a private script; exposed to user's script and private scripts.
  [OnlyExposedToPrivateScript] void f3();  // DOM method implemented in C++; exposed only to private scripts.
  [ImplementedInPrivateScript, OnlyExposedToPrivateScript] void f4();  // DOM method implemented in a private script; V8XXX::PrivateScript::f4Method() is provided as a static method so that Blink can invoke the private script.
};
```

For more details, see test cases in
[PrivateScriptTest.idl](https://code.google.com/p/chromium/codesearch#chromium/src/third_party/WebKit/Source/core/testing/PrivateScriptTest.idl&sq=package:chromium&type=cs).

DOM attributes/methods that have \[OnlyExposedToPrivateScript\] IDL attribute
are "backdoors". **Backdoors are strongly discouraged** unless you are certain
that the backdoors are APIs that are missing in the current web platform and
should be exposed to the web in the future. Ideally Blink-in-JS should be
implemented only by using existing web platform APIs. The only case where
backdoors are allowed is a case where we think that the API is a missing part of
the current web platform and should be exposed to the web in the future. (One of
the goals of Blink-in-JS is to understand what APIs are missing in the web
platform by trying to implement built-in contents of Blink in JavaScript.)

## Where Your Private Script lives?

Your private script lives in `blink_resources.pak` in
`out/*config*/gen/blink/public/resources/`, which is generated by
[`blink_resouces.gyp`](https://chromium.googlesource.com/chromium/blink/+/HEAD/public/blink_resources.gyp),
then it repacked into `content_shell.pak`, `resources.pak` and so.

So, you need to do following steps putting your private script into resource
file:

1.  Change
            [`blink_resources.grd`](https://chromium.googlesource.com/chromium/blink/+/HEAD/public/blink_resources.grd)
            in Blink repository to include your private script, e.g.
            [crrev/570863002](http://crrev.com/570863002)
    *   Since we also need to change file in Chromium repository, I
                recommend to create dummy file and check-in this before
                reviewing your private script.
2.  Change
            [`content/child/blink_platform_impl.cc`](https://chromium.googlesource.com/chromium/src/+/HEAD/content/child/blink_platform_impl.cc)
            in Chromium repository, e.g.
            [crrev/556793006](http://crrev.com/556793006)
    *   **You should wait step #1 blink change is rolled into Chromium
                rather than landed into blink repository.**

Note: Due to [crbug/415908](http://crbug.com/415908), blink_resources.pak isn't
rebuild when your private script file changed. You may want to remove it,
`out/Debug/gen/blink/public/resources/blink_resources.pak`.

## Contacts

If you have any questions or comments, feel to free to ask haraken@. I'm
planning to factor out more things from C++ to Blink-in-JS to improve the
hackability of the Blink core. Your contributions are super welcome!
