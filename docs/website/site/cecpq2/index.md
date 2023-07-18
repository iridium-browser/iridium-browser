---
breadcrumbs: []
page_name: cecpq2
title: CECPQ2
---

CECPQ2 is the name given to the combination of
[X25519](https://tools.ietf.org/html/rfc7748) and a post-quantum key-agreement
based on [NTRU-HRSS-KEM](https://ntru-hrss.org/). This combination provides at
least the security of X25519, combined with the likelihood of resistance to
future, large quantum computers that can otherwise decrypt all existing TLS
connections.

Chrome optionally supports CECPQ2 in TLS 1.3 connections. The results of
previous experiments with CECPQ2, and an isogeny-based key agreement, are
detailed [here](https://blog.cloudflare.com/the-tls-post-quantum-experiment/).

CECPQ2 makes some TLS messages larger. Some non-compliant network middleware
doesn't correctly handle these larger messages and can cause unexpected
connection failures or timeouts. While quantum computers are still in the
future, it's important to start the process of making the internet workable for
quantum-resistant algorithms today so that it'll be ready.

Starting with Chrome 91 we plan on restarting a previous rollout of CECPQ2
support that was paused due to the pandemic. Initially CECPQ2 will be enabled
for a deterministic subset of domains: those whose eTLD+1 begins with "aa".
Since the errors caused by bad network middleware are ambiguous and varied, we
are hoping that this distinctive pattern will help us categorize bug reports and
identify those that might be related.

(The eTLD+1 of a domain is the effective TLD plus one label. For example, the
eTLD of "www.example.co.uk" is "co.uk", so the eTLD+1 is "example.co.uk".)

Starting with Chrome 92, this rollout was expanded to all domains where the
eTLD+1 beings with just a single 'a'. This uncovered bugs in Fortigate and
(possibly) Palo Alto Networks devices which we are currently investigating.

With Chrome 93 and 94, this will be disabled by default.

CECPQ2 can be enabled for all domains by enabling
chrome://flags/#post-quantum-cecpq2.

There are two ways in which bad middleware can cause problems with CECPQ2:
Firstly, buggy middleware close to a specific site will cause that site to fail
to work globally when CECPQ2 is enabled for it. Secondly, buggy middleware in a
local network can cause all sites to fail when CECPQ2 is enabled for them, but
only when the client is on that network. It's important to categorize the type
of failure because it determines who can fix it: the first case is the sites'
failure, the second must be fixed by local network administrators.

If CECPQ2 is suspected of causing problems then administrators can disable it by
setting the CECPQ2Enabled policy to false. It is automatically disabled if
ChromeVariations has a non-default value. If the failure is of the second kind,
i.e. in the local network, then this should be considered a temporary workaround
as, eventually, everything will very likely deploy some form of post-quantum
security and thus use larger TLS messages. We are interested in collecting
information about known-bad middleware
