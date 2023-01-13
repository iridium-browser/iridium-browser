---
breadcrumbs:
- - /teams
  - Teams
- - /teams/animations
  - Animations Team
- - /teams/animations/highlights-archive
  - Highlights Archive
page_name: june-2021---new-features-stability-fixes-interop-fixes-and-more
title: June 2021 - New features, stability fixes, Interop fixes and more!
---

<table>
<tr>

<td>June 2021</td>

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

<td><td><img alt="image" src="https://lh4.googleusercontent.com/rHya85oxF853pbXs1D5q8q9SV2xekZtWIWWGcmYQ4s6DlvXcyGeuS_9Zk9Vb5qk73JxTEjFTzLQHUxj8laup5dbCOZ4vXP0fXUL_xtd-WNFlLQn6El0whQwe5Do1V192eHW1hJrzKA" height=19 width=283></td></td>

<td><td><img alt="image" src="https://lh5.googleusercontent.com/RNatxDtrJzdHYIC5oHcgxL7ajcpK2Iv2l2-_ShR6Idv0KXInV0qYhlB-Bh9n7Ft7UW70gucCbUWJAw0bBwMP-taUrZUjttltaiFpnzfw6Yugeniws_M4LjMjajXuT628gNL1cKRKKA" height=19 width=283></td></td>

<td><td><img alt="image" src="https://lh3.googleusercontent.com/IJMi8TbxUl9qriGc6ChrTf74Z7Bhz0Cv3kJ31Ja-JSoR4c-HKqULxw2yBiC7JZzHbExW3uuug6BEcxqyfpZCWXOh3hFEg9Njp4eALgbJLCPtJi7JZwttbKeUZqOZJIzV-BVbOlhUAQ" height=19 width=283></td></td>

<td><td><img alt="image" src="https://lh4.googleusercontent.com/hlpW92nOALGoQeoWIryCOpkDHyUL7jSqn5k41puVwjNqgz2JZNUmY5lPlHNu2udqf1xarZjBsvaeIaInsD-2Vrh4lEeGLMvg7HvKxWCEM90y_EkbUjWZcUKQ9mfT2kPs1p2b68X-mg" height=16 width=283></td></td>

<td><td><img alt="image" src="https://lh5.googleusercontent.com/Lmtm-9xfWUAEkzirIRr0U0UyWXKpC21Po57pjhX3P_Zx6_97j_yToFzmmXvL373Z2pMZs_ihciB-8qnPbqPDlzB_xXizKfkt1aiL4ynidq2PXRmOZNMawljumRcslU3kG5KmuKyT-A" height=59 width=283></td></td>

<td><td><img alt="image" src="https://lh3.googleusercontent.com/Rs6HguE0Zjz-SYNiIOZ83SbSe3Mdfb0ogO8T14iY7WAp2DOr96Jbn5HVUnjKui8AApsIToC1WJQzwPfXU6aSJh3IW_-ZjZssMtjWn9uHmwjPVSw1up81oN2B211uLI9OLLkvEkve8w" height=21 width=283></td></td>

<td><td><img alt="image" src="https://lh5.googleusercontent.com/scZffxszmv6AWdv6iaxx2_56xvqqTe6bBLPlV8hMp3SVoDKEOmFms5N48S0KIjVMteqCeu5ZWiuv93U0AdZmQhVyN6ubSRPANxtJZc6wCEywMlqNs0uTFdkFvqtNrDiGAyHIKwV4rQ" height=40 width=283></td></td>

<td><td>xidachen@ launched finch study for this feature and this <a href="https://docs.google.com/document/d/1Fkp7udbCgYqVtNf4gn-NXGVYamPrd_n0qKqalruTq6E/edit#">doc</a> summarizes its result. Here are some high-level points:</td></td>

    <td><td>We are seeing more animations being composited.</td></td>

    <td><td>Unfortunately we don’t see improvement in the “PercentDroppedFrame”,
    which reflects the smoothness of the animations overall. One possible reason
    is that background-color animation is mostly applied on small buttons and it
    is not the dominant factor of dropped animation frames.</td></td>

    <td><td>We are seeing memory regression and this is tracked by <a
    href="http://crbug.com/1210221">crbug.com/1210221</a>.</td></td>

<td><td>Moreover, this feature is now experimental, which means one can enable it by enabling “Experimental Web Platform features” in chrome://flags.</td></td>

<td><td>Capability Delegation Spec</td></td>

<td><td><img alt="image" src="https://lh5.googleusercontent.com/QZpkzz-EcZyPGYxfeNVDeC5oyrwyLaDXLDJDPXlZXIouySfVIVqPvOWJPliQFiOlc2EbyU467uu5dVFd9exgj2aYo5kg6-rg59ma-zb_Yxp7jmQdyNJb3GbX3K7t2ppDOh_tDUOD8w" height=88 width=283></td></td>

<td><td>mustaq@ finished the draft spec (<a href="https://wicg.github.io/capability-delegation/spec.html">link</a>) and it is ready for review.</td></td>

