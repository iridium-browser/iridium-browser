---
breadcrumbs:
- - /spdy
  - SPDY
- - /spdy/spdy-protocol
  - SPDY Protocol
page_name: spdy-protocol-draft2
title: SPDY Protocol - Draft 2
---

<table>
<tr>

<td>*DRAFT*</td>

<td>[TOC]</td>

<td>## Overview</td>

<td>One of the bottlenecks of current HTTP implementations is that HTTP relies
solely on multiple connections for concurrency. This causes several problems,
including additional round trips for connection setup, slow-start delays, and a
constant rationing by the client where it tries to avoid opening too many
connections to a single server. HTTP "pipelining" doesn't help, as each
connection may be blocked on the request at the <a
href="http://en.wikipedia.org/wiki/Head-of-line_blocking">head of the line</a>;
in addition, many proxies have poor support for pipelining. Web applications, in
their desire to create many connections, create many sub-domains to work around
browser per-domain connection throttling.</td>

<td>SPDY aims to address these and other application-layer problems associated
with modern web applications, while requiring little or no change from the
perspective of web application writers.</td>

<td>In a nutshell, SPDY adds a framing layer for multiplexing multiple,
concurrent streams across a single TCP connection (or any reliable transport
stream). The framing layer is optimized for HTTP-like request-response
streams.</td>

<td>The SPDY session offers three basic improvements over HTTP:</td>

*   <td>Multiplexed requests. There is no limit to the number of
            requests that can be issued concurrently over a single SPDY
            connection. Because requests are interleaved on a single channel,
            the protocol is more efficient over TCP.</td>

*   <td>Prioritized requests. Clients can request certain resources to
            be delivered first. This avoids the problem of congesting the
            network channel with non-critical resources when a high-priority
            request is pending.</td>

*   <td>Compressed headers. Clients today send a significant amount of
            redundant data in the form of HTTP headers. Because a single web
            page may require 50 or 100 subrequests, this data is significant.
            Compressing the headers saves a significant amount of latency and
            bandwidth compared to HTTP.</td>
*   <td>Server pushed streams. This enables content to be pushed from
            servers to clients without a request.</td>

<td>Note that for the most part, SPDY attempts to preserve the existing
semantics of HTTP. All features such as cookies, etags, vary headers,
content-encoding negotiations, etc work as they do with HTTP; SPDY only replaces
the way the data is written to the network.</td>

<td>### Document Organization</td>

<td>The SPDY Specification is split into two parts: <a
href="/spdy/spdy-protocol/spdy-protocol-draft2#TOC-Framing-Layer">a framing
layer</a>, which multiplexes a TCP connection into independent, length-prefixed
frames, and <a
href="/spdy/spdy-protocol/spdy-protocol-draft2#TOC-HTTP-Layering-over-SPDY">an
HTTP layer</a>, which specifies the mechanism for overlaying HTTP
request/response pairs on top of the framing layer. While some of the framing
layer concepts are isolated from the HTTP layer, building a generic framing
layer has not been a goal. The framing layer is tailored to the needs of the
HTTP protocol and server push. </td>

<td>### Definitions</td>

*   <td>*client*: The endpoint initiating the SPDY session. </td>
*   <td>*connection*: A TCP-level connection between two endpoints.
            </td>
*   <td>*endpoint*: Either the client or server of a connection. </td>
*   <td>*frame*: A header-prefixed sequence of bytes sent over a SPDY
            session.</td>
*   <td>*server*: The endpoint not initiating the SPDY session.</td>
*   <td>*session*: A sequence of frames sent over a single connection.
            This is the same as the "framing layer". </td>
*   <td>*stream*: A bi-directional flow of bytes across a virtual
            channel within a SPDY session. </td>

<td>## Framing Layer</td>

<td>### Connections</td>

<td>The SPDY framing layer (or "session") runs atop TCP, similarly to how HTTP
works today. The client is expected to be the TCP connection initiator. Because
it runs on TCP, we have a reliable transport. Unlike HTTP, all connections with
SPDY are persistent connections. The HTTP connection header does not apply.</td>

<td>For best performance, it is expected that clients will not close open
connections until the user navigates away from all web pages referencing a
connection, or until the server closes the connection. Servers are encouraged to
leave connections open for as long as possible, but can terminate idle
connections after some amount of inactivity if necessary. When either side
closes the TCP connection, it should first send a <a
href="/spdy/spdy-protocol/spdy-protocol-draft2#TOC-GOAWAY">GOAWAY</a> frame so
that the endpoints can more reliably determine if requests finished before the
closure.</td>

<td>### Framing</td>

<td>Once the connection is established, clients and servers exchange framed messages. There are two types of frames: <a href="/spdy/spdy-protocol/spdy-protocol-draft2#TOC-Control-frames">control frames</a> and <a href="/spdy/spdy-protocol/spdy-protocol-draft2#TOC-Data-frames">data frames</a>. Frames always have a common header which is 8 bytes.</td>

<td>The first bit is a control bit indicating whether a frame is a control frame or data frame. Control frames carry a version number, a frame type, flags, and a length. Data frames contain the stream ID, flags, and the length for the payload carried after the common header. The simple header is designed to make reading and writing of frames easy.</td>

<td>All integer values, including length, version, and type, are in network byte order. SPDY does not enforce alignment of types in dynamically sized frames.</td>

<td>#### Protocol versioning</td>

<td>SPDY does lazy version checking on receipt of any control frame, and does version enforcement only on SYN_STREAM frames. If an endpoint receives a SYN_STREAM frame with an unsupported version, the endpoint must return a RST_STREAM frame with the status code UNSUPPORTED_VERSION. For any other type of control frame, the frame must be ignored.</td>

<td>### Control frames</td>

<td> +----------------------------------+</td>

<td> |C| Version(15bits) | Type(16bits) |</td>

<td> +----------------------------------+</td>

<td> | Flags (8) | Length (24 bits) |</td>

<td> +----------------------------------+</td>

<td> | Data |</td>

<td> +----------------------------------+</td>

<td> Control bit: The 'C' bit is a single bit indicating that this is a control message. For control frames this value is always 1. </td>
<td> Version: The version number of the session protocol (currently 2).</td>
<td> Type: The type of control frame. See <a href="/spdy/spdy-protocol/spdy-protocol-draft2#TOC-Control-frames">Control Frames</a> for the complete list of control frames. </td>

<td>Flags: Flags related to this frame. Flags for control frames and data frames
are different.</td>
<td> Length: An unsigned 24-bit value representing the number of bytes after the
length field. </td>

<td>Data: data associated with this control frame. The format and length of this
data is controlled by the control frame type.</td>

<td>Control frame processing requirements:</td>

