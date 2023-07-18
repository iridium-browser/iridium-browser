---
breadcrumbs:
- - /spdy
  - SPDY
page_name: spdy-data
title: SPDY data
---

Overall network efficiency of SPDY compared to HTTP. A lot of the speed benefits
of flip come from simply using the network more efficiently. Using fewer
connections means fewer packets. Using more compression, means fewer bytes. Due
to multiplexing, SPDY turns HTTPs many virtually half-duplex channels into a
single, full duplex channel.

When used loading our top-25 web pages, SPDY sent 40% fewer packets and 15%
fewer bytes than the same pages loaded over HTTP.

[<img alt="image"
src="/spdy/spdy-data/spdy-bytes.png">](/spdy/spdy-data/spdy-bytes.png)

Here is a graph of SPDY and HTTP, measuring the top-25 web pages as the packet
loss rate varies. Packet loss is an area ripe for more research; generally we
believe global worldwide packet loss is ~1 to 1.5%. Some areas are better, and
some areas are worse.

[<img alt="image"
src="/spdy/spdy-data/spdy-pktloss.png">](/spdy/spdy-data/spdy-pktloss.png)

Here is a graph of SPDY and HTTP average page load times as the RTT (round-trip
time) varies. RTT can be as low as 5-10ms for high speed networks, or 500ms for
some mobile or distant networks. Satellite links, known for their high
throughput, still have high latency, and can have RTTs of 150ms or more.
Generally, a 20ms RTT is great, 50ms RTT is not too shabby, and a 115ms RTT is
about average.

[<img alt="image"
src="/spdy/spdy-data/spdy-rtt2.png">](/spdy/spdy-data/spdy-rtt2.png)
