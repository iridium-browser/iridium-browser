---
breadcrumbs:
- - /teams
  - Teams
- - /teams/animations
  - Animations Team
- - /teams/animations/highlights-archive
  - Highlights Archive
page_name: june-2021-vol-2---new-features-stability-fixes-interop-fixes-and-more
title: June 2021 (Vol 2) - New features, stability fixes, Interop fixes and more!
---

<table>
<tr>

<td>June 2021 (Vol 2)</td>

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

<td><td>Scroll Unification: fix autoscroll crash</td></td>

<td><td>kevers@ fixed an autoscroll crash. There are few problems including:</td></td>

    <td><td>WeakPtrs to the same object were dereferenced on different
    threads.</td></td>

    <td><td>Waiting for input to be processed on main</td></td>

    <td><td>Waiting for a main thread hit test on the compositor</td></td>

<td><td>Here are the fixes:</td></td>

    <td><td>WidgetInputHandlerManager is now ref counted.</td></td>

    <td><td>Safe to pass around the “this” pointer for callbacks that will run
    on the compositor.</td></td>

    <td><td>Use AsWeakPtr only for callbacks that will be run on the main
    thread.</td></td>

    <td><td>Fixes 5 crashing tests!</td></td>

<td><td>Scroll Unification: untangling iframe issues</td></td>

<td><td><img alt="chain.png" src="https://lh6.googleusercontent.com/GF5f11bWr7884waypGiE3bi4y-AEY-OWfnVskKZlpAfGuLvFak_GwhKYVOfaJMocFKSX6zEsg7ytdkkqej1kRACQO_oIeAcS7O-Og690MGOnW2gZ_T6LxG9h17QxAaikOAAMVLA7ZJlmSOF6E481OYnLBiSzkCoe7DF0yxBvOf0gKafc" height=190 width=279></td></td>

<td><td>skobes@ landed the <a href="https://chromium-review.googlesource.com/c/chromium/src/+/2983821">CL</a> to handle the case when a scroll hits an iframe that isn’t scrollable for some reason. In this case, we need to look for scrollable container in the parent frame.</td></td>

<td><td>Composite BG-color animation</td></td>

<td><td>xidachen@ fixed two crash cases.</td></td>

<td><td><img alt="image" src="https://lh5.googleusercontent.com/lIiIaL2x898PdeBhWHW8u22XUNxlSPfhGHbIRsbbVcLhWEq7ornl3q_7rqwz85Eqt2wOmtrue4nsh4Jr0rVb9-JqMtVBum-9M0y_W05tuPEaWwJbfwIh-Mqd5WQRvfmcQLBmCPoVlGkSsc0pw4774yejCh96gNjknKZIWCyq--RxPayt" height=76 width=283></td></td>

<td><td>The first case is shown above where the frame is detached during animation. The fix landed <a href="https://chromium-review.googlesource.com/c/chromium/src/+/2951382">here</a>.</td></td>

<td><td><img alt="image" src="https://lh3.googleusercontent.com/_yovKVwj7Cc83Md-0ngT8poGR47TZ24Xs5qvWizKG_aWszkI_gKZwblELJCEksA8_JGcHB___vtzdsHpvSUEJTtByTLafZFbrqdRTEU0Lyv8ttXBhfYiEiGLNjBIozw90YO26V6mzdYCOa_HoJxYx8CnFP4Pj5QsRUyDRgMqxZ4Y6-kN" height=157 width=141></td></td>

<td><td>The second case is shown above, and the crash is due to antialiasing that changes the draw operation bounds. This is the <a href="https://chromium-review.googlesource.com/c/chromium/src/+/2954479">fix</a>.</td></td>

<td><td>Progress based timelines/Scroll Timelines</td></td>

<td><td><img alt="image" src="https://lh6.googleusercontent.com/sXJITc1_VocnlLddIAps2gR_spLrMg3hNfh1OfeKOdMn8S0WSTEoIqcBTat3D_6PJEn94VejqwJVbDJrBi72Q401j9syWMLmmwnNGCEo-fouH0tC8VVdr9sjpHneH24A6PFe9kjugsJXltHSN8iN7DYRFtL53G4fPc5wAOFZZLwSJ-bv" height=200 width=283></td></td>