<td>If an endpoint receives a control frame for a type it does not recognize, it
must ignore the frame.</td>

<td>### Data frames</td>

<td> +----------------------------------+</td>

<td> |C| Stream-ID (31bits) |</td>

<td> +----------------------------------+</td>

<td> | Flags (8) | Length (24 bits) | </td>

<td> +----------------------------------+</td>

<td> | Data |</td>

<td> +----------------------------------+</td>

<td>Control bit: For data frames this value is always 0.</td>
<td> Stream-ID: A 31-bit value identifying the stream.</td>
<td> Flags: Flags related to this frame. Valid flags are:</td>

*   <td>0x01 = FLAG_FIN - signifies that this frame represents the last
            frame to be transmitted on this stream. See <a
            href="/spdy/spdy-protocol/spdy-protocol-draft2#TOC-Stream-close">Stream
            Closure</a> below.</td>

<td> Length: An unsigned 24-bit value representing the number of bytes after the
length field. The total size of a data frame is 8 bytes + length. It is valid to
have a zero-length data frame.</td>
<td> Data: The variable-length data payload; the length was defined in the
length field.</td>

<td>Data frame processing requirements:</td>

<td>If an endpoint receives a data frame for a stream-id which does not exist,
it must return a <a
href="/spdy/spdy-protocol/spdy-protocol-draft2#TOC-RST_STREAM">RST_STREAM</a>
with error code INVALID_STREAM for the stream-id.</td>

<td>If the endpoint which created the stream receives a data frame before
receiving a SYN_REPLY on that stream, it is a protocol error, and the receiver
should close the connection immediately.</td>

<td>Implementors note: If an endpoint receives multiple data frames for invalid
stream-ids, it may terminate the session.</td>

<td>### Streams</td>

<td>Streams are independent sequences of bi-directional data cut into frames.
Streams can be created either by the client or the server, can concurrently send
data interleaved with other streams, and can be cancelled. The usage of streams
with HTTP is such that a single HTTP request/response occupies a single stream,
and the stream is not reused for a second request. This is because streams can
be independently created without incurring a round-trip. See the <a
href="/spdy/spdy-protocol/spdy-protocol-draft2#TOC-HTTP-Layering-over-SPDY">HTTP
layering</a> section for details.</td>

<td>Upon stream initiation, streams allow for each side to transmit a list of
name/value pairs (headers) to the other endpoint.</td>

<td>#### Stream creation</td>

<td>A stream is created by sending a control frame with the type set to <a
href="/spdy/spdy-protocol/spdy-protocol-draft2#TOC-SYN_STREAM">SYN_STREAM</a>.
If the server is initiating the stream, the Stream-ID must be even. If the
client is initiating the stream, the Stream-ID must be odd. 0 is not a valid
Stream-ID. Stream-IDs from each side of the connection must increase
monotonically as new streams are created. E.g. Stream 2 may be created after
stream 3, but stream 7 must not be created after stream 9.</td>

<td>Streams are bi-directional. However, the stream creator can optionally set
the UNIDIRECTIONAL flag as part of the SYN_STREAM to indicate to the receiver
that the receiver cannot reply on the stream.</td>

<td>Upon receipt of a SYN_STREAM frame, if the UNIDIRECTIONAL flag is not set,
the receiver replies with a <a
href="/spdy/spdy-protocol/spdy-protocol-draft2#TOC-SYN_REPLY">SYN_REPLY</a>
frame. Note that the stream initiator (sending the SYN_STREAM) does not need to
wait for a SYN_REPLY before sending any data frames.</td>

<td>If the endpoint receiving a SYN_STREAM does not want to accept the new
stream, it can immediately respond with a RST_STREAM control frame. Note,
however, that the initiating endpoint may have already sent data on the stream
as well; this data must be ignored.</td>

<td>#### Stream data exchange</td>

<td>Once a stream is created, it can be used to send arbitrary amounts of data.
Generally this means that a series of data frames will be sent on the stream
until a frame containing the FLAG_FIN flag is set. The FLAG_FIN can be set on a
SYN_STREAM, SYN_REPLY, or a DATA frame. Once the FLAG_FIN has been sent, the
stream is considered to be half-closed. (See <a
href="/spdy/spdy-protocol/spdy-protocol-draft2#TOC-Stream-half-close">Stream
half-close</a>)</td>

<td>#### Stream half-close</td>

<td>When one side of the stream sends a control or data frame with the FLAG_FIN
flag set, the stream is considered to be half-closed from that side. The sender
of the FLAG_FIN is indicating that no further data will be sent from the sender
on this stream. When both sides have half-closed, the stream is considered to be
closed. (See <a
href="/spdy/spdy-protocol/spdy-protocol-draft2#TOC-Stream-close">Stream
close</a>)</td>

<td>#### Stream close</td>

<td>There are 3 ways that streams can be terminated:</td>

1.  <td>Normal termination</td>
    <td> Normal stream termination occurs when both sender and receiver have
    half-closed the stream by sending a FLAG_FIN.</td>
2.  <td>Abrupt termination</td>
    <td> Either the client or server can send a RST_STREAM control frame at any
    time. A RST_STREAM contains an error code to indicate the reason for
    failure. When a RST_STREAM is sent from the stream originator, it indicates
    a failure to complete the stream and that no further data will be sent on
    the stream. When a RST_STREAM is sent from the stream receiver, the sender,
    upon receipt, should stop sending any data on the stream. The stream
    receiver should be aware that there is a race between data already in
    transit from the sender and the time the RST_STREAM is received.</td>
3.  <td><b>TCP connection teardown</b></td>
    <td><b> If the TCP connection is torn down while unterminated streams are
    active (no RST_STREAM frames have been sent or received for the stream),
    then the endpoint must assume that the stream was abnormally interrupted and
    may be incomplete.</b></td>

<td>If a client or server receives data on a stream which has already been torn
down, it must ignore the data received after the teardown.</td>

<td>TODO(mbelshe): Reference how UNIDIRECTIONAL works here.</td>

<td>### Data flow</td>

<td> Because TCP provides a single stream of data on which SPDY multiplexes
multiple logical streams, it is important for clients and servers to interleave
data messages for concurrent sessions.</td>

<td>Implementors should note that sending data in smallish chunks will result in
lower end-user latency for a page as this allows the browser to begin parsing
metadata early, and, in turn, to finalize the page layout. Sending large chunks
yields a small increase in bandwidth efficiency at the cost of slowing down the
user experience on pages with many resources.</td>

<td>### Control frames</td>

<td>#### SYN_STREAM</td>

<td>The SYN_STREAM control frame allows the sender to create a stream between
the sender and receiver with zero negotiation from the receiver. The stream can
be used to send metadata and data from the sender to the receiver.</td>

