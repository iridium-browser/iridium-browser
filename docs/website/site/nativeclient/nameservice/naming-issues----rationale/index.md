---
breadcrumbs:
- - /nativeclient
  - Native Client
- - /nativeclient/nameservice
  - nameservice
page_name: naming-issues----rationale
title: 'naming: issues and use cases'
---

Naming is important for many reasons, among which are being able to precisely
and unambiguously refer to an object, being able to flexibly add new objects to
name spaces without causing problems, etc. This document enumerates a few
scenarios from DSO support that should be supported for NaCl application
authors.

First, let us specify what (sub-) namespaces exist. "Shared object names" or
sonames are used by ld.so to refer to shared objects to be loaded, and the
manifest file provides a mapping from short sonames to longer, less ambiguous
names. But what might application authors want to be able to refer to? One class
of file-like objects are web-hosted data. The long name might be an URL or URI
that resolves -- via HTTP -- to a sequence of bytes that happen to be a NaCl
shared object. Another class of file-like objects are pre-supplied shared
libraries that are shipped with the NaCl distribution, e.g. libc.so. A third
class are files in WebFS -- perhaps the application downloaded and installed
into WebFS persistent storage shared libraries that represent code used for
certain game levels, say.

Suppose an application needs to use libpng.so, which depends on libc.so.
Initially, the NaCl distribution may not provide a libpng.so and only libc.so,
and libpng.so might be mapped to a web-hosted file that would be handled by the
app cache. The following scenarios may occur:

1.  The NaCl distribution add commonly-used libraries to reduce startup
            time. Since a bundled version of libpng.so showed up in a new Chrome
            NaCl runtime release, the application author may decide to switch to
            the bundled version. There should be a way to fall back, in case the
            application is running on an as-yet not updated version of Chrome
            NaCl runtime where the new bundled version of libpng is not
            available.
2.  The application author finds a bug or performance issue in the
            bundled version of libc, and substitutes a different version, so
            both libc.so and libpng.so are provided by a web-based resource or
            by WebFS persistent store.
3.  The application author provides his/her own version of libc, but
            decides to use the bundled version of libpng.
4.  The application also depends on and supplies as a web-hosted
            resource libpng_basics, and relies on the bundled version of libpng.
            Later the NaCl distribution splits the bundled version of libpng
            into libpng and libpng_basics, where libpng depends on
            libpng_basics. The two identically named libpng_basics.so are
            completely different libraries and are not compatible. (This may be
            insoluble since the same soname is used.)
5.  The application relies on the bundled version of libc.so, which
            (transparently) depends on libnacl.so. The application author builds
            a new, web-hosted library also named libnacl.so, creating a name
            conflct with the bundled library. This is likely to be discovered in
            testing, unlike the libpng_basics scenario above; this is here for
            completeness: there might not be a way to transparently load
            internal libraries, since the sonames form a global name space.

These use cases should be supported. They might be automatic, or might require a
manifest file change. For the scenario of bundling commonly-used libraries, it
is probably safe to assume that new versions can be safely added, and old
version might either never be removed or be retired only after a long grace
period.

Some goals:

1.  Adding bundled libraries should not break existing code.
2.  Application authors should be able to precisely name the libraries
            to use, so that there is a precise testing target.
3.  Application authors should be able to optionally allow their
            applications to use newer, ABI-compatible releases of a (bundled)
            library.

Some non-goals:

*   Provide a per-library soname resolution order.

Potential solutions / solution components:

*   Extend URL syntax so that a private protospec can be used to refer
            to bundled files, webfs files, etc. This does not solve the
            sonames-are-a-global-namespace issue (4, 5).
*   Provide a library search path, so that the versioning fallback issue
            from (1) is avoided. This should be done with care, since as noted
            in
            <http://code.google.com/p/nativeclient/wiki/NameResolutionForDynamicLibraries>
            improper use of this can result in excess network traffic /
            roundtrips.
*   Encode in sonames whether an object is bundled version of a library,
            a WebFS-persistent store copy, or a manifest-mapped version.
