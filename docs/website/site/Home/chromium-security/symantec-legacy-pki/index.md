---
breadcrumbs:
- - /Home
  - Chromium
- - /Home/chromium-security
  - Chromium Security
page_name: symantec-legacy-pki
title: Symantec Legacy PKI
---

Following our
[announcement](https://security.googleblog.com/2017/09/chromes-plan-to-distrust-symantec.html)
in September 2017 to distrust the Legacy Symantec PKI, Chrome has executed on
this plan in incremental phases across several releases. As described in detail
in previous announcements, this distrust is the result of a consensus of
cross-browser efforts to maintain a trusted and secure web. The phased distrust
of this PKI has been implemented as follows:

<table>
<tr>

<td>Release</td>

<td>Description of Changes</td>

</tr>
<tr>

<td>Chrome 65</td>

<td>Remove trust in certificates issued after December 1, 2017, effectively stopping trust in new issuance from the Legacy Symantec PKI.</td>

</tr>
<tr>

<td>Chrome 66</td>

<td>Remove trust in certificates issued from the Legacy Symantec PKI before June 01, 2016, which were the most at-risk certificates based on the <a href="https://wiki.mozilla.org/CA:Symantec_Issues">numerous issues</a> identified by the Browser and Web PKI communities.</td>

</tr>
<tr>

<td>Chrome 70</td>

<td>Remove trust in all certificates issued from the Legacy Symantec PKI. Trust will be removed via <a href="https://textslashplain.com/2017/10/18/chrome-field-trials/">staged rollout</a>.</td>

</tr>
</table>

We are approaching the final phase of distrust now that M70 is about to reach
Stable. The following describes Chrome’s behavior with respect to Legacy
Symantec TLS certificates across our various release channels.

### Chrome 71 and beyond

### Following the ramp-up to 100% of users in Chrome 70 Stable, Chrome 71 and beyond will be enabling the Legacy Symantec PKI distrust by default. This change is present in all release channels: Canary, Dev, Beta, and Stable. Users observing the distrust in Chrome 70 should experience the exact same behavior in Chrome 71 and up.

### Chrome 70 Stable

The distrust of the Legacy Symantec PKI is rolling out to users of Chrome
throughout the release of Chrome 70. At the initial release of Chrome 70, this
change will reach a small percentage of Chrome 70 users, and then slowly scaling
up to 100% of Chrome 70 users over several weeks. This approach seeks to
minimize any remaining breakage associated with this change while still
providing affected sites the ability to diagnose and take corrective action.

Site Operators receiving problem reports from users are strongly encouraged to
take corrective action by replacing their website certificates as soon as
possible. Instructions on how to determine whether your site is affected as well
as what corrective action is needed can be found
[here](https://security.googleblog.com/2018/03/distrust-of-symantec-pki-immediate.html).

### Chrome 70 Early Release Channels

Chrome 70 Canary and Dev

In M70 Canary and Dev, the Symantec PKI has been distrusted by default for all
users. In these release channels, page loads over TLS connecting to sites using
Legacy Symantec TLS certificates display a full page interstitial with the error
code NET::ERR_CERT_SYMANTEC_LEGACY. Additionally, subresources served over such
connections will fail to load, possibly degrading site functionality.

Chrome 70 Beta

Distrust in the Legacy Symantec PKI was rolled out over time, starting with the
first Chrome 70 Beta. As a result of this Beta period, many sites still using
Legacy Symantec TLS certificates replaced their certificates.
