---
breadcrumbs:
- - /Home
  - Chromium
- - /Home/chromium-security
  - Chromium Security
page_name: pdfium-security
title: PDFium Security
---

Welcome to PDFium Security!

## Basic Info

*   [PDFium project page](https://code.google.com/p/pdfium/)
*   [PDFium Git repository](https://pdfium.googlesource.com/)
*   [Known PDFium security
            issues](https://code.google.com/p/chromium/issues/list?can=2&q=Cr%3DInternals-Plugins-PDF+Type%3DBug-Security+&colspec=ID+Pri+M+Iteration+ReleaseBlock+Cr+Status+Owner+Summary+OS+Modified&cells=tiles)
            (Please pick 1 and fix it!)

## Integer Overflow

We want to standardize on handling integer overflows by:

1.  Preferring new\[\] and new instead of calloc, wherever possible.
2.  In places where the code is not ready to be turned into idiomatic
            C++, preferring calloc to malloc; definitely prefer calloc to malloc
            + memset.
3.  Preferring CheckedNumeric&lt;T&gt; to ad hoc checks.
    *   For convenience, use the existing typedefs for clarity, e.g.
                typedef base::CheckedNumeric&lt;FX_DWORD&gt; FX_SAFE_DWORD;. If
                you need more typedefs like this, or if you need them more
                widely visible, don't hesitate to make the change.

Yes, that might look odd. Currently, the codebase mixes C++ and C memory
allocation, and ultimately, we'd like to get the code to idiomatic C++11, but
we're going to get there incrementally.

## Uninitialized Memory References

We want to standardize on handling uninitialized memory references with:

1.  Default constructors that do the right thing.
2.  Explicit initial values for all POD members in header files.

## Git Workflow

*   The top line/subject line of the commit message should always be as
            explicit as possible. Not just "fix bug", but "Fix UAF in
            ModulateFooContainer" or "Fix UMR in thing::DoStuff".

## Future Desiderata

*   No more non-const references (especially when used as
            out-parameters).
*   Use std::unique_ptr and pdfium::RetainPtr. No more naked new.
