---
breadcrumbs:
- - /teams
  - Teams
- - /teams/animations
  - Animations Team
- - /teams/animations/highlights-archive
  - Highlights Archive
page_name: august-2021-vol-2---new-features-code-health-stability-security-fixes
title: August 2021 (Vol.2) - New features, Code health, Stability/security fixes
---

<table>
<tr>

<td>August 2021 (Vol. 2)</td>

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

<td><td>Composite BG-color animation</td></td>

<td><td>xidachen@ resolved a few problems and this feature is now close to finch on Beta. Specifically we have enabled field trial testing (<a href="https://chromium-review.googlesource.com/c/chromium/src/+/3085961">CL</a>) so all perf bots can run tests with this feature. All regressions and improvements are summarized <a href="https://docs.google.com/document/d/1Fkp7udbCgYqVtNf4gn-NXGVYamPrd_n0qKqalruTq6E/edit#heading=h.6genmqxclwba">here</a>.</td></td>

<td><td>We have resolved the memory increase (<a href="https://chromium-review.googlesource.com/c/chromium/src/+/3088776">CL</a>). Here is an <a href="https://bugs.chromium.org/p/chromium/issues/detail?id=1238995">example</a> where a perf test used to crash due to OOM now works fine.</td></td>

<td><td><img alt="image" src="https://lh3.googleusercontent.com/BlgpoWYHjGO-GwxXaaDY5Bx0AFAp6cww8WbcxkZecQRKvkvY5tIw60xzDE6XiWEbGIzGoH7f9deTeZKU_0QNoQkLwKJzl9PGfYVYW_SI3iFnwP-9j3zQ2EWUpytmUK4qtebbkwU6_w=s0" height=47 width=283></td></td>

<td><td>There is a performance regression (<a href="http://crbug.com/1238554">crbug.com/1238554</a>) that has been addressed as well. It was fixed by this <a href="https://chromium-review.googlesource.com/c/chromium/src/+/3099785">CL</a> where we found that we were doing a lot of un-necessary work which slows it down.</td></td>

<td><td>Note that the above curve has improved, but didn’t go back to the original level. With further investigation, we found that the root cause is because the tests aren’t well written. Specifically, the tests contains background-color animation on a solid-color layer, and our system has optimizations towards solid-color layer which is not implemented in CompositeBGColorAnimation yet.</td></td>

<td><td><img alt="image" src="https://lh6.googleusercontent.com/No7kJ2RfnZ3hpi1d_b0LDRdEFLDcUr-vyYLSOJode6Ie9Hz1lc5Eb1tE_seIIroBsuH0th5CDWb5D3h4DGzXOBFDYIxBzmQAyCvhEi0zdnuV5hUfXjcTm7agkTIxhL4znpgP4biLzQ=s0" height=111 width=283></td></td>

<td><td>A <a href="https://pinpoint-dot-chromeperf.appspot.com/job/16e27f8d320000">pinpoint job</a> was started where the layer made to be not long solid-color and the above shows the result. The right column is with CompositeBGColorAnimation and we can see that it actually made a performance improvement.</td></td>

<td><td><img alt="image" src="https://lh5.googleusercontent.com/_h4jB7RpT2aAS7wMfWe_gT486MtA5VxCbyNwkZT9JN3Z4GP8bMaWrucDgES4rAxjw-SQcdoJUSgrJ7DMixmtbIc9kVCaDPGWSZ2iFK9ExmHguggBqnFXEuzggI1040MFLAKwO7AueQ=s0" height=239 width=283></td></td>

<td><td>mehdika@ modified Element Fragment code (that already exists) to work for any other element.</td></td>

    <td><td>For now we just scroll to the element, later we will add more
    features like highlight the element, etc.</td></td>

    <td><td>We use the <a
    href="https://github.com/WICG/scroll-to-text-fragment/blob/main/EXTENSIONS.md#proposed-solution">following
    syntax</a> and find the element with the help of QuerySelector().</td></td>

        <td><td>https://example.org#:~:selector(type=CssSelector,value=img\[src$="example.org"\])</td></td>

<td><td>Elastic overscroll</td></td>

<td><td> flackr@ fixed two problems in elastic overscroll.</td></td>

<td><td><img alt="image" src="https://lh4.googleusercontent.com/f60VPX4rF8xa05SmWJoUaHAyIB6tYNNOsJiT6bDIH68vIkGxrFu6P80qxYfSkQgkCxN1KavWA1Nl6-i79f-LrePYMQ1diCPvWemIgasnclYNVnqmES49Vb3OXAXruuDkhYNoxCCO6A=s0" height=271 width=140></td></td>

<td><td>The first problem is the <a href="https://crbug.com/1241128">subtle shift in content</a> during overscroll bounce. The fix is to use ScrollTree::container_bounds which includes container_bounds_delta.</td></td>

<td><td><img alt="image" src="https://lh3.googleusercontent.com/luovHFpiZXnYWJflHP4bK1bmtxsX3if5-7Wb1n9bRMnptIcAWRvBr4bjsAjpQtM1K3JEApvhm8rZ_xBvepJsBvv_hftRmUhJUZsvhqHIHm_OVkkEMLen2WWZ6lkoyaMLSdwtalM5BA=s0" height=304 width=144></td></td>

