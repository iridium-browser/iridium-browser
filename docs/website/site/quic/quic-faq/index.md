---
breadcrumbs:
- - /quic
  - QUIC, a multiplexed transport over UDP
page_name: quic-faq
title: QUIC FAQ
---

[TOC]

#### What versions of QUIC does Chromium support?

In June of 2021, Chromium defaults to supporting IETF QUIC draft29 and gQUIC
Q050.

#### Are there any documents about how QUIC performs?

In 2017, we published a [SIGCOMM
paper](https://dl.acm.org/doi/10.1145/3098822.3098842) which detailed QUIC's
performance.

**Are there other implementations of QUIC?**

Yes, there are a number of other independent implementations. See the IETF Wiki
for a [full list](https://github.com/quicwg/base-drafts/wiki/Implementations).

#### Can I build the toy client and server without building all of chrome?

Yes, the quic_server target can be built without building the rest of Chrome.
You can follow these [instructions to build and run the stand-alone QUIC client
and server](/quic/playing-with-quic).

#### **How do I aim Chrome at the test server?**

If you have an HTTP server, you'll need it to emit a response header that looks
like:

> `Alternate-Protocol: quic:<QUIC server port>`

Then you can just run chrome as usual and it will automatically start using
QUIC.

If you're testing only with the toy quic server, you can do something like:

> `% chrome --disable-setuid-sandbox --enable-quic
> --origin-to-force-quic-on=localhost:6121
> `[`http://localhost:6121/`](http://localhost:6121/)

If you need help troubleshooting, try running the QUIC server with --v=1 or
check out [playing-with-quic](/quic/playing-with-quic)

#### **Why is the transport layer encrypted?**

The transport information for QUIC (congestion related information) is encrypted
mainly to guarantee the transport can always evolve. If the acks were in the
clear, or even checksummed, the concern is that eventually middle boxes would
start parsing the congestion information and would break with any forward
changes. This is currently a problem for TCP; the wire format allows for
negotiated options and flexible features which are practically unusable because
of the expectations of current hardware on the internet.

The down side, of course, is that hiding these details from middle boxes also
means QUIC flows are hard to analyze if you don't control an endpoint. The
tcpdump tool can let one visualize the rate of packets, and the gaps in packets,
but it's unclear which packets contain payload, which have congestion
information, which have retransmits etc. Both client and server side code are
architected to have hooks to easily dump this information during userspace
processing. Such logs can are more data rich than tcpdumps and can be tied
together with kernel level packet traces to get a better idea of latency across
the whole system.
