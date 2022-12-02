---
breadcrumbs:
- - /teams
  - Teams
- - /teams/animations
  - Animations Team
- - /teams/animations/highlights-archive
  - Highlights Archive
page_name: september-2020-vol-2---code-health-logical-property-animation-off-thread-paintworklet-capability-delegation-and-more
title: September 2020 (Vol. 2) - Code Health, Logical Property Animation, Off-thread
  PaintWorklet, Capability Delegation and more!
---

<table>
<tr>

<td>September 2020 (Vol 2)</td>

<td>Chrome Interactions Highlights</td>

<td>Archives: <a href="http://go/animations-team-highlights">go/interactions-team-highlights</a></td>

</tr>
</table>

<table>
<tr>

<td><table></td>
<td><tr></td>

<td><td>Code health</td></td>

<td><td><img alt="image" src="https://lh5.googleusercontent.com/Ux83vqp9MnKpYUaJ908wGb3hTCOBEja8SrjjHF6iWiUkrPoB4VN24rnjkJH95Did60R5tZch82shQjpZOCiq2Q1yGjzK7f-7Ndes6IB0R_GdWeO3RJhoNg3r0iAAnARjJxiaidFWZQ" height=156 width=280></td></td>

<td><td>kevers@ presented our bug fixing effort for the sprint. We closed a fairly large number of bugs and kept the number of P1 bugs fairly steady. Overall the number of bugs has increased.</td></td>

<td><td rowspan=2>Logic clearly dictates…<img alt="image" src="https://lh6.googleusercontent.com/jJoZEGRrSbXcZ-3L4zcBHdorNiKa7ubKF14bTirLSh4vjhqZS16DOkKRue12sCHBVds5tpeJILUDWT_XkH703_TGCE0M_doid4HTeXbH6VVIinP8dj6ln4CmHWkSYRIbX0ZH-dBRbQ" height=228 width=217></td></td>

<td><td rowspan=2>kevers@ implemented support for animating logical properties in CSS and programmatic animations. Logical properties depend on the writing mode and text direction. As these animation types have different rules for keyframe construction and reporting, resolving property values when there are potential collisions in longhand property names presented some interesting challenges. </td></td>

<td><td rowspan=2>As a bonus, CSS animations now report computed values when fetching keyframes, fixing an issue with the resolution of variable references.</td></td>

<td><td rowspan=2>The above <a href="https://codepen.io/kevers-google/full/wvGNBoL">demo</a> programmatically creates an animated overlay on the insert-inline-start property, which in turn maps to the physical property left, top or right depending on the writing system. Upwards of 60 logical properties can now be animated. </td></td>

<td></tr></td>
<td><tr></td>

<td><td>Off-thread PaintWorklet</td></td>

<td><td><img alt="image" src="https://lh4.googleusercontent.com/Tz4cZdN5QEhQBj5vVeVoiHliFj9_8G2Sw7VqlOT6i_Nd-_Y_ZWRHcKZ-AvP38LygMWT6Iyl_vvndRt1To3_XJOpJ9Z3yg_w1HDqSAuZBPAPUmZ8NO0CRcAoCv28Tm4Odox7bwbOTIA" height=127 width=280></td></td>

<td><td>xidachen@ has been improving the off-thread paint worklet.</td></td>

    <td><td>Previously, it was required to have ‘will-change: transform’ to
    composite a paint worklet animation.</td></td>

    <td><td>With this improvement, that requirement is no longer needed and we
    save some memory because there is no longer any composited layer for running
    the paint worklet animation.</td></td>

<td></tr></td>
<td><tr></td>

<td><td><img alt="image" src="https://lh3.googleusercontent.com/ORNcx8zCjjHrDtuq4C2KwtJyV1AhaWjpkDAWUG54x7ubD1y8c2BWFQPqz2YgeQkehuonh5m3-fRePBQyU98lskx3TZ2uzKlN6ArNRRoPtAGE8_UhLVeqqZxlmof4giGq3BNcaZOYkw" height=156 width=280></td></td>

<td><td>Wheel WPT infrastructure tests</td></td>

<td><td>lanwei@ has been working on adding the Wheel input to the Webdriver Action API, writing a WPT infrastructure test and making some wheel WPT tests running automatically on WPT dashboard.</td></td>

<td><td>Capability delegation</td></td>

<td><td>mustaq@ now has an <a href="https://github.com/mustaqahmed/capability-delegation">explainer</a> and a WICG <a href="https://discourse.wicg.io/t/capability-delegation/4821">discussion</a> thread ready for wider review.</td></td>

<td><td>Scroll unification</td></td>

<td><td>liviutinta@ communicated with bokan@ on the scroll unification project, and</td></td>

    <td><td>Started this <a
    href="https://docs.google.com/document/d/1O4vOub0CuXSbO-wAmDU1qe9D0wbdKT0y6sXIWTV67kY/edit">doc</a>
    that captures the remaining work on scroll unification.</td></td>

    <td><td>Added a virtual test suite for scroll unification.</td></td>

<td></tr></td>
<td></table></td>

</tr>
</table>

<table>
<tr>

<td>Chrome Interactions Highlights | Sept. 2020 (Vol 2)</td>

<td><a href="http://go/interactions-team">go/interactions-team</a></td>

</tr>
</table>