<td><td>Android 12 Overscroll</td></td>

<td><td><img alt="image" src="https://lh6.googleusercontent.com/0h7u2zpQbuiN4fQIIkFzibmx86KdLvK5X-P-vFKr1bGrj5AhP5FV5_ZMHQ6_YnWA-a24li0BjD2_JMwr5vC4FHCIunYc9StHR8m4MLuViGUoYS8xQewQYJIVIQ9nY1vTN70pCqRHzw" height=507 width=240></td></td>

<td><td>flackr@ has finished the initial version of working overscroll.</td></td>

    <td><td>It re-uses elastic overscroll physics from Mac overscroll.</td></td>

    <td><td>Instead of translating, it applies overscroll stretch on the
    transform node.</td></td>

    <td><td>There are lots of follow-on work including:</td></td>

        <td><td><a
        href="http://crbug.com/1213217">crbug.com/1213217</a></td></td>

        <td><td><a
        href="http://crbug.com/1213248">crbug.com/1213248</a></td></td>

        <td><td><a
        href="http://crbug.com/1213252">crbug.com/1213252</a></td></td>

        <td><td>Enable only on Android-S</td></td>

<td></tr></td>
<td></table></td>

<td><table></td>
<td><tr></td>

<td><td>Chapter II: Stability fixes</td></td>

<td></tr></td>
<td></table></td>

<td><table></td>
<td><tr></td>

<td><td>Fix privacy leak of visited links</td></td>

<td><td><img alt="image" src="https://lh3.googleusercontent.com/U7PEC5Lsr_AZGA2ft-FNFL1QB__w3tp_c5JqGfL20L0qnE_MSu6L3U62BNNoSEAt3GO_dAmvcgkMac1Ue0xGHp0tfU7xK0h1eK2XLksH5vlhTbAqCaNoo_8n6akgSW-6dATvDJIWrw" height=119 width=283></td></td>

<td><td>kevers@ fixed a privacy leak issue of visited links. A repro case is shown above. The fix to the issue is:</td></td>

    <td><td>Extract the visited style even if link is unvisited during style
    resolution.</td></td>

    <td><td>Generate a transition if either the visited or unvisited style
    changed.</td></td>

    <td><td>animation.effect.getKeyframes reports unvisited style regardless of
    whether visited.</td></td>

    <td><td>Visually render with the correct style. This is possible since the
    interpolation code retains a pair of colors to interpolate corresponding to
    the visited and unvisited style.</td></td>

<td><td>Fix a crash in Mac IME fix</td></td>

<td><td><img alt="image" src="https://lh5.googleusercontent.com/dpHdLwkQP2mvEqUTWU9WqebZlmcMHpkvca2sQQ_PDj6vI0xu_U5Gfuj2nZtp_1qr0sPM9ii3oWPPjHzGoT8yXVrrgF8Zoa48yDC5IrR91f9oJHF9nEqPZ63rcY_B3wkVntyVeRBHsQ" height=112 width=283></td></td>

<td><td>flackr@ fixed a crash by performing a null check.</td></td>

<td><td>Removed one fake user activation</td></td>

<td><td><img alt="image" src="https://lh6.googleusercontent.com/UFQIE4kr5dmWZAXVGBKCYupnAMRfM5mnJOZzxQV5hNium6j1f8EZNgiiYpTU3ANfDA86IdaiwzE6A5ROxhkidFQSZ6c_41ESS9MmG-4TaIUx6Qva4YSktkMXi7RzNesE_lsQ6kTVaw" height=40 width=283></td></td>

<td><td>mustaq@ fixed a fake activation issue as a suspect for <a href="http://crbug.com/1201355">a navigation problem</a>.</td></td>

    <td><td>UMA investigation revealed <a
    href="https://bugs.chromium.org/p/chromium/issues/detail?id=1082258#c7">&lt;0.005%</a>
    page loads could be affected.</td></td>

    <td><td>Tentatively removed the fake user activation notification from JS
    play() and everything seems good as before!</td></td>

<td></tr></td>
<td></table></td>

<td><table></td>
<td><tr></td>

<td><td>Chapter III: Interop fixes</td></td>

<td><td>Overscroll-behavior</td></td>

<td><td><img alt="image" src="https://lh6.googleusercontent.com/S2OGqJ8UBP5vXu-AHhhmG08aQ8vAqNZiIfalExhOPEMLceQvuMFl46LyzwWSQ8z7GuIAN72tD8D1ggtAWgoxMyyMu3fT_mq_a7C8pEzfn6AJGJQpdqNOC9TBCy0ETSsiXw1rFtJ7Pw" height=249 width=219></td></td>

<td><td>xidachen@ kept working on the <a href="https://bugs.chromium.org/p/chromium/issues/detail?id=954423">bug</a>, where chrome propagates overscroll-behavior from the &lt;body&gt; element, while by spec it should be from the &lt;html&gt; element.</td></td>

    <td><td>A detailed <a
    href="https://docs.google.com/document/d/1-pqljDgzzgRVve2oWUq237JPgtT12wcoctxbtBaHZdU/edit?ts=609ec0c2#heading=h.1xrzhx8chbuz">doc</a>
    describing the problem.</td></td>

    <td><td>Break the feature counter into two cases. (<a
    href="https://chromium-review.googlesource.com/c/chromium/src/+/2896470">CL</a>)</td></td>

    <td><td>Querying UKM table shows that google.com is the major site that have
    this problem.</td></td>

