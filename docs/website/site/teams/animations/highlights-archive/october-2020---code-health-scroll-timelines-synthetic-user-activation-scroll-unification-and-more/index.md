---
breadcrumbs:
- - /teams
  - Teams
- - /teams/animations
  - Animations Team
- - /teams/animations/highlights-archive
  - Highlights Archive
page_name: october-2020---code-health-scroll-timelines-synthetic-user-activation-scroll-unification-and-more
title: October 2020 - Code Health, Scroll Timelines, Synthetic User Activation, Scroll
  Unification and more!
---

<table>
<tr>

<td>October 2020</td>

<td>Chrome Interactions Highlights</td>

<td>Archives: <a href="http://go/animations-team-highlights">go/interactions-team-highlights</a></td>

</tr>
</table>

<table>
<tr>

<td><table></td>
<td><tr></td>

<td><td>Code health</td></td>

<td><td><img alt="image" src="https://lh6.googleusercontent.com/xJwdlxgVeI_5sRA_9M84xtpBFGxhvbM3k5rh8WkY1amz0En2FzMyDTzkbees703Q1p3NI8CCXvPeZKa3d6sws_iBMp_ghQCabFfO8UNZhaAIbfy0RyaOyHAdm24N6UZInFfdN-d4Xw" height=156 width=280></td></td>

<td><td>The graph shows our latest bug fixing efforts in this sprint:</td></td>

    <td><td>We made steady progress on bug fixes.</td></td>

    <td><td>We held steady on P0s and P1s.</td></td>

    <td><td>Nearly held steady on P2s, which is awesome!</td></td>

<td><td>Scroll timeline</td></td>

<td><td><img alt="image" src="https://lh6.googleusercontent.com/ZbP3FFl7dRFLp2GGpe3p8KczY5F8HvAf8MZ1u3Qtg6WCfEUFazVxKtu1Gx3eD549C0XrFdj56u0_-JMEieRwgxJ03l7FgQc20XXdQ91LQYCJ6WI5ienDXocT5akvryNMG9r6ZKnnhA" height=173 width=280></td></td>

<td><td>flackr@ made tremendous progress on scroll timeline polyfill:</td></td>

    <td><td>Landed 13 patches.</td></td>

    <td><td>Discovered two bugs.</td></td>

        <td><td><a
        href="https://bugs.chromium.org/p/chromium/issues/detail?id=1136516">Wrong
        error thrown</a>.</td></td>

        <td><td><a
        href="https://github.com/w3c/csswg-drafts/issues/5599">rootMargin
        missing in spec</a>.</td></td>

    <td><td>215 new passing tests out of 354 total tests. The above graph shows
    the number of passing tests when each patch landed.</td></td>

<td></tr></td>
<td><tr></td>

<td><td>Synthetic user activation</td></td>

<td><td><img alt="image" src="https://lh3.googleusercontent.com/c6IkBfhps0LESw400Jqxdjjpj7XsHgR2b6t-XUN9lfxnFPr90s_6w52JVkEwfmDhXopgnIkZr_7e8oog5hKmSNCSNxxuZ2LOtQZjpfsQzCZZT5DbbIyDk47-tCqnTwjRy6nFdSG8fg" height=61 width=280></td></td>

<td><td>TriggerForConsuming</td></td>

<td><td><img alt="image" src="https://lh6.googleusercontent.com/JjVgSv_R4_qqTxquFfOongu6_TRaLH6OFw5UMoL7mORq0-dTwwQgdk9rct47sueeDvMVmO9xuXPCfRDc_td0pjyhGgEpYGScfCY6L12DwPKLnw-TPfNfzonohCE378LwS9AEWv0gzA" height=73 width=280></td></td>

<td><td>TriggerForTrasient</td></td>

<td><td><img alt="image" src="https://lh3.googleusercontent.com/sh5326ffsVwtpQokg4bUu_7WDewdbY7pGXuf_KmzMREI4LJSqSbCWzGy4Zh4mtiYzaQaYp18zmM_JNWYOjagj8uRfAu-Fxr3c_5ObfCVeMhjiYHT7W4z4KweHJxSevTeWuvGcI9YDg" height=73 width=280></td></td>

<td><td>TriggerForSticky</td></td>

<td><td>mustaq@ has done with UMA for synthetic triggers.</td></td>

    <td><td>Seeing expected results for non-extensions, but not expected for
    extensions!</td></td>

<td><td>Scroll unification</td></td>

<td><td>liviutinta@ organized the remaining work on scroll unification, and splits the work between Q4 2020 and Q1 2021. This <a href="https://docs.google.com/document/d/1O4vOub0CuXSbO-wAmDU1qe9D0wbdKT0y6sXIWTV67kY/edit">doc</a> captures the work nicely.</td></td>

<td><td>Click/Auxclick as pointer event</td></td>

<td><td>mustaq@ is working on a finch experiment on this feature..</td></td>

    <td><td>Ran finch experiment at 25% on Canary and Dev channels.</td></td>

    <td><td>Expanded finch experiment to 50% Canary and Dev
    (08-Oct-2020)</td></td>

    <td><td>Currently in progress of moving to 50% on Canary, Dev and Beta
    channels.</td></td>

<td></tr></td>
<td><tr></td>

<td><td>Composite transform animations containing percents</td></td>

<td><td><img alt="image" src="https://lh3.googleusercontent.com/nQK5QSx3LIfliPdWvIqzLwi6DnDQfUoeKfI1WhQVbbsrxkk1ia7ArlTZJv4gTLWRWOkmcJgfEHYBu_i3RTYsBxVEm7boLhw_uMefaacgKyx9Nbdpgv4_FRD3-xwrLYIywp6GxOWhUg" height=152 width=280></td></td>

<td><td>gtsteel@ has landed a complete implementation for animations targeting CSS boxes. The above <a href="https://codepen.io/george-steel/pen/PozWObd">demo</a> shows the difference of running the animation on the main vs the compositor thread (with artificial jank).</td></td>

    <td><td>This makes ~10% of transform animations potentially less
    janky.</td></td>

<td><td>Devtool input protocol</td></td>

<td><td>lanwei@ has merged the spec change of adding pointerevent’s additional properties to Webdriver Action API and added these properties to Devtool Input Protocol.</td></td>

<td><td><img alt="image" src="https://lh6.googleusercontent.com/RQny9ZhNN2UyqoAqjiwuXqdTyl37qRD1F-nz_QzO0Ek9C8SfAi0NDPVJ2u98y_4sOnKitQoTKZLgNhavYFMXEltcjdwWsZlFSoECxfDXjNvfdpYja9lEUuIuUuT7o8xRO5aKKU3D8w" height=138 width=131><img alt="image" src="https://lh4.googleusercontent.com/kHpOsjzL05GfwKCnwHLX2ADNBpz5Cd4KiARdKGms-JT1T6ZTfgMcjet25TebhGV1D4w6gSH7wClwo9ZhvVhkpr0MbgYhbSc1WXfu1ZBITGep1yYN1X3_lMNlMHUOgbpFFHWwLbAYmA" height=132 width=144></td></td>

<td></tr></td>
<td></table></td>

</tr>
</table>

<table>
<tr>

<td>Chrome Interactions Highlights | October 2020</td>

<td><a href="http://go/interactions-team">go/interactions-team</a></td>

</tr>
</table>
