---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: structure-of-layered-components-within-the-chromium-codebase
title: Structure of Layered Components and iOS Code Within the Chromium Codebase
---

The purpose of this document is to describe the structure of layered components
and iOS code within the Chromium source tree. This document assumes the context
of the[
accompanying](https://docs.google.com/a/google.com/document/d/1AMPEkXKgVB7vYoSwLrLaZNC3UDJBh0rjkj4W5BA-TUU/edit#heading=h.c66otrtl06ql)
[high-level design](/developers/design-documents/layered-components-design) and
[technical
strategy](/developers/design-documents/layered-components-technical-approach)
documents.

# Overview

There will be a new top-level directory, ios/. This directory will contain the
following:

    web/: iOS’s src/content/ equivalent

    chrome/: iOS’s src/chrome equivalent

    provider/: the API via which Chromium code will call downstream iOS code

    consumer/: the API via which downstream iOS code will call Chromium code
    ***\[On hold:** Due to the significant flux with componentization and
    upstreaming, it's not yet clear which code will actually need consumer APIs,
    so for now this is not enforced and does not need to be built out
    further.\]*

Each layered component will be divided as follows:

    core/: shared code that is src/content/- and src/ios/-free.

    content/: Content-based driver for the component.

    ios/: Driver for the component based on src/ios/web/ and src/ios/embed/web/.

For components that are multi-process (i.e., currently have browser/, renderer/,
etc. subdirectories), the process division will move to be under the content/
driver (since Chrome for iOS is single-process and almost never uses Chromium
code that is not browser-process code).

# Detailed Example/Discussion of DEPS Rules

## Autofill-Focused Example

The below example indicates a structure for form autofill wherein:

    content- or ios-specific code detects that a form is being rendered

    core code determines whether there is data with which to fill the form

    content- or ios-specific code renders the data

Additionally, we assume that key components of the iOS flow (its
WebContents-like object and the object that renders autofill data) are
implemented downstream and exposed to upstream code via the embed layer, which
will indeed be the case for some amount of time.

Structure under components/autofill/:

    core/

        AutofillManager interface/impl

        AutofillDriver interface

    content/

        browser/

            AutofillDriverImpl

                Listens for IPC from the renderer to know when a form is
                rendered and call into AutofillManager

                Receives OnFormDataFilled calls from AutofillManager and sends
                IPC, which is caught by the renderer

        renderer/

    ios/

        AutofillDriverIOS

            Observes WebState to know when a form is rendered and call into
            AutofillManager

            Receives OnFormDataFilled calls from AutofillManager and asks
            AutofillDriverBridge to render the data

Structure under ios/:

    web/ (upstream impl code of the web layer)

    chrome/ (startup, glue code, etc.)

    provider/

        web/ (interfaces that downstream code provides to the web layer)

            WebState interface (WebContents-like object)

        components/

            autofill/

                AutofillDriverBridge interface

                    RenderDataForForm()

    consumer/

        components/

            autofill/ (Wrapper API for downstream code to call into autofill)

# DEPS Rules

    ios/foo can depend on ios/provider/foo

        Purpose of ios/provider/foo is for foo (and higher layers) to use it

    ios/provider/foo can depend only on lower-layer consumer/ targets

        Might need lower-level consumer API to pass parameters to downstream
        code

    ios/consumer/foo will depend on ios/foo (which can’t depend on
    ios/consumer/foo)

    components/foo/ios can depend only on ios/web, ios/provider/web, and lower
    layers