<td><td>kevers@ made a lot of progress on <a href="https://drafts.csswg.org/web-animations-2">specs</a>. </td></td>

<td><td>Highlights:</td></td>

    <td><td>No longer need time-range for a scroll timeline; Times are converted
    to percentages. (<a
    href="https://github.com/w3c/csswg-drafts/pull/6410">patch</a>)</td></td>

    <td><td>Support duration = ‘auto’ (<a
    href="https://github.com/w3c/csswg-drafts/pull/6337/">patch</a>)</td></td>

<td><td>Here’s an example of un-timed timelines:</td></td>

<td><td><img alt="image" src="https://lh3.googleusercontent.com/-pz0DHZNano7yrdoIHbn_3L08AT_InCiZDZ7A8uvHkgf1csjMi-2M1PJV6YnGb5Sjqyk1ozrW3TvmhIr0AucjD8FTfWpSm1qUX7atIsK2NUePkd48J1gp4HLBzrbMABQ6seLIRdAy6io1qeUvy6MadntwNpOw12W4dCdx9Ta-8ah0hro" height=124 width=283></td></td>

<td><td>Another example of empowering progress based animations</td></td>

<td><td><img alt="image" src="https://lh3.googleusercontent.com/sOOLJ5DSjTrxW8oYTqIHccfgzcUvqe2c8Z70ao3NeQH7rtIDIwLu6dvtH3GeHTKOPAjdWm9kIwixT6ZbQdUvHgdFKRAdDZ3_2ewjncEoeZkodfSUw8BqK5tEG2EC-8CJwY7WlpXy8vUJ9be6EJdHF89jsajz_dmehjYkiy75rRMc0DJe" height=155 width=215></td></td>

    <td><td>Normalization procedure to convert time-based animations to
    progress-based.</td></td>

    <td><td>Animations are scaled to 100% with iteration and delays adjusted
    accordingly.</td></td>

    <td><td>Smooth integration with web-animations API</td></td>

<td><td>Capability Delegation is ready for TAG review</td></td>

    <td><td>mustaq@ <a
    href="https://github.com/w3ctag/design-reviews/issues/655">kicked off</a> a
    TAG review for the <a
    href="https://wicg.github.io/capability-delegation/">explainer</a> and the
    <a href="https://wicg.github.io/capability-delegation/spec.html">draft
    spec</a>.</td></td>

<td><td><img alt="image" src="https://lh6.googleusercontent.com/DTQM0AAbpo4H-S7bcy9udCr-jtZ4QenLeuLj6qu_C-HloTqU51QYym46H5gYpYoGN8TQBCqPRt_J91bfgicQfD1706FD7n2eHxcXzhbJTxqw19M9XJy-oVpZOLnebaoqy3PCFSSjIABAAAd61QwsZN8h8Sl74mghfvGMKZbMwEOzBXyj" height=83 width=256></td></td>

    <td><td>Also investigated possible shipping options and finalized <a
    href="https://docs.google.com/document/d/1L66B1QtqHCzAKlLQXdtv-YCmlXrJhi2Je2Vo91XWMsQ/edit?usp=sharing">a
    plan</a> working with smcgruer@ (Web Payments) and jyasskin@ (Web
    Standards).</td></td>

<td></tr></td>
<td></table></td>

<td><table></td>
<td><tr></td>

<td><td>Chapter II: Stability fixes</td></td>

<td></tr></td>
<td></table></td>

<td><table></td>
<td><tr></td>

<td><td>DCHECK in ConvertTimingForCompositor</td></td>

