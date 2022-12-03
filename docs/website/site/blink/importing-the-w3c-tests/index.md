---
breadcrumbs:
- - /blink
  - Blink (Rendering Engine)
page_name: importing-the-w3c-tests
title: Working with web-platform-tests in blink
---

## OBSOLETE

**See
[here](https://chromium.googlesource.com/chromium/src/+/HEAD/docs/testing/web_platform_tests.md)
instead**

[TOC]

Interoperability between browsers is [critical](/blink/platform-predictability)
to chromium's mission of improving the web. We believe that leveraging and
contributing to a shared test suite is one of the most important tools in
achieving interoperability between browsers. The [web-platform-tests
repository](https://github.com/w3c/web-platform-tests) is the primary shared
test suite where all browser engines are collaborating.

Chromium has mirrors
([web-platform-tests](https://chromium.googlesource.com/external/w3c/web-platform-tests/),
[csswg-test](https://chromium.googlesource.com/external/w3c/csswg-test/)) of the
GitHub repos, and regularly (at least daily) imports a subset of the tests so
that they are run as part of the regular Blink layout test testing process.

Note that currently the main reason we do not run more of the W3C tests is
because they are (probably) mostly redundant with Blink's existing tests, and so
we would double the running time of the layout tests for little benefit. Ideally
we would identify the tests that are redundant and remove Blink's copies, so
that we run just the W3C tests where possible.

The end goals for this whole process are to:

1.  Be able to run the W3C tests unmodified locally just as easily as we
            can run the Blink tests
2.  Ensure that we are tracking tip-of-tree in the W3C repositories as
            closely as possible
3.  Run as many of the W3C tests as possible

## Automatic Import Process

There is an automatic [w3c-test-autoroller
bot](https://build.chromium.org/p/chromium.infra.cron/builders/w3c-test-autoroller)
for regularly updating our local copies of web-platform-tests.
[w3c_test_autroller
recipe](https://cs.chromium.org/chromium/infra/recipes/recipes/w3c_test_autoroller.py).

```none
Tools/Scripts/wpt-import --auto-import wpt
Tools/Scripts/wpt-import --auto-import css
```

## Manually Importing New W3C Tests

Updating the set of tests run by Blink requires commit access to Chromium like
anything else, so make sure you have that first.

We control which tests are imported via
[LayoutTests/W3CImportExpectations](https://code.google.com/p/chromium/codesearch?q=W3CImportExpectations#chromium/src/third_party/WebKit/LayoutTests/W3CImportExpectations),
which has a list of directories to skip during import. This means that any new
tests and directories that show up in the W3C repos are automatically imported.

To pull the latest versions of the tests that are currently being imported
(i.e., you don't need to change the blocklist), all you have to do is run:

```none
Tools/Scripts/wpt-import wpt
Tools/Scripts/wpt-import css
```

That script will pull the latest version of the tests from our mirrors of the
W3C repos. If any new versions of tests are found, they will be committed
locally to your local repository. You may then upload the changes.

If you wish to add more tests (by un-skipping some of the directories currently
skipped in
[W3CImportExpectations](https://code.google.com/p/chromium/codesearch?q=W3CImportExpectations#chromium/src/third_party/WebKit/LayoutTests/W3CImportExpectations)),
you can modify that file locally and commit it, and on the next auto-import, the
new tests should be imported. If you want to import immediately, you can also
run wpt-import --allow-local-commits.

## Contributing Blink tests back to the W3C

### If you need to make changes to [Web Platform Tests](https://github.com/w3c/web-platform-tests), just commit your changes directly to our version in [LayoutTests/external/wpt](https://cs.chromium.org/chromium/src/third_party/WebKit/LayoutTests/external/wpt/) and the changes will be automatically upstreamed within 24 hours.

Note: if you’re adding a new test in external/wpt, you’ll need to re-generate
MANIFEST.json manually until <https://crrev.com/2644783003> is landed. The
command to do so is:

```none
third_party/WebKit/Tools/Scripts/webkitpy/thirdparty/wpt/wpt/manifest \
```

```none
--work \ 
```

```none
--tests-root=third_party/WebKit/LayoutTests/external/wpt
```

### Where can I find the code for the WPT import and export tools?

    Exporter:
    [//third_party/WebKit/Tools/Scripts/wpt-export](https://cs.chromium.org/chromium/src/third_party/WebKit/Tools/Scripts/wpt-export)

    Importer:
    [//third_party/WebKit/Tools/Scripts/wpt-import](https://cs.chromium.org/chromium/src/third_party/WebKit/Tools/Scripts/wpt-import)

    Libraries:
    [//third_party/WebKit/Tools/Scripts/webkitpy/w3c/](https://cs.chromium.org/chromium/src/third_party/WebKit/Tools/Scripts/webkitpy/w3c/?q=local_wpt&sq=package:chromium)

### Will the exported commits be linked to my GitHub profile?

The email you commit with (e.g. user@chromium.org) will be the author of the
commit on GitHub. You can [add it as a secondary address on your GitHub
account](https://help.github.com/articles/adding-an-email-address-to-your-github-account/)
to link your exported commits to your GitHub profile.

### What if there are conflicts?

This cannot be avoided entirely as the two repositories are independent, but
should be rare with frequent imports and exports. When it does happen, manual
intervention will be needed and in non-trivial cases you may be asked to help
resolve the conflict.

### Direct pull requests

It's still possible to make direct pull requests to web-platform-tests. The
processes for getting new tests committed the W3C repos are at
<http://testthewebforward.org/docs/>. Some specifics are at
<http://testthewebforward.org/docs/github-101.html>.
