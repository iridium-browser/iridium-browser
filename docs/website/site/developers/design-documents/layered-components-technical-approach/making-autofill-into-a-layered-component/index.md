---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
- - /developers/design-documents/layered-components-technical-approach
  - 'Layered Components: Technical Approach'
page_name: making-autofill-into-a-layered-component
title: Making Autofill Into a Layered Component
---

[TOC]

## Objective

Restructure `components/autofill` into a [layered
component](/developers/design-documents/layered-components-design) in order to
enable iOS to cleanly share code in `components/autofill/browser` and
`components/autofill/common`. Concretely, eliminate usage of the content layer
from `components/autofill` code that iOS wishes to share (for details on why iOS
will not be using the content layer, see the motivation and background sections
of the layered components [design
document](/developers/design-documents/layered-components-design)).

## Analysis

On the browser side, Autofill's usage of the content layer falls into several
broad categories:

*   **Receiving communication from the renderer/external world.**
    *   Observing WebContents
    *   Receiving IPC
    *   Listening for notifications
*   **Communicating to the renderer via IPC.**
*   **Querying the external world for information.**
    *   e.g., asking BrowserContext whether it is off-the-record
    *   e.g., getting the values of UserPrefs
*   **Convenience usages**
    *   The FormData struct has a content::SSLStatus field
    *   AutofillManager uses a constant defined in the content layer to
                reference HTTPS scheme

## Plan

*   **Introduce AutofillDriver interface to abstract communication from
            autofill core code to the renderer**
    *   **Features:**
        *   Interface declared in shared browser code
        *   Content-based implementation will use IPC to communicate to
                    the renderer
        *   iOS-based implementation will forward calls directly to iOS
                    autofill UI implementation
    *   **Content-based: AutofillDriverImpl class that will live in
                autofill's content driver and handle communication from the
                renderer/external world**
        *   Will implement the AutofillDriver interface
        *   Will be a WebContentsObserver, using observations to drive
                    flow of the shared browser code
        *   Will be a WebContentsUserData, providing access to per-tab
                    instances of shared browser classes
        *   Will receive IPC, transforming the received messages into
                    method calls on shared browser code
        *   Will listen for notifications, transforming them into method
                    calls on shared browser code
    *   **iOS-based: Introduce a conceptually-similar AutofillDriverIOS
                class that will live in autofill's iOS driver**
*   **Extend AutofillDriver interface to supply the information that
            shared code needs from the external world**
    *   New interfaces will replace direct calls that the shared code
                makes on the content API (e.g., to BrowserContext)
    *   Content-based implementation will back these interfaces by
                content layer objects
    *   iOS implementation will back these interfaces by iOS web layer
                objects
*   **Eliminate convenience usages on a case-by-case basis**
    *   e.g., for SSLStatus, determine exactly what SSL information
                FormData needs and have FormData maintain just that information

## Status

*   Put autofill/components into the structure of a layered component
            with temporary DEPS allowances for violations (and a comment
            instructing devs not to add any more allowances) **\[DONE\]**
    *   Completed over a progression of several CLs
    *   CL finishing the move and introducing DEPS restrictions with
                temporary allowances:
                <https://chromiumcodereview.appspot.com/17392006>
*   [Introduce the AutofillDriver
            interface](https://codereview.chromium.org/16286020/) **\[DONE\]**
*   Introduce the content-based AutofillDriverImpl class and
            incrementally expand it to:
    *   [Abstract WebContentsObserver from shared
                code](https://codereview.chromium.org/16286020/) **\[DONE\]**
    *   [Abstract WebContentsUserData from shared
                code](https://codereview.chromium.org/17225008/) **\[DONE\]**
    *   [Abstract listening for notifications from shared
                code](https://codereview.chromium.org/17893010/) **\[DONE\]**
    *   [Abstract IPC reception from shared
                code](https://codereview.chromium.org/17382007/) **\[DONE\]**
*   Extend the AutofillDriver interface to abstract sending IPC from
            autofill core code **\[STARTED\]**
    *   <https://codereview.chromium.org/17572015/>
    *   <https://codereview.chromium.org/18563002/>
    *   Several other CLs, similar in nature
*   Introduce the AutofillDriverIOS class **\[NOT STARTED\]**
*   Eliminate convenience usages of the content layer **\[DONE\]**
*   Abstract BrowserContext usage **\[DONE\]**
    *   Several CLs, see tracking
                [bug](https://code.google.com/p/chromium/issues/detail?id=303034)
    *   Example
                [CL](https://code.google.com/p/chromium/issues/detail?id=303047)
                abstracting BrowserContext::IsOffTheRecord()
*   Abstract BrowserThread usage **\[DONE\]**
    *   Several CLs, see tracking
                [bug](https://code.google.com/p/chromium/issues/detail?id=303009)
    *   Example [CL](https://codereview.chromium.org/25783002)
*   [Build Autofill core code and component-level unittests on
            iOS](https://codereview.chromium.org/108013004) **\[DONE\]**
*   Change all Autofill core unittests to be component-level unittests
            **\[NOT STARTED\]**

## Help! How Do I Get My Autofill-Related Work Done?

The intended audience of this section is engineers working on
components/autofill, and its purpose is to answer questions of the form "I need
to do Foo, but I can't do it via Bar anymore! What do I do?".

*   **How do I get an AutofillManager instance/AutofillExternalDelegate
            instance from a WebContents instance?** Get the AutofillDriver
            instance (which is a WebContentsUserData) from the WebContents and
            then call the relevant accessor of AutofillDriver.
*   **How do I take some action in shared code based on a new IPC
            message/WebContents observation/notification?** Add a new method(s)
            as appropriate in shared code to implement the desired behavior;
            then have AutofillDriver listen for the IPC/observation/notification
            and invoke the new flow in shared code as appropriate.
*   **Where are the common/ and browser/ directories under autofill/?**
            Code that is shared with iOS has been moved to `autofill/core/`;
            code that is not shared with iOS has been moved to
            `autofill/content/`.
*   **I need to use the content layer in foo.cc, but I'm getting a DEPS
            violation when I try to upload!** The reason for the DEPS violation
            is that foo.cc is used on iOS and thus cannot use the content layer
            directly. See if you can implement your desired flow via one of the
            suggestions earlier in this set of questions. If not, consider
            whether the information that you need to obtain from the content
            layer can be done by extending one of the existing delegate
            interfaces via which shared code obtains information indirectly from
            the content layer. If all else fails, contact blundell@chromium.org
            to assist.
*   **I want to add a new browser-side file; where should I put it?** It
            depends on whether you think that the functionality of the file
            should be shared on iOS. If so, put it in `autofill/core/browser`,
            in which case it cannot reference the content layer directly.
            Otherwise, put it under `autofill/content/browser/`, in which case
            it can freely reference the content layer. In cases of doubt, feel
            free to contact blundell@chromium.org or stuartmorgan@chromium.org.
