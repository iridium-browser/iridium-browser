---
breadcrumbs:
- - /teams
  - Teams
- - /teams/animations
  - Animations Team
- - /teams/animations/highlights-archive
  - Highlights Archive
page_name: animations-documentation-throughput-metrics-code-health-capability-delegation-user-input-security-and-more
title: Animations Documentation, Throughput Metrics, Code Health, Capability Delegation,
  User Input Security and more!
---

<table>
<tr>

<td>July 2020</td>

<td>Chrome Interactions Highlights</td>

<td>Archives: <a href="http://go/animations-team-highlights">go/interactions-team-highlights</a></td>

</tr>
</table>

<table>
<tr>

<td><table></td>
<td><tr></td>

<td><td><img alt="image" src="https://lh4.googleusercontent.com/WRVW9VUhxNCsev-DZjUkmjGKVA5J4bJnm3JRhenzeZq7Pgfod74bwJpYLHF68ZuglJn38aoXqe2r-mkLdxk0y196l2NCsIfKIwRDhorwPGv5YbU5ZJgyQgGcup7X84_9vIx3yeQ8lA" height=432 width=561.3834586466165></td></td>

<td><td>Animations Documentation</td></td>

<td><td>kevers@ has made a lot of progress updating documentations in <a href="https://chromium.googlesource.com/chromium/src/+/HEAD/third_party/blink/renderer/core/animation/README.md">README.md</a>. Specifically,</td></td>

    <td><td>Landed 5 CLs with roughly 1300 lines of documentation
    added.</td></td>

<td><td><img alt="image" src="https://lh4.googleusercontent.com/c25Dc0nGruNseouHuux9h-1LZiVy0K2G8cGHUGscwbC_7pz3bbvFIqu6k03395tpNJ35MsSg9ppQyA6UNCrptuzTg7v6HcHMmMqAdx9hOZKu3fCyxukWibg19WENJUms0CwlxcKosQ" height=175 width=282></td></td>

<td><td>Throughput Metrics</td></td>

<td><td>xidachen@ started discussion on adjusting throughput reporting interval. They collected data with different reporting intervals and shown in the above graph.</td></td>

    <td><td>A lot of details are missed when the interval is large such as 5
    seconds</td></td>

    <td><td>The graph can be very noisy if the interval is too small such as 0.2
    second.</td></td>

    <td><td>A 1-second interval is likely a good choice.</td></td>

<td><td>Bug Triage</td></td>

<td><td>Our entire team has been working to formalize the bug triaging process. We now have a great <a href="https://docs.google.com/document/d/1II4W6ymxKNc8mxBAxwHjWIhzL5PPO4AtSrorzgiU3tM/edit#heading=h.3ma2fxg0g60f">doc</a> that describes the triage process and our un-triaged bug is coming to 0.</td></td>

<td></tr></td>
<td><tr></td>

<td><td>Capability Delegation</td></td>

<td><td>mustaq@ has completed the first draft of <a href="https://docs.google.com/document/d/1IYN0mVy7yi4Afnm2Y0uda0JH8L2KwLgaBqsMVLMYXtk/edit">Capability Delegation API</a>, and restarted the <a href="https://github.com/w3c/payment-request/issues/917">payment spec issue</a> discussion.</td></td>

<td><td>User Input Security</td></td>

<td><td>liviutinta@ started finch experiments for Browser Verified <a href="https://critique-ng.corp.google.com/cl/324039223">Keyboard</a>/<a href="https://critique-ng.corp.google.com/cl/324039272">Mouse</a> Activation Trigger.</td></td>

<td></tr></td>
<td><tr></td>

<td><td>Scroll Unification</td></td>

<td><td>lanwei@ improved many web tests by replacing eventsender with gpuBenchmarking.smoothScrollByXY. Currently there are 2 out of 42 remaining.</td></td>

<td><td>WebDriver Actions API Spec</td></td>

<td><td>lanwei@ added webdriver <a href="https://chromium-review.googlesource.com/c/chromium/src/+/2316405">wpt tests</a>, and <a href="https://chromium-review.googlesource.com/c/chromium/src/+/2324972">implemented</a> the wheel input source in Chromedriver.</td></td>

<td></tr></td>
<td><tr></td>

<td><td>Code Health</td></td>

<td><td>During the no-meetings week, our team made a lot of contributions to code health.</td></td>

    <td><td>The team landed 52 CLs and started a design doc (summary <a
    href="https://docs.google.com/document/d/1hk5N4NH-kYhEqyz1glnVGvWgT9NEMlMtaC4AJWE5CfA/edit#heading=h.lowhafniytq6">here</a>).</td></td>

    <td><td>lanwei@ removed experimental delegation code for user
    activation.</td></td>

    <td><td>kevers@ landed <a
    href="https://chromium-review.googlesource.com/c/chromium/src/+/2313157">patch</a>
    to fix flaky layout tests.</td></td>

    <td><td>kevers@ also <a
    href="https://chromium-review.googlesource.com/c/chromium/src/+/2310969">addressed</a>
    animation/style regressions due to recent refactoring.</td></td>

    <td><td>liviutinta@ fixed a <a
    href="https://bugs.chromium.org/p/chromium/issues/detail?id=1076078">bug</a>
    to ensure that when right clicking on a mis-spelled word and contextmenu
    event prevented, the mis-spelled word should not be highlighted.</td></td>

    <td><td>liviutinta@ also <a
    href="https://critique-ng.corp.google.com/cl/323644938">started</a>
    experimenta with Click as Pointer Event.</td></td>

    <td><td>Everybody joined in to create <a
    href="https://docs.google.com/document/d/1PCz35DkhYHT8zXJoKQl--WhQZpimJRyNlfEWSSjpUJ0/edit#">Interactions
    RTF</a> (Read this first)</td></td>

<td><td>Azimuth/Altitude for Pointer Events</td></td>

<td><td><a href="https://patrickhlauke.github.io/touch/pen-tracker/index.html?azimuth"><img alt="image" src="https://lh5.googleusercontent.com/4UlT7gRKaxiEjFSMUNpgY83bcFVmaS2YqhPN2mnU5RxMJ2hSrjwi8Ky1ZKJLa5iVm2h8cvVbaLT9vI8WgL9Mw3PWQ_6NqFVL9O_HkKQZlzlNDffgv8kGPUDzmcuxI3ehHhuIoyKguw" height=155 width=282></a></td></td>

<td><td>liviutinta@ has <a href="https://chromium-review.googlesource.com/c/chromium/src/+/2165457">implemented</a> this feature.</td></td>

    <td><td>Sent <a
    href="https://groups.google.com/a/chromium.org/forum/#!topic/blink-dev/ZRI-7X_4GwM">Intent
    to Ship</a>.</td></td>

    <td><td>Sent request for official position to <a
    href="https://github.com/mozilla/standards-positions/issues/411">Gecko</a>
    and <a
    href="https://lists.webkit.org/pipermail/webkit-dev/2020-July/031313.html">WebKit</a>.</td></td>

    <td><td>Sent <a
    href="https://github.com/w3ctag/design-reviews/issues/537">TAG Review</a>
    request.</td></td>

<td></tr></td>
<td></table></td>

</tr>
</table>

<table>
<tr>

<td>Chrome Interactions Highlights | July 2020</td>

<td><a href="http://go/interactions-team">go/interactions-team</a></td>

</tr>
</table>
