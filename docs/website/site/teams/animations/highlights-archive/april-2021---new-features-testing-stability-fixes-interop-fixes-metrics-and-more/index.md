---
breadcrumbs:
- - /teams
  - Teams
- - /teams/animations
  - Animations Team
- - /teams/animations/highlights-archive
  - Highlights Archive
page_name: april-2021---new-features-testing-stability-fixes-interop-fixes-metrics-and-more
title: April 2021 - New features, Testing, Stability fixes, Interop fixes, Metrics
  and more!
---

<table>
<tr>

<td>April 2021</td>

<td>Chrome Interactions Highlights</td>

<td>Archives: <a href="http://go/animations-team-highlights">go/interactions-team-highlights</a></td>

</tr>
</table>

<table>
<tr>

<td><table></td>
<td><tr></td>

<td><td>Chapter I: New features</td></td>

<td></tr></td>
<td></table></td>

<td><table></td>
<td><tr></td>

<td><td>Composite background-color animation</td></td>

<td><td>xidachen@ has been working on this feature aiming to relaunch the finch experiment. There are a few problems fixed during this sprint.</td></td>

<td><td><img alt="image" src="https://lh5.googleusercontent.com/F7Gd8acS30ufVNV31rACPtetP79aWl1e7A_6-egImHkIvP0HkTw8JezzJ9sGWDaTq90ESIT-AD2GzFziWEWT22vS8jQWypRx6XGa5jh1PhpJOqwv_e_l23yaTbPfPv7xIJzLY2ndjTOWTfw8mKN0kM5Utu3_KDwC3YtMCRXUdPwSR8VE" height=236 width=283></td></td>

<td><td>The first one is decouple paint and compositing, which is shown in the above diagram. Before, we call “CheckCanStartAnimationOnCompositor” during the paint stage to determine whether the element should be painted off the main thread or not. Then at the compositing stage, we call the function again to determine whether or not the animation can run on the compositor thread. This causes problems because the property tree node can change during the paint and compositing. The problem is now fixed in this <a href="https://chromium-review.googlesource.com/c/chromium/src/+/2740697">CL</a>, by not calling the above function during the paint stage, which requires us to implement a paint worklet code path to paint the element off the main thread.</td></td>

<td><td><img alt="image" src="https://lh5.googleusercontent.com/uMKCYFltvvzDFGgiOR0YM4RJAxCcXs2v1-CB6NjDKik24WSMmQ4l7QENYqmVBrwJXK_iYU-twKbkwe3qdmxuN4EhiUlAe-VzEbf3XzIQzcjdQAr2dF9fcDasCv9NrbXsOkwxvFDgd_G3PkzWVhUZFLJZpKkocEsCzFemLRAPJXyMr6c4" height=156.15577889447238 width=160.92156862745097><img alt="image" src="https://lh4.googleusercontent.com/mApGNxMJzTxfJyspM3dC2hDD5PxZPI0IbHlJXiUBojIatvcvUKruCfZ1dUaHo9oClrZybovSobha5abys1xu7JcVsoLuqVEnryeXE8O97ERzAPzQj5q_c3hfyxODc3alzEnHI9fSq6QtvcViSgCPrt1au2FDar9Yqf6hjyU7G3S6khE-" height=102 width=158></td></td>

<td><td>The second problem is missing repaint in a few cases, which is tracked in this <a href="http://crbug.com/1184832">bug</a>. During this sprint we have fixed most of them.</td></td>

    <td><td>The first case is missing repaint when the background is transparent
    during the animation. It is fixed in this <a
    href="https://chromium-review.googlesource.com/c/chromium/src/+/2739318">CL</a>.
    The animated gif on the left shows a background-color animation runs
    correctly with the first frame being transparent.</td></td>

    <td><td>The second case is background-color animation on table row or table
    col, whose background is painted into its table cell. In this case, we let
    the animation fall back to the main thread. This is fixed by this <a
    href="https://chromium-review.googlesource.com/c/chromium/src/+/2774182">CL</a>.</td></td>

    <td><td>The third case is background-color animation on the body animation,
    while the view is responsible to paint its background. It is fixed by this
    <a
    href="https://chromium-review.googlesource.com/c/chromium/src/+/2779551">CL</a>,
    which lets the animation run on the main thread.</td></td>

