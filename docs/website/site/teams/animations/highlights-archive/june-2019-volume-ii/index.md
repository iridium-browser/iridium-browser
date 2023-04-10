---
breadcrumbs:
- - /teams
  - Teams
- - /teams/animations
  - Animations Team
- - /teams/animations/highlights-archive
  - Highlights Archive
page_name: june-2019-volume-ii
title: June 2019, Volume II
---

<table>
<tr>

<td>June 2019, Volume II</td>

<td>Chrome Animations Highlights</td>

<td>Archives: <a href="http://go/animations-team-highlights">go/animations-team-highlights</a></td>

</tr>
</table>

<table>
<tr>

<td><img alt="image" src="https://lh4.googleusercontent.com/et9ryoulZxztot-SLE6W1uC3kLdirsX9ha2wyFDxr7nGs0gAH_64JV5xiHk97i4w-urHvJpsmbzHGwdHtOgwBPdEuQTyw_nvlmguJ4eH27gSTADAB7QjeIdEjSoua_BoDKNL4uNs" height=334.29598821989526 width=593></td>

<td>Is there a poltergeist trapped inside Chromium? Not any more, thanks to flackr@.</td>

<td>Gmail, possessed?</td>

<td>A spooky bug floated our way this sprint as users began reporting that their gmail and twitter pages were <a href="https://bugs.chromium.org/p/chromium/issues/detail?id=962346">scrolling without any user input</a>! After initial analysis from the input team (great work by bokan@!) identified it as a BlinkGenPropertyTrees-related Animations bug, Rob (flackr@) spent days teasing out first a reproduction, then a diagnosis (surprise, <a href="http://crbug.com/962346">it was complicated</a>), and finally landing <a href="https://chromium-review.googlesource.com/c/chromium/src/+/1256422">a fix</a> that took care of this ghostly occurrence. Who you gonna call? Rob Flack, apparently.</td>

<td><table></td>
<td><tr></td>

<td><td><img alt="image" src="https://lh5.googleusercontent.com/dG77X96y5b6huezy55beT998DAxMlJb5Bv_bI9d2WFqcyiJS0Fgx9K8JHcZIaRVEn-ljQusGlsLThtcV54ChP-tZx4mgIgdNmzie2pMDvcBT4fBUbzg1_zlbNfFvR9Ev5ywFVVXm" height=179 width=251></td></td>

<td><td>Animation Worklet - it's (more) official!</td></td>

<td><td>Animation Worklet took not one, but two big steps forward this sprint. We sent our official <a href="https://groups.google.com/a/chromium.org/forum/#!topic/blink-dev/aRKT0BkrF-8/discussion">Intent to Ship</a> to blink-dev@, reflecting our opinion that the first version of Animation Worklet has reached maturity and is ready for real users. Coincidentally - and nearly simultaneously - the spec was also <a href="https://www.w3.org/blog/news/archives/7830">promoted to First Public Working Draft</a> during this sprint.</td></td>

<td><td>Congrats to all the people who have and continue to work on Animation Worklet!</td></td>

<td><td><img alt="image" src="https://lh4.googleusercontent.com/6640vwTykAzTgcwoKk0u4ZqXpZioR6PYeo48yUPWIb9S4jaLgOc-kfxXGrGvTscFRVW51tJVpdt0bUiD0VhYZhhNbRrEhgiAZC2vrT6-uZgjHVaPDIeUpOAHAd9Shz8Jknd38Xef" height=179 width=277></td></td>

<td><td>ScrollTimeline prototype lands</td></td>

<td><td>Browser support for scroll-linked animations is a common request from web developers, as JS based solutions suffer badly when the page janks. We took one step closer to fulfilling such requests this sprint with Olga (<a href="mailto:gerchiko@microsoft.com">gerchiko@microsoft.com</a>) landing a (main-thread only) prototype of <a href="https://chromium-review.googlesource.com/c/chromium/src/+/1597286">ScrollTimeline for Web Animations</a>. </td></td>