<td> +----------------------------------+</td>

<td> |1| 2 | 1 |</td>

<td> +----------------------------------+</td>

<td> | Flags (8) | Length (24 bits) |</td>

<td> +----------------------------------+</td>

<td> |X| Stream-ID (31bits) |</td>

<td> +----------------------------------+</td>

<td> |X|Associated-To-Stream-ID (31bits)|</td>

<td> +----------------------------------+</td>

<td> | Pri | Unused | |</td>

<td> +------------------ |</td>
<td> | Name/value header block |</td>
<td> | ... |</td>

<td>Flags: Flags related to this frame. Valid flags are:</td>

*   <td>0x01 = FLAG_FIN - signifies that this frame represents the last
            frame to be transmitted on this stream</td>
*   <td>0x02 = FLAG_UNIDIRECTIONAL - a stream created with this flag is
            already considered to be half closed to the receiver.</td>

<td>Length: An unsigned 24 bit value representing the number of bytes after the
length field. The total size of a SYN_STREAM frame is 8 bytes + length. The
length for this frame must be greater than or equal to 12.</td>

<td>Stream-ID: The 31-bit identifier for this stream. This stream-id will be
used in a frames which are part of this stream.</td>

<td>Associated-To-Stream-ID: The 31-bit identifier for a stream which this
stream is associated to. If this stream is independent of all other streams, it
should be 0.</td>

<td>Priority: A 2-bit priority field. If an endpoint has initiated multiple
streams, the priority field represents which streams should be given first
precidence. Servers are not required to strictly enforce the priority field,
although best-effort is assumed. 0 represents the lowest priority and 3
represents the highest priority. The highest-priority data is that which is most
desired by the client.</td>

<td> Unused: 14 bits of unused space, reserved for future use.</td>
<td> NV entries: (16 bits) The number of name/value pairs that follow. Note:
</td>

<td>The <a
href="/spdy/spdy-protocol/spdy-protocol-draft2#TOC-Name-Value-header-block-format">Name/Value
block</a> is described below.</td>

<td>If an endpoint receives multiple SYN_STREAM frames for the same active
stream ID, it must drop the stream, and send a RST_STREAM for the stream with
the error PROTOCOL_ERROR.</td>

<td>#### SYN_REPLY</td>

<td>SYN_REPLY provides the acceptance of a stream creation by the receiver of a
SYN_STREAM frame.</td>

<td> +----------------------------------+</td>

<td> |1| 2 | 2 |</td>

<td> +----------------------------------+</td>

<td> | Flags (8) | Length (24 bits) |</td>

<td> +----------------------------------+</td>

<td> |X| Stream-ID (31bits) |</td>

<td> +----------------------------------+</td>

<td> | Unused | |</td>

<td> +---------------- |</td>
<td> | Name/value header block |</td>
<td> | ... |</td>

<td> Flags: Flags related to this frame. Valid flags are:</td>

*   <td>0x01 = FLAG_FIN - signifies that this frame represents the
            half-close of this stream. When set, it indicates that the sender
            will not produce any more data frames in this stream..</td>

<td> Length: An unsigned 24-bit value representing the number of bytes after the
length field. The total size of a SYN_STREAM frame is 8 bytes + length. The
length for this frame must be greater than or equal to 8.</td>

<td>Stream-ID: The 31-bit identifier for this stream. This stream-id will be
used in a frames which are part of this stream.</td>

<td>Unused: 16 bits of unused space, reserved for future use.</td>
<td> NV entries: (16 bits) The number of name/value pairs that follow. </td>

<td>The <a
href="http://www.chromium.org/spdy-protocol/spdy-protocol-draft2#TOC-Name-Value-header-block-format">Name/Value
block</a> is described below.</td>

<td>If an endpoint receives multiple SYN_REPLY frames for the same active stream
ID, it must drop the stream, and send a RST_STREAM for the stream with the error
PROTOCOL_ERROR.</td>

<td>#### RST_STREAM</td>

<td>The RST_STREAM frame allows for abnormal termination of a stream. When sent
by the creator of a stream, it indicates the creator wishes to cancel the
stream. When sent by the receiver of a stream, it indicates an error or that the
receiver did not want to accept the stream, so the stream should be closed.</td>

<td> +-------------------------------+</td>

<td> |1| 2 | 3 |</td>

<td> +-------------------------------+</td>

<td> | Flags (8) | 8 |</td>

<td> +-------------------------------+</td>

<td> |X| Stream-ID (31bits) |</td>

<td> +-------------------------------+</td>

<td> | Status code | </td>

<td> +-------------------------------+</td>

<td> Flags: Flags related to this frame. RST_STREAM does not define any flags.
This value must be 0.</td>

<td> Length: An unsigned 24-bit value representing the number of bytes after the
length field. For RST_STREAM control frames, this value is always 8.</td>
<td> Status code: (32 bits) An indicator for why the stream is being
terminated.The following status codes are defined:</td>

*   <td>1 - PROTOCOL_ERROR. This is a generic error, and should only be
            used if a more specific error is not available. The receiver of this
            error will likely abort the entire session and possibly return an
            error to the user.</td>
*   <td>2 - INVALID_STREAM should be returned when a frame is received
            for a stream which is not active. The receiver of this error will
            likely log a communications error.</td>
*   <td>3 - REFUSED_STREAM. This is error indicates that the stream was
            refused before any processing has been done on the stream. This
            means that request can be safely retried.</td>
*   <td>4 - UNSUPPORTED_VERSION. Indicates that the receiver of a stream
            does not support the SPDY version requested.</td>
*   <td>5 - CANCEL. Used by the creator of a stream to indicate that the
            stream is no longer needed.</td>
*   <td>6 - INTERNAL_ERROR. The endpoint processing the stream has
            encountered an error.</td>
*   <td>7 - FLOW_CONTROL_ERROR. The endpoint detected that its peer
            violated the flow control protocol.</td>

<td>Note: 0 is not a valid status code for a RST_STREAM.</td>

<td>After receiving a RST_STREAM on a stream, the receiver must not send
additional frames for that stream.</td>

<td>#### SETTINGS</td>

<td>*Note: In earlier drafts, this frame was called a "HELLO" frame.*</td>

<td>A SETTINGS frame contains a set of id/value pairs for communicating
configuration data about how the two endpoints may communicate. SETTINGS frames
can be sent at any time by either endpoint, are optionally sent, and fully
asynchronous. Further, when the server is the sender, the sender can request
that configuration data be persisted by the client across SPDY sessions and
returned to the server in future communications.</td>

