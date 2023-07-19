---
breadcrumbs:
- - /developers
  - For Developers
page_name: generated-files
title: Generated files
---

Many files are generated as part of the Chrome build. Fundamentally these are
treated the same as static files (files checked into the repo), but have various
subtleties. In many cases the build configuration file will be named
`generated.{gn,gyp,gypi}` or similar.

## Location

Generated files are output in `$CHROMIUM_DIR/out/$BUILD_TYPE/gen,` as in
`src/out/Release/gen`.

These are indexed in code search in
[src/out/Debug/gen](https://code.google.com/p/chromium/codesearch#chromium/src/out/Debug/gen/).

Because this is a *different* directory than static files, any behavior must be
separately specified for generated files, notably:

*   **Include search path:** if you compile generated C++ files, you
            need to list the appropriate directory for generated files in the
            include search path, e.g., `<(SHARED_INTERMEDIATE_DIR)/blink` in a
            .gyp file for Blink generated files.
    *   Note that static and generated files are not distinguished by
                path in includes; this is because qualified paths are for
                componentization, and static and generated files are in the same
                component.
*   **Possibly different directory tree:** life is much simpler if the
            generated files follow the same directory structure as the static
            files: this allows them to be treated uniformly, only needing to
            specify two roots.
    *   E.g., for qualified paths in includes, you don't need to
                distinguish whether a file is generated or not: you just need to
                list both roots in the include search path.
    *   For the same reason, `checkdeps` is simpler if generated files
                have the same directory structure.

## Outputs (esp. .pyc)

*All* generated files must be listed in the `outputs` of the build step. If not,
these files won't be cleaned up (notably by `ninja -t clean`), and then the
build won't function properly (it will rebuild excessively). One case of this
are byte-compiled Python files (`.pyc`), which are implicitly generated on
import: if you generate Python files, you must *also* list the .pyc files in the
outputs. See Bug [397909](http://crbug.com/397909) and CL
[463063003](http://crrev.com/463063003).

## Clobber build

[Moving generated
files](/Home/chromium-clobber-landmines#TOC-Moving-generated-files) can break
incremental builds, and thus requires a "clobber build" (i.e., full, clean
build: delete all generated files). This is because the build does not *delete*
old generated files, so a stale file may be used, notably via includes of
headers with unqualified paths.

This shows up as generated files appearing to not be regenerated, though in fact
the files are being generated in a new location; see for example Issue
[381111](https://code.google.com/p/chromium/issues/detail?id=381111)
([comment](https://code.google.com/p/chromium/issues/detail?id=381111#c4)).

To avoid this (see also [Chromium clobber landmines: moving generated
files](/Home/chromium-clobber-landmines#TOC-Moving-generated-files)):

*   *(Required):* Warn the [tree sheriff](/developers/tree-sheriffs) or
            gardener (if other repo).
*   *(Required):* Add a [clobber
            landmine](/Home/chromium-clobber-landmines), which forces a rebuild.
    *   This *should* be done as part of the original CL (if in
                Chromium), or part of the roll (if in another repo).
    *   If you forget this, it can also be done in a followup CL (E.g.,
                CL [316343003](https://codereview.chromium.org/316343003)).
    *   For files in other repos (like Blink), this requires a roll, and
                thus causes breakage if there are other changes before the roll.
*   *(Optional):* In case of moving headers, put destination directory
            *after* origin directory in search path, so new files are found
            first. (E.g., CL
            [325483002](https://codereview.chromium.org/325483002/)
            ([comment](https://codereview.chromium.org/325483002/#msg14)))
    *   This does not always work, and can cause cruft if the order is
                not natural.
    *   However, this does not require a roll, and thus is good for
                changes in other repos.

## Dependencies

Currently `checkdeps` is *not* run on generated files (Issue
[365190](https://code.google.com/p/chromium/issues/detail?id=365190)), so
generated files can break componentization.