<td><td>Experimental: native paint worklet</td></td>

<td><td><img alt="image" src="https://lh6.googleusercontent.com/u4OXEWMRPI3jWZR3_7-yw4U3MTZzhp0HWcChG2GQzAplntSgS630soR4OHdF5FRPXDkcH7SBj2VLeubgwnZXuw5a2NbofZ5oLc_XC8DJtsB9DDmlo5WoSII-Ycf9sF9uSjis53JpIbp1CYXgoC1LR0CdJdR01p5Lc-eHpGfYPVoMxUSD" height=115 width=156><img alt="image" src="https://lh5.googleusercontent.com/LF-91rvLBmPzQjggeMwXQvees1eS98AvSlHfVn4zHgGdU-x2XVxiA9FOP3wfl2PGn_KACQX_Sl3nxmb2ve4xDOyLEohQ8TB0Nbt9NUh69KX4qok_Y8m6JT6fticXtTzNMTb0GRTKOc_FxERI9MKAcJiV_VILCai3Ip_iFD44VPgK6b9t" height=155.50314465408803 width=159.22981366459626></td></td>

<td><td>Our team is partnering with the skia team, to develop a variant of CSS paint worklet (a native paint worklet), to paint the background faster using native code.</td></td>

<td><td>The picture on the left shows example usage (full <a href="https://jsbin.com/foxasib/10/edit?html,css,js">JSBin</a> example). In the example, we don’t need to use the traditional CSS paint APIs such as “addModule” or “registerPaint”. Rather we use the “skottie” as the name to identify that this is native paint worklet.</td></td>

<td><td>The change is in a work-in-progress <a href="https://chromium-review.googlesource.com/c/chromium/src/+/2776692">CL</a>, currently the animation is running on the main thread, we need some more work to move it off the main thread.</td></td>

<td><td>Declarative show-hide explainer</td></td>

<td><td><img alt="image" src="https://lh4.googleusercontent.com/GBgzBa5UiaD5eZJ8uQvoFS6XgwJLNzTJXNxrAfI7YVCf1xXRAB_60I4NWtH6wq-0Hyu-CIIZfixOSVpjwGl4f084v0fUa2DX2UWifvj8i22ige0EO9VuvUDXKdamGPV72_e-DrmhwDxqb31lo6H1mlzbINbXAcxAg3cejGkZU4bpwYzY" height=181 width=283></td></td>

<td><td>flackr@ published an explainer for the plethora of options to implement tabs.</td></td>

<td><td>It has received early feedback from Brian Kardell regarding additional constraints and other options.</td></td>

<td></tr></td>
<td></table></td>

<td><table></td>
<td><tr></td>

<td><td>Chapter II: Testing</td></td>

<td><td>liviutinta@ fixed flaky test pointer_event_pointercapture_in_frame in this <a href="https://chromium-review.googlesource.com/c/chromium/src/+/2745276">CL</a>. Specifically, the issues found are:</td></td>

    <td><td>Successive test_driver.Actions().pointerMove might lead to coalesced
    pointermove events.</td></td>

    <td><td>test_driver.Actions().send() is asynchronous, the test assumed that
    it was synchronous.</td></td>

    <td><td>Differences between the test_driver.Actions() event streams between
    Windows/Linux.</td></td>

    <td><td>Order of pointerup/lostpointercapture when pointer travels across
    frames not well defined. Opened <a
    href="https://github.com/w3c/pointerevents/issues/355">PEWG</a> issue. This
    still leads to rare flaky runs.</td></td>

<td></tr></td>
<td></table></td>

<td><table></td>
<td><tr></td>

<td><td>Chapter III: Stability fixes</td></td>

<td></tr></td>
<td></table></td>

<td><table></td>
<td><tr></td>

<td><td>CHECK failure during cc animation timing calculation</td></td>

<td><td><img alt="image" src="https://lh3.googleusercontent.com/I6C6OMx1NCkE_II9gODyBjZWWhx5p3W5EP1eu9QYQ67bqgqud2m57pDBpCIjCTdU5W_UVj0ttJ926PJAUFi94fpXp_gFb2NcMzrkiFnZaSF2j7029gYhIZp6rd82bikH3O59P5zMboIwE1PrKfpfKt1Dz5xQjddJsCPcnp_3QOO6UW37" height=133 width=283></td></td>

<td><td>kevers@ worked on the problem where cc does timing calculations a bit differently from WAAPI with respect to animations. The problem is shown in the above code snippets. We verified the timing conversion as part of the check for eligibility to run on the compositor, but we didn’t consider time_offset when making the determination. The solution is to include time_offset in the calculation.</td></td>

<td><td>Crash GestureNavSimple::OnOverscrollModeChange</td></td>

<td><td><img alt="image" src="https://lh5.googleusercontent.com/esadj0fzfzQpcgNrzGxyJ_syNrllUSVLwrHXAJv_OCtJn6hHlnI3pe0Hg7DbE6G92o7-7S5sUrqYDAta_U2MKB_UOtr8p6Xm5myTsS2j_viQ4ZP_jNghQwYv3nJbCCe0BiHzUnXG9hrpPnGY4bQdzBqfy08kllRyV4tyks-2_FrQakoq" height=59 width=111><img alt="image" src="https://lh6.googleusercontent.com/xGKhq1xxbus-Etdfb4zkdnRLnUntPYGC8TSsG-pdoi1Za2hlfrNOvoPfHAwiRf-BcEJRJfkMI8pfWlpt9QOyXP5cEtYhLRU6kxLtpbTOTtKu139Jkt4t-NK1DXrM27SWOR9bmqQ6x-CejXQQ_KUV31DjCQ5Q6mNAPaWvKiPu7JnEfbu2" height=50 width=160></td></td>

<td><td>flackr@ <a href="https://chromium-review.googlesource.com/c/chromium/src/+/2791883">fixed</a> a crash that is due to object lifetime issue. The problem is that an unowned pointer was given to the RenderWidgetHostViewAura’s OverscrollController delegate, when the view did not guarantee that the delegate stayed alive. In some cases the web contents (and delegate) could be destroyed before the view resulting in using the deleted delegate.</td></td>

<td></tr></td>
<td><tr></td>

<td><td>Browser-verified user activation shows misleading data!</td></td>

<td><td>mustaq@ worked on the finch experiment on browser-verified user activation. Currently the finch data suggests verification failure rate is about 0.7%.</td></td>

    <td><td>Similar failures even in the Control groups where the feature is
    disabled.</td></td>

    <td><td>The Control group is consistently better/worse than the Enabled
    groups.</td></td>

    <td><td>This is the <a
    href="https://docs.google.com/document/d/1_4Tg9Bt1OXO6mjAF3a-gaH07G-siGKeSkj13GQnOmjo/edit#bookmark=id.qztvoysscbtv">report</a>
    for the finch.</td></td>

<td><td>The only explanation is maybe extension messaging <a href="https://bugs.chromium.org/p/chromium/issues/detail?id=957553">clobbering</a> user activation again. </td></td>

<td></tr></td>
<td><tr></td>

<td><td>Mac crash blink::Scrollbar::SetNeedsPaintInvaliation</td></td>

<td><td><img alt="image" src="https://lh4.googleusercontent.com/sVOLsU_m4QNyn5mYqClGfgBsCZW3e1ys7TI4OvXfsRN1GzmvXVlCMxpP-rS4tLhnzfTWG-dBOwXiSM-nvHq50uJTDahsBCMfOWL2m8FgR16jVOjzVbpduB2E67yyTUiwggkNj5RxYpArtk4jh0FIk5wXfJU5m-AoXfWegbEGQ6IeMLvb" height=108 width=139></td></td>

<td><td><img alt="image" src="https://lh4.googleusercontent.com/P2M4ziyZ_7xRlM5REV6gCNre90jI8MGnM0wPP1Tsz5cDmmCOS8-CxzwZVix1UvwO_OoM0vKFxKfYylxt4N0aUEDMiCvP_mKXovThUMrkQaKV2unX3V1wkg_6z33x_oTJf0zpkFljE89WuswLLelb079kVXKwkPeL0WmEYmte7lxhNrK0" height=63 width=135><img alt="image" src="https://lh6.googleusercontent.com/WYXgzyOiSZdyTOQXAMJHKLg5Yceo01s6NAzNrkeO5i_xJMPj-r9ohXH2UCj8yVMZizs9BFbKtdxKV1Dr1VBvz5d6vakEddv-skoYOF-DlmhjXSWDYAJ3VjfUALC8krWRz5WHEiwX560Zj2zg57vfY5tcmt6pU2BzSQgJ8Wf_q_EiGNFg" height=64 width=138></td></td>

<td><td>liviutinta@ fixed a crash related to scrollbar on mac.</td></td>

    <td><td>At first it looked like Scrollbar is used after free.</td></td>

    <td><td>In reality, in some cases animation_ is released/deallocated in the
    middle of the call to setCurrentProgress.</td></td>

    <td><td>The solution was to keep animation alive during setCurrentProgress
    by using Objective C retain/release.</td></td>

    <td><td>This is the top 7th renderer crash on latest beta on mac. It is
    responsible for 4 bugs (2 <a
    href="https://bugs.chromium.org/p/chromium/issues/list?q=id%3A1183276%2C1189926%2C&can=1">P1</a>
    release blockers, 2 <a
    href="https://bugs.chromium.org/p/chromium/issues/list?q=id%3A1194276%2C1193025%2C&can=1">P2s</a>).</td></td>

<td></tr></td>
<td></table></td>

<td><table></td>
<td><tr></td>

<td><td>Chapter IV: Interop fixes</td></td>

<td></tr></td>
<td></table></td>

<td><table></td>
<td><tr></td>

<td><td>Created sticky demo for web.dev interop article</td></td>

<td><td><img alt="image" src="https://lh6.googleusercontent.com/mbLti-1KairtHNLRhylAT2mv25lCbch_Io8o0tZr-K8Rpve-Mh_AQD221he3KB0b8SLzKv2iZ5VWToKlBXN0bZhj9OWJR1epEGleEwA5iHC7WvAv-6UDjJZM7XWwpfDiLSuD2iktSB1qV-ziDBEvrOUs9o62O_UwlAm4k7GMoEa-VuH4" height=231 width=283></td></td>

<td><td>flackr@ created a demo of one of the common remaining position sticky interop issues for the web.dev article <a href="https://web.dev/compact2021/">https://web.dev/compact2021/</a> which was published this sprint.</td></td>

<td><td>Scroll-snap support for writing modes</td></td>

<td><td><img alt="image" src="https://lh4.googleusercontent.com/GFUNUuZfoyToayBRC3SSWP7IphGoj22B-uK2MI6w_I6-1JtZRro9P9nAkTOEesL5x2mFSNpJupHzLMUACAijmKdKCzzfsDS-RiFYUfh0dfzJLFGKG2lohrqF4Yk8MQuQRd0ajDXD1YiyppnmXH0Sx_uXMNppwU92ya6FyHo4re7x62X7" height=94 width=134><img alt="image" src="https://lh3.googleusercontent.com/OHW5kXcg6uHP0ISGXjpN70Xv5R_pWv074djPWXZhWHlTCoRJCUepJecn0pdOMLx_uaQlqibW4a5Ck9tRyFNof02BWjS0B0Y_OQoGqPgOqeN6atzqWEeqpq-uLyZPKlGZIwOTHGMlpzLYmOVefrIZPJO7jatDhyf7FsmvLiRI4VT_9b5A" height=91 width=137></td></td>

<td><td> Old New</td></td>

<td><td>kevers@ worked on scroll-snap to enable support for writing modes.</td></td>

    <td><td>The image on the left-hand side shows the old behavior where some of
    the writing modes aren’t supported.</td></td>

    <td><td>The image on the right-hand side shows that we now support all the
    writing modes.</td></td>

<td></tr></td>
<td><tr></td>

<td><td>Scroll-snap outside of scroll corridors</td></td>

<td><td><img alt="image" src="https://lh6.googleusercontent.com/nbVYNy4089lUbQyUYlN0tceM9q7lgvjWwnyoBPEL_EzqB1gwtDQDOkkF5G5kOnUUKoW7ArYZyZOtkD_BxoFLxfV2Xr8n7j0oPOcOR3guzmksRFD6QIK1CPXYrM90TNWYBQupSn1CJtKBWoq-YSpqkoKPxym0rCELfP8qLIi_gQsanV4Z" height=95 width=284></td></td>

<td><td> Old</td></td>

<td><td><img alt="image" src="https://lh6.googleusercontent.com/rsWuZoc7YVjlNpE7wREzQc0RTkv9pZw8F-bkZj3TLFvKNtogc6x9jab1cmBSv_XCoSgAxcgJwRpe-_mjoYmHtxeItWjfLja5UnvUoTqw5TOpqM-OEm8gS4etqmfmNUPKsaXmAR3ZVjDAANclCHOazcLl73SIs9gE25SMWm9VFSqHVF-n" height=95 width=283></td></td>

<td><td> New</td></td>

<td><td>The missing block was offscreen in both x and y direction. Missed as a candidate search position when combining independent x & y searches.</td></td>

<td><td>Fixed tests in css/css-scroll-snap</td></td>

<td><td><img alt="image" src="https://lh3.googleusercontent.com/l9uxIeMwmRJdi2FjqcXUx3KYyqDwvM5YFvNQVSEaCrRzV7fQYVxQiB6ngmfhW780IaHHUz1WbDlcqIyRPAYbWRgDvdbgPYyYZN1RhfMok6hJ4CF1jwzMzHAEdG4gurz-n38Ao0gI7idIK5BGTqc2tRmlw3eS7EZ9MLDUgIPk3-0oa1mm" height=97 width=283></td></td>

<td><td>With the scroll-snap now support for writing mode and for snaps outside of the scroll corridor, a few tests has been fixed as shown in the above table.</td></td>

<td></tr></td>
<td></table></td>

<td><table></td>
<td><tr></td>

<td><td>Chapter V: Metrics</td></td>

<td></tr></td>
<td></table></td>

<td><table></td>
<td><tr></td>

<td><td>go/composite-relative-transform-finch</td></td>

<td><td>kevers@ worked on a document highlighting finch results for internal use only, the efforts includes</td></td>

    <td><td>Communicated highlights in finch tracker bug with confidential bits
    removed.</td></td>

    <td><td>Two cleanup CLs: one to remove the GCL for experiment, and one to
    remove the web feature flag.</td></td>

<td><td>Histograms for finger/pen drag distance</td></td>

<td><td>We need data for the ChromeOS proposal to allow bigger slop for pen taps.</td></td>

    <td><td>The existing Event.TouchMaxDistance won’t work for us. Because it is
    touch-only data, no data beyond slop rectangle.</td></td>

    <td><td><a
    href="https://chromium-review.googlesource.com/c/chromium/src/+/2779754">Added</a>
    new histograms that splits distance-data into pointer-types. Specifically,
    the histograms are Event.MaxDragDistance.{ERASER, FINGER, STYLUS}</td></td>

<td></tr></td>
<td></table></td>

<td><table></td>
<td><tr></td>

<td><td>Chapter VI: Bug Updates</td></td>

<td><td><img alt="image" src="https://lh3.googleusercontent.com/Z-g42rQ4ADjdsT_O_WaXL9uVbo-Cp7XWcC3wUaUyzb_n_sLC_kxtVvS6HIUmX3vFkWv9xDXlZ-3eigN7eyJ47GzDU8-W4b3Nvv1k5d_k6svPePvj-Az6a4P0nHLwPq03aufydp4s9KJsjB69ReM5ZbwjsvrkHv3oD1W_MlqM4q1bp751" height=150 width=273> <img alt="image" src="https://lh6.googleusercontent.com/lXMwy691OYhQjVo0UFkzcmVZq7P_QCCZeJjqC_C0_4Nt2LQXIVy8F4Q0bQYOLJZ0YBnQVHiM58ut2jBYn4dJGelO60TSiRxbCqYzkdGh7x4aN5hob-OFE1RIFBXAo-a5zxM8p8Gz85P4uKpdhONqppEp7-h5rBOkuwVyIB-GvKC13XQT" height=152 width=278></td></td>

<td><td>Our team is seeing a recent spike in P1s, and we are working on addressing the issues.</td></td>

<td></tr></td>
<td></table></td>

</tr>
</table>

<table>
<tr>

<td>Chrome Interactions Highlights | April 2021</td>

<td><a href="http://go/interactions-team">go/interactions-team</a></td>

</tr>
</table>