<td><td>Interested developers can now run Chrome with --enable-blink-features=ScrollTimeline to play with the prototype - but be warned, this is still very early stage!</td></td>

<td></tr></td>
<td><tr></td>

<td><td><img alt="image" src="https://lh3.googleusercontent.com/RSKmCRIPTTUphzkYEdus7axPNzy7YgZaToyRaJ2gSvyS4RECsUSwcBvKOqXBxHJ63uoZFLFCJOAzm2lEq5KBKmVo8vL5Qs149NrizzxlPxqY-Xx5ZC4_IpTwwBEQ7eBnm_hvTEXu" height=164 width=283></td></td>

<td><td>Exploring the Animations space</td></td>

<td><td>It can be easy when working in the browser space to lose sight of what our corner of the web platform is actually like for web developers. This sprint Gene (girard@) put together and published <a href="https://docs.google.com/document/d/1hPfNx9aM7KHRO7DZDTvrBAQUbTdYIap9U__uqp_-EQo/edit?usp=sharing">an overview</a> of the tools and processes web developers are using to create and deliver animations on the web. This form of insight is vital to better understand what features we should be prioritizing, and to deliver a better web for everyone.</td></td>
<td><td>(Chart above uses data originally from <a href="https://trends.builtwith.com/">https://trends.builtwith.com</a>.)</td></td>

<td><td><img alt="image" src="https://lh6.googleusercontent.com/8HBpbkDCHE7aPyKADf56885lm6O9DjjjxZ4PEpqWoW2oc0n89WUMnJK6ySLkUMH7afHHKB5U7CP-fggLDrFcHD0uYKOHTZbb3JPk8gha16UvPrpMQVp-2HWvkb7eHNt6I2ImCoX8" height=173 width=283></td></td>

<td><td>PaintWorklet HiDPI bug squashing</td></td>

<td><td>When developing his PaintWorklet-based Lottie renderer <a href="/teams/animations/highlights-archive/june-2019">last sprint</a>, Rob discovered a few rendering bugs relating to PaintWorklet on Mac HiDPI devices - such as the unexpectedly cropped image above. Thankfully such bugs are no more, as Xida (xidachen@) spent time this sprint hunting down multiple zoom and HiDPI related PaintWorklet bugs and fixing them.</td></td>

<td></tr></td>
<td></table></td>

<td><img alt="image" src="https://lh4.googleusercontent.com/5698PMmqluSrFNc6Ejfmw43rLCwUvxnbGistZt8RddT1RsxFRiHQdBm5P2RbliVc3cr77vgp0PS8Vou8XaLGFmLd4MsBQn5pJbKstBDX4u3o-T58FCv5_mmwOgk3pdAFTitVJODA" height=413 width=418></td>

<td>Do you understand this? Yeah, me neither. But kevers@ does!</td>

<td>Better Beziers</td>

<td>Sometimes, you just have to get down into the weeds to improve browser interop. This sprint Kevin (kevers@) did exactly that as he tackled the hairy problem of bezier curves - namely, why does Chromium's implementation produce different values than other browsers? Details of Kevin's explorations could probably fill a small maths textbook, but in the end he was able to <a href="https://chromium-review.googlesource.com/c/chromium/src/+/1643973">land a new approach for Bezier estimation</a> that took half the time of the old method, made Chromium pass almost 20 previously-failing WPT tests, fix two Chrome bugs (issues <a href="http://crbug.com/591607">591607</a> and <a href="http://crbug.com/827560">827560</a>), and exposed a <a href="https://github.com/w3c/csswg-drafts/issues/4046">hole in the spec</a>. We think Monsieur Bézier would have been proud. </td>

</tr>
</table>

<table>
<tr>

<td>Chrome Animations Highlights | June 2019, Volume II</td>

<td><a href="http://go/animations-team">go/animations-team</a></td>

</tr>
</table>
