---
breadcrumbs:
- - /spdy
  - SPDY
page_name: spdy-best-practices
title: SPDY Best Practices
---

Application level

Use a single connection - it’s better for SPDY performance and for the internet
to use as few connections as possible. For SPDY, this will result in better
packing of data into packets, better header compression, less connection state,
fewer handshakes, etc. It improves TCP behavior across the internet and reduces
bufferbloat. Interacts better with NATs as well as it requires less state.

Don’t shard hostnames - This is a hack used by web apps to work around browser
parallelism limits. For all the reasons that we suggest using a single
connection, hostname sharding is suboptimal when you can use SPDY instead.
Furthermore, hostname sharding requires extra DNS queries and complicates web
apps due to using multiple origins.
Use server push instead of inlining - This is a hack used by web apps to reduce
RTTs. Inlining reduces the cacheability of web pages and may increase web page
sizes due to base64 encoding. Instead, the server can just push the content.
Use request prioritization - Clients can advise the server on relative
priorities for resources. Some basic heuristics which clients should all do
generally is to make sure that html &gt; js, css &gt; \*.
Use reasonable SPDY frame sizes - Although the spec allows for large frames, it
is often desirable to use smaller frames, because this allows for better
interleaving of frames from different streams.
*Don't use CSS sprites with external stylesheets* - Resources in external
stylesheets are obviously only discovered after the external stylesheet has been
downloaded, and only once the rule matches an element. The advantage they
provide of reducing HTTP requests is unnecessary with SPDY due to its
multiplexing. Therefore, CSS sprites just make it slower.
SSL
Use smaller, but complete, certificate chains - The size of certificate chains
can substantially affect connection startup performance due to increased
serialization latency and using up precious bytes in the initcwnd, costing extra
RTTs. Also, when the server does not present the full certificate chain, then
clients have to fetch intermediate certificates, adding yet more roundtrips
before the connection becomes usable by the application.
Use wildcard certs - SPDY allows for connection sharing, which helps reduce the
number of connections, which is beneficial for all the reasons already stated.
Don’t use overly large SSL write buffers - Large TLS application records will
span multiple packets. Since applications must process complete TLS application
records, this may add latency. Google servers use 2k buffers.
TCP
Servers should ensure initcwnd is at least 10 - initcwnd is a major bottleneck
that impacts initial page load times. HTTP works around this by opening multiple
connections simultaneously, essentially achieving n \* initcwnd initial
congestion window sizes.
Turn off tcp_slow_start_after_idle - Linux by default will have
tcp_slow_start_after_idle set to 1. This drops cwnd back down to initcwnd and
goes back to slow start after the connection has been “idle” (defined as the
RTO). This kills persistent single connection performance and should be turned
off.
Persist cwnd - SPDY provides a SETTINGS frame to allow for persisting settings
across connections. Clients can use this information to remember their previous
stable cwnd level and replay that to the server. Servers can use kernel
modifications to set the cwnd for the connection.
