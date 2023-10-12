---
breadcrumbs:
- - /Home
  - Chromium
- - /Home/chromium-security
  - Chromium Security
- - /Home/chromium-security/memory-safety
  - Memory safety
page_name: rust-and-c-interoperability
title: Rust and C++ interoperability
---

(written August 2020)

Chrome engineers are experimenting with Rust. For the foreseeable future, C++ is
the reigning monarch in our codebase, and any use of Rust will need to fit in
with C++ — not the other way around. This seems to present some C++/Rust
interoperability challenges which nobody else has faced.

We'd need to solve these before considering Rust as (nearly) a first-class
citizen in our codebase. If we can’t solve these, Rust would at best be isolated
to “leaf nodes” which don’t interact much with the rest of our codebase. And if
that’s all we can use Rust for, that calls into question whether the costs of an
extra language are justified in the first place.

As C++ is the ruler, we are primarily concerned with the ability for new Rust
code to call into existing C++ code, rather than C++ to Rust calls.

We think it’s important for Rust to be able to call C++ functions in a way that 
meets the following **criteria**:

**No need for the “unsafe” keyword unless something is known to be less safe than normal C++**

For a Rustacean, this is controversial - all C++ is unsafe! But “unsafe”
should be a really bad code smell. If “unsafe” is needed for all C++ calls,
there will be thousands of them, and the “unsafe” keyword will lose its
meaning. Where objects are simply passed backwards and forwards between Rust
and C++, we must avoid the word ‘unsafe’. It should be restricted to patches
of genuinely unsafe Rust code, and for C++ interoperability code where
there’s shared ownership or other complexities.
This particular property is satisfied by dtolnay’s marvellous
[cxx](https://github.com/dtolnay/cxx) library already.

**No overhead in the general case**

LTO and [cross-language
inlining](http://blog.llvm.org/2019/09/closing-gap-cross-language-lto-between.html)
already solve this in principle. There are cases where overhead is necessary
at the C++ boundary — especially, the UTF check required when strings are
passed from C++ to Rust. This can be dealt with by handling such strings as
&\[u8\] in Rust code, until string manipulation is really necessary, so we
do not need any further innovations here. This box is checked.

**No boilerplate or redeclarations. No C++ annotations. Ideally, no allowlist**

If a C++ API exists, Rust should be able to call it. It’s that simple. The
declaration in C++ should be sufficient. There should be no need for an
allowlist, a redeclaration in Rust, or any Rust shim. Rare exceptions will
exist (e.g. overloaded functions) and in some cases we’ll want to make an
idiomatic Rust wrapper, but in general, that shouldn’t be necessary.
This is not just aesthetic preference. Our codebase is complex and polluting
it with extra annotations would be a small, but noticeable, tax on how
everyone works.

**Broad type support - with safety**

[cxx](https://github.com/dtolnay/cxx) is the current state-of-the-art for
safely exchanging data between C++ and Rust. Our
“[base](https://source.chromium.org/chromium/chromium/src/+/HEAD:base/)”
library exposes 1768 APIs which are used by other parts of Chrome. 1052 of
those functions only take parameters which are types that can already be
supported by cxx. 12 more are planned in the near term for cxx (e.g. more
flexible slices).
That’s ~60% of our APIs, which is good but not great.
Another 12% can be supported if we are able to pass std::string and similar
string types into existing C++ APIs. These can’t be represented in a Rust
struct due to an internal pointer, but as cxx generates code on both the C++
and Rust side, it should be possible to own a UniquePtr&lt;CxxString&gt; on
the Rust side, yet [pass it into an existing C++
API](https://github.com/dtolnay/cxx/issues/250) which takes a std::string by
value.
(That sounds fairly straightforward, but it becomes much more complex when
you’re talking about structs containing std::strings, such as
[url::Origin](https://source.chromium.org/chromium/chromium/src/+/HEAD:url/origin.h;l=141?q=url::Origin&ss=chromium%2Fchromium%2Fsrc).
Such a struct could only be owned as a UniquePtr&lt;opaque type&gt; from the
Rust side, which would prevent field access. Solutions can be imagined but
need more thought.)
Another ~20% are functions which take pointer parameters - in our case,
these are very often [out
parameters](https://source.chromium.org/chromium/chromium/src/+/HEAD:base/rand_util.h;l=40?q=base::RandBytes&sq=).
We need to see how we can programmatically identify those which are ‘simple’
out parameters and allow Rust to populate them safely.
The good news is that this leaves just 8% of our functions which can’t be
supported by the cxx model of interop. Most of these are passing C++ structs
(by value) which have [raw pointers within
them](https://source.chromium.org/chromium/chromium/src/+/HEAD:base/memory/shared_memory_mapping.h;l=169?q=base::WritableSharedMemoryMapping).
This seems largely insoluble in Rust but they’re so rare that we can create
case-by-case idiomatic wrappers.
There are some caveats here: this analysis is based on symbols exported by
the binary, rather than source code analysis. In some cases these APIs would
be wrapped by inline functions, templates or macros, which this analysis
ignores. It also ignores return values and direct field access. And of
course, “base” isn’t the only set of APIs which our code would need to call
- it’s probably that higher-level functions would have [more complex
arguments on
average](https://source.chromium.org/chromium/chromium/src/+/HEAD:content/public/browser/render_frame_host.h;bpv=1;bpt=1;l=88?q=RenderFrameHost&gsn=RenderFrameHost&gs=kythe%3A%2F%2Fchromium.googlesource.com%2Fchromium%2Fsrc%3Flang%3Dc%252B%252B%3Fpath%3Dsrc%2Fcontent%2Fpublic%2Fbrowser%2Frender_frame_host.h%23G1w6QPBQL82Xkcn4l7LDpzClmBPa_c18lFVVZbHK5h0&gs=kythe%3A%2F%2Fchromium.googlesource.com%2Fchromium%2Fsrc%3Flang%3Dc%252B%252B%3Fpath%3Dsrc%2Fcontent%2Fpublic%2Fbrowser%2Fcontent_browser_client.h%23q1iJpNllgNKY5mVu_-89ZVL29Rk5wCUukrTj6kdjLOA&gs=kythe%3A%2F%2Fchromium.googlesource.com%2Fchromium%2Fsrc%3Flang%3Dc%252B%252B%3Fpath%3Dsrc%2Fcontent%2Fpublic%2Fbrowser%2Fnavigation_controller.h%23xJPcd1uTfK8sLQidlLN1nkzka5MM8UOqJQ-vd1RMLUI&gs=kythe%3A%2F%2Fchromium.googlesource.com%2Fchromium%2Fsrc%3Flang%3Dc%252B%252B%3Fpath%3Dsrc%2Fcontent%2Fpublic%2Fbrowser%2Fnavigation_handle.h%23Cla5SVTQ5b0yHjvdzazehvdZiPAwZPiL7hw9jGrMBMg&gs=kythe%3A%2F%2Fchromium.googlesource.com%2Fchromium%2Fsrc%3Flang%3Dc%252B%252B%3Fpath%3Dsrc%2Fcontent%2Fpublic%2Fbrowser%2Frender_frame_host.h%23RenderFrameHost%253Acontent%2523c%2523dhOuyZmgB2x&gs=kythe%3A%2F%2Fchromium.googlesource.com%2Fchromium%2Fsrc%3Flang%3Dc%252B%252B%3Fpath%3Dsrc%2Fcontent%2Fpublic%2Fbrowser%2Fweb_contents.h%23XrbufdRG9Y--Dfi3iq7WkPh30Aby0bBIixBApZ1fXG4&gs=kythe%3A%2F%2Fchromium.googlesource.com%2Fchromium%2Fsrc%3Flang%3Dc%252B%252B%3Fpath%3Dsrc%2Fcontent%2Fpublic%2Fbrowser%2Fweb_contents_delegate.h%23xPu14kaNu58lQUD_9A9MEXyxUFs8Jibf73XNjZze6zM&gs=kythe%3A%2F%2Fchromium.googlesource.com%2Fchromium%2Fsrc%3Flang%3Dc%252B%252B%3Fpath%3Dsrc%2Fcontent%2Fpublic%2Fbrowser%2Fweb_contents_observer.h%23Gc6rUNIeosEK0hOGxwCGtN3e4cQRlW1pbI9SJnAffaY&gs=kythe%3A%2F%2Fchromium.googlesource.com%2Fchromium%2Fsrc%3Flang%3Dc%252B%252B%3Fpath%3Dsrc%2Fcontent%2Fpublic%2Ftest%2Fcontent_browser_test_utils.h%23KxzkBqHDzmvePnTKOrHQpGDfOD2Oy4yWL0xQ17sxS7g&gs=kythe%3A%2F%2Fchromium.googlesource.com%2Fchromium%2Fsrc%3Flang%3Dc%252B%252B%3Fpath%3Dsrc%2Fcontent%2Fpublic%2Ftest%2Ftest_utils.h%23EVUuAhYNNhFIy0tQc4euuLdHh_H2gYKx6Cde5mGUDdw&gs=kythe%3A%2F%2Fchromium.googlesource.com%2Fchromium%2Fsrc%3Flang%3Dc%252B%252B%3Fpath%3Dsrc%2Fcontent%2Fshell%2Fbrowser%2Fshell_platform_delegate.h%23JlDkk2QY2af2K98dJC7keSobXw9Lm7Vvr2DnHqWQpGM&gs=kythe%3A%2F%2Fchromium.googlesource.com%2Fchromium%2Fsrc%3Flang%3Dc%252B%252B%3Fpath%3Dsrc%2Fcontent%2Fpublic%2Fbrowser%2Frender_frame_host.h%23_RceGKzxaDRVeDDFRGV34cgVys-lQ22yR_wwBOBzXkI&gs=kythe%3A%2F%2Fchromium.googlesource.com%2Fchromium%2Fsrc%3Flang%3Dc%252B%252B%3Fpath%3Dsrc%2Fchrome%2Fbrowser%2Fchromeos%2Flogin%2Ftest%2Fjs_checker.h%23r_Gj569ozVIUJq-1POIJ4-qqNaRvHC-N7_01P9qsSSc&gs=kythe%3A%2F%2Fchromium.googlesource.com%2Fchromium%2Fsrc%3Flang%3Dc%252B%252B%3Fpath%3Dsrc%2Fchrome%2Fbrowser%2Ftask_manager%2Fproviders%2Fweb_contents%2Fweb_contents_task_provider.h%23m8QtJrZjgtNRJveh4olsmT3NIvAFrJmS62j1jaLIjVk&gs=kythe%3A%2F%2Fchromium.googlesource.com%2Fchromium%2Fsrc%3Flang%3Dc%252B%252B%3Fpath%3Dsrc%2Fchrome%2Fbrowser%2Fui%2Fexclusive_access%2Ffullscreen_controller.h%232v5FtWIP5lic0jku3uPv0aA35WhmxS5zkZ32A3UXfsc&gs=kythe%3A%2F%2Fchromium.googlesource.com%2Fchromium%2Fsrc%3Flang%3Dc%252B%252B%3Fpath%3Dsrc%2Fcomponents%2Fblocked_content%2Fpopup_blocker_tab_helper.h%23ZAwLD36-n3HV1_IIGmPbytosIMmfek4iFBO1CLfnwAI&gs=kythe%3A%2F%2Fchromium.googlesource.com%2Fchromium%2Fsrc%3Flang%3Dc%252B%252B%3Fpath%3Dsrc%2Fcomponents%2Fpage_load_metrics%2Fbrowser%2Fobservers%2Flargest_contentful_paint_handler.h%23MGktmDqVePSQ5i5YsOaSyZQO_PsptqLzf_yMxl4Dyuk&gs=kythe%3A%2F%2Fchromium.googlesource.com%2Fchromium%2Fsrc%3Flang%3Dc%252B%252B%3Fpath%3Dsrc%2Fcomponents%2Fpage_load_metrics%2Fbrowser%2Fpage_load_metrics_observer.h%23lb9A7wZugxQD54-kaHBU9xBztc_qtW0v7d_yxHFitMo&gs=kythe%3A%2F%2Fchromium.googlesource.com%2Fchromium%2Fsrc%3Flang%3Dc%252B%252B%3Fpath%3Dsrc%2Fcomponents%2Fperformance_manager%2Fpublic%2Fperformance_manager.h%237RnPbKgruVz8UTqkRkQA5AJD6a87ucr-9eRsxXEQR_U&gs=kythe%3A%2F%2Fchromium.googlesource.com%2Fchromium%2Fsrc%3Flang%3Dc%252B%252B%3Fpath%3Dsrc%2Fcomponents%2Fsubresource_filter%2Fcontent%2Fbrowser%2Fsubresource_filter_observer.h%23TXHYSiScLwmoB1m4zPDJABkqvxFdAWfy1Ex2IFX_daM&gs=kythe%3A%2F%2Fchromium.googlesource.com%2Fchromium%2Fsrc%3Flang%3Dc%252B%252B%3Fpath%3Dsrc%2Fcontent%2Fbrowser%2Fcontent_index%2Fcontent_index_service_impl.h%23MhjsqAeDxF0FYJo_wllpmTcv6tuLPn_nPnwDJtbrGzs&gs=kythe%3A%2F%2Fchromium.googlesource.com%2Fchromium%2Fsrc%3Flang%3Dc%252B%252B%3Fpath%3Dsrc%2Fcontent%2Fpublic%2Fbrowser%2Frender_document_host_user_data.h%23QFc-GZRe6mkj2fUPpd_OOzdlMfR9TPXA7kdFH2P0y0c&gs=kythe%3A%2F%2Fchromium.googlesource.com%2Fchromium%2Fsrc%3Flang%3Dc%252B%252B%3Fpath%3Dsrc%2Fcontent%2Fpublic%2Fbrowser%2Frender_frame_host.h%23RenderFrameHost%253Acontent%2523c%2523bJIWE93K2Uw&gs=kythe%3A%2F%2Fchromium.googlesource.com%2Fchromium%2Fsrc%3Flang%3Dc%252B%252B%3Fpath%3Dsrc%2Fcontent%2Fpublic%2Fbrowser%2Frender_view_host.h%23j-yNEITJJhL9H_N7SAWIFM00eSi9Do5KqVHnnw6-wYY&gs=kythe%3A%2F%2Fchromium.googlesource.com%2Fchromium%2Fsrc%3Flang%3Dc%252B%252B%3Fpath%3Dsrc%2Fcontent%2Fpublic%2Ftest%2Fnavigation_simulator.h%23LdLUS0zBCndDaYelBCIrAtQAP5CjgrKoBWc4UkaDpPE&gs=kythe%3A%2F%2Fchromium.googlesource.com%2Fchromium%2Fsrc%3Flang%3Dc%252B%252B%3Fpath%3Dsrc%2Fextensions%2Fbrowser%2Fapp_window%2Fapp_window.h%23cLpWpVzgBXk0gZyi-2THfz57SRACA-LHUveF0YbUgCg&gs=kythe%3A%2F%2Fchromium.googlesource.com%2Fchromium%2Fsrc%3Flang%3Dc%252B%252B%3Fpath%3Dsrc%2Fextensions%2Fbrowser%2Fextension_function.h%23_Ogl2-DYz2iqCw9dhPLcqQwcXfV194_P3uWT2Fnkj-4&gs=kythe%3A%2F%2Fchromium.googlesource.com%2Fchromium%2Fsrc%3Flang%3Dc%252B%252B%3Fpath%3Dsrc%2Fextensions%2Fbrowser%2Fprocess_manager.h%23fdDKt3wBoBo8B9ujQiKlNlTWllc9d6Nucx_LH5PyFg4&gs=kythe%3A%2F%2Fchromium.googlesource.com%2Fchromium%2Fsrc%3Flang%3Dc%252B%252B%3Fpath%3Dsrc%2Fchrome%2Fbrowser%2Fbanners%2Fapp_banner_manager.h%236njZ8hjl2ANDhn7cc2_h2Obhvi0hty24_cozfCfXvP4&gs=kythe%3A%2F%2Fchromium.googlesource.com%2Fchromium%2Fsrc%3Flang%3Dc%252B%252B%3Fpath%3Dsrc%2Fchrome%2Fbrowser%2Fdevtools%2Fdevtools_window.h%23UvVTsinMZfkvO_28gCFM_S742snUFO4dF9lUEBCOOzA&gs=kythe%3A%2F%2Fchromium.googlesource.com%2Fchromium%2Fsrc%3Flang%3Dc%252B%252B%3Fpath%3Dsrc%2Fchrome%2Fbrowser%2Fui%2Fsearch%2Flocal_ntp_test_utils.h%23MFOhcxTDmwcb9ct68b2WSzYF6YDjdalt24KDHCBega8&gs=kythe%3A%2F%2Fchromium.googlesource.com%2Fchromium%2Fsrc%3Flang%3Dc%252B%252B%3Fpath%3Dsrc%2Fcomponents%2Fautofill%2Fcontent%2Fbrowser%2Fcontent_autofill_driver_factory.h%232_1CXjN2hUKgzzlJeHBUY-03W_tFo-iunU62bByBM6s&gs=kythe%3A%2F%2Fchromium.googlesource.com%2Fchromium%2Fsrc%3Flang%3Dc%252B%252B%3Fpath%3Dsrc%2Fcomponents%2Fpage_load_metrics%2Fbrowser%2Fobservers%2Fpage_load_metrics_observer_tester.h%23ek2Ilzd8IEKzZMxB7pfXiVtl3hub7QKlESaQXb9wefE&gs=kythe%3A%2F%2Fchromium.googlesource.com%2Fchromium%2Fsrc%3Flang%3Dc%252B%252B%3Fpath%3Dsrc%2Fcomponents%2Fpayments%2Fcontent%2Fpayment_app_factory.h%23UXMzp2clHfiq_I9jqU5EYPV35LFAS4Qeb30UgiYYmPM&gs=kythe%3A%2F%2Fchromium.googlesource.com%2Fchromium%2Fsrc%3Flang%3Dc%252B%252B%3Fpath%3Dsrc%2Fcontent%2Fbrowser%2Fweb_contents%2Fweb_contents_impl.h%23bGo7lh_Cz-AMlV0vnrg7N1hekIc2k69yWIXNjppwLvM&gs=kythe%3A%2F%2Fchromium.googlesource.com%2Fchromium%2Fsrc%3Flang%3Dc%252B%252B%3Fpath%3Dsrc%2Fcontent%2Fpublic%2Fbrowser%2Fdevtools_agent_host.h%23MVlcGNudSwRbhwAb7qjmDJ5EIF5RuWjJo6dcUqHEdwE&gs=kythe%3A%2F%2Fchromium.googlesource.com%2Fchromium%2Fsrc%3Flang%3Dc%252B%252B%3Fpath%3Dsrc%2Fcontent%2Fpublic%2Fbrowser%2Fdevtools_manager_delegate.h%23fUonhxAGMNCMCnWgzJszvsBnMePO8z2cucV5m0H1gig&gs=kythe%3A%2F%2Fchromium.googlesource.com%2Fchromium%2Fsrc%3Flang%3Dc%252B%252B%3Fpath%3Dsrc%2Fcontent%2Fpublic%2Fbrowser%2Fmedia_player_id.h%237GEQk0IkYgezyBz93D_esGwqUhoYfor4M2JjgzOkHKY&gs=kythe%3A%2F%2Fchromium.googlesource.com%2Fchromium%2Fsrc%3Flang%3Dc%252B%252B%3Fpath%3Dsrc%2Fcontent%2Fpublic%2Fbrowser%2Fweb_ui_controller.h%23iktvwQuWbLO8hlEAR-DJYt9o0xju62S_9nfg7R7j2Kc&gs=kythe%3A%2F%2Fchromium.googlesource.com%2Fchromium%2Fsrc%3Flang%3Dc%252B%252B%3Fpath%3Dsrc%2Fcontent%2Fpublic%2Ftest%2Fweb_contents_tester.h%23zPltnaBNY45XLxRyREmM8T-NumTILcwnWNRC8FiI-ug&gs=kythe%3A%2F%2Fchromium.googlesource.com%2Fchromium%2Fsrc%3Flang%3Dc%252B%252B%3Fpath%3Dsrc%2Fchrome%2Ftest%2Fpayments%2Fpayment_request_platform_browsertest_base.h%23H2AtSVUS0Q9k7jDXyRtrFB1Po_9evqkWUVvvo7NB6oM&gs=kythe%3A%2F%2Fchromium.googlesource.com%2Fchromium%2Fsrc%3Flang%3Dc%252B%252B%3Fpath%3Dsrc%2Fcomponents%2Fautofill%2Fcore%2Fbrowser%2Fautofill_client.h%2356w8agBOZjtRclF5Wfl-QJmO500HiS1mPke0i1kt91s&gs=kythe%3A%2F%2Fchromium.googlesource.com%2Fchromium%2Fsrc%3Flang%3Dc%252B%252B%3Fpath%3Dsrc%2Fcomponents%2Fpermissions%2Fpermission_request_id.h%23TutwfaiAmQOM6OkwwuX-_Hdp-H4pq5HSE0N5mqrhrxg&gs=kythe%3A%2F%2Fchromium.googlesource.com%2Fchromium%2Fsrc%3Flang%3Dc%252B%252B%3Fpath%3Dsrc%2Fcontent%2Fbrowser%2Fmedia%2Fforwarding_audio_stream_factory.h%23KgICheBanWRA3eZKFVxi-8bxZCOO4ujLJ-uUMvvordk&gs=kythe%3A%2F%2Fchromium.googlesource.com%2Fchromium%2Fsrc%3Flang%3Dc%252B%252B%3Fpath%3Dsrc%2Fcontent%2Fbrowser%2Frenderer_host%2Fmedia%2Frender_frame_audio_output_stream_factory.h%23kXXZI4k7E_bVhhmyaIbCYnCIOsddZlredLwzqVUEl14&gs=kythe%3A%2F%2Fchromium.googlesource.com%2Fchromium%2Fsrc%3Flang%3Dc%252B%252B%3Fpath%3Dsrc%2Fcontent%2Fbrowser%2Frenderer_host%2Frender_widget_host_owner_delegate.h%23KEHWC06-LjLVJcNPotW9smK0WoGIjQcjFcs3m6c3Jdw&gs=kythe%3A%2F%2Fchromium.googlesource.com%2Fchromium%2Fsrc%3Flang%3Dc%252B%252B%3Fpath%3Dsrc%2Fcontent%2Fpublic%2Fbrowser%2Fback_forward_cache.h%23KU6Chs-FixtEn-uy4ELETeGq_e5amqVQ51_xw8jmwP4&gs=kythe%3A%2F%2Fchromium.googlesource.com%2Fchromium%2Fsrc%3Flang%3Dc%252B%252B%3Fpath%3Dsrc%2Fextensions%2Fbrowser%2Fextension_function_dispatcher.h%23NlU0-Xo74M-G5-Eq_D2VfP9PAf4UX6pBv8o202lwdR8&gs=kythe%3A%2F%2Fchromium.googlesource.com%2Fchromium%2Fsrc%3Flang%3Dc%252B%252B%3Fpath%3Dsrc%2Fextensions%2Fbrowser%2Fprocess_manager_observer.h%23ypSAGYvQtRkkYdE3rjreepcXMaKO7JxH13lwLq6DGKQ)
so be less likely to fall in the ‘good’ bucket.

**Ergonomics - with safety**

From Rust code, we need to be able to instantiate C++ objects, safely pass
around ownership (there are no significant problems with cxx’s
[UniquePtr](https://docs.rs/cxx/0.3.4/cxx/struct.UniquePtr.html) here), call
methods on them (both plain and virtual). For “plain old data” types in C++,
containing simple, cxx-compatible fields, we need to be able to manipulate
those fields. Most of this can be achieved with cxx already (though we need
a way to call through to
[std::make_unique](https://github.com/dtolnay/cxx/issues/228) from Rust code
for a type that’s opaque to Rust).
We need this to be smooth enough that we do not need to wrap a typical C++
type in a Rust wrapper.
So far, so good. But we also need: to act (at Rust build time) upon #defines
set up by our C++ headers and build-time rules, figure out a plan for
calling C++ overloaded functions and operators, call macros (e.g. LOG(ERROR)
&lt;&lt; “eek”), make templated functions and types available (possibly very
hard, though bindgen does a remarkably good job here), and probably many
other things we haven’t yet thought of.
It may be that the best way to handle some of these cases is some inline C++
code within Rust (like the [cpp crate](https://crates.io/crates/cpp) but
with the benefit of cxx’s safety).
One specific challenge is [reference-counted
objects](https://source.chromium.org/chromium/chromium/src/+/HEAD:base/memory/scoped_refptr.h;l=175?q=scoped_refptr).
We need the reference count to be shared between referees on the Rust and
Chrome side. The bigger challenge here is how to deal with the prevalence of
multiple mutable references on the C++ side, without the ability to do
something like a
[RefCell::borrow_mut](https://doc.rust-lang.org/std/cell/struct.RefCell.html#method.borrow_mut)
to ensure even runtime safety. It may be that we need to mark involvement
with all such reference-counted objects as truly ‘unsafe’ from the Rust
side.
In general we think we can live without Rust types inheriting from C++
types, but there’s one exception: pure virtual observers. cxx provides the
ability to [pass function pointers from Rust to
C++](https://github.com/dtolnay/cxx#builtin-types), so it’s quite possible
for us to make wrapper types here. Ideally, though, this becomes ergonomic
and seamless as well. More investigation is needed here.

**What we don’t need**

We believe we can live without: self-referential C++ types being passed by value
into Rust (except for strings), Rust types inheriting from non-pure-virtual C++
types; variadic arguments, “safe” reference counting. (There will be cases where
the absence of these features is annoying, but hopefully rare.) All this may be
wrong: we still have much learning to do.

**Our plan**

We think the hardest part of this is imagining a safe way to pass types between
Rust and C++. That requires auto-generated shim code on both the Rust and C++
side. That’s already achieved by cxx with terrific emergent safety properties.
And so that’s our basic model.

But, we don’t want to specify a cxx::bridge section for every API. We therefore
[need the cxx::bridge to be generated using a bindgen-like
tool](https://github.com/dtolnay/cxx/issues/235).

We don’t believe Rust language changes are needed. Some C++ types can’t be owned
by value in Rust — for example std::string with its self-referential pointer —
but we believe that good C++ interoperability can be achieved even if Rust can
only own such objects by pointer. We may be wrong here as well!

For now, Chrome investment in Rust will remain a background investigation
(mostly directed towards prototyping these tools and techniques). If we become
convinced this sort of interoperability is possible, we’ll revisit widespread
use of Rust in Chrome, and at that point we plan to work hard to achieve this
with robust production-quality solutions.

(update September 2021) There's progress across the Rust community in solving
many of these problems - see for example
[moveit](https://crates.io/crates/moveit),
[autocxx](https://crates.io/crates/autocxx) and
[mosaic](https://github.com/google/mosaic/).
