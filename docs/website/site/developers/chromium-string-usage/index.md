---
breadcrumbs:
- - /developers
  - For Developers
page_name: chromium-string-usage
title: Chromium String usage
---

Types of Strings
In the Chromium code base, we use std::string and std::u16string. Blink uses
WTF::String instead, which is patterned on std::string, but is a slightly
different class (see the
[docs](https://chromium.googlesource.com/chromium/src/+/HEAD/third_party/blink/renderer/platform/wtf/text/README.md)
for their guidelines, we’ll only talk about Chromium here). We also have a
StringPiece\[16\] class, which is basically a pointer to a string that is owned
elsewhere with a length of how many characters from the other string form this
“token”. Finally, there is also WebCString and WebString, which is used by the
webkit glue layer.
String Encodings
We use a variety of encodings in the code base. UTF-8 is most common, but we
also use ASCII and UTF-16.

*   ASCII is the older 7-bit encoding which includes 0-9, a-z, A-Z, and
            a few common punctuation characters, but not much else. ASCII text
            (common in HTML, CCS, and JavaScript) uses one byte per character.
*   UTF-8 is an encoding where characters are one or more bytes (up to
            6) in length. Each byte indicates whether another byte follows. All
            ASCII characters are UTF-8 characters, so code that works correctly
            with UTF-8 will handle ASCII.
*   UTF-16 is an encoding where all characters are at least two bytes
            long. There are also four-byte UTF-16 characters (a pair of two
            16-bit code units, called a "surrogate pair"). Common cases of
            4-byte characters include most Emoji as well as some Chinese
            characters.
*   **UTF-32** is an encoding where all characters are four bytes long.

When to use which encoding
The most important rule here is the meta-rule, code in the style of the
surrounding code. In the frontend, we use std::string/char for UTF-8 and
std::u16string16/char16_t for UTF-16 on all platforms. Even though std::string
is encoding agnostic, we only put UTF-8 into it. std::wstring/wchar_t is rarely
used in cross-platform code (in part because it's differently-sized on different
platforms), but common in Windows-specific code to interface with native APIs
(which often take wchar_t\* or similar). Most UI strings are UTF-16. URLs are
generally UTF-8. Strings in the webkit glue layer are typically UTF-16 with
several exceptions. Chromium code does not use UTF-32.
The GURL class and strings
One common data type using strings is the GURL class. The constructor takes a
std::string in UTF-8 for the URL itself. If you have a GURL, you can use the
spec() method to get the std::string for the entire URL, or you can use
component methods to get parsed parts, such as scheme(), host(), port(), path(),
query(), and ref(), all of which return a std::string. All the parts of the GURL
with the exception of the ref string will be pure ASCII. The ref string *may*
have UTF-8 characters which are not also ASCII characters.
Guidelines for string use in our codebase

*   Use std::string from the C++ standard library for normal use with
            strings
*   Length checking - if checking for empty, prefer “string.empty():” to
            “string.length() == 0”
*   When you make a string constant, use char\[\] (or char16_t\[\])
            instead of a std::string:
    *   const char kFoo\[\] = “foo”;
    *   const char16_t kBar\[\] = u"bar";
    *   This is part of our style guidelines. It also makes code faster
                because there are no destructors, and more maintainable because
                there are no shutdown order dependencies.
*   There are many handy routines which operate on strings. You can use
            IntToString() if you want to do atoi(), and StringPrintf() if you
            need the full power of printf. You can use WriteInto() to make a C++
            string writeable by a C API. StringPiece makes it easy and efficient
            to write functions that take both C++ and C style strings.
*   For function input parameters, prefer to pass a string by const
            reference instead of making a new copy.
*   For function output parameters, it is OK to either return a new
            string or pass a pointer to a string. Performance wise, there isn’t
            much difference.
*   Often, efficiency is not paramount, but sometimes it is - when
            working in an inner loop, pay special attention to minimize the
            amount of string construction, and the number of temporary copies
            made.
    *   When you use std::string, you can end up constructing lots of
                temporary string objects if you aren’t careful, or copying the
                string lots of times. Each copy makes a call to malloc, which
                needs a lock, and slows things down. Try to minimize how many
                temporaries get constructed.
    *   When building a string, prefer “string1 += string2; string1 +=
                string3;” to “string1 = string1 + string2 + string3;” Better
                still, use base::StrCat().
*   For localization, we have the ICU library, with many useful helpers
            to do things like find word boundaries or convert to lowercase or
            uppercase correctly for the current locale.
*   We try to avoid repeated conversions between string encoding
            formats, as converting them is not cheap. It's generally OK to
            convert once, but if we have code that toggles the encoding six
            times as a string goes through some pipeline, that should be fixed.
