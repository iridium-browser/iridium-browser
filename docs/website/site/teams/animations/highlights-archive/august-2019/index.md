---
breadcrumbs:
- - /teams
  - Teams
- - /teams/animations
  - Animations Team
- - /teams/animations/highlights-archive
  - Highlights Archive
page_name: august-2019
title: August 2019
---

<table>
<tr>

<td>August 2019</td>

<td>Chrome Animations Highlights</td>

<td>Archives: <a href="http://go/animations-team-highlights">go/animations-team-highlights</a></td>

</tr>
</table>

<table>
<tr>

<td><img alt="image" src="https://lh4.googleusercontent.com/oITVrglJINBJXDLj60V-z4SMEW9MG6rx2JrEdK6SyHqpBrYlhTL2l8LaJlvJh2jpm1XIXfjt788fB0wNN2xzzaDC1sy5tQ9jLTL-kFadhFnPNQdGNnB_vWtZHqp6QY9S4j02jkHR" height=267 width=596></td>

<td>The opening slide from smcgruer@'s <a href="https://docs.google.com/presentation/d/1qSNpFJaCvuqibe0iPc1tBSwAPp1LSXFqPIKun-_Pj_U/edit#slide=id.g5f8061889d_0_0">presentation</a>. Fun fact: Google Slides is convinced 'casually' is not a word.</td>

<td>Investing in knowledge</td>

<td>To borrow a quote that was (maybe) said by Benjamin Franklin: "An investment in knowledge pays the best interest". Stephen (smcgruer@) embodied Franklin this sprint as - faced with <a href="https://crbug.com/979952">a bug</a> that he could not understand - he went back to basics and taught himself the Blink Animations interpolation stack from the ground up. His investment paid off. Stephen was not only able to fix the original bug, but he also discovered and fixed <a href="https://crbug.com/992378">another bug</a>, and gave <a href="https://docs.google.com/presentation/d/1qSNpFJaCvuqibe0iPc1tBSwAPp1LSXFqPIKun-_Pj_U/">a presentation</a> to the rest of the team sharing what he had learnt. The S&P 500 would be jealous of that <a href="https://www.investopedia.com/terms/r/rateofreturn.asp">RoR</a>.</td>

<td><table></td>
<td><tr></td>

<td><td><img alt="image" src="https://lh5.googleusercontent.com/z53pe_NaaKwQUwC7wEzpQwDm0SgTT8WqRF91CAbqIKQ_pmh8HP_ZkhaUFx4kepVUD8QA09z961YZIfgy3OVEPy9D3oaj41aErpS_e-gypoow_vzRXtxiZKgsen6COqvZTQe_U3fn" height=171 width=281></td></td>

<td><td>Constantly in style</td></td>

<td><td>The interaction between Animations and Style is a subtle and sometimes fragile one. Animated objects are always changing, but we try to avoid causing full CSS style updates because those are expensive. Sometimes, however, we miss cases. This sprint, Rob (flackr@) discovered that pseudo-elements could <a href="https://crbug.com/988834#c9">override our no-change detection</a> code and cause unnecessary main frames! Thankfully our friends from the Style team were able to put together <a href="https://chromium.googlesource.com/chromium/src/+/b1cadc00d4f06846f7c426f66ee4a49e6a543177">a fix</a> and give our users back some frames.</td></td>

<td><td><img alt="image" src="https://lh6.googleusercontent.com/ScnUy4fn8zMfH0LcXR9ggqqatjErYxfCtLmcWEGgG06MI16f4PZWkHluvlzbAApSo0hE7BXCDDMSz5l8NBl6CJ2ncMTimLiaISR5eY954baOZHR3Hii6a_o8D7IxJ3XAXp7GP10q" height=227 width=115> <img alt="image" src="https://lh4.googleusercontent.com/lkKYKI9N9e8vb0OxTHmKn9UFUQcRb1JKezJUy9iDMY3anAvzIxMAGXB2VfOs03tiSfDcZKpmevU9DRz860L_O7pD6sdYUS90lfsXbcKAWKLQlSw3GS0DiL4mJTbwEvlSM3JhA9-a" height=227 width=115></td></td>

<td><td>Smooth Paint Worklet animations</td></td>

<td><td>An important goal of the <a href="https://docs.google.com/document/d/1USTH2Vd4D2tALsvZvy4B2aWotKWjkCYP5m0g7b90RAU/edit?ts=5bb772e1#heading=h.2zu1g67jbavu">Off-thread PaintWorklet</a> project is being able to animate Paint Worklets on the compositor thread. This keeps them smooth even if the main thread is busy. As of our most recent sprint, this is now working (behind a flag) in the latest Chromium code! The example above shows that the animation is smooth (left-hand side) with the flag turned on, and less smooth (right-hand side) with the flag off.</td></td>

<td></tr></td>
<td><tr></td>

<td><td>Timing is everything</td></td>

<td><td>Kevin (kevers@) has been celebrating the summer months by tackling an ambitious code health project - bringing blink::Animation in line with the web-animations spec. Thanks to Kevin's hard work (and a steady supply of strong coffee), our implementation is slowly but surely converging with the web-animations spec. This not only makes it much easier to understand, but a healthy number of web platform test failures based on tricky timing issues have been squashed. Fantastic work by Kevin!</td></td>

<td><td>Countable CSS</td></td>

<td><td>Taking a brief break from direct Animations work, Majid (majidvp@) discovered some <a href="https://crbug.com/993039">problems</a> with how CSS UseCounters are created this sprint. The manual (!) process was complex, missing a step in it could cause cascading failures for later-added CSS properties, and there were no automatic checks at all that it was all correct. No longer, thanks to Majid - he managed to <a href="https://chromium-review.googlesource.com/c/chromium/src/+/1752503">remove one step</a> from the manual process and also added a set of <a href="https://chromium-review.googlesource.com/c/chromium/src/+/1755413">automated presubmit checks</a> so nobody else will have to go through the pain he did!</td></td>

<td></tr></td>
<td></table></td>

<td><a href="https://www.lucidchart.com/documents/edit/069541b2-a5d7-4b5f-89f3-56455afac3f3/0?callback=close&name=docs&callback_type=back&v=972&s=660"><img alt="image" src="https://lh5.googleusercontent.com/h1XoTiLsay-bJT3_4WgJYlI7xCNMbSjeppOoc04D-JAZ-d3JInkO3NQcRt13bDSifCULj3olIczD_QH9d9XEg29oG6TlP285eaX8ebrPOJeXhoGKAxG5egQ6VJ9G9Upg1wgAqiGJ" height=394.6268181818181 width=511></a></td>

<td>Figure that shows how the verification of hit test result works and how it does not work</td>

<td>False positives + mismatches == matches…?</td>

<td>Viz hit testing v2 is heading to stable (yay!). This sprint Yi (yigu@) investigated the remaining cases where the v2 result does not match what Blink comes up with. In a moment of serendipity, it turns out that half of the mismatches were false positives due to an imperfect verification path. When iframes are slow to load, there were <a href="https://docs.google.com/document/d/1AlDyVvKtZ_SZZey_76srFqaTM-QPor0pKc6VXE7lEMU/edit">three points in time</a> where Viz and Blink used different hit test data for the verification - which lead to mismatched results being reported. With this bug fixed, the mismatch rate dropped from 0.04% to 0.02%.</td>

</tr>
</table>

<table>
<tr>

<td>Chrome Animations Highlights | August 2019</td>

<td><a href="http://go/animations-team">go/animations-team</a></td>

</tr>
</table>