<td>Persistence of SETTINGS ID/Value pairs is done on a per domain/IP pair. That
is, when a client connects to a server, and the server persists settings within
the client, the client MUST only return the persisted settings on future
connections to the same domain AND IP address and TCP port. Clients MUST NOT
request servers to use the persistence features of the SETTINGS frames, and
servers MUST ignore persistence related flags sent by a client.</td>

<td> +----------------------------------+</td>

<td> |1| 2 | 4 |</td>

<td> +----------------------------------+</td>

<td> | Flags (8) | Length (24 bits) | </td>

<td> +----------------------------------+</td>

<td> | Number of entries |</td>

<td> +----------------------------------+</td>
<td> | ID/Value Pairs |</td>
<td> | ... |</td>

<td> Control bit: The control bit is always 1 for this message.</td>
<td> Version: The SPDY version number.</td>
<td> Type: The message type for a SETTINGS message is 4.</td>

<td>Flags: FLAG_SETTINGS_CLEAR_PREVIOUSLY_PERSISTED_SETTINGS (0x1): When set,
the client should clear any previously persisted SETTINGS ID/Value pairs. If
this frame contains ID/Value pairs with the FLAG_SETTINGS_PERSIST_VALUE set,
then the client will first clear its existing, persisted settings, and then
persist the values with the flag set which are contained within this frame.
Because persistence is only implemented on the client, this flag can only be
used when the sender is the server.</td>

<td>Length: An unsigned 24-bit value representing the number of bytes after the
length field. The total size of a SETTINGS frame is 8 bytes + length.</td>

<td>Number of entries: A 32-bit value representing the number of ID/value pairs
in this message.</td>

<td>Each ID/value pair is as follows:</td>

<td> +----------------------------------+</td>

<td> | ID (24 bits) | ID_Flags (8) |</td>

<td> +----------------------------------+</td>

<td> | Value (32 bits) |</td>

<td> +----------------------------------+</td>

<td>ID: 24-bits in <b>little-endian</b> byte order. This is inconsistent with
other values in SPDY and is the result of a bug in the initial v2
implementation. Defined IDs:</td>

*   <td>1 - SETTINGS_UPLOAD_BANDWIDTH - allows the sender to send its
            expected upload bandwidth on this channel. This number is an
            estimate. The value should be the integral number of kilobytes per
            second that the sender predicts as an expected maximum upload
            channel capacity.</td>
*   <td>2 - SETTINGS_DOWNLOAD_BANDWIDTH - allows the sender to send its
            expected download bandwidth on this channel. This number is an
            estimate. The value should be the integral number of kilobytes per
            second that the sender predicts as an expected maximum download
            channel capacity.</td>
*   <td>3 - SETTINGS_ROUND_TRIP_TIME - allows the sender to send its
            expected round-trip-time on this channel. The round trip time is
            defined as the minimum amount of time to send a control frame from
            this client to the remote and receive a response. The value is
            represented in milliseconds.</td>
*   <td>4 - SETTINGS_MAX_CONCURRENT_STREAMS allows the sender to inform
            the remote endpoint the maximum number of concurrent streams which
            it will allow. By default there is no limit. For implementors it is
            recommended that this value be no smaller than 100.</td>
*   <td>5 - SETTINGS_CURRENT_CWND allows the sender to inform the remote
            endpoint of the current CWND value. This value is currently
            interpreted as packets.</td>
*   <td>6 - SETTINGS_DOWNLOAD_RETRANS_RATE - downstream byte
            retransmission rate in percentage</td>
*   <td>7 - SETTINGS_INITIAL_WINDOW_SIZE - initial window size in
            bytes</td>

<td>Flags: 8 bits. Defined Flags:</td>

<td> FLAG_SETTINGS_PERSIST_VALUE (0x1): When set, the sender of this SETTINGS
frame is requesting that the receiver persist the ID/Value and return it in
future SETTINGS frames sent from the sender to this receiver. Because
persistence is only implemented on the client, this flag is only sent by the
server.</td>

<td> FLAG_SETTINGS_PERSISTED (0x2): When set, the sender is notifying the
receiver that this ID/Value pair was previously sent to the sender by the
receiver with the FLAG_SETTINGS_PERSIST_VALUE, and the sender is returning it.
Because persistence is only implemented on the client, this flag is only sent by
the client.</td>

<td>Value: A 32-bit value in network byte order</td>

<td> The message is intentionally extensible for future information which may
improve client-server communications. The sender does not need to send every
type of ID/value. It must only send those for which it has accurate values to
convey. When multiple ID/value pairs are sent, they should be sent in order of
lowest id to highest id. A single SETTINGS frame MUST not contain multiple
values for the same ID. If the receiver of a SETTINGS frame discovers multiple
values for the same ID, it MUST ignore all values except the first one.</td>

<td>Implementation Notes: Persisted storage from the SETTINGS is similar to a
cookie, in that it is persisted state. It differs from HTTP cookies in that it
applies at the session layer rather than at the HTTP layer. When SPDY is used in
conjunction with HTTP, browser implementors should be careful that any persisted
SETTINGS information follows similar handling to HTTP cookies, and that a
mechanism for clearing data is provided to the user. Servers MUST NOT attempt to
use SETTINGS as a mechanism for storing arbitrary data on the client. </td>

<td>Note also that a server may send multiple SETTINGS frames containing
different ID/Value pairs. When the same ID/Value is sent twice, the most recent
value overrides any previously sent values. If the server sends IDs 1, 2, and 3
with the FLAG_SETTINGS_PERSIST_VALUE in a first SETTINGS frame, and then sends
IDs 4 and 5 with the FLAG_SETTINGS_PERSIST_VALUE, when the client returns the
persisted state on its next SETTINGS frame, it SHOULD send all 5 settings (1, 2,
3, 4, and 5 in this example) to the server.</td>

<td>#### NOOP</td>

<td>The NOOP control frame is a no-operation frame. It can be sent from the
client or the server. Receivers of a NO-OP frame should simply ignore it.</td>

<td> +----------------------------------+</td>

<td> |1| 2 | 5 |</td>
<td> +----------------------------------+</td>
<td> | 0 (Flags) | 0 (Length) |</td>

<td> +----------------------------------+</td>

<td><b> Control-bit: The control bit is always 1 for this message.</b></td>
<td><b> Version: The SPDY version number.</b></td>
<td><b> Type: The message type for a NOOP message is 5.</b></td>
<td><b> Length: This frame carries no data, so the length is always 0.</b></td>

<td><b>#### PING</b></td>

<td><b>The PING control frame is a mechanism for measuring a minimal round-trip
time from the sender. It can be sent from the client or the server. Receivers of
a PING frame should send an identical frame to the sender as soon as possible
(if there is other pending data waiting to be sent, PING should take highest
priority). Each ping sent by a sender should use a unique ID.</b></td>

