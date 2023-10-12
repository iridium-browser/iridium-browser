---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/testing
  - Testing and infrastructure
page_name: fake-bidi
title: Fake Bidi
---

**Motivation**

Fixing right-to-left \[RTL\] bugs in languages such as Hebrew and Arabic can be
challenging if you don't speak an RTL language.

Fake BiDi introduces the concept of a "fake" locale \[one that doesn't
correspond to a real language and isn't incorporated into the binary\] which
displays reversed English words in an RTL context. This allows any developer who
understands English to reason about RTL issues and UI.

Example: the English string "Hello World!" becomes "!dlroW olleH" under Fake
Bidi.

Another important advantage of having a Fake BiDi locale is that it allows
running tests that depend on RTL language strings without actually having those
strings translated. This means that we can use automated testing tools (such as
[BiDiChecker](/system/errors/NodeNotFound)) on UI where either the strings have
been updated and need re-translation or there exist new strings which haven't
been translated yet.

For additional information about fake bidi and pseudo locales, check out
<http://code.google.com/p/chromium/issues/detail?id=73052>

**==Using Fake Bidi==The following directions are obsolete. Support for the --locale_pak flag was removed in https://codereview.chromium.org/291913003/. (The fake-bidi.pak file is still being generated, however, and BidiChecker browser tests still exist.)**During the build process, ==a== *==fake-bidi.pak==* ==is generated next to the Chrome executable==, in a directory called *pseudo_locales*. I.e., if your build uses make, it will be in out/Debug/pseudo_locales.**==~~Mac:~~==**==~~To launch Chrome with Fake Bidi, pass the following arguments:~~~~*--locale_pak=/path/to/pseudo_locales/fake-bidi.pak -AppleLanguages "(he)"*~~~~(Note that you may pass any (RTL) language code to -AppleLanguages in order to simulate any language you need.)~~~~Known Issue: On launch, chrome will attempt to navigate to "(he)". Just close this tab and move on.~~
**==~~Linux / Windows:~~==**==~~To launch Chrome with Fake Bidi, pass the
following arguments:~~~~*--locale_pak=/path/to/pseudo_locales/fake-bidi.pak
--lang=he*~~~~(Note that you may pass any (RTL) language code to --lang in order
to simulate any language you need.)~~==
**==Automated tests using Fake Bidi==**BidiChecker is currently being run both
in English and in Fake Bidi from the *browser_tests* executable. Check out the
page about [BidiChecker](/system/errors/NodeNotFound) for more info.
