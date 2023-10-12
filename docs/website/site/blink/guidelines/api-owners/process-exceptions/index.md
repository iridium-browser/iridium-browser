---
breadcrumbs:
- - /blink
  - Blink (Rendering Engine)
- - /blink/guidelines
  - Guidelines and Policies
page_name: process-exceptions
title: Exceptions to the feature-launching process
---

# TAG review

All [intents to ship](/blink/launching-features) are required to have a
specification TAG review, unless one of the following apply:

* The intent ships a new API or augmentation of an API, or changing an API to
  match a spec, that is
   - already specified and accepted by the relevant
  standardization body, and

   - has already shipped in at least one other
  browser.

* The intent removes a feature or part of a feature.

* The intent is for a JavaScript/WebAssembly language feature that has reached
  [TC39 stage 3](https://tc39.es/process-document/) or WebAssembly
  [phase 4](https://github.com/WebAssembly/proposals/blob/main/README.md#phase-4---standardize-the-feature-wg).

* The intent is for WebDriver / WebDriver BiDi support for a feature already shipped in Chromium.

* An exception was granted by consensus of the Blink API owners through a public
  communication. [Approval](/blink/guidelines/api-owners/procedures/) of an
  intent to ship by the Blink API owners with no TAG review implies such an
  exception.

* An Early Design Review was performed by the TAG and the final specification
does not differ in any significant way from the the form reviewed by the TAG.
In this situation, a comment on the Early Design Review issue (no need to
re-open the issue) linking to the specification suffices.
