---
breadcrumbs:
- - /developers
  - For Developers
page_name: webui
title: Creating Chrome WebUI Interfaces
---

### General guidelines
When creating or modifying WebUI resources, follow the [Web
Development Style Guide](/developers/web-development-style-guide). Note that most
WebUI code is using TypeScript, and any new additions must use TypeScript.

A general explanation of how WebUI works, including the interaction between
C++ and TypeScript code, can be found in the [WebUI Explainer](https://chromium.googlesource.com/chromium/src/+/HEAD/docs/webui_explainer.md).

Shared, cross-platform resources can be found in [ui/webui/resources](https://source.chromium.org/chromium/chromium/src/+/main:ui/webui/resources/).

A detailed example of how to create a WebUI in can be found at
[Creating WebUI interfaces in chrome](https://chromium.googlesource.com/chromium/src/+/HEAD/docs/webui_in_chrome.md).

If you need additional information on how to set up the BUILD.gn file to build
your WebUI, there is detailed information and additional examples for BUILD
files specifically at [WebUI Build Configurations](https://chromium.googlesource.com/chromium/src/+/HEAD/docs/webui_build_configuration.md).

### Debug vs User-Facing UIs
Before adding a new WebUI, the first question to consider is whether you are
creating a user-facing or a debug UI. Some differences:
- User facing UIs should be internationalized and fully accessible by screen
  readers. Debug UIs should not require internationalization or full a11y
  features.
- Debug UIs should not be used by anyone other than Chromium developers. If
  a UI is expected to be used by third party developers or some subset of
  users (e.g., enterprise admins), it's not a debug UI.
- Debug UIs should be able to be removed from official builds without major
  disruption.
- Debug UIs may have occasional breakages, particularly if the team that owns
  them doesn't test them regularly or add any automated tests. If this happens,
  the bug fix likely won't be approved for merge to a release that is already
  well into Beta or Stable, because debug UIs are considered non-critical.
- Debug UIs will be allowed to break by the WebUI platform team if their
  owners cannot be reached and they are blocking horizontal infrastructure
  updates.

### Creating a new WebUI
There are a few questions to answer before creating any new WebUI:
- What will the UI be used for, and who will use it?
- How long is the UI needed? While most user facing WebUIs are kept indefinitely,
  it's possible for debug UIs to only be a temporary need.
- What is the expected size/complexity of the new UI? Be specific here; a proof
  of concept CL, a mock, and/or a specific example of a UI that you expect to
  imitate may be useful.
- What existing shared WebUI resources/infrastructure can and should it use?
  Specifically: Do you expect to use anything in ui/webui/resources? What WebUI
  build rules do you expect to need? Will the UI use Polymer and/or the
  cr-elements library? Will it use Mojo?
- Is the WebUI needed on all platforms? Android, iOS, and ChromeOS differ from
  Windows/Mac/Linux in terms of what shared resources are available.
- Who will maintain the WebUI going forward? Each new WebUI should have an
  OWNERS file listing individuals or teams that the WebUI team can contact
  regarding any deprecations, updates to best practices, etc. Make sure your
  OWNERS file contains multiple people or a full team; it's easy for single-
  owner UIs to end up unowned.

User facing UIs are expected to go through a full feature launch process,
since by definition you're adding a significant new user-visible feature to
Chromium. For debug UIs, prepare a design doc addressing all the questions
above as well as the ones below, and consider also linking a proof of concept/
draft CL and/or basic mock.

### Additional Questions for Debug UIs
When adding a debug UI, answer the following questions in addition to the
questions listed above:
- Why does this debug information require a separate debug UI? Could it
  instead be incorporated into an existing debug UI? Chromium already has
  several dozen debug UIs. Each one adds to binary size and maintenance
  burden and slows down development for critical user facing UIs. Strongly
  consider whether you can re-use an existing page before adding a new one.
- How can this debug UI be tested by Chromium developers performing
  horizontal updates? Does testing it require just navigating to the page,
  or are additional feature flags/user actions/external hardware required
  to trigger some or all of the page features? This information should also
  be added in a README alongside your code when you add your UI.
- Will you add and maintain automated tests for this UI?
- Are you planning to copy large amounts of code from another debug UI?
  Can or should any of this be refactored instead? Also note that debug UIs
  are frequently not well maintained or up to date with WebUI best
  practices, so copy/pasting code may result in a large number of comments/
  changes required during code review.