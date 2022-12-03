---
breadcrumbs: []
page_name: july-2021---new-features-fixit-stability-security-fixes-interop-fixes-and-more
title: July 2021 - New features, fixit, stability/security fixes, interop fixes and
  more!
---

<table>
<tr>

<td>July 2021</td>

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

<td><td>Composite multiple transformations</td></td>

<td><td><img alt="image" src="https://lh3.googleusercontent.com/EQb-Syn7LREAM34ygxbX7L1pyekr1XQeqvfoCv0Poe8jHCTs2dHv9mOzYksuZnT2iALiRptpqTxNAm_4BoeMVSYmzOyQlxpgYNdBA0IrPPG--eKJ4HDVl1TrWAshwt8eCdA1fQP57w" height=165 width=283></td></td>

<td><td>kevers@ landed a <a href="https://chromium-review.googlesource.com/c/chromium/src/+/3016295">CL</a> to composite multiple transformations.</td></td>

<td><td>Background</td></td>

    <td><td>CSS has 4 transformation properties: translate, scale, rotate and
    transform.</td></td>

    <td><td>Previously, we could only composite a single
    transformation.</td></td>

<td><td>Challenges</td></td>

    <td><td>Compositor only know about transform, and the others were
    converted.</td></td>

    <td><td>Ordering is important</td></td>

    <td><td>WorkletAnimations tick later than the rest to allow time for worklet
    code to run.</td></td>

<td><td>Solution</td></td>

    <td><td>Expand set of target properties in CC</td></td>

    <td><td>ScopedCompoundTransformResolver</td></td>

<td><td>Capability Delegation</td></td>

<td><td>mustaq@ finalized <a href="https://docs.google.com/document/d/1L66B1QtqHCzAKlLQXdtv-YCmlXrJhi2Je2Vo91XWMsQ/edit?usp=sharing">Q3-Q4 shipping plan</a>:</td></td>

    <td><td>origin trial at M94</td></td>

    <td><td>ship at M96 without PaymentRequest changes, and</td></td>

    <td><td>ship PaymentRequest changes after holidays.</td></td>

    <td><td>We considered possible <a
    href="https://github.com/w3ctag/design-reviews/issues/655">TAG</a> delays,
    no-breakage during holiday shopping, and motivating Stripe changes. Thanks
    to smcgruer@ and jyasskin@.</td></td>

<td><td>Android Elastic Overscroll</td></td>

<td><td><img alt="image" src="https://lh3.googleusercontent.com/WsDKsJilisvGaVwqumYvYi0GpamD51sqGPFMoRAE9VMNw-rw73rnzHlA225AUGnkK8yPsvOCK7KO_GeAqfD5SWEkAsyKLhCE-cNiJEgMZzGHL2Kqd3yfMFQzgJ3OJY3A_xx0tnqYuA" height=284 width=134> <img alt="image" src="https://lh5.googleusercontent.com/FRDw2s9Fo_jgXugwAG_xmw4TiAGL62LmQDjVLyo2ZWa5Z5XxZfGqTofo0NYPmKDQmSDupTmkNt58QbnpZeBNYF3nVQiOb68XUK-LNq6iCjIm3Ab2-hZTMPYv-2fS05T-tqaqqo5uOQ" height=283 width=134></td></td>

<td><td>flackr@ has landed all patches for elastic overscroll, which includes:</td></td>

    <td><td>Transform overscroll stretch</td></td>

    <td><td>Pixel shader stretch</td></td>

    <td><td>Disable glow when stretch is enabled</td></td>

    <td><td>Disable stretch on webview when native stretch is used</td></td>

    <td><td>Disable stretch in non-scrollable directions</td></td>

    <td><td>Only enable stretch on Android 12 or later</td></td>

<td><td><img alt="image" src="https://lh3.googleusercontent.com/05E_HxvKHpAv6SpaGfMb_HYRcgow4USMOtoMnLe8-bn6K9TlBnpZBbwhpw4R_wZFZg_HI9iArKEvMAeeuqpI8NC7XdxCRfRdodF-BLnEduUv636G9nZYCPiCqxjkddQUQLbnYaTsSg" height=60 width=135><img alt="image" src="https://lh3.googleusercontent.com/0scdtf1EyP1uWp8seL2Y3fYmoUTAgrGdZUwyY9UWqflcC2O1BUAkZt0uKZ0L5Mk08oThn8CgYrZJOtsQaeduhapwBuBNFUbQHriV1OsvWu1H8aTP6KVu24XNRGYMe8ig3SKBNJyp2Q" height=64 width=138></td></td>

<td><td>Memory.GPU.PrivateMemoryFootprint and Memory.GPU.PeakMemoryUsage2.Scroll</td></td>

<td><td>The preliminary finch result for “Android Elastic Overscroll” is shown above. The only substantial difference seemed to be GPU memory usage:</td></td>

    <td><td>Roughly 10% increase in Memory.GPU.PeakMemoryUsage2.Scroll</td></td>

    <td><td>Slightly increase in Memory.Gpu.PrivateMemoryFootprint</td></td>

