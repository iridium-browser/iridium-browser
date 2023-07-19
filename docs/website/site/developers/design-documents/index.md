---
breadcrumbs:
- - /developers
  - For Developers
page_name: design-documents
title: Design Documents
---

## Start Here: Background Reading

*   [Multi-process
            Architecture](/developers/design-documents/multi-process-architecture):
            Describes the high-level architecture of Chromium
    **Note:** Most of the rest of the design documents assume familiarity with
    the concepts explained in this document.
*   [How Blink
            works](https://docs.google.com/document/d/1aitSOucL0VHZa9Z2vbRJSyAIsAz24kX8LFByQ5xQnUg)
            is a high-level overview of Blink architecture.
*   The "Life of a Pixel" talk ([slides](http://bit.ly/lifeofapixel) /
            [video](http://bit.ly/loap-2020-video)) is an introduction to
            Chromium's rendering pipeline, tracing the steps from web content to
            displayed pixels.
*   \[somewhat outdated\] [How Chromium Displays Web
            Pages](/developers/design-documents/displaying-a-web-page-in-chrome):
            Bottom-to-top overview of how WebKit is embedded in Chromium

## See Also:

*   [Design docs in source
            code](https://chromium.googlesource.com/chromium/src/+/HEAD/docs/README.md)
*   [Design doc
            template](https://docs.google.com/document/d/14YBYKgk-uSfjfwpKFlp_omgUq5hwMVazy_M965s_1KA/edit)

## General Architecture

*   [Conventions and patterns for multi-platform
            development](/developers/design-documents/conventions-and-patterns-for-multi-platform-development)
*   [Extension Security
            Architecture](http://webblaze.cs.berkeley.edu/2010/secureextensions/):
            How the extension system helps reduce the severity of extension
            vulnerabilities
*   [HW Video Acceleration in
            Chrom{e,ium}{,OS}](https://docs.google.com/a/chromium.org/document/d/1LUXNNv1CXkuQRj_2Qg79WUsPDLKfOUboi1IWfX2dyQE/preview#heading=h.c4hwvr7uzkfl)
*   [Inter-process
            Communication](/developers/design-documents/inter-process-communication):
            How the browser, renderer, and plugin processes communicate
*   [Multi-process Resource
            Loading](/developers/design-documents/multi-process-resource-loading):
            How pages and images are loaded from the network into the renderer
*   [Plugin
            Architecture](/developers/design-documents/plugin-architecture)
*   [Process Models](/developers/design-documents/process-models): Our
            strategies for creating new renderer processes
*   [Profile
            Architecture](/developers/design-documents/profile-architecture)
*   [SafeBrowsing](/developers/design-documents/safebrowsing)
*   [Sandbox](https://chromium.googlesource.com/chromium/src/+/HEAD/docs/design/sandbox.md)
*   [Security
            Architecture](http://crypto.stanford.edu/websec/chromium/): How
            Chromium's sandboxed rendering engine helps protect against malware
*   [Startup](https://chromium.googlesource.com/chromium/src/+/HEAD/docs/design/startup.md)
*   [Threading](https://chromium.googlesource.com/chromium/src/+/HEAD/docs/design/threading.md):
            How to use threads in Chromium

Also see the documentation for [V8](http://code.google.com/apis/v8/), which is
the JavaScript engine used within Chromium.

## UI Framework

*   [UI Development
            Practices](/developers/design-documents/ui-development-practices):
            Best practices for UI development inside and outside of Chrome's
            content areas.
*   [Views framework](/developers/design-documents/chromeviews): Our UI
            layout layer used on Windows/Chrome OS.
*   [views Windowing
            system](/developers/design-documents/views-windowing): How to build
            dialog boxes and other windowed UI using views.
*   [Aura](/developers/design-documents/aura): Chrome's next generation
            hardware accelerated UI framework, and the new ChromeOS window
            manager built using it.
*   [NativeControls](/developers/design-documents/native-controls):
            using platform-native widgets in views.
*   [Focus and Activation with Views and
            Aura](/developers/design-documents/aura/focus-and-activation).
*   [Input Event Routing in Desktop
            UI](https://chromium.googlesource.com/chromium/src/+/HEAD/docs/ui/input_event/index.md).

**Graphics**

*   [Overview](/developers/design-documents/chromium-graphics)
*   [GPU Accelerated Compositing in
            Chrome](/developers/design-documents/gpu-accelerated-compositing-in-chrome)
*   [GPU Feature Status
            Dashboard](/developers/design-documents/gpu-accelerated-compositing-in-chrome/gpu-architecture-roadmap)
*   [Rendering Architecture
            Diagrams](/developers/design-documents/rendering-architecture-diagrams)
*   [Graphics and Skia](/developers/design-documents/graphics-and-skia)
*   [RenderText and Chrome UI text
            drawing](/developers/design-documents/rendertext)
*   [GPU Command
            Buffer](/developers/design-documents/gpu-command-buffer)
*   [GPU Program
            Caching](https://docs.google.com/a/chromium.org/document/d/1Vceem-nF4TCICoeGSh7OMXxfGuJEJYblGXRgN9V9hcE/edit)
*   [Compositing in Blink/WebCore: from WebCore::RenderLayer to
            cc::Layer](https://docs.google.com/presentation/d/1dDE5u76ZBIKmsqkWi2apx3BqV8HOcNf4xxBdyNywZR8/edit?usp=sharing)
*   [Compositor Thread
            Architecture](/developers/design-documents/compositor-thread-architecture)
*   [Rendering
            Benchmarks](/developers/design-documents/rendering-benchmarks)
*   [Impl-side
            Painting](/developers/design-documents/impl-side-painting)
*   [Video Playback and
            Compositor](http://www.chromium.org/developers/design-documents/video-playback-and-compositor)
*   [ANGLE architecture
            presentation](https://docs.google.com/presentation/d/1CucIsdGVDmdTWRUbg68IxLE5jXwCb2y1E9YVhQo0thg/pub?start=false&loop=false)

**Network stack**

*   [Overview](/developers/design-documents/network-stack)
*   [Network Stack
            Objectives](/developers/design-documents/network-stack/network-stack-objectives)
*   [Crypto](/developers/design-documents/crypto)
*   [Disk Cache](/developers/design-documents/network-stack/disk-cache)
*   [HTTP Cache](/developers/design-documents/network-stack/http-cache)
*   [Out of Process Proxy Resolving Draft
            \[unimplemented\]](/system/errors/NodeNotFound)
*   [Proxy Settings and
            Fallback](/developers/design-documents/network-stack/proxy-settings-fallback)
*   [Debugging network proxy
            problems](/developers/design-documents/network-stack/debugging-net-proxy)
*   [HTTP
            Authentication](/developers/design-documents/http-authentication)
*   [View network internals tool](/system/errors/NodeNotFound)
*   [Make the web faster with SPDY](http://www.chromium.org/spdy/) pages
*   [Make the web even faster with QUIC](/quic) pages
*   [Cookie storage and
            retrieval](/developers/design-documents/network-stack/cookiemonster)

**Security**

*   [Security
            Overview](/chromium-os/chromiumos-design-docs/security-overview)
*   [Protecting Cached User
            Data](/chromium-os/chromiumos-design-docs/protecting-cached-user-data)
*   [System
            Hardening](/chromium-os/chromiumos-design-docs/system-hardening)
*   [Chaps Technical
            Design](/developers/design-documents/chaps-technical-design)
*   [TPM Usage](/developers/design-documents/tpm-usage)
*   [Per-page
            Suborigins](/developers/design-documents/per-page-suborigins)
*   [Encrypted Partition
            Recovery](/developers/design-documents/encrypted-partition-recovery)
*   [XSS Auditor](/developers/design-documents/xss-auditor)

## Input

*   See [chromium input](/teams/input-dev) for design docs and other
            resources.

## Rendering

*   [Multi-column
            layout](/developers/design-documents/multi-column-layout)
*   [Style Invalidation in
            Blink](https://docs.google.com/document/d/1vEW86DaeVs4uQzNFI5R-_xS9TcS1Cs_EUsHRSgCHGu8/)
*   [Blink Coordinate
            Spaces](/developers/design-documents/blink-coordinate-spaces)

## Building

*   [IDL build](/developers/design-documents/idl-build)
*   [IDL
            compiler](https://chromium.googlesource.com/chromium/src/+/HEAD/third_party/blink/renderer/bindings/IDLCompiler.md)

See also the documentation for [GYP](https://code.google.com/p/gyp/w/list),
which is the build script generation tool.

## Testing

*   [Layout test results
            dashboard](/developers/design-documents/layout-tests-results-dashboard)
*   [Generic theme for Test
            Shell](/developers/design-documents/generic-theme-for-test-shell)
*   [Moving LayoutTests fully upstream](/system/errors/NodeNotFound)

## Feature-Specific

*   [about:conflicts](/developers/design-documents/about-conflicts)
*   [Accessibility](/developers/design-documents/accessibility): An
            outline of current (and coming) accessibility support.
*   [Auto-Throttled Screen Capture and
            Mirroring](/developers/design-documents/auto-throttled-screen-capture-and-mirroring)
*   [Browser Window](/developers/design-documents/browser-window)
*   [Chromium Print
            Proxy](http://www.chromium.org/developers/design-documents/google-cloud-print-proxy-design):
            Enables a cloud print service for legacy printers and future
            cloud-aware printers.
*   [Constrained Popup
            Windows](/developers/design-documents/constrained-popup-windows)
*   [Desktop
            Notifications](/developers/design-documents/desktop-notifications)
*   [DirectWrite Font Cache for Chrome on
            Windows](/developers/design-documents/directwrite-font-cache)
*   [DNS Prefetching](/developers/design-documents/dns-prefetching):
            Reducing perceived latency by resolving domain names before a user
            tries to follow a link
*   [Download Page (Material Design
            Refresh)](https://docs.google.com/document/d/1XkUDOc6085tir4D5yYEyjL2GsIGBslJBHXiNQDzJawI/edit)
*   [Embedding Flash Fullscreen in the Browser
            Window](/developers/design-documents/embedding-flash-fullscreen-in-the-browser-window)
*   [Extensions](/developers/design-documents/extensions): Design
            documents and proposed APIs.
*   [Find Bar](/developers/design-documents/find-bar)
*   [Form Autofill](/developers/design-documents/form-autofill): A
            feature to automatically fill out an html form with appropriate
            data.
*   [Geolocation](https://docs.google.com/a/chromium.org/document/pub?id=13rAaY1dG0nrlKpfy7Txlec4U6dsX3PE9aXHkvE37JZo):
            Adding support for [W3C Geolocation
            API](http://www.w3.org/TR/geolocation-API/) using native WebKit
            bindings.
*   [Generic-Sensor](/developers/design-documents/generic-sensor) :
            Access sensor data
*   [IDN in Google
            Chrome](/developers/design-documents/idn-in-google-chrome)
*   [IndexedDB](/developers/design-documents/indexeddb) (early draft)
*   [Info Bars](/developers/design-documents/info-bars)
*   [Installer](/developers/installer): Registry entries and shortcuts
*   [Instant](/developers/design-documents/instant)
*   [Isolated Sites](/developers/design-documents/isolated-sites)
*   [Linux Resources and Localized
            Strings](/developers/design-documents/linuxresourcesandlocalizedstrings):
            Loading data resources and localized strings on Linux.
*   [Media Router & Web Presentation
            API](/developers/design-documents/media-router)
*   [Memory Usage Backgrounder:
            ](/developers/memory-usage-backgrounder)Some information on how we
            measure memory in Chromium.
*   [Mouse Lock](/developers/design-documents/mouse-lock)
*   Omnibox Autocomplete: While typing into the omnibox, Chromium
            searches for and suggests possible completions.
    *   [HistoryQuickProvider](/omnibox-history-provider): Suggests
                completions from the user's historical site visits.
*   [Omnibox/IME
            Coordination](/developers/design-documents/omnibox-ime-coordination)
*   [Omnibox Prefetch for Default Search
            Engines](/developers/design-documents/omnibox-prefetch-for-default-search-engines)
*   [Ozone Porting
            Abstraction](https://chromium.googlesource.com/chromium/src/+/HEAD/docs/ozone_overview.md)
*   [Password Form Styles that Chromium
            Understands](/developers/design-documents/form-styles-that-chromium-understands)
*   [Password
            Generation](/developers/design-documents/password-generation)
*   [Pepper plugin
            implementation](/developers/design-documents/pepper-plugin-implementation)
*   [Plugin Power
            Saver](https://docs.google.com/document/d/1r4xFSsR4gtjBf1gOP4zHGWIFBV7WWZMgCiAHeepoHVw/edit?usp=sharing)
*   [Preferences](/developers/design-documents/preferences)
*   [Prerender](/developers/design-documents/prerender)
*   [Print Preview](/developers/design-documents/print-preview)
*   [Printing](/developers/design-documents/printing)
*   [Rect-based event targeting in
            views](/developers/design-documents/views-rect-based-targeting):
            Making it easier to target views elements with touch.
*   [Replace the modal cookie
            prompt](/developers/design-documents/cookie-prompt-replacement)
*   [SafeSearch](/developers/design-documents/safesearch)
*   [Sane Time](/developers/design-documents/sane-time): Determining an
            accurate time in Chrome
*   [Secure Web Proxy](/developers/design-documents/secure-web-proxy)
*   [Service Processes](/developers/design-documents/service-processes)
*   [Site Engagement](/developers/design-documents/site-engagement):
            Tracking user engagement with sites they use.
*   [Site Isolation](/developers/design-documents/site-isolation):
            In-progress effort to improve Chromium's process model for security
            between web sites.
*   [Software Updates:
            Courgette](/developers/design-documents/software-updates-courgette)
*   [Sync](/developers/design-documents/sync)
*   [Tab Helpers](/system/errors/NodeNotFound)
*   [Tab to search](/tab-to-search): How to have the Omnibox
            automatically provide tab to search for your site.
*   [Tabtastic2
            Requirements](/developers/design-documents/tabtastic-2-requirements)
*   [Temporary downloads](/system/errors/NodeNotFound)
*   [Time Sources](/developers/design-documents/time-sources):
            Determining the time on a Chrome OS device
*   [TimeTicks](https://docs.google.com/document/d/1ypBZPZnshJ612FWAjkkpSBQiII7vYEEDsRkDOkVjQFw/edit?usp=sharing):
            How our monotonic timer, TimeTicks, works on different OSes
*   [UI Mirroring
            Infrastructure](/developers/design-documents/ui-mirroring-infrastructure):
            Describes the UI framework in ChromeViews that allows mirroring the
            browser UI in RTL locales such as Hebrew and Arabic.
*   [UI Localization](/developers/design-documents/ui-localization):
            Describes how localized strings get added to Chromium.
*   [User scripts](/developers/design-documents/user-scripts):
            Information on Chromium's support for user scripts.
*   [Video](/developers/design-documents/video)
*   WebSocket: A message-oriented protocol which provides bidirectional
            TCP/IP-like communication between browsers and servers.
    *   [Design
                doc](https://docs.google.com/document/d/11n3hpwb9lD9YVqnjX3OwzE_jHgTmKIqd6GvXE9bDGUg/edit):
                mostly still relevant but some parts have been revised. In
                particular, mojo is used for the communication between Blink and
                //content now.
    *   [WebSocket
                handshake](https://docs.google.com/document/d/1r7dQDA9AQBD_kOk-z-yMi0WgLQZ-5m7psMO5pYLFUL8/edit):
                The HTTP-like exchange before switching to WebSocket-style
                framing in more detail.
    *   [Obsolete design
                doc](https://docs.google.com/document/d/1_R6YjCIrm4kikJ3YeapcOU2Keqr3lVUPd-OeaIJ93qQ/pub):
                WebSocket code has been drastically refactored. Most of the code
                described in this doc is gone. It is mostly only of historical
                interest.
*   [Web MIDI](/developers/design-documents/web-midi)
*   [WebNavigation API
            internals](/developers/design-documents/webnavigation-api-internals)
*   [Web NFC](/developers/design-documents/web-nfc)

## OS-Specific

*   **Android**
    *   [Java Resources on
                Android](/developers/design-documents/java-resources-on-android)
    *   [JNI Bindings](/developers/design-documents/android-jni)
    *   [Android WebView docs](/developers/androidwebview)
    *   [Child process
                importance](https://docs.google.com/document/d/1PTI3udZ6TI-R0MlSsl2aflxRcnZ5blnkGGHE5JRI_Z8/edit?usp=sharing)
*   **Chrome OS**
    *   See the [Chrome OS design
                documents](/chromium-os/chromiumos-design-docs) section.
*   **Mac OS X**
    *   [AppleScript Support](/developers/design-documents/applescript)
    *   [BrowserWindowController Object
                Ownership](/system/errors/NodeNotFound)
    *   [Confirm to
                Quit](/developers/design-documents/confirm-to-quit-experiment)
    *   [Mac App Mode](/developers/design-documents/appmode-mac) (Draft)
    *   [Mac Fullscreen
                Mode](/developers/design-documents/fullscreen-mac) (Draft)
    *   [Mac NPAPI Plugin
                Hosting](/developers/design-documents/mac-plugins)
    *   [Mac specific notes on UI
                Localization](/developers/design-documents/ui-localization/mac-notes)
    *   [Menus, Hotkeys, & Command
                Dispatch](/developers/design-documents/command-dispatch-mac)
    *   [Notes from meeting on IOSurface usage and
                semantics](/developers/design-documents/iosurface-meeting-notes)
    *   [OS X Interprocess Communication
                (Obsolete)](/developers/design-documents/os-x-interprocess-communication)
    *   [Password Manager/Keychain
                Integration](/developers/design-documents/os-x-password-manager-keychain-integration)
    *   [Sandboxing
                Design](/developers/design-documents/sandbox/osx-sandboxing-design)
    *   [Tab Strip Design (Includes tab layout and tab
                dragging)](/developers/design-documents/tab-strip-mac)
    *   [Wrench Menu
                Buttons](/developers/design-documents/wrench-menu-mac)
*   **iOS**
    *   [WKWebView Problems Solved by web//
                layer](https://docs.google.com/document/d/1qgFVIhUdQf_RxhzF-sAsETvo7CNqSLmzlz5YY7rfRe0/edit)

## Other

*   [64-bit Support](/developers/design-documents/64-bit-support)
*   [Layered
            Components](/developers/design-documents/layered-components-design)
*   [Closure Compiling Chrome
            Code](https://docs.google.com/a/chromium.org/document/d/1Ee9ggmp6U-lM-w9WmxN5cSLkK9B5YAq14939Woo-JY0/edit#heading=h.34wlp9l5bd5f)
*   [content module](/developers/content-module) / [content
            API](/developers/content-module/content-api)
*   [Design docs that still need to be
            written](http://code.google.com/p/chromium/wiki/DesignDocsToWrite)
            (wiki)
*   [In progress refactoring of key browser-process architecture for
            porting](/developers/Web-page-views)
*   [Network Experiments](/network-speed-experiments)
*   [Transitioning InlineBoxes from floats to
            LayoutUnits](https://docs.google.com/a/chromium.org/document/d/1fro9Drq78rYBwr6K9CPK-y0TDSVxlBuXl6A54XnKAyE/edit)

## Full listing of all sub-pages:

{% subpages collections.all %}
