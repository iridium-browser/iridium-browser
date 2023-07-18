---
breadcrumbs:
- - /teams
  - Teams
- - /teams/animations
  - Animations Team
- - /teams/animations/highlights-archive
  - Highlights Archive
page_name: september-2020---code-heath-animation-timelines-smoothness-metrics-animation-event-handlers-and-more
title: September 2020 - Code Heath, Animation Timelines, Smoothness Metrics, Animation
  event handlers and more!
---

<table>
<tr>

<td>September 2020</td>

<td>Chrome Interactions Highlights</td>

<td>Archives: <a href="http://go/animations-team-highlights">go/interactions-team-highlights</a></td>

</tr>
</table>

<table>
<tr>

<td><table></td>
<td><tr></td>

<td><td>Code health</td></td>

<td><td><img alt="image" src="https://lh6.googleusercontent.com/0HrjwnQsCSygr54-qmbiNzfVBw1RqBieD343JVyXPbrhYBqGlnv50ir3q4le1GU1buG2e-8iKD_UaD3I3Te7_4eIjCvj-1I9mtl7KRt453N_CEkU9jP7E6yT9v-DTuebsgckqmSJHQ" height=153 width=280></td></td>

<td><td>kevers@ presented a graph to show the progress of bug fixing.</td></td>

    <td><td>In this sprint we closed a lot of bugs, but more were opened than
    closed.</td></td>

    <td><td>We now have breakdowns by priority. Nice to see that we did not lose
    ground on P0s or P1s.</td></td>

<td><td>xidachen@ removed the usage of setTimeout in animations layout test, and replaced them with rAF.</td></td>

    <td><td>setTimeout can easily cause flakiness, especially on debug
    bots.</td></td>

    <td><td>Animations are driven by animation frames, using rAF leads to more
    robust tests.</td></td>

<td><td><img alt="image" src="https://lh4.googleusercontent.com/02ubEg_i85VG8_GcBTpvZdboNqtTJc85Q0f-ZaSo5QFTjU7_PEGRpiYSKHQ7XnZ1cr3ioGu3F87MfJWBh3NYtfF9zonWHQfhQ0OMxTyuyo4JK6byuDzTPP3B2wUuFTgh4zNKtnmeug" height=224 width=280></td></td>

<td><td>Mutable timelines</td></td>

<td><td>kevers@ has been working on scroll timelines and the above <a href="https://codepen.io/kevers-google/pen/VweomWY">demo</a> showcases mutable timelines, which are required for supporting css animation-timeline.</td></td>

    <td><td>The demo illustrates updating the animation timeline via the
    web-animation API.</td></td>

    <td><td>Presently it is behind the scroll-timeline feature flag.</td></td>

<td><td>kevers@ showed another fun <a href="https://codepen.io/kevers-google/full/YzqaLKQ">demo</a> for scroll timelines, which uses scroll position to drive a paint worklet animation..</td></td>

<td><td><img alt="image" src="https://lh4.googleusercontent.com/XdD8nBBUsBXIe8J93a7ntjcLK2u30rEa-HahPlPUqLJoa30tSCebdA5oisoQCfrF4E7xkkqn7epvzJfX66TSyC9jPmq9AexhaIjBt7FWhKLEAo3HzfCbTHKA8DN9wedehK1sq_Vogg" height=133 width=280></td></td>

<td></tr></td>
<td><tr></td>

<td><td>Smoothness metrics</td></td>

<td><td><img alt="image" src="https://lh3.googleusercontent.com/UOyYLsJ4A5m-FFWw5T2q6owRPLZloZXc-WO9zGJvAZ-RUXhQp5zj8rvZ5O7caDnbScfIIgHkEoHfj9n_7Ct9jVzjlvqk5apzJzV6_KhLmu5yBndzQR2VdzINsEgJjXT32c9VpvPfGg" height=217 width=280></td></td>

<td><td>xidachen@ improved fps meter by making it account for page loading.</td></td>

    <td><td>The fps meter will reset itself at first contentful paint (which is
    regarded as loading completed).</td></td>

    <td><td>The stats before first contentful paint will be discarded when we
    report to UMA.</td></td>

<td><td>Animation event handlers</td></td>

<td><td>gtsteel@ landed the implementation for ontrasition\* event handler properties. Along with that, gtsteel@ also fixed animationcancel event so that it doesn’t fire after animationend.</td></td>

<td><td>Zooming no longer breaks transitions</td></td>

<td><td>gtsteel@ fixed a bug where zooming would cause transition event listeners to fire.</td></td>

    <td><td>Previously, we compared zoomed values when starting transitions,
    then transitionend based on computed value.</td></td>

    <td><td>Now we calculate and compare computed values (as per spec) if zoom
    changed.</td></td>

<td></tr></td>
<td><tr></td>

<td><td>Web tests ⇒ WPT</td></td>

<td><td>liviutinta@ shared a <a href="https://docs.google.com/spreadsheets/d/1WCg_odYm124kf4Se5SLYq0nCk4yRQU3O6aFV7cWi2DI/edit#gid=0">sheet</a> that lists input related layout tests that we would like to move to wpt/. The spreadsheet also tracks the progress. </td></td>

<td><td>WebDriver Actions API Spec</td></td>

<td><td>lanwei@ has landed <a href="https://github.com/w3c/webdriver/pull/1522">spec change</a> that adds wheel input to the webdriver action API.</td></td>

<td></tr></td>
<td></table></td>

</tr>
</table>

<table>
<tr>

<td>Chrome Interactions Highlights | September 2020</td>

<td><a href="http://go/interactions-team">go/interactions-team</a></td>

</tr>
</table>