<td><td><img alt="image" src="https://lh6.googleusercontent.com/eyBq9_FmfclUlfQjaMQmR5adk-l1XtWKism64bdBFe0qbCFnFEiCztGpEhF5olqDkZNr-ggEO28XfJ9t2MGgILKt6a-890Vvsz9mWiS5cYTNNIMAXgsStdc55cjCYdO9ZOIjhiDfjQpER8Njl16ghE6oEKQ8Cdlt8G2XzDXVEQwU1BbU" height=163 width=283></td></td>

<td><td>This function did a bunch of math to check whether the result is finite or not, then either abort or carry on. But what if the intermediate result is indeterminate?</td></td>

<td><td>To solve that, we check intermediate results if there is any risk of the following in subsequent calculations:</td></td>

    <td><td>infinity - infinity</td></td>

    <td><td>0 / 0</td></td>

    <td><td>infinity / infinity</td></td>

    <td><td>Division by zero</td></td>

<td><td>FullscreenControllerInteractiveTest flaky fix</td></td>

<td><td><img alt="image" src="https://lh6.googleusercontent.com/ATmakcwLRkU0fhQgB3lhLXeJry1hUqEuSkZ9OB_Y6HXVuTsg0Q4kDYWlzGUg6la_dmmlxhtO5CuA0M7ToABMYZXutB04sXdz5SaseLymhW45u0-nWRu5VnVeMNIq_ohJlI1pa1AoJ0z3E_2KhMp924z0ryu-4oGHAICgPTP_O6A7tWxM" height=44 width=283> </td></td>

<td><td>mustaq@ found an issue that might cause flakiness in a large suite of tests: an <a href="https://source.chromium.org/chromium/chromium/src/+/main:net/test/embedded_test_server/embedded_test_server.h;drc=172a2a554e8b556203c962152d3b19d55240725d;l=341">initialization problem</a> with EmbeddedTestServer could be causing flakiness in all FullscreenControllerInteractiveTests! A <a href="https://chromium-review.googlesource.com/c/chromium/src/+/2969924">fix</a> has been landed, and we’re hopeful that flakiness will be greatly reduced moving forward.</td></td>

<td></tr></td>
<td></table></td>

<td><table></td>
<td><tr></td>

<td><td>Chapter III: Interop fixes</td></td>

<td></tr></td>
<td></table></td>

<td><table></td>
<td><tr></td>

<td><td>Fix interpolation of integers</td></td>

<td><td><img alt="image" src="https://lh5.googleusercontent.com/dwWcE_WQ7lAnDlzDHXYS4PX74JG45uW1NkiqcVk_jHGHWcftBOl9VGSk8HgiGNQ4FqxAkKEDpaSaS2a185e2i3jqNrUVjra8HJFGsYvafm3QfjVApujEw_OgnLB-abUQIMVbSsQo_D4wz-3YINcUx5px_w77bzy8cLZ-AcJUSXNoDgNv" height=268 width=283></td></td>

<td><td>The test case above demonstrates a bug: the 8 digit CSS number was serialized to 6 significant digits, and the out-of-bounds treatment turned it into a 0..</td></td>

<td><td>Below is the fix to the problem.</td></td>

<td><td><img alt="image" src="https://lh3.googleusercontent.com/7VFctlPazHy6OcCxSRmLho80W_1aRSct288IUC88n4aA-mlgpWy7OGMJLnRqcl-9y4gZ04A8h9qCI5i9i-j1dXZhvcl9NFwDx6JXTh_VnOWn3djnBh0taY3ichwvnmSlIMHTcKDDPE1TYSDUiq2QIS4lQvdv_h2qdmlrLThih4j8UZg3" height=47 width=283></td></td>

<td><td>overscroll-behavior</td></td>

<td><td>Our ongoing effort to reduce mis-usage of overscroll-behavior (<a href="https://docs.google.com/document/d/1-pqljDgzzgRVve2oWUq237JPgtT12wcoctxbtBaHZdU/edit#heading=h.1xrzhx8chbuz">details</a> here) moved forward with a fix landing on Google Search.</td></td>