<td></tr></td>
<td></table></td>

<td><table></td>
<td><tr></td>

<td><td>Chapter IV: à la carte</td></td>

<td><td>Bisecting on Android</td></td>

<td><td><img alt="image" src="https://lh5.googleusercontent.com/m4--LoEEaNPk4c_0-r7W7qUQT-Mt9LxW1868uj1C_nkaaI0eRXELyBdeKBxezMPUxQOOYdeVdDp49kFfesUTzjUcnIH0LbsriCUxgNahkomeazrCDesL0A3wSlGxxz09goTyRACTzQ" height=197 width=283></td></td>

<td><td>skobes@ found that it is quite annoying to bisect on Android, but it is possible, and also that the tool could be improved.</td></td>

    <td><td><a
    href="https://docs.google.com/document/d/1e6IXv9hZQKeOsBx5ywjsBnqS0ZLW9kY2kald_RErJ0w/edit">go/chrome-android-bisect</a>
    contains all the details on how to bisect on Android.</td></td>

    <td><td>skobes@ used that for two recent top-controls regressions</td></td>

        <td><td><a
        href="http://crbug.com/1167400">crbug.com/1167400</a></td></td>

        <td><td><a
        href="http://crbug.com/1207888">crbug.com/1207888</a></td></td>

<td><td>AtomicString for PointerEvent type</td></td>

<td><td><img alt="image" src="https://lh4.googleusercontent.com/-60-y1VG2NM4PHDvKf5ZjwIymMNYmYsGdxDZXwvQurFZFpIpsgzJ3VtVNqtOCofZHu0HEd8rPwAk6Wx89o2n4AhjHqLYtZL-fWGiwZuf5SIcEUDafQaa8NpUqOPuPFGQPw6uZ5vQ2A" height=222 width=283> <img alt="image" src="https://lh4.googleusercontent.com/FaaGEYPh_5yWVoGm20LLQ6CEutr55-nK29wKIhTRLLqcZ-4_oyH4A5NDAEcfVmcusUa04evUcPSucEzzCnDcnqcAU9PgdRL_vCikZY3xrpuE-EilMwRQZ4pB7YkE65zZx6r9O3qVdw" height=216 width=280></td></td>

<td><td>flackr@ <a href="https://chromium-review.googlesource.com/c/chromium/src/+/2878101">replaced char\* with AtomicString</a> for PointerEvent type. Why?</td></td>

    <td><td>The returned string is passed to PointerEventInit::setPointerType
    implicitly cast to a WTF::string for the rest of its existence. What’s the
    problem?</td></td>

    <td><td>First, we take the strlen of the string, which is O(n)
    operation.</td></td>

    <td><td>Then, we allocate storage space and copy the string constant, which
    is O(n) time + space.</td></td>

    <td><td>If we instead construct a String from another String, then it just
    adds a ref to the StringImpl, and that’s O(1). Refer to the screenshot on
    the left hand side for details.</td></td>

<td><td>Now the String is constructed, are we done? Well, not yet, we have to compare two strings.</td></td>

    <td><td>Comparing two arbitrary Strings is O(n), see EqualStringView
    implementation.</td></td>

    <td><td>But, comparing two AtomicStrings is O(1)! Refer to the screenshot on
    the right hand side for details.</td></td>

<td></tr></td>
<td></table></td>

<td><table></td>
<td><tr></td>

<td><td>Chapter V: Bug Updates</td></td>

<td><td><img alt="image" src="https://lh4.googleusercontent.com/9pOwepddVaq1FOyV6gnlnpNiwcjT7bJTpRPEsbmAF4_pHGvoXvHSXPVJT3564WtjQMcPaf2kBxdO8LI3PYbh6UAuL4knuYVgCgQK6hR3OdHwCATe4UcB1-B8z_I02CMj6XCpIBSbcA" height=155 width=280> <img alt="image" src="https://lh3.googleusercontent.com/DQwfAQnP-zJ455WAFGOv6VvpLyKTwXBlBTnDiz9EopH_xeLuGmQVvFH-OqjbEqjBczQfcHPpe-lW4yxg7v5iQ5P09SRMiVUXYdRMriK-oNKkfctmtimxAfJEaVsLIPTwTFN7ZMgEMg" height=153 width=278></td></td>

<td><td>Our team kept on top of bugs. We closed almost the same amount of P1 bugs vs opened P1 bugs.</td></td>

<td></tr></td>
<td></table></td>

</tr>
</table>

<table>
<tr>

<td>Chrome Interactions Highlights | June 2021</td>

<td><a href="http://go/interactions-team">go/interactions-team</a></td>

</tr>
</table>
