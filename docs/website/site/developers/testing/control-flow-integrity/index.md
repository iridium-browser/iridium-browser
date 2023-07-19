---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/testing
  - Testing and infrastructure
page_name: control-flow-integrity
title: Control Flow Integrity
---

We are planning to deploy Clang's [control flow
integrity](http://clang.llvm.org/docs/ControlFlowIntegrity.html) mechanisms in
Chrome.

The current status:

*   CFI for virtual calls is enabled for the official Chrome on Linux
            x86-64 (M54 and newer).
*   CFI for indirect (C-style) calls is enabled for the official Chrome
            on Linux x86-64 (M68 and newer).
*   Chrome is bad-cast clean, and we have a
            [bot](https://ci.chromium.org/p/chromium/builders/luci.chromium.ci/Linux%20CFI)
            on chromium.memory that keeps it that way
*   We're working on additional compiler improvements to allow deploying
            CFI on more platforms.

To build Chrome with control flow integrity for virtual calls, indirect calls,
and bad casts (Linux x86_64 only):

gn gen out/cfi '--args=is_debug=false is_cfi=true use_cfi_icall=true
use_cfi_cast=true use_thin_lto=true' --check**

**ninja -C out/cfi chrome # Chrome will take 6 minutes or so to link.**

Building with additional diagnostics:

**gn gen out/cfi-diag '--args=is_debug=false is_cfi=true use_cfi_icall=true
use_cfi_cast=true use_cfi_diag=true use_thin_lto=true' --check**

**ninja -C out/cfi-diag chrome # Chrome will take 6 minutes or so to link.**

The deployment is being tracked here:

Meta bug: [crbug.com/701937](http://crbug.com/701937)

Linux: [crbug.com/464797](http://crbug.com/464797)

Android: [crbug.com/469376](http://crbug.com/469376)

ChromeOS: [crbug.com/537386](http://crbug.com/537386)

## Diagnosing problems with the CFI instrumentation

By default, a program compiled with CFI will crash with SIGILL if it detects a
CFI violation.

For better error messages (but not for production use) add **use_cfi_diag=true**
to your **args.gn**

## Indirect call failures

CFI indirect call (cfi-icall) failures are primarily caused by either bad
functions casts or dynamically resolved function pointers:

*   CFI-icall checks that a function pointer points to a function
            matching the function pointer type. Casting function pointers breaks
            that check and such function pointer casts should be eliminated.
            Third party libraries that make significant use of such casts to
            change pointer types may potentially be resolved using [type
            generalization](https://clang.llvm.org/docs/ControlFlowIntegrity.html#fsanitize-cfi-icall-generalize-pointers),
            like
            [so](https://chromium-review.googlesource.com/c/chromium/src/+/938494).
*   CFI-icall function pointer checks are optimized to execute quickly
            at the expense of not being able to check dynamically resolved
            function pointers, e.g. pointers resolved through the use of dlsym()
            or GetProcAddress(). In order to provide a security guarantee
            similar to CFI-icall for dynamically resolved function pointers,
            e.g. that the pointer can not be maliciously modified to an
            arbitrary pointer, the pointer can be placed in read-only memory
            (base::ProtectedMemory) and be called without cfi-icall checking
            (base::UnsanitizedCfiCall), like
            [so](https://chromium-review.googlesource.com/c/chromium/src/+/1000426).

## Overhead (only tested on x64)

- CPU overhead is &lt; 1%

- Code size overhead is 5%-9%

- RAM overhead is a small constant (read-only tables inside the binary shared
between all chrome processes)

## Trophies (bugs found or prevented)

- <https://crbug.com/658955> - invalid cast in
ProcessManagerBrowserTest.NestedURLNavigationsToAppBlocked

- <https://crbug.com/646615> - invalid cast in
blink::BytesConsumerTestUtil::TwoPhaseReader

- <https://crbug.com/605337> - invalid cast in SkTArray.h

- <https://crbug.com/600808> - invalid cast in
PaymentRequestTest.NullShippingOptionWhenNoOptionsAvailable

- <https://crbug.com/601193> - HeapVector::operator= is deleting garbage
collected elements

- <https://crbug.com/577972> - improper deserialization of
cc::HeadsUpDisplayLayer

- <https://crbug.com/569102> - invalid cast in AutofillDialogControllerTest

- <https://crbug.com/568738> - invalid cast in
CachingCorrectnessTest.ReuseImageExpiredFromExpires

- <https://crbug.com/568736> - invalid cast in
AnimationTranslationUtilTest.filtersWork

- <https://crbug.com/568407> - invalid cast in TileManagerPerfTest.PrepareTiles

- <https://crbug.com/565515> - invalid cast in tile_manager_unittest.cc

- <https://crbug.com/557969> - invalid cast in cdm_wrapper

- <https://crbug.com/557802> - Bad-cast to blink::HTMLOptionElement from
blink::HTMLOptGroupElement

- <https://crbug.com/552699> - Bad-cast to webrtc::ProcessThreadImpl from
invalid vptr;process_thread_impl.cc

- <https://crbug.com/551611> - invalid cast in
SafeBrowsingService::GetProtocolManagerDelegate

- <https://crbug.com/569108> - invalid cast in browser_plugin_guest

- <https://crbug.com/524194> - invalid cast in AutofillDialogControllerTest

- <https://crbug.com/541708> - invalid cast in
net/http/http_proxy_client_socket_pool_unittest.cc

- <https://crbug.com/520760> - invalid cast in
ppapi/proxy/nacl_message_scanner.cc

- <https://crbug.com/538952> - Bad-cast to Profile from invalid
vptr;chrome_extensions_network_delegate.cc

- <https://crbug.com/538754> - invalid cast in SkTextBlobBuilder::build

- <https://crbug.com/517959> - invalid cast in sfntly/port/refcount.h

- <https://crbug.com/568891> - invalid cast in
HeapTest.VectorDestructorsWithVtable

- <https://crbug.com/537398> - invalid cast in blink::LifecycleNotifier

- <https://crbug.com/531664> - invalid cast in list_container.h

- <https://crbug.com/528799> - Bad-cast to icu_54::UnicodeSet from
icu_54::Quantifier

- <https://crbug.com/528798> - Bad-cast to blink::ScriptWrappable from
blink::WebGLRenderingContextBase::TypedExtensionTracker&lt;blink::ANGLEInstancedArrays&gt;
- <https://crbug.com/526339> - invalid cast in RenderFrameHostManagerTest

- <https://crbug.com/524194> - invalid cast in AutofillDialogControllerTest

- <https://crbug.com/520760> - Invalid cast in
ppapi/proxy/nacl_message_scanner.cc

- <https://crbug.com/520732> - Invalid cast in message_loop.cc / ToPumpIO

- <https://crbug.com/520699> - Invalid static_cast in ThreadLocalPointer::Get

- <https://crbug.com/516538> - invalid cast in ChromeLauncherControllerTest

- <https://crbug.com/516528> - invalid cast in
content::TestWebContents::GetMainFrame

- <https://crbug.com/516523> - invalid cast in
NavigatorTestWithBrowserSideNavigation

- <https://crbug.com/516513> - Invalid cast in AppCacheHost::AddUpdateObserver

- <https://crbug.com/515973> - Invalid cast in SkTArray.h

- <https://crbug.com/515679> - invalid cast in GrTRecorder.h

- <https://crbug.com/515215> - Invalid cast in
v8::internal::compiler::Typer::Visitor::Reduce
- <https://crbug.com/514817> - Invalid cast in SocketsTcpUnitTest

- <https://crbug.com/513953> - Invalid cast in FeedbackUploaderTest

- <https://crbug.com/513816> - Bad cast in KeyedServiceBaseFactory

- <https://crbug.com/513425> - Invalid cast Event -&gt; CancelModeEvent for
NonCancelableEvent

- <https://crbug.com/513021> - Invalid cast Task -&gt; RasterTask in
gpu_tile_task_worker_pool.cc

- <https://crbug.com/531057> - Bad-cast to blink::ScriptWrappable from
blink::WorkerWebSocketChannel;DOMWrapperMap.h

- <https://crbug.com/530432> - CHECK failed:
io_thread_.StartWithOptions(thread_options) in child_process.cc

- <https://crbug.com/586639> - Invalid cast in TabDesktopMediaListTest
