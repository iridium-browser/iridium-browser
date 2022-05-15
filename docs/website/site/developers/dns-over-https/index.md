---
breadcrumbs:
- - /developers
  - For Developers
page_name: dns-over-https
title: DNS over HTTPS (aka DoH)
---

## Motivation

When you navigate to a website, your browser first needs to determine which
server is responsible for delivering said website, a step known as DNS
resolution. With DNS over HTTPS, all DNS resolutions occur over an encrypted
channel, helping to further safeguard user security and privacy.

## Launches

See these blog posts to learn more about DoH in Chrome:
[Desktop](https://blog.chromium.org/2020/05/a-safer-and-more-private-browsing-DoH.html)
(exception: Linux),
[Android](https://blog.chromium.org/2020/09/a-safer-and-more-private-browsing.html).

### DoH Providers

The latest version of DoH providers recognized by Chrome (canary) can be found
[here](https://source.chromium.org/chromium/chromium/src/+/HEAD:net/dns/public/doh_provider_entry.cc)
(the format should be self explanatory).

Note that users can configure any DoH providers of their choosing, the providers
included in Chrome are for the auto-upgrade mechanism and for a list of
popular/relevant options as a convenience (see the [requirements and
process](https://docs.google.com/document/d/128i2YTV2C7T6Gr3I-81zlQ-_Lprnsp24qzy_20Z1Psw/edit?usp=sharing)).

For technical questions, please send an email to
[net-dev@](https://groups.google.com/a/chromium.org/forum/#!forum/net-dev/) with
the \[DoH\] prefix in the subject line.

### **FAQ**

**Q:** Do you plan to support a canary domain similar to Mozilla's
[use-application-dns.net](http://use-application-dns.net/)?

**A:** We have no plans to support this approach. We believe that our deployment
model is significantly different from Mozilla's, and as a result canary domains
won't be needed. In particular, our deployment model is designed to preserve the
current user experience, i.e. auto-upgrading to the current DNS provider's DoH
server which offers the same features.

**Q:** How will Chrome's auto-upgrade approach work with Split Horizon?

**A:** Chrome's auto-upgrade approach does not change the DNS provider, and is
designed to preserve the same user experience. Split Horizon setups should
continue to work as is. Furthermore, managed deployments should be automatically
opted-out, and administrators can use policies to control the feature.