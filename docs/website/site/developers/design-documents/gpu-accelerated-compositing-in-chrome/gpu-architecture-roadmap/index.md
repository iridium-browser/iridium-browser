---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
- - /developers/design-documents/gpu-accelerated-compositing-in-chrome
  - GPU Accelerated Compositing in Chrome
page_name: gpu-architecture-roadmap
title: GPU Architecture Roadmap
---

The Chromium graphics stack is complicated, and has been evolving rapidly for
the last several years. As a result of this rapid evolution, plus the pressure
to try to ship performance-improving features as fast as possible, we’ve
developed a large matrix of feature configurations on different platforms. This
document lays out what’s enabled where, as well as our long-term plans for the
architectural evolution of the stack.

Our end-goal architecture consists of:

    Force compositing mode in the Renderer (accelerated compositing on all
    pages, see our [hardware acceleration overview doc for
    details](http://www.chromium.org/developers/design-documents/gpu-accelerated-compositing-in-chrome))

    A browser compositor (which is typically Aura, although we might do
    something slightly different on Mac \[called “Purlieus” below as a
    placeholder\] and the Android WebView) ([design doc for
    Aura](/developers/design-documents/aura-desktop-window-manager))

    Ubercompositor ([design
    doc](https://docs.google.com/a/chromium.org/document/d/1ziMZtS5Hf8azogi2VjSE6XPaMwivZSyXAIIp0GgInNA/edit))

    Threaded compositing in both the Browser and Renderer ([design
    doc](/developers/design-documents/compositor-thread-architecture))

    Impl-side painting in the Renderer and Browser ([design
    doc](http://www.chromium.org/developers/design-documents/impl-side-painting))

    BrowserInputController and our zero-input-latency scheduler ([design
    doc](https://docs.google.com/a/chromium.org/document/d/1LUFA8MDpJcDHE0_L2EHvrcwqOMJhzl5dqb0AlBSqHOY/edit))

    A software backend for the compositor, used when we don’t have a viable GPU
    (blocklisted or the GPU process crashes repeatedly). This is the only
    configuration variable we intend to support indefinitely. (covered in the
    [ubercomp design
    doc](https://docs.google.com/a/chromium.org/document/d/1ziMZtS5Hf8azogi2VjSE6XPaMwivZSyXAIIp0GgInNA/edit))

    Hybrid accelerated rasterization that, when possible, rasterizes layer
    contents using the GPU ([design
    doc](https://docs.google.com/a/chromium.org/document/d/1Vi1WNJmAneu1IrVygX7Zd1fV7S_2wzWuGTcgGmZVRyE/edit#heading=h.7g13ueq2lwwd))

This implies that the following code paths are eventually going away:

    Legacy software path in the Renderer

    Anything that puts UI onscreen with OS-specific graphics APIs (e.g.
    extensions bubbles, menus, popups, form controls, etc that talk to HWNDs,
    Gtk handles, etc)

    Single-threaded compositing scheduler

    Main-thread rasterization (i.e. non-impl-side-painting)

    BrowserInputController and the old scheduler

    Orphaned compositing (self-drawing cc in the renderer or browser (i.e.
    drawing without ubercomp and a browser compositor))

Conceptually, the architecture has various features, some of which depend on
each other. These features can be enabled or not on every platform (i.e. all
five operating systems), plus based on whether or not we have a trusted GPU
device (non-blocklisted card, drivers, and OS version). We’re trying hard to get
all of these features enabled on all platform configurations, and then delete a
significant amount of legacy code that we no longer need (i.e. everything
above).

The current status of these features on all platform configs is captured in the
below spreadsheet. Platform configs are the cross product of OS, have GPU /
don’t have GPU, and Aura / non-Aura. Note that Aura is included in the platform
configuration, rather than as a feature column, because it has such a large
impact on everything else. We end up with quite a few configurations. Each
column represents a “feature” and each row represents a platform configuration.
Current work is marked, as well as what’s finished and what’s yet to be
attempted.

[Link to
spreadsheet](https://docs.google.com/a/chromium.org/spreadsheet/ccc?key=0AmUAouCtyY6-dDIwV19qNzJvb3RvOG1QWnNnUlpLS1E#gid=0),
or see below (see "overview" tab for key):

#### GPU Feature Dashboard

We want to get all the columns green for rows with good viability, and delete
code that supports the rows with red viability.

Keep this goal in mind when working on features. Contributions to the codebase
that add dependencies to cells in rows with bad viability are problematic
because we’re trying to delete these codepaths -- we’re actively working to
remove dependencies on them. Furthermore, keep in mind that we need to
eventually make the whole column green, not a single cell. If we don’t, we incur
technical debt and add complexity by moving our architecture further out of sync
across platforms.
