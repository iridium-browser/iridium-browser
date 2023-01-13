---
breadcrumbs:
- - /teams
  - Teams
- - /teams/animations
  - Animations Team
- - /teams/animations/highlights-archive
  - Highlights Archive
page_name: august-2020---code-health-animations-context-menu-and-more
title: August 2020 - Code Health, Animations, Context Menu and more!
---

<table>
<tr>

<td>August 2020</td>

<td>Chrome Interactions Highlights</td>

<td>Archives: <a href="http://go/animations-team-highlights">go/interactions-team-highlights</a></td>

</tr>
</table>

<table>
<tr>

<td><table></td>
<td><tr></td>

<td><td>Code Health</td></td>

<td><td>Our team had an awesome sprint on bug fixing.</td></td>

<td><td><img alt="image" src="https://lh4.googleusercontent.com/owiAyDpZzfTYW5rAik3nJan9W6E4DsI0s127oetssl1Zpt1CrnI3BFqNW_HoOR3IRQFga05EQ1eiobzNGqISyuo4jxVC6H8eGZyxNXd_uGdBAVUImHk-koHYjRv4E26J-BPZ-7h9QA" height=168 width=307></td></td>

<td><td>Thanks to kevers@, we now have a bug dashboard (<a href="http://goto.google.com/interactions-dashboard">go/interactions-dashboard</a>). The above graph shows opened vs closed bugs during this sprint. The dashboard also includes:</td></td>

    <td><td>Breakdown of cumulative issues opened and closed by
    component</td></td>

    <td><td>Total number of open issues by component</td></td>

    <td><td>Breakdown of closed issues by reason</td></td>

<td><td><img alt="image" src="https://lh5.googleusercontent.com/v5UeZaDBwG0E8xifSNGs1zWxbO1IrJjsnUf1CXk-Urydx7EyS6ooei0deoEbs5Zxn2p9FvWQq4XeLlnaRMnY8lgGND36PfW6qnDmRWKVcqzaDuUg3lpezDfWp5g0iKll5StZi2xG7Q" height=158 width=492></td></td>

<td><td><img alt="image" src="https://lh3.googleusercontent.com/tyCAkYty-BGkwHg3GgW3MJwsn17hn1f6OmLDsZ03KTF6EdRRbYxvo2NkYmnipCd8-FmCIlNjy8P6wogQz9n0y6ArNgEXT6PQqFAGjutSPARPCT_rry5ERxuWaYkgjxCem5iPYZHgow" height=164 width=500></td></td>

<td><td>flackr@ fixed <a href="https://crbug.com/1116020">flicker when setting style in finish callback</a> (<a href="https://output.jsbin.com/darepen">demo</a>)</td></td>

    <td><td>The top demo is broken, and the bottom one is fixed.</td></td>

<td><td>liviutinta@ fixed several bugs:</td></td>

    <td><td>A Pointer Events tiltY reversed on Mac bug (<a
    href="http://crbug.com/1111347">1111347</a>).</td></td>

    <td><td>Fixed flaky test where pointermove on chorded mouse buttons when
    pointer is locked (<a href="http://crbug.com/1025944">1025944</a> <a
    href="https://chromium-review.googlesource.com/c/chromium/src/+/2360494">CL</a>)</td></td>

    <td><td>Fixed 3 failing browser tests for Unified Scrolling: <a
    href="https://chromium-review.googlesource.com/c/chromium/src/+/2364055">CL</a></td></td>

<td><td>gtsteel@ made great contributions to bug fixing.</td></td>

    <td><td>Fixed crash in run_web_tests caused by increased number of X11
    connections used: <a
    href="https://chromium-review.googlesource.com/c/chromium/src/+/2368420">CL</a>.</td></td>

    <td><td>Fixed AnimatedCSSProperties metric to count properties when animated
    (by anything which uses a KeyframeEffect) (<a
    href="https://crbug.com/992430">Bug</a>)</td></td>

    <td><td>Started UMA study towards fixing transition cancelling when
    resetting style (<a href="https://crbug.com/934700">Bug</a>)</td></td>

    <td><td>Relanded patch adding ontransition\* event handlers: <a
    href="https://chromium-review.googlesource.com/c/chromium/src/+/2258467">CL</a>.</td></td>

<td></tr></td>
<td></table></td>

<td><table></td>
<td><tr></td>

<td><td><img alt="image" src="https://lh3.googleusercontent.com/hvsHNNrrxW1mAKU5UZUYURv7Man5793AmBYMJKxYNpazzfTZNao081OYbnQXawm_DaQC8mU9TcmEqfv1OoJBozBgQ5mFfUWptIfor2IkQq3hC9L5XKdms_CGuLy1YoNIjPOG21J6Ng" height=146 width=273></td></td>

    <td><td>Top animation is composited</td></td>
    <td><td>Bottom animation is main thread</td></td>

    <td><td>After the fix, the composited animation aligns with the main thread
    animation and there is no jump on reversal.</td></td>

<td><td>Animations</td></td>