<td><td><img alt="image" src="https://lh6.googleusercontent.com/AbvzIFWaIWqOw7OrIaCNCm-UlAonjWT78-em1KgWUDV4cDzbEPzdgFSlRhemYow7gzCrffeb7nMrj7c72IkFAAQdN9ZT9-XmzEjPDHMudAU52i_PAUcU8QaS_oEayjSLxvrOM7Z5n9ae_z6-0qbC1d92JeU-kFD2ELY3lRswb8q9zGbv" height=59 width=283></td></td>

<td><td><img alt="image" src="https://lh5.googleusercontent.com/pzrNAt-Kl8VKbRTrvOvrtQLcFb7im-nW5--4Wx_BNkOCa-ZL3MIOWXyzbrPRz_PR8A6dyCoUHITbPe-6PjOYvDy4Vt7b4G-xQ4PSq95b7gPqUTbpyDkcyHpuKyBwD_6YWIG_jfjHMRoaKYoSv4PXtdi7RKXltISb0zih3FeC5vG29dCi" height=105 width=283></td></td>

<td><td>The first graph shows the usage on Google, and the second graph shows the usage of all page load, which is about 0.3%.</td></td>

<td><td>There are two ways to move this forward:</td></td>

    <td><td>Through devrel outreach (<a
    href="https://docs.google.com/document/d/1-pqljDgzzgRVve2oWUq237JPgtT12wcoctxbtBaHZdU/edit#heading=h.1xrzhx8chbuz">doc</a>)</td></td>

    <td><td>Push a spec change (<a
    href="https://github.com/w3c/csswg-drafts/issues/6406">proposal</a>)</td></td>

<td></tr></td>
<td><tr></td>

<td><td>Position: sticky 100% pass rate on chrome!</td></td>

<td><td><img alt="image" src="https://lh4.googleusercontent.com/eNLvAI5MdHjZRW1284KVgALG5aQcZUluy3twk8P9GsZyr6XsfvXlTb0s3ZBjx_C8nGiy7F2LNFVsnlXIHT8Rr4ofTVtuhDxA2OaXu0BpLokp99uOff3GpAIM05Y24HtZCNE54glfHGM6kZX2HSzobneiU-46_SMZA-dZjohBHddExuM9" height=135 width=283></td></td>

<td><td>flackr@ wrapped up one of our team’s key product excellence/interop OKRs by fixing all WPT tests for position:sticky. Thanks to all the folks on the DOM team that contributed. (<a href="https://wpt.fyi/compat2021?feature=position-sticky">link</a>)</td></td>

<td></tr></td>
<td></table></td>

<td><table></td>
<td><tr></td>

<td><td>Chapter IV: Bug Updates</td></td>

<td><td><img alt="image" src="https://lh6.googleusercontent.com/DgKWR43wOdhft-vh_ub8eFt_JbD7XgQ4r4mFRz1DRPLdLvd2RmZI4rnzkPmD1nnkYPYXPXGYB1QglE3LFnB2FFW-oZ_O4CBqgwLn16DG9YlutSRXV2-xDU6SQ2B8HjG93Jl2OSwxO3lpHqVPlVk_tf7IrmXl7Xsh4ukG0KcyQl5i9aBo" height=159 width=288> <img alt="image" src="https://lh4.googleusercontent.com/x5OkzI6ZArtT0yPVOCJoEmt_fxe7tCcE_L5pq5eZ_4uns0qpnNfmQErEXdu3iq9mCaVBgg3jxjIqH-OqRRbGQueSwqi8NJN91sRc8xVcw_YBGAntWLmXtQblkkYtrl3dzT6me0wNvIyfr_8dqiK6UudVqcXv4vPbveQ6gOx1rsMF44ZS" height=158 width=284></td></td>

<td><td>Our team made great progress on fixing P1&P2 bugs and the numbers are going down!</td></td>

<td></tr></td>
<td></table></td>

</tr>
</table>

<table>
<tr>

<td>Chrome Interactions Highlights | June 2021 (Vol 2)</td>

<td><a href="http://go/interactions-team">go/interactions-team</a></td>

</tr>
</table>