<td><b><b> +----------------------------------+</b></b></td>

<td><b> |1| 2 | 6 |</b></td>

<td><b> +----------------------------------+</b></td>

<td><b> | 0 (flags) | 4 (length) |</b></td>

<td><b> +----------------------------------|</b></td>

<td><b> | 32-bit ID |</b></td>

<td><b> +----------------------------------+</b></td>

<td><b> Control bit: The control bit is always 1 for this message.</b></td>
<td><b> Version: The SPDY version number.</b></td>
<td><b> Type: The message type for a PING message is 6.</b></td>
<td><b> Length: This frame is always 4 bytes long.</b></td>
<td><b> ID: A unique ID for this ping. When the client initiates a ping, it must
use an odd numbered ID. When the server initiates a ping, it must use an even
numbered ping. Use of odd/even IDs is required in order to avoid accidental
looping on PINGs (where each side initiates an identical PING at the same
time).</b></td>
<td><b> Note: If a sender uses all possible PING ids (e.g. has sent all 2^31
possible IDs), it can wrap and start re-using IDs.</b></td>

<td><b>If a server receives an even numbered PING which it did not initiate, it
must ignore the PING.</b></td>

<td><b>If a client receives an odd numbered PING which it did not initiate, it
must ignore the PING.</b></td>

<td>#### GOAWAY</td>

<td>The GOAWAY control frame is a mechanism to tell the remote side of the
connection that it should no longer use this session for further requests. It
can be sent from the client or the server. Once sent, the sender agrees not to
initiate any new streams on this session. Receivers of a GOAWAY frame must not
send additional requests on this session, although a new session (connection)
can be established for new requests. The purpose of this message is to allow an
endpoint to gracefully stop accepting new requests (perhaps for a reboot or
maintenance), while still finishing processing of previously established
streams.</td>

<td>There is an inherent race condition between a client sending requests and
the server sending a GOAWAY message. To deal with this case, the GOAWAY message
contains a last-stream identifier indicating the last stream which was accepted
in this session. If the client issued requests for sessions after this
stream-id, they were not accepted by the server and should be re-issued later at
the client's discretion.</td>

<td>Endpoints should always send a GOAWAY message before closing a connection so
that the remote can know whether a request has been partially processed or not.
(For example, if the client sends a POST at the same time that a server closes a
connection, the client cannot know if the server started to process that POST
request if the server does not send a GOAWAY frame to indicate where it stopped
working)</td>

<td>After sending a GOAWAY message, the sender must ignore all SYN_STREAM frames
for new streams.</td>

<td> +----------------------------------+</td>

<td> |1| 2 | 7 |</td>

<td> +----------------------------------+</td>

<td> | 0 (flags) | 4 (length) |</td>

<td> +----------------------------------|</td>

<td> |X| Last-good-stream-ID (31 bits) |</td>

<td> +----------------------------------+</td>

<td> Control bit: The control bit is always 1 for this message.</td>
<td> Version: The SPDY version number.</td>
<td> Type: The message type for a GOAWAY message is 7.</td>
<td> Length: This frame is always 4 bytes long.</td>
<td> Last-good-stream-Id: The last stream id which was accepted by the sender of
the GOAWAY message. If no streams were accepted, this value must be 0.</td>

<td>#### HEADERS</td>

<td> This frame augments a stream with additional headers. It may be optionally
sent on an existing stream at any time. Specific application of the headers (and
dealing with duplicates) in this frame is application-dependent.</td>

<td> +----------------------------------+</td>

<td> |C| 2 | 8 |</td>

<td> +----------------------------------+</td>

<td> | Flags (8) | Length (24 bits) |</td>

<td> +----------------------------------+</td>

<td> |X| Stream-ID (31bits) |</td>

<td> +----------------------------------+</td>

<td> | Unused (16 bits) | |</td>
<td> |-------------------- |</td>

<td> | Name/value header block |</td>

<td> +----------------------------------+</td>

<td> Length: An unsigned 24 bit value representing the number of bytes after the
length field. The total size of a HEADERS frame is 8 bytes + length. The length
for this frame must be greater than or equal to 8. The minimum length of the
length field is 4 (when the number of name value pairs is 0).</td>

<td>NV entries: The number of name/value pairs that follow.</td>

<td>The <a
href="/spdy/spdy-protocol/spdy-protocol-draft2#TOC-Name-Value-header-block-format">Name/Value
block</a> is the same as a SYN_REPLY.</td>

<td>Name/Value header block format</td>

<td>The SYN_STREAM, SYN_REPLY, and HEADERS frames contain a Name/value header
block. The header block used by both the request and the response is the same.
It is designed so that headers can be easily appended to the end of a list and
also so that it can be easily parsed by receivers. Each numeric value is 2
bytes.</td>

<td> +------------------------------------+</td>

<td> | Number of Name/Value pairs (int16) |</td>

<td> +------------------------------------+</td>

<td> | Length of name (int16) |</td>

<td> +------------------------------------+</td>

<td> | Name (string) |</td>

<td> +------------------------------------+</td>

<td> | Length of value (int16) |</td>

<td> +------------------------------------+</td>

<td> | Value (string) |</td>

<td> +------------------------------------+</td>

<td> | (repeats) |</td>

<td>Each header name must have at least one value. Header names must be all
lower case. The length of each name and value must be greater than zero. A
receiver of a zero-length name or value must send a RST_STREAM with code
PROTOCOL error. </td>

<td>TODO(mbelshe): Verify that the 64KB limits are sufficient. JoeyH notes that
it is possible to send 50 4KB cookies over HTTP today, and that would not work
in this model.</td>

<td>Duplicate header names are not allowed. To send two identically named
headers, send a header with two values, where the values are separated by a
single NUL (0) byte. Senders must not send multiple, in-sequence NUL characters.
Receivers of multiple, in-sequence NUL characters must send a RST_STREAM with
code PROTOCOL_ERROR on the stream.</td>

<td>Strings are utf8 encoded and are not NUL terminated.</td>

<td>The entire contents of the name/value header block is compressed using zlib
deflate. There is a single zlib stream (context) for all name value pairs in one
direction on a connection. SPDY uses a SYNC_FLUSH between frame which uses
compression. The stream is initialized with the following dictionary (without
line breaks and IS null-terminated):</td>