<td></tr></td>
<td></table></td>

<td><table></td>
<td><tr></td>

<td><td>Chapter II: Fixit</td></td>

<td></tr></td>
<td></table></td>

<td><table></td>
<td><tr></td>

<td><td>Fix a float-cast-overflow</td></td>

<td><td><img alt="image" src="https://lh6.googleusercontent.com/taf18yv_lIiyJDR7c2R24z21SlOOeuwcJtiToZj3u3ZHYtyfVy39kRXRfGJBf3Te_D0Xvz-asqkNXs4TPbvEP6pFbR20HMdQgwdIvHeM3c_RYb_qPoF5DiQImlk3XrYfeAihgK4PAQ" height=61 width=283></td></td>

<td><td>xidachen@ fixed a float-cast-overflow bug. There is no local repro, and a speculative <a href="https://chromium-review.googlesource.com/c/chromium/src/+/3010014">fix</a> actually fixed the problem.</td></td>

<td><td>Fix a float-cast-overflow</td></td>

<td><td><img alt="image" src="https://lh3.googleusercontent.com/SVQqB5L19CNwvVZIMEnLZn2wgTyNlWlMOvqChcgw9_nM0eKkddYYasmUnWfua-N5DvfwH8HFKH1vsV4x6dcLwUzEpbV06X5TRyx5mt8RCXIpQHPyblwzpwf2b3xToqWq4GYEc2uirA" height=356 width=283></td></td>

<td><td>xidachen@ added more documentation in the csspaint/ folder to explain the workflow of the paint worklet.</td></td>

<td><td>Fix a flaky animations layout test</td></td>

<td><td>This layout test is one of the top flakes under the Blink&gt;Animation category. The root cause seems to be a precision issue with animation time comparison, which is addressed in this <a href="https://chromium-review.googlesource.com/c/chromium/src/+/3027227">CL</a>.</td></td>

<td><td>Fix a few PointerLock crashes</td></td>

<td><td>mustaq@ fixed a 9-year-old <a href="https://crbug.com/143780">exit instruction bug</a> between fullscreen and repeated lock-unlock requests, by untangling exit instruction confusion between browser-vs-content fullscreen exit instructions to keep the “nested fullscreening” case clear to users.</td></td>

<td><td>Also fixed a <a href="http://crbug.com/1213769">crash</a> with repeated locks around a removed element.</td></td>

<td></tr></td>
<td></table></td>

<td><table></td>
<td><tr></td>

<td><td>Chapter III: Stability/security fixes</td></td>

<td><td><table></td></td>
<td><td><tr></td></td>

<td><td><td>Chrome_Android: Crash report</td></td></td>

<td><td><td><img alt="image" src="https://lh4.googleusercontent.com/-sEsKdESILmJBjSwoPSxnfYIh7MT-duKyYLxXLdJ-MoGXeVN0OmKyNzDsrc47eJKg7so5pVmNDAgN4JLuegp6n388dSBfPfAx_ITfqhx3VSB0JB-40mklhDWvLpFL3OYtRFAwDdzhg" height=47 width=277></td></td></td>

<td><td><td>flackr@ fixed a <a href="https://crbug.com/1228047">crash</a> on chrome android. The fix is that the overscroll stretch effect node needed an output clip set (<a href="https://chromium-review.googlesource.com/c/chromium/src/+/3027140">CL</a>). </td></td></td>

<td><td><td>Prevent script execution during lifecycle update</td></td></td>

<td><td><td>flackr@ landed a <a href="https://chromium-review.googlesource.com/c/chromium/src/+/2815619">fix</a> that prevents further security bugs resulting from accidentally running script during the lifecycle. Specific exceptions made for cases where script execution is known to be safe or handled:</td></td></td>

    <td><td><td>ResizeObserver</td></td></td>

    <td><td><td>IntersectionObserver</td></td></td>

    <td><td><td>PaintWorklet::Paint</td></td></td>

    <td><td><td>LayoutWorklet</td></td></td>

    <td><td><td>Plugin::Dispose\*</td></td></td>

<td><td><td>Crashed hittesting with “uneditable” editable elements</td></td></td>

<td><td><td><img alt="image" src="https://lh6.googleusercontent.com/79QhFw8veAOa5QuPOd526oGT8RMnbdrC92KsMj0M1eVDPPxkq2oLAazEAHrbCQbcLs77tQ15B87YU9FmvTDcoarlqngKkuDVrvJlGfjX0AHI47LfHq3vflDsjgweFIKzz8DZsG49ow" height=11 width=277></td></td></td>

<td><td><td>mustaq@ fixed a crash <a href="https://bugs.chromium.org/p/chromium/issues/detail?id=1196872">bug</a>. The root cause is simply that hittest code is unaware that the contenteditable element is not editable (<a href="https://chromium-review.googlesource.com/c/chromium/src/+/3006934">CL</a>).</td></td></td>

<td><td><td>Binary size increase</td></td></td>

