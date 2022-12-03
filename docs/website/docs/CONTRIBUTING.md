# Contributing to www.chromium.org

In order to contribute to this repo you must have signed the
[Google Contributor License Agreement](https://cla.developers.google.com/clas)
and have an active account on
[Chromium's Gerrit Host](https://chromium-review.googlesource.com).

## Making edits to pages via the web

The site contains a fairly rudimentary in-page editor. To edit a page,
click on the "Edit this Page" button in the left nav bar. That will take
you to [edit.chromium.org](https://edit.chromium.org/edit?repo=chromium/website/main)
and open the page in the editor automatically.

You can edit the Markdown text directly, and, once you're ready to upload
the change, if you you click on the "Create Change" box in the bottom right
corner of the page, that will create a Gerrit CL for review. A builder
will automatically run to build out a copy of the site containing your
changes so that you can preview them.

Any current Chromium/ChromiumOS contributor (basically anyone with with
try-job access or bug-editing privileges) can review CLs, but you also
need OWNERS approval to land them.

This functionality is limited to just editing the text of existing pages,
and there's not yet any way to preview the change before you upload it
for review.

If you need to upload new images or other assets, or add new pages, or
change multiple pages at once, or do anything else more complicated,
keep reading ...

## Making bigger changes using a local Git checkout

*NOTE: If you have an existing Chromium or ChromiumOS checkout, you will
hopefully soon have this repo DEPS'ed in in automatically.*

1.  Install depot_tools:

    ```bash
    $ git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
    $ export PATH=/path/to/depot_tools:$PATH
    ```

2. Check out the repo and its dependencies:

    ```bash
    $ git clone https://chromium.googlesource.com/website
    $ cd website
    $ gclient sync
    ```

    or

    ```bash
    $ fetch website
    ```

3.  Make your changes! Check out [AUTHORING.md](AUTHORING.md) for guidelines.

4.  Build all of the static pages up-front to check for errors.
    The content will be built into `//build` by default.

    ```bash
    $ ./npmw build
    ```

    It should only take a few seconds to build out the website.

    (`npmw` is a simple wrapper around the version of `npm` that is bundled
    as part of this checkout.)

5.  Start a local web server to view the site. The server will (re-)generate
    the pages on the fly as needed if the input or conversion code changes.
    The content will be built into `//build`.

    ```bash
    $ ./npmw start
    ```

6.  Check in your changes and upload a CL to the Gerrit code review server.

    ```bash
    $ git commit -a -m 'whatever'
    $ git-cl upload
    ```

    If you are adding binary assets (images, etc.) to the site, you will
    need to upload them to the GCS bucket using `//scripts/upload-lobs.py`.

7.  Get one of the [//OWNERS](../OWNERS) to review your changes, and then
    submit the change via the commit queue.

    *NOTE:* If this is your first time contributing something to Chromium
    or ChromiumOS, please make sure you (or your company) has signed
    [Google's Contributor License Agreement](https://cla.developers.google.com/),
    as noted above, and also add yourself to the [//AUTHORS](../AUTHORS) file
    as part of your change.