<td>optionsgetheadpostputdeletetraceacceptaccept-charsetaccept-encodingaccept-</td>
<td>
languageauthorizationexpectfromhostif-modified-sinceif-matchif-none-matchi</td>
<td>
f-rangeif-unmodifiedsincemax-forwardsproxy-authorizationrangerefererteuser</td>
<td>
-agent10010120020120220320420520630030130230330430530630740040140240340440</td>
<td>
5406407408409410411412413414415416417500501502503504505accept-rangesageeta</td>
<td>
glocationproxy-authenticatepublicretry-afterservervarywarningwww-authentic</td>
<td>
ateallowcontent-basecontent-encodingcache-controlconnectiondatetrailertran</td>
<td>
sfer-encodingupgradeviawarningcontent-languagecontent-lengthcontent-locati</td>
<td>
oncontent-md5content-rangecontent-typeetagexpireslast-modifiedset-cookieMo</td>
<td>
ndayTuesdayWednesdayThursdayFridaySaturdaySundayJanFebMarAprMayJunJulAugSe</td>
<td>
pOctNovDecchunkedtext/htmlimage/pngimage/jpgimage/gifapplication/xmlapplic</td>
<td>
ation/xhtmltext/plainpublicmax-agecharset=iso-8859-1utf-8gzipdeflateHTTP/1</td>
<td> .1statusversionurl</td>

<td>TODO(mbelshe): Add Brian O's compression data and notes.</td>

<td>#### WINDOW_UPDATE</td>

<td>Although WINDOW_UPDATE was originally defined in DRAFT 2, it was never
implemented. The specification for this frame is thus removed, and this note
exists for those who might remember its existence.</td>

<td>Please look at DRAFT 3 for flow-control related definitions.</td>

<td>---</td>

<td>## HTTP Layering over SPDY</td>

<td>SPDY is intended to be as compatible as possible with current web-based
applications. This means that, from the perspective of the server business logic
or application API, the features of HTTP must not change. To achieve this, all
of the application request and response header semantics are preserved, although
the syntax of conveying those semantics has changed. Thus, the rules from the <a
href="http://www.w3.org/Protocols/rfc2616/rfc2616.html">HTTP/1.1 specification
in RFC 2616</a> apply with the changes in the sections below.</td>

<td>Standard Transactions</td>

<td>HTTP request/responses will generally be mapped as one request/response to
one stream. Multiple requests can be issued in parallel, with each request
issued on an independent stream.</td>

<td>Request</td>

<td>The client initiates a request by sending a SYN_STREAM frame. For requests
which do not contain a body, the SYN_STREAM must set the FIN_FLAG, indicating
that the client intends to send no further data on this stream. For requests
which do contain a body, the SYN_STREAM will not contain the FIN_FLAG, and the
body will follow the SYN_STREAM in a series of DATA frames. The last DATA frame
will set the FIN_FLAG to indicate the end of the body.</td>

<td>The SYN_STREAM Name/Value section will contain all of the HTTP headers which
are associated with an HTTP request. The HTTP header block in SPDY is mostly
unchanged from today's HTTP header block, with the following differences:</td>

*   <td>The first line of the request is unfolded into name/value pairs
            like other HTTP headers and must be present:</td>
    *   <td>"method" - the HTTP method for this request (e.g. "GET",
                "POST", "HEAD", etc)</td>
    *   <td>"scheme" - the scheme portion of the URL for this request
                (e.g. "https")</td>
    *   <td>"url" - the absolute path for this request (e.g.
                "/foo/bar.html")</td>
    *   <td>"version" - the HTTP version of this request (e.g.
                "HTTP/1.1")</td>
*   <td>Duplicate header names are not allowed.</td>
*   <td>Header names are all lowercase.</td>
*   <td>The Connection and Keep-Alive headers are no longer valid and
            are ignored if present.</td>
*   <td>HTTP request headers are compressed. This is accomplished by
            compressing all data sent by the client with gzip encoding.</td>
*   <td>The "host" header is ignored. The host:port portion of the HTTP
            URL is the definitive host.</td>
*   <td>User-agents are expected to support gzip and deflate
            compression. Regardless of the Accept-Encoding sent by the
            user-agent, the server may select gzip or deflate encoding at any
            time.</td>
*   <td>POST-specific changes:</td>
    *   <td>POST requests are expected to contain a data stream as part
                of the post; see Data flow below.</td>
    *   <td>Content-length</td>
    *   <td>Chunked encoding is no longer valid.</td>

<td> The browser is free to prioritize requests as it sees fit. If the browser
cannot make progress without receiving a resource, it should attempt to raise
the priority of that resource. Resources such as images, should use the lowest
priority whenever possible.</td>

<td>If a client sends a SYN_STREAM without all of the method, url, and version
headers, the server must reply with a HTTP 400 BAD REQUEST reply.</td>

<td>Implementors Note: Section 5.1 of the HTTP/1.1 specification specifies that
HTTP/1.1 compliant servers MUST support absolute URIs in the request line. SPDY
is using absolute URIs only, instead of relative URIs + Host headers. From
practical experience, we have noticed that many HTTP/1.1 servers do not support
absolute URIs and are in violation of the HTTP/1.1 standard. As a result, any
implementors of SPDY -&gt; HTTP proxies may wish to translate from SPDY's
absolute URIs into relative URI + Host headers to maximize compatibility with
existing HTTP/1.1 servers.</td>

<td>#### Response</td>

<td> The server responds to a client request with a SYN_REPLY frame. The
Name/Value section of the frame contains the HTTP response headers. Symmetric to
the client's upload stream, the server will follow the SYN_REPLY with a series
of DATA frames, and the last data frame will contain the FLAG_FIN to indicate
successful end-of-stream.</td>