<td><td><td><img alt="image" src="https://lh4.googleusercontent.com/-aonhm4OSowxVTec2CwsQtYUymc7eYGOUaq0JfDAw64zTSXv8nZcux24FLSxWn86NJv7Yn7g5dMpy3SJcw5dIolEBnJB2IVQfS0psqddF0GdSP6YBpu5na0NjnctDO1Xm43G89Uokw" height=540 width=123></td></td></td>

<td><td><td>flackr@ investigated a binary size increase <a href="https://bugs.chromium.org/p/chromium/issues/detail?id=1226170">bug</a> on Android, where Android stretch shader adds a large (~6KB) shader string (shown above).</td></td></td>

    <td><td><td>Short term, only include shader string on Android (<a
    href="https://chromium-review.googlesource.com/c/chromium/src/+/3016756">CL</a>).</td></td></td>

    <td><td><td>Long term, find a way to <a
    href="https://bugs.chromium.org/p/chromium/issues/detail?id=1227747">compress
    string</a>.</td></td></td>

<td><td></tr></td></td>
<td><td></table></td></td>

<td></tr></td>
<td></table></td>

<td>Chapter IV: Interop fixes</td>

<td><table></td>
<td><tr></td>

<td><td>Simplify pointer-events spec tracking of changed targets</td></td>

<td><td><img alt="image" src="https://lh5.googleusercontent.com/P-HzHNNtlp05ptKnPf_8_jqGCpOv98oHus5DwDLOuTsn9Lv4GNgX_uoeYgOMhQ5psX9ESERKMVsbRQttED2FCxXsu-gT4rBGcjmpocn2pnhYaVRgvIiyO36RHUNsxXrA7VV0ShfgXA" height=176 width=237></td></td>

<td><td>flackr@ simplifies pointer-events spec. Previously the spec had a dirty flag which lazily updated the targets of coalesced/predicted events.</td></td>

    <td><td>Leaks implementation details into the spec</td></td>

<td><td>Updated this to immediately update the dependent lists</td></td>

    <td><td>Remove dirty flag</td></td>

    <td><td>Well-known when targets change.</td></td>

<td></tr></td>
<td></table></td>

<td><table></td>
<td><tr></td>

<td><td>Chapter V: à la carte</td></td>

<td><td><table></td></td>
<td><td><tr></td></td>

<td><td><td>Refactor native paintworklet</td></td></td>

<td><td><td>xidachen@ proposed a refactor to the native paintworklet code. Detailed doc is <a href="https://docs.google.com/document/d/12g1OLIxZk9ayLNbOI87ru_yoUUWdxcKewDLRR4tqzi8/edit#heading=h.nu1jp6hyj0mz">here</a>, the first <a href="https://chromium-review.googlesource.com/c/chromium/src/+/3016115">CL</a> has landed. </td></td></td>

<td><td><td>Scroll Unification - Google Docs jumpy scrollbar drags</td></td></td>

<td><td><td><img alt="image" src="https://lh3.googleusercontent.com/H3RZ16cqhZwwNG8O_ea_qJsLmxaiZLSeduLfiRnetgWljFW6x3Fvca8uzLrQD__RY42Lv-Zx5NaFTQmCprz95vdlNrIva0T-3aaIqCS0arWFcg0xyn_Y6HkV4a_eELFzd6cRAo-fKQ" height=113 width=277></td></td></td>

<td><td><td>skobes@ landed a <a href="https://chromium-review.googlesource.com/c/chromium/src/+/3002029">CL</a> that fixes a jumpy scrollbar <a href="https://bugs.chromium.org/p/chromium/issues/detail?id=1175210">bug</a>.</td></td></td>

<td><td></tr></td></td>
<td><td></table></td></td>

<td></tr></td>
<td></table></td>

<td><table></td>
<td><tr></td>

<td><td>Chapter VI: Bug Updates</td></td>

<td><td><img alt="image" src="https://lh6.googleusercontent.com/xVq-pGe3-RAvFhB6HoAb1qGwQavMEbRBNFZczC2FgQ0GgqnxHP3tddLvuzMZJ2Y9wWXrRLF1NH8PsPJNJMiJr6GN6RDrJ1mzG5upm4oXbbh_FIPT-Cq6_RIRPA2jWlr2bQ7C-tSbsg" height=158 width=284> <img alt="image" src="https://lh4.googleusercontent.com/CzOEl3V5_tEZsAi8uhknTkhUzWJ2aNZvVEDrIbyXkBwIPa_4mGbojhLftwa9C3KmS2GJU91bXyDxO7nd5aYCL__etSHMH6NtORK5Pl6tUFlxJHOdFw7iVMOzA_NAAk6mgOd-OOkqjg" height=161 width=288></td></td>

<td><td>The increase in P1 bugs appears to be linked to influx of fixit-2021-testing-flaky bugs. Some efforts is required to properly triage them.</td></td>

<td></tr></td>
<td></table></td>

</tr>
</table>

<table>
<tr>

<td>Chrome Interactions Highlights | July 2021</td>

<td><a href="http://go/interactions-team">go/interactions-team</a></td>

</tr>
</table>