<td><td><img alt="image" src="https://lh4.googleusercontent.com/S-vMAJInKPvA5rr9AV0AYucyWnksNnOyqjFuvQbDy7dXrl7Kl34d7ZEwu5h5EcVbqoGORR25GkHp3quncnezDchwCW90pLQ_RYg70bDBioUBYyqWL1zpbTuZu8c8ZoLSSrszZ0jhXg=s0" height=120 width=283></td></td>

<td><td>Another fix is to reduce elastic overscroll stretch. Particularly, we have updated parameters in ElasticOverscrollControllerExponential to more closely match native overscroll.</td></td>

<td></tr></td>
<td></table></td>

<td><table></td>
<td><tr></td>

<td><td>Chapter II: Code health</td></td>

<td><td>Native paint worklet</td></td>

<td><td><img alt="image" src="https://lh3.googleusercontent.com/9afv77mmImR7H4z7fyr8ZS-XToIG4RhBMkejhY3AOIQUKMXWlA_ySV-EVb-NMYPjga5O-022spge9UB5YYU9CNEOY0Oec9VAL2qqmnRIDm_bTFBoprjXri3myxnLJR-gMvL-Q473BA=s0" height=166 width=267></td></td>

<td><td>xidachen@ moved some common variables && APIs among different native paint definitions to their super class, which results in a negative line count <a href="https://chromium-review.googlesource.com/c/chromium/src/+/3107065">CL</a>.</td></td>

<td></tr></td>
<td></table></td>

<td><table></td>
<td><tr></td>

<td><td>Chapter III: Stability/security fixes</td></td>

<td><td>Elastic overscroll crash</td></td>

<td><td>flackr@ fixed a crash in blink::ElasticOverscrollController::ObserveGestureEventAndResult.</td></td>

<td><td><img alt="image" src="https://lh4.googleusercontent.com/jzBDWMpXaZA89CHIfAgkHHbtj7ZUhGSTXZKdgHtXbrWMBeg5qnpQNg-W8tJo-ZJ1PFVVJwEF1spteUiqZZAU916zfZGTQkitaBZKNJeDnG33GUDr0_u2yZH0tWB87DvIJmbeN68bMg=s0" height=113 width=281> <img alt="image" src="https://lh6.googleusercontent.com/l1LVluQlx5yCSgI90jRXu7kapwviFAWiwQ6LLmuf49essWuTiHcwWtgLrBLAjSMxHZzzLHbHCd2UN0jlCkXQvfS6TBVeEzsEUCdDtX3GZ6gBZ1nUwXNhYHNDlMQVHJJ_vcGJ9lVUGg=s0" height=91 width=289></td></td>

<td><td>The above shows the stack trace, which indicates that ObserveGestureEventAndResult is called on nullptr ElasticOverscrollController.</td></td>

<td><td><img alt="image" src="https://lh5.googleusercontent.com/cNwoM-2-Z2HdN09LJm6TuAH9Rr79DKL_UmMgnLhe4O0pXArSweGnVnflrGUHKj6JeckhafOcK5-YIDsiFah6HinZqgc31dCj4N9ef5GlQNEpVNxyQvRiJXqEm-Vxet4T5Aq0lNXXMA=s0" height=94 width=303> <img alt="image" src="https://lh5.googleusercontent.com/FHMh0qf5yC9jQ3ur_xkp88Gu0beOaqKMd4W8Pj9igoOa2kRaoBVANIhouGa-EKmOsU75nVrdW3yeBH5aDi6GQtz8q5mwNdrXovTda4qGKGxuxPAxgdZUxY_OFLbOrFK1YLgHQJmlRA=s0" height=101 width=261></td></td>

<td><td>It looks like we have added check, as shown in the above. But why didn’t it crash before? It appears that the crash is from the posted task.</td></td>

<td><td><img alt="image" src="https://lh6.googleusercontent.com/baTU4Jeg3NHXIYO6WXzlHVNOW2Z0qFZ3A-zQ7Wj-osP-2nk2FG1QuLk1bf6VqOuy4QsziT0yXkLpwNpPZvf3YBRjYUodulN5EpQ2e38l0256PTmNr1kaQg2jlQF5xYlHZOJ-3f9N9Q=s0" height=177 width=367></td></td>

<td><td>Further investigation shows that the ElasticOverscroll is now controlled dynamically by prefers-reduced-motion, so the fix is to check that too.</td></td>

<td></tr></td>
<td></table></td>

<td><table></td>
<td><tr></td>

<td><td>Chapter IV: Interop fixes</td></td>

<td><td><img alt="image" src="https://lh4.googleusercontent.com/hcw7zn56d5_NPAcmaJy1PaiMmtLKJul2Ze_tfE610DNIQs-wca7nZtOxOtXW-_v2i4IlpP-wyb4_zpJRm2uL2KPlUEGdciq-POm2T7UwR4dcwUyuFeOtLPomHUtDd8TvyNXOqHC1lA=s0" height=236 width=264></td></td>

<td><td>skobes@ fixed a <a href="https://bugs.chromium.org/p/chromium/issues/detail?id=1114794">bug</a> where scrollTo doesn’t abort mouse wheel scroll animation. The solution is to cancel impl-side scroll animation when we get a programmatic scroll. This also fixed a bug in scroll unification.</td></td>

<td></tr></td>
<td></table></td>

</tr>
</table>

<table>
<tr>

<td>Chrome Interactions Highlights | August 2021 (Vol. 2)</td>

<td><a href="http://go/interactions-team">go/interactions-team</a></td>

</tr>
</table>