*   <td>The response status line is unfolded into name/value pairs like
            other HTTP headers and must be present:</td>
    *   <td>"status" - The HTTP response status code (e.g. "200" or "200
                OK")</td>
    *   <td>"version" - The HTTP response version (e.g. "HTTP/1.1")</td>
*   <td>If the SPDY reply happens before a SYN_STREAM, then it includes
            parameters that inform the client regarding the request that would
            have been made to receive this response, by including url and method
            keys. </td>
*   <td>All header names must be lowercase.</td>
*   <td>The Connection and Keep-alive response headers are no longer
            valid.</td>
*   <td>Content-length is only advisory for length.</td>
*   <td>Chunked encoding is no longer valid.</td>
*   <td>Duplicate header names are not allowed.</td>

<td>If a client receives a SYN_REPLY without a status or without a version
header, the client must reply with a RST_STREAM frame indicating a PROTOCOL
ERROR.</td>

<td>TODO(mbelshe): Describe use of GOAWAY.</td>

<td>### Server Push Transactions</td>

<td> SPDY enables a server to send multiple replies to a client for a single
request. The rationale for this feature is that sometimes a server knows that it
will need to send multiple resources in response to a single request. Without
server push features, the client must first download the primary resource, then
discover the secondary resource(s), and request them. Pushing of resources
avoids this delay, but also creates a potential race where a server can be
pushing content which a browser is in the process of requesting. The following
mechanics attempt to prevent the race condition while enabling the performance
benefit. </td>

<td>#### Server Implementation</td>

<td>When the server intends to push a resource to the client, it opens a new
stream by sending a unidirectional SYN_STREAM. The SYN_STREAM must include an
Associated-To-Stream-ID. The SYN_STREAM must also include a header for "url"
which contains the full URL for the resource being pushed. The purpose of the
association is to differentiate which request induced the pushed stream; without
it, if the browser had two tabs open to the same page, each pushing unique
content under a fixed URL, the browser would not be able to differentiate the
requests. Server pushed streams are unidirectional, and must set the
FLAG_UNIDIRECTIONAL flag in the SYN_STREAM.</td>

<td>The Associated-To-Stream-ID must be the ID of an existing, open stream with
the client. The reason for this restriction is to have a clear endpoint for
pushed content. If the client requested a resource on stream 11, the server
replies on stream 11. It can push any number of additional streams to the client
before it issues a FLAG_FIN on stream 11. However, once the originating stream
is closed no further push streams may be created. The pushed streams do not need
to be closed (FIN set) before the originating stream is closed, they only need
to be created before the originating stream closes.</td>

<td>It is illegal for a server to push a resource with the
Associated-To-Stream-ID of 0.</td>

<td>To avoid race conditions with the client, the SYN_STREAM for the pushed
resources must be sent prior to sending any content which could allow the client
to discover the pushed resource and request it.</td>

<td>The server must only push resources which would have been returned from a
GET request.</td>

<td>Note: If the server does not have all of the Name/Value Response headers
available at the time it issues the SYN_STREAM for the pushed resource, it may
later use a HEADER frame to augment the name/value pairs to be associated with
the pushed stream. The HEADER frame must not contain a header for 'url' (e.g.
the server can't change the identity of the resource to be pushed). If the
HEADERS frame must not contain duplicate headers with a previously sent
SYN_STREAM or HEADERS frame. The server must send a HEADERS before sending any
data frames on the stream.</td>

<td>TODO(mbelshe): Consider Set-Cookie effects on push streams.</td>

<td>#### Client Implementation</td>

<td>When fetching a resource the client has 3 possibilities:</td>

1.  <td>the resource is not being pushed</td>
2.  <td>the resource is being pushed, but the data has not yet
            arrived</td>
3.  <td>the resource is being pushed, and the data has started to
            arrive</td>

<td>When a SYN_STREAM frame which contains an Associated-To-Stream-ID is
received, the client must not issue GET requests for the URL in the pushed
stream, and instead wait for the pushed stream to arrive. </td>

<td>When a client receives a SYN_STREAM from the server with an
Associated-To-Stream-ID of 0, it must reply with a RST_STREAM with error code
INVALID_STREAM.</td>

<td>When a client receives a SYN_STREAM from the server without an 'url' in the
Name/Value section, it must reply with a RST_STREAM with error code
PROTOCOL_ERROR.</td>

<td>To cancel server push streams, the client can use a RST_STREAM on an
individual pushed stream with error code CANCEL. Upon receipt, the server will
stop sending on this stream immediately (this is an Abrupt termination). Note
that there may be data already in transit for this stream.</td>

<td>To cancel all server push streams related to a request, the client can use a
RST_STREAM on the original stream with error code CANCEL. By closing that
stream, the server will no longer be able to push any streams with
in-association-to for the original stream.</td>

<td>If the server sends a HEADER frame containing duplicate headers with a
previous SYN_STREAM or HEADERS frame for the same stream, the client must reply
with a RST_STREAM with error code PROTOCOL ERROR.</td>

<td>If the server sends a HEADER frame after sending a data frame for the same
stream, the client must ignore the HEADERS frame. TODO(mbelshe): This is really
undesirable for server-push. We want the push notifications to be able to flow
asynchronously in the stream.</td>

<td>TODO(mbelshe): Define how HTTP Trailers should work: "For HTTP, the rules
for http trailers apply, as specified in: <a
href="http://www.w3.org/Protocols/rfc2616/rfc2616-sec14.html#sec14.40">http://www.w3.org/Protocols/rfc2616/rfc2616-sec14.html#sec14.40</a>"</td>

<td>### Deployment</td>

<td>Since SPDY does provide faster access to resources, it should be used in
preference to HTTP/HTTPS. To facilitate this, clients and servers are encouraged
to employ these features.</td>

<td>#### Server Advertisement of SPDY through the HTTP Alternate-Protocol header</td>

<td>When a server receives a non-SPDY request which could have been served via
SPDY, it should append a Alternate-Protocol header into the response stream.
Note that it is valid to have multiple Alternate-Protocols headers. The
field-value can also be specified as a comma-separated list, as per RFC2616
section 4.2.</td>

<td>Syntax:</td>

> <td>Alternate-Protocol: &lt;port&gt;:&lt;protocol&gt;</td>

<td> To specify SPDY as an alternate protocol available on port 123, use:</td>
> <td>Alternate-Protocol: 123:spdy/2</td>

<td> To specify SPDY via TLS/NPN as an alternate protocol available on port 443, use:</td>
> <td>Alternate-Protocol: 443:npn-spdy/2</td>

<td>To specify support for SPDY versions 1 and 2 (currently non-existent) via
TLS/NPN as alternate protocols available on port 443, use:</td>

> <td>Alternate-Protocol: 443:npn-spdy/2,443:npn-spdy/2</td>

<td>When a client receives a Alternate-Protocol header, it should attempt to
piggyback future http requests over SPDY by using the specified port. Note that
the server may reply with multiple field values or a comma-separated field value
for Alternate-Protocol (and in this manner indicate the various SPDY versions it
supports). The client may select any protocol it supports and attempt to use
that. If it cannot establish a SPDY connection on that port, it should fallback
to standard HTTP and persist the failure for the browser session so that it
doesn't re-attempt in the near future.</td>

<td>#### Server Advertisement of SPDY through TLS NPN extension</td>

<td>When a server receives a connection to its HTTPS port that includes the TLS
NPN extension (<a
href="http://www.ietf.org/id/draft-agl-tls-nextprotoneg-00.txt">http://www.ietf.org/id/draft-agl-tls-nextprotoneg-00.txt</a>),
it should respond with all the SPDY versions that it supports. Currently, the
only valid string is "spdy/2" (spdy/1 isn't implemented anywhere anymore). The
client can thereby choose which SPDY version it supports and proceed from
there.</td>

<td>#### Piggybacking HTTP requests over SPDY sessions</td>

<td>SPDY does not introduce a new protocol scheme specific to SPDY. However, all
HTTP requests can "piggyback" on an existing SPDY session. So, if a client has
successfully negotiated any SPDY session to a port that the server has indicated
(via Alternate-Protocol) is an acceptable alternate protocol for the HTTP host
port pair, all future http requests to that host port pair should use the SPDY
session rather than opening a new HTTP connection.</td>

<td>Note: If your site does load balancing in such a way that http content would
not be available over the SPDY session, your site may not be able to use
SPDY.</td>

<td>## Incompatibilities with SPDY Draft #1</td>

*   <td>Renamed the FIN_STREAM to RST_STREAM</td>
*   <td>Added the FLAG_UNIDIRECTIONAL to the SYN_STREAM</td>
*   <td>Defined behavior of SPDY-protocol version checking</td>
*   <td>Made GOAWAY message stronger - it should always be sent before
            closing a connection.</td>
*   <td>Added Associated-To-Stream-ID into the SYN_STREAM, allowing for
            a stream to declare its relationship to an existing stream.</td>
*   <td>Reworked the SUBRESOURCE frame. Instead of first declaring a
            SUBRESOURCE, with an associated stream, the SYN_REPLY can be used.
            Added a HEADERS frame for late-bound headers to be added to a
            stream.</td>
*   <td>Reworked the Server Push mechanism.</td>
*   <td>Added flow control: new WINDOW_UPDATE frame, new status code in
            RST_STREAM, added new field to SETTINGS for initial window size, and
            made SETTINGS always the first frame of the connection (still
            optional and asynchronous).</td>
*   <td>Updated HELLO frame format and renamed to SETTINGS.</td>

<td>## Future work/experiments</td>

<td> The following are thoughts/brainstorms for future work and
experiments.</td>

*   <td>Caching</td>
    <td> Since we're proposing to do almost everything over an encrypted
    channel, we're making caching either difficult or impossible.</td>
    <td> We've had some discussions about having a non-secure, static-only
    content channel (where the resources are signed, or cryptographic hashes of
    the insecure content are sent over a secure link), but have made no headway
    yet...</td>
*   <td>DoS potential</td>

<td>The potential for DoS of the server (by increasing the amount of state) is
no worse than it would be for TCP connections. In fact, since more of the state
is managed in userspace, it is easier to detect and react to increases in
resource utilization. The client side, however, does have a potential for DoS
since the server may now attempt to use client resources which would not
normally be considered to be large per stream state.</td>

<td>More investigation needs to be done about attack modes and remediation
responses.</td>

*   <td>Stream capacity.</td>

<td>Today, HTTP has no knowledge of approximate client bandwidth, and TCP only
slowly and indirectly discovers it as a connection warms up. However, by the
time TCP has warmed up, the page is often already completely loaded. To ensure
that bandwidth is utilized efficiently, SPDY allows the client to tell the
server its approximate bandwidth. The SETTINGS message is part of this, but
measurement, reporting and all of the other infrastructure and behavioral
modifications are lacking.</td>

*   <td>Server-controlled connection parameters.</td>

<td>The "server" -- since either side may be considered a "server" in SPDY,
"server" refers here to the side which receives requests for new sessions -- can
inform the other side of reasonable connection or stream limits which should be
respected. As with stream capacity, the SETTINGS message provides a container
for signaling this, but no work has yet been done to figure out what values
should be set, when, and what behavioral changes should be expected when the
value does change.</td>

    <td>Prefetching/server push</td>

> <td>If the client can inform the server (or vice versa) how much bandwidth it
> is willing to allocate to various activities, then the server can push or
> prefetch resources without a) impacting the activities that the user wants to
> perform; or b) being too inefficient. While this approach still has
> inefficiencies (the server still send sparts of resources that the client
> already has before the client gets around to sending a FIN to the server), it
> doesn't seem to be grossly inefficient when coupled with the client telling
> the server how much slack (bandwidth) it is is willing to give to the server.
> However, this means that the client must be able to dynamically adjust the
> slack that it provides for server push or prefetching, for example, if it sees
> an increase in PING time.</td>

    <td>Re-prioritization of streams</td>

> <td>Users often switch between multiple tabs in the browser. When the user
> switches tasks, the protocol should allow for a change in priority as the user
> now wants different data at a higher priority. This would likely involve
> creating priority groups so that the relative priority of resources for a tab
> can be managed all at once instead of having to make a number of changes
> proportional to the number of requests (which is 100% likely to be the same or
> larger!)</td>

*   <td>Flow control</td>

<td>Each side can announce how much data or bandwidth it will accept for each
class of streams. If this is done, then speculative operations such as server
push can soak up a limited amount of the pipe (especially important if the pipe
is long and thin). This may allow for the elimination of more complex "is this
already in the cache" or "announce what I have in my cache" schemes which are
likely costly and complex.</td>

*   <td>DNS data</td>

<td>It is suboptimal that the browser must discover new hostnames and then look
them up in cases where it is fetching new resources controlled by the same
entity. For example, there shouldn't be a need to do another DNS lookup for a
resource from static.foo.com. when the browser has already resolved xxx.foo.com.
In these cases, it would seemingly make sense to send (over the SPDY channel)
the DNS data (signed in such a way that the user-agent can verify that it is
authoritative).</td>

*   <td>Redirect to a different IP while retaining the HOST header</td>

<td>For large sites or caches, it may be advantageous to supplement DNS frontend
load balancing with the ability to send the user to a sibling that is less
loaded, but also able to serve the request. This is not possible today with
HTTP, as redirects must point to a different name (or use an IP, which amounts
to the same thing), so that cookies, etc. are treated differently. This feature
would likely be tied to the DNS data feature, or a more interesting verification
mechanism would need to exist.</td>

*   <td>New headers</td>
    *   <td>Request headers:</td>
        *   <td>Never been to this site before header. When the server
                    receives this header, it is used as permission to open
                    multiple, server-initiated streams carrying subresource
                    content. If sent, then the server can freely push all the
                    resources necessary for the page.</td>
    *   <td>Response</td>
        *   <td>Subresource headers</td>
*   <td>Mark Nottingham notes that having the method/uri/version in
            separate headers, rather than at the front of the block, may make it
            harder for servers to quickly access this information. Being
            compressed hurts too.</td>

<td>## Contributors</td>

<td>Thank you to all those who read, commented, and contributed to this
work:</td>
<td> Mike Belshe, Roberto Peon, Adam Langley, Jim Morrison, Mark Nottingham,
Alyssa Wilk, Costin Manolache, William Chan, Vitaliy Lvin, Joe Chan</td>

</tr>
</table>