<td><td>kevers@ fixed a weird reversal of composited animations with start-delay.</td></td>

    <td><td>The problem is that the process of converting timing properties to
    time offset for the compositor assumes the animation is running in the
    forward direction. The time offset is incorrect if playing in the reverse
    direction and there is a start delay (<a
    href="https://bugs.chromium.org/p/chromium/issues/detail?id=1095813">1095813</a>).</td></td>

    <td><td>The solution is to Include the tweak for start delay only if the
    playback rate is positive. (<a
    href="https://chromium-review.googlesource.com/c/chromium/src/+/2339712">CL</a>).</td></td>

<td><td>kevers@ Fixed 5 WPT test flakes and one non-WPT test flake for animations. (<a href="https://bugs.chromium.org/p/chromium/issues/detail?id=1093451">1093451</a>,<a href="https://bugs.chromium.org/p/chromium/issues/detail?id=1092177"> 1092177</a>,<a href="https://bugs.chromium.org/p/chromium/issues/detail?id=1092141"> 1092141</a>, <a href="https://bugs.chromium.org/p/chromium/issues/detail?id=1064065">1064065</a>, <a href="https://bugs.chromium.org/p/chromium/issues/detail?id=1085564">1085564</a>, <a href="https://bugs.chromium.org/p/chromium/issues/detail?id=623434">623434</a>)</td></td>

<td><td>WebDriver Actions API Spec</td></td>

<td><td>lanwei@ finished the implementation in Chromedriver and Webdriver (<a href="https://chromium-review.googlesource.com/c/chromium/src/+/2324972">CL</a>, <a href="https://chromium-review.googlesource.com/c/chromium/src/+/2316405">CL</a>)</td></td>

<td><td>Azimuth/Altitude for Pointer Events</td></td>

<td><td>liviutinta@ shipped the Azimuth and Altitude.</td></td>

    <td><td>Received 3 LGTMs on <a
    href="https://groups.google.com/a/chromium.org/forum/?utm_medium=email&utm_source=footer#!msg/blink-dev/ZRI-7X_4GwM/Sp1ZMIw5AgAJ">Intent
    to Ship</a></td></td>

    <td><td>Landed <a href="http://crrev.com/c/2343385">CL</a> to enable the
    feature flag by default</td></td>

    <td><td>No feedback yet from <a
    href="https://github.com/w3ctag/design-reviews/issues/537">TAG
    Review</a></td></td>

    <td><td>Positive signal from Webkit, no signal from Gecko.</td></td>

<td><td>Context Menu with Touch-Drag</td></td>

<td><td>mustaq@ Added context menu support to draggable elements: behind a flag, show Window native draggable behavior of showing context menu on drag end (<a href="https://chromium-review.googlesource.com/c/chromium/src/+/2340287">CL</a>).</td></td>

    <td><td>Before: draggable divs can’t show context menu because a touch
    interaction has to choose between dragging vs context-menu. We have links
    and images always non-draggable for this reason. (Hi-res video <a
    href="https://drive.google.com/file/d/1W2zX7_SdCVVoV0yh9nwK6SY7X29ozOII/view?usp=sharing">here</a>).</td></td>

<td><td><img alt="image" src="https://lh4.googleusercontent.com/mI0QBIs9oloJh6Foi_XwnQJzfH4VwmPZejBZkcPwmMOEjj2k4R9l-ledgLWkVXpVONHg6sHtsZU1JhlPqWkoz5RZqbv8MoMBmkjcL70QaqXT9n9fvov5ltkyj_5xqNOF_dDNh1jbZQ" height=200 width=280></td></td>

    <td><td>After: draggable elements shows context menu on drag-end, like
    Windows desktop icons. Context menu depends on where the element is dropped.
    Works on divs, links and images. (Hi-res video <a
    href="https://drive.google.com/file/d/1MkUOjoi6qJnUl_XFphFzqonVQjg9XyO5/view?usp=sharing">here</a>.)</td></td>

<td><td><img alt="image" src="https://lh4.googleusercontent.com/Y4BB2dlHUTM3gmFARKxmlPRJ0WWZhSLkaCwNr_r7LrbsRPBBqaoY6sSvKYgEe13wMVEmZFWsUI4bTnTSHnZnewJiJWDqjhChVNLLbkOZqw_klQOQ1ZusQw8cvyAApXSkCLFRDuM3Rg" height=200 width=280></td></td>

<td></tr></td>
<td></table></td>

<td><table></td>
<td><tr></td>

<td><td>Throughput Metrics</td></td>

<td><td>xidachen@ fixed wrong reporting in video frame sequence length (<a href="https://chromium-review.googlesource.com/c/chromium/src/+/2342247">CL</a>), where 50% had length of 0 frames.</td></td>

<td></tr></td>
<td></table></td>

</tr>
</table>

<table>
<tr>

<td>Chrome Interactions Highlights | August 2020</td>

<td><a href="http://go/interactions-team">go/interactions-team</a></td>

</tr>
</table>
