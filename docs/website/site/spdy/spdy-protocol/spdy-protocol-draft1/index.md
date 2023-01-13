---
breadcrumbs:
- - /spdy
  - SPDY
- - /spdy/spdy-protocol
  - SPDY Protocol
page_name: spdy-protocol-draft1
title: SPDY Protocol - Draft 1
---

<table>
<tr>

<td>Mike Belshe (mbelshe at google.com) & Roberto Peon (fenix at
google.com)</td>

<td>*DRAFT*</td>

<td>## Overview</td>

<td>One of the bottlenecks of current HTTP is that it relies solely on multiple
connections for concurrency. This causes several problems, including additional
round trips for connection setup, slow-start delays, and a constant rationing by
the client where it tries to avoid opening too many connections to a single
server. HTTP "pipelining" doesn't help, as each connection may be blocked on the
request at the head of the line; in addition, many proxies apparently have poor
support for pipelining. Applications, in their desire to create many
connections, create many sub-domains to work around browser per-domain
connection throttling.</td>

<td>SPDY aims to address this and other application-layer problems associated
with modern web applications, while requiring little or no change from the
perspective of web application writers.</td>

<td>In a nutshell, SPDY adds a framing layer for multiplexing multiple,
concurrent streams across a single TCP connection. The framing layer is
optimized for HTTP-like request-response streams.</td>

<td>The SPDY session offers three basic improvements over HTTP:</td>

*   <td>Multiplexed requests. There is no limit to the number of
            requests that can be issued concurrently over a single SPDY
            connection. Because requests are interleaved on a single channel,
            the efficiency of TCP is much higher.</td>

*   <td>Prioritized requests. Clients can request certain resources to
            be delivered first. This avoids the problem of congesting the
            network channel with non-critical resources when a high-priority
            request is pending.</td>

*   <td>Compressed headers. Clients today send a significant amount of
            redundant data in the form of HTTP headers. Because a single web
            page may require 50 or 100 subrequests, this data is significant.
            Compressing the headers saves a significant amount of latency and
            bandwidth compared to HTTP.</td>

<td>Note that for the most part, SPDY attempts to preserve the existing
semantics of HTTP features. All features such as cookies, etags, vary headers,
content-encoding negotiations, etc work exactly as they do with HTTP; SPDY only
replaces the way the data is written to the network.</td>

<td>## Definitions</td>

*   <td>*connection*: A TCP-level connection between two endpoints.</td>
*   <td>*endpoint*: Either the client or server of a connection.</td>
*   <td>*session*: A framed sequence of data chunks. Frames are defined
            as SPDY frames; see <a
            href="/spdy/spdy-protocol/spdy-protocol-draft1#framing">Framing</a>
            below.</td>
*   <td>*stream*: A bi-directional flow of bytes across a virtual
            channel within a SPDY session.</td>

<td>## Main differences from HTTP</td>

<td>SPDY is intended to be as compatible as possible with current web-based
applications. This means that, from the perspective of the server business logic
or application API, nothing has changed. To achieve this, all of the application
request and response header semantics are preserved. SPDY introduces a "session"
which resides between the HTTP application layer and the TCP transport to
regulate the flow of data. This "session" is akin to an HTTP request-response
pair. The following changes represent the differences between SPDY and
HTTP:</td>

<td>#### The request</td>

<td>#### To initiate a new request, clients first create a new SPDY session. Once the session is created, the client can create a new SPDY stream to carry the request. Part of creating the stream is sending the HTTP header block. The HTTP header block in SPDY is almost unchanged from today's HTTP header block, with the following differences:</td>

*   <td>The first line of the request is unfolded into name/value pairs
            like other HTTP headers. The names of the first line fields are
            `method`, `url`, and `version`. These keys are required to be
            present. The 'url' is the fully-qualified URL, containing protocol,
            host, port, and path.</td>
*   <td>Duplicate header names are not allowed.</td>
*   <td>Header names are all lowercase.</td>
*   <td>The `Connection` and `Keep-Alive` headers are no longer valid
            and are ignored if present.</td>
*   <td>Clients are assumed to support `Accept-Encoding: gzip`. Clients
            that do not specify any body encodings receive gzip-encoded data
            from the server.</td>
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
                of the post; see <a
                href="/spdy/spdy-protocol/spdy-protocol-draft1#dataflow">Data
                flow</a> below.</td>
    *   <td>`Content-length` is only advisory for length (so that
                progress meters can work).</td>
    *   <td>Chunked encoding is no longer valid.</td>
    *   <td>The POST data stream is terminated by a zero-length data
                frame.</td>

<td>#### The response</td>

<td>#### When responding to a HTTP request, servers will send data frames using the SPDY stream created by the client. The response is similar to HTTP/1.1 in that it consists of a header block followed by a body. However, there are a few notable changes:</td>

*   <td>The response status line is unfolded into name/value pairs like
            other HTTP headers. The names of the status line are `status `and
            `version`. These keys are required to be present</td>
*   <td>If the SPDY reply happens before a SYN_STREAM, then it includes
            parameters that inform the client regarding the request that would
            have been made to receive this response, by including `url `and
            `method` keys. </td>
*   <td>All header names must be lowercase.</td>
*   <td>The `Connection` and `Keep-alive` response headers are no longer
            valid.</td>
*   <td>`Content-length` is only advisory for length.</td>
*   <td>Chunked encoding is no longer valid.</td>
*   <td>Duplicate header names are not allowed.</td>

<td>## Connections</td>

<td>The first implementation of the SPDY session runs atop TCP, similarly to how
HTTP works today. The client is expected to be the TCP connection initiator.
Because it runs on TCP, we have a reliable transport. Unlike HTTP, all
connections with SPDY are persistent connections. The HTTP connection header
does not apply.</td>

<td>For best performance, it is expected that clients will not close open
connections until the user navigates away from all web pages referencing a
connection, or until the server closes the connection. Servers are encouraged to
leave connections open for as long as possible, but can terminate idle
connections after some amount of inactivity if necessary.</td>

<td>## Framing</td>

<td>Once the TCP connection is established, clients and servers exchange framed
messages. There are two types of frames: control frames and data frames. Frames
always have a common header which is 8 bytes.</td>

<td>The first bit is a control bit indicating whether a frame is a control frame
or data frame. Control frames carry a version number, a frame type, flags, and a
length. Data frames contain the stream ID, flags, and the length for the payload
carried after the common header. The simple header is designed to make reading
and writing of frames easy.</td>

<td>All integer values, included length, version, and type, are in network byte
order. SPDY does not enforce alignment of types in dynamically sized
frames.</td>

<td>### Control frames</td>

<td> +----------------------------------+</td>

<td> |C| Version(15bits) | Type(16bits) |</td>

<td> +----------------------------------+</td>

<td> | Flags (8) | Length (24 bits) |</td>

<td> +----------------------------------+</td>

<td> | Data |</td>

<td> +----------------------------------+</td>

<td> Control frame fields:</td>
<td> Control bit: The 'C' bit is a single bit indicating that this is a control
message. For control frames this value is always 1. </td>
<td> Version: The version number of the session protocol (currently 1).</td>
<td> Type: The type of control frame. Control frames are SYN_STREAM, SYN_REPLY,
etc. </td>

<td>Flags: Flags related to this frame. Flags for control frames and data frames
are different.</td>
<td> Length: An unsigned 24-bit value representing the number of bytes after the
length field. </td>

<td>Data: data associated with this control frame. The format and length of this
data is controlled by the control frame type.</td>

<td>Data frames</td>

<td> +----------------------------------+</td>

<td> |C| Stream-ID (31bits) |</td>

<td> +----------------------------------+</td>

<td> | Flags (8) | Length (24 bits) | </td>

<td> +----------------------------------+</td>

<td> | Data |</td>

<td> +----------------------------------+</td>

<td>Data frame fields:</td>
<td> Control bit: For data frames this value is always 0.</td>
<td> Stream-ID: A 31-bit value identifying the stream.</td>
<td> Flags: Flags related to this frame. Valid flags are:</td>

*   <td>0x01 = FLAG_FIN - signifies that this frame represents the
            half-close of this stream. See <a
            href="/spdy/spdy-protocol/spdy-protocol-draft1#streamhalfclose">Stream
            half-close</a> below.</td>

<td> Length: An unsigned 24-bit value representing the number of bytes after the
length field. The total size of a data frame is 8 bytes + length. It is valid to
have a zero-length data frame.</td>
<td> Data: A variable-length field containing the number of bytes in the
payload. </td>

<td>## ### Hello message</td>

<td>After the connection has been established, SPDY employs an asynchronous
Hello sequence where each side informs the other about the communication details
it knows about. Unlike most protocols, this Hello sequence is optional and fully
asynchronous. Because it is asynchronous, it does not add a round-trip latency
to the connection startup. But because it is asynchronous and optional, both
sides must be prepared for this message to arrive at any time or not at
all.</td>

<td>To initiate a Hello sequence, either side can send a HELLO control frame.
The Hello frame is optional, but if it is to be sent, it must be the first frame
sent. When a Hello message is received, the receiver is not obligated to reply
with a Hello message in return. The message is therefore completely
informational.</td>

<td>HELLO control message:</td>

<td> +----------------------------------+</td>

<td> |1| 1 | 4 |</td>

<td> +----------------------------------+</td>

<td> | Flags (8) | Length (24 bits) | </td>

<td> +----------------------------------+</td>

<td> | Unused |Number of entries |</td>

<td> +----------------------------------|</td>
<td> | ID/Value Pairs |</td>
<td> | ... |</td>

<td> HELLO message fields:</td>
<td> Control bit: The control bit is always 1 for this message.</td>
<td> Version: The SPDY version number.</td>
<td> Type: The message type for a HELLO message is 4.</td>

<td>Unused: 16 bits of unused space, reserved for future use.</td>

<td> Number of entries: A 16-bit value representing the number of ID/value pairs
in this message.</td>
<td> ID: A 32-bit ID number. The following IDs are valid:</td>

*   <td> 1 - HELLO_BANDWIDTH_TO_YOU allows the sender to send its
            expected upload bandwidth on this channel. This number is an
            estimate. The value should be the integral number of kilobytes per
            second that the sender predicts as an expected maximum upload
            channel capacity.</td>
*   <td> 2 - HELLO_BANDWIDTH_FROM_YOU allows the sender to send its
            expected download bandwidth on this channel. This number is an
            estimate. The value should be the integral number of kilobytes per
            second that the sender predicts as an expected maximum download
            channel capacity.</td>
*   <td>3 - HELLO_ROUND_TRIP_TIME allows the sender to send its expected
            round-trip-time on this channel. The round trip time is defined as
            the minimum amount of time to send a control frame from this client
            to the remote and receive a response. The value is represented in
            milliseconds.</td>
*   <td> 4 - HELLO_MAX_CONCURRENT_STREAMS allows the sender to inform
            the remote endpoint the maximum number of concurrent streams which
            it will allow. By default there is no limit. For implementors it is
            recommended that this value be no smaller than 100. </td>

<td> Value: A 32-bit value.</td>

<td> The message is intentionally expandable for future information which may
improve client-server communications. The sender does not need to send every
type of ID/value. It must only send those for which it has accurate values to
convey. When multiple ID/value pairs are sent, they should be sent in order of
lowest id to highest id.</td>

<td>## Streams</td>

<td>Streams are independent sequences of bi-directional data cut into frames.
Streams can be created either by the client or the server, can concurrently send
data interleaved with other streams, and can be cancelled. The usage of streams
with HTTP is such that a single HTTP request/response occupies a single stream,
and the stream is not reused for a second request. This is because streams can
be independently created without incurring a round-trip.</td>

<td>Upon stream initiation, streams allow for each side to transmit a
fixed-length list of name/value pairs to the other endpoint.</td>

<td>### Stream creation</td>

<td>A stream is created by sending a control packet with the type set to
SYN_STREAM(1). If the server is initiating the stream, the Stream-ID must be
even. If the client is initiating the stream, the Stream-ID must be odd. 0 is
not a valid Stream-ID. Stream-IDs from each side of the connection must increase
monotonically as new streams are created. E.g. Stream 2 may be created after
stream 3, but stream 7 must not be created after stream 9.</td>

<td>Upon receipt of a SYN_STREAM frame, the server replies with a SYN_REPLY
frame. The client does not need to wait for a SYN_REPLY before sending any data
frames.</td>

<td>If the endpoint receiving a SYN_STREAM does not want to accept the new
stream, it can immediately respond with a FIN_STREAM control frame. Note,
however, that the initiating endpoint may have already sent data on the stream
as well; this data must be ignored.</td>

<td> SYN_STREAM control message: </td>

<td> +----------------------------------+</td>

<td> |1| 1 | 1 |</td>

<td> +----------------------------------+</td>

<td> | Flags (8) | Length (24 bits) |</td>

<td> +----------------------------------+</td>

<td> |X| Stream-ID (31bits) |</td>

<td> +----------------------------------+</td>

<td> | Pri | Unused | NV Entries |</td>

<td> +----------------------------------|</td>
<td> | Name/value header block |</td>
<td> | ... |</td>

<td>SYN_STREAM message fields:</td>
<td> Flags: Flags related to this frame. Valid flags are:</td>

*   <td>0x01 = FLAG_FIN - signifies that this frame represents the
            half-close of this stream. When set, it indicates that the sender
            will not produce any more data frames in this stream. </td>

<td>Length: An unsigned 24 bit value representing the number of bytes after the
length field. The total size of a SYN_STREAM frame is 8 bytes + length. The
length for this frame must be greater than or equal to 8.</td>
<td> Priority: A 2-bit priority field. If an endpoint has initiated multiple
streams, the priority field represents which streams should be given first
precidence. Servers are not required to strictly enforce the priority field,
although best-effort is assumed. 0 represents the lowest priority and 3
represents the highest priority. The highest-priority data is that which is most
desired by the client.</td>
<td> Unused: 14 bits of unused space, reserved for future use.</td>
<td> NV entries: (16 bits) The number of name/value pairs that follow.</td>

<td>The <a
href="/spdy/spdy-protocol/spdy-protocol-draft1#nameheaderblock">Name/value
block</a> is described below.</td>

<td>SYN_REPLY control message:</td>

<td> +----------------------------------+</td>

<td> |1| 1 | 2 |</td>

<td> +----------------------------------+</td>

<td> | Flags (8) | Length (24 bits) |</td>

<td> +----------------------------------+</td>

<td> |X| Stream-ID (31bits) |</td>

<td> +----------------------------------+</td>

<td> | Unused | NV entries |</td>

<td> +----------------------------------|</td>
<td> | Name/value header block |</td>
<td> | ... |</td>

<td> SYN_REPLY message fields:</td>
<td> Flags: Flags related to this frame. Valid flags are:</td>
<td> 0x01 = FLAG_FIN - signifies that this frame represents the half-close of
this stream. When set, it indicates that the sender will not produce any more
data frames in this stream..</td>
<td> Length: An unsigned 24-bit value representing the number of bytes after the
length field. The total size of a SYN_STREAM frame is 8 bytes + length. The
length for this frame must be greater than or equal to 8.</td>
<td> Unused: 16 bits of unused space, reserved for future use.</td>
<td> NV entries: (16 bits) The number of name/value pairs that follow. </td>

<td>The <a href="/spdy/spdy-protoco#nameheaderblock">Name/value block</a> is
described below.</td>

<td>### Name/value header block format</td>

<td>Both the SYN_STREAM and SYN_REPLY frames contain a Name/value header block.
The header block used by both the request and the response is the same. It is
designed so that headers can be easily appended to the end of a list and also so
that it can be easily parsed by receivers. Each numeric value is 2 bytes.</td>

<td> +----------------------------------+</td>

<td> | Length of name (int16) |</td>

<td> +----------------------------------+</td>

<td> | Name (string) |</td>

<td> +----------------------------------+</td>

<td> | Length of value (int16) |</td>

<td> +----------------------------------+</td>

<td> | Value (string) |</td>

<td> +----------------------------------+</td>

<td> | (repeats) |</td>

<td>Each header name must have at least one value. The length of each name and
value must be greater than zero. Although sending of names or values of zero
length is invalid, receivers of zero-length name or zero-length values must
ignore the pair and continue.</td>

<td>Duplicate header names are not allowed. To send two identically named
headers, send a header with two values, where the values are separated by a
single NUL (0) byte. Senders must not send multiple, in-sequence NUL characters.
Receivers of multiple, in-sequence NUL characters must ignore the name/value
pair.</td>

<td>Strings are utf8 encoded and are not NUL terminated.</td>

<td>The entire contents of the name/value header block is compressed using zlib
deflate. There is a single zlib stream (context) for all name value pairs in one
direction on a connection. SPDY uses a SYNC_FLUSH between frame which uses
compression. The stream is initialized with the following dictionary (without
line breaks, NULL terminated):</td>

<td>optionsgetheadpostputdeletetraceacceptaccept-charsetaccept-encodingaccept-</td>
<td>languageauthorizationexpectfromhostif-modified-sinceif-matchif-none-matchi</td>
<td>f-rangeif-unmodifiedsincemax-forwardsproxy-authorizationrangerefererteuser</td>
<td>-agent10010120020120220320420520630030130230330430530630740040140240340440</td>
<td>5406407408409410411412413414415416417500501502503504505accept-rangesageeta</td>
<td>glocationproxy-authenticatepublicretry-afterservervarywarningwww-authentic</td>
<td>ateallowcontent-basecontent-encodingcache-controlconnectiondatetrailertran</td>
<td>sfer-encodingupgradeviawarningcontent-languagecontent-lengthcontent-locati</td>
<td>oncontent-md5content-rangecontent-typeetagexpireslast-modifiedset-cookieMo</td>
<td>ndayTuesdayWednesdayThursdayFridaySaturdaySundayJanFebMarAprMayJunJulAugSe</td>
<td>pOctNovDecchunkedtext/htmlimage/pngimage/jpgimage/gifapplication/xmlapplic</td>
<td>ation/xhtmltext/plainpublicmax-agecharset=iso-8859-1utf-8gzipdeflateHTTP/1</td>
<td>.1statusversionurl</td>

<td>TODO(mbelshe): Remove the NULL termination on the dictionary; not useful.
(thanks CostinM)</td>

<td>TODO(mbelshe): Remove the empty block (4 bytes) inserted in the stream after
a SYNC_FLUSH. (thanks, CostinM)</td>

<td>## Stream data exchange</td>

<td>Once a stream is created, it is used to send arbitrary amounts of data in
either direction. When either side has finished sending data it can send a frame
with the FIN_FLAG set. (See "TCP connection teardown" below.)</td>

<td>### Stream half-close</td>

<td> When one side of the stream sends a control or data frame with the FLAG_FIN
flag set, the stream is considered to be half-closed from that side. The sender
of the FLAG_FIN is indicating that no further data will be sent from the sender
on this stream. When both sides have half-closed, the stream is considered to be
closed. </td>

<td>### Stream close</td>

<td>There are 3 ways that streams can be terminated: normal termination, abrupt
termination, and TCP connection teardown.</td>

<td>#### Normal termination</td>

<td>Normal stream termination occurs when both sides of the stream have
half-closed the stream.</td>

<td>#### Abrupt termination</td>

<td>Either the client or server can send a FIN_STREAM control packet at any
time. Upon receipt of the FIN_STREAM, both sides must ignore further data
received on the stream and both sides must stop transmitting to the stream. The
FIN_STREAM is intended for abnormal stopping of a stream.</td>

<td>FIN_STREAM control frame:</td>

<td> +-------------------------------+</td>

<td> |1| 1 | 3 |</td>

<td> +-------------------------------+</td>

<td> | Flags (8) | 8 |</td>

<td> +-------------------------------+</td>

<td> |X| Stream-ID (31bits) |</td>

<td> +-------------------------------+</td>

<td> | Status code | </td>

<td> +-------------------------------+</td>

<td> FIN_STREAM message fields:</td>
<td> Flags: Flags related to this frame. Valid flags are:</td>

*   <td>0x01 = FLAG_FIN - When set, it indicates that the sender will
            not produce any more data frames in this stream. See "Stream
            half-close" above.</td>

<td> Length: An unsigned 24-bit value representing the number of bytes after the
length field. For FIN_STREAM control frames, this value is always 8.</td>
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
            refused before any processing has been done on the stream. For
            non-indepotent methods this means that request can be retried.</td>

<td>Note: 0 is not a valid status code for a FIN_STREAM.</td>

<td>TODO - Define more specific errors.</td>

<td>#### TCP connection teardown</td>

<td>If the TCP connection is torn down while unterminated streams are active (no
FIN_STREAM frames have been sent or received for the stream), then the endpoint
must assume that the stream was abnormally interrupted and may be
incomplete.</td>

<td>If a client or server receives data on a stream which has already been torn
down, it must ignore the data received after the teardown.</td>

<td>## Data flow</td>

<td> Because TCP provides a single stream of data on which SPDY multiplexes
multiple logical streams, it is important for clients and servers to interleave
data messages for concurrent sessions.</td>

<td>Implementors should note that sending data in smallish chunks will result in
lower end-user latency for a page as this allows the browser to begin parsing
metadata early, and, in turn, to finalize the page layout. Sending large chunks
yields a small increase in bandwidth efficiency at the cost of slowing down the
user experience on pages with many resources.</td>

<td>## Other control frames</td>

<td>### <b>NOOP</b></td>

<td>The NOOP control frame is a no-operation frame. It can be sent from the
client or the server. Receivers of a NO-OP frame should simply ignore it.</td>

<td>NOTE: This control frame may ultimately be removed. It has been implemented
for experimentation purposes.</td>

<td>NOOP control message:</td>

<td> +----------------------------------+</td>

<td> |1| 1 | 5 |</td>
<td> +----------------------------------+</td>
<td> | 0 (Flags) | 0 (Length) |</td>

<td> +----------------------------------+</td>

<td><b> Control-bit: The control bit is always 1 for this message.</b></td>
<td><b> Version: The SPDY version number.</b></td>
<td><b> Type: The message type for a NOOP message is 5.</b></td>
<td><b> Length: This frame carries no data, so the length is always 0.</b></td>

<td><b>### <b>PING</b></b></td>

<td><b>The PING control frame is a mechanism for measuring a minimal round-trip
time from the sender. It can be sent from the client or the server. Receivers of
a PING frame should send an identical frame to the sender as soon as possible
(if there is other pending data waiting to be sent, PING should take highest
priority). Each ping sent by a sender should use a unique ID.</b></td>

<td><b>NOTE: This control frame may ultimately be removed. It has been
implemented for experimentation purposes.</b></td>

<td><b>PING control message:</b></td>

<td><b> +----------------------------------+</b></td>

<td><b> |1| 1 | 6 |</b></td>

<td><b> +----------------------------------+</b></td>

<td><b> | 0 (flags) | 4 (length) |</b></td>

<td><b> +----------------------------------|</b></td>

<td><b> | 32-bit ID |</b></td>

<td><b> +----------------------------------|</b></td>

<td><b> Control bit: The control bit is always 1 for this message.</b></td>
<td><b> Version: The SPDY version number.</b></td>
<td><b> Type: The message type for a PING message is 6.</b></td>
<td><b> Length: This frame is always 4 bytes long.</b></td>
<td><b> ID: A unique ID for this ping.</b></td>
<td><b> Note: If a sender uses all possible PING ids (e.g. has sent all 2^32
possible IDs), it can "wrap" and start re-using IDs.</b></td>

<td>### <b>GOAWAY</b></td>

<td>The GOAWAY control frame is a mechanism to tell the remote side of the
connection that it should no longer use this session for further requests. It
can be sent from the client or the server. Once sent, the sender agrees not to
initiate any new streams on this session. Receivers of a GOAWAY frame must not
send additional requests on this session, although a new session can be
established for new requests. The purpose of this message is to allow the server
to gracefully stop accepting new requests (perhaps for a reboot or maintenance),
while still finishing processing of previously established requests.</td>

<td>There is an inherent race condition between a client sending requests and
the server sending a GOAWAY message. To deal with this case, the GOAWAY message
contains a last-stream identifier indicating the last stream which was accepted
in this session. If the client issued requests for sessions after this
stream-id, they were not accepted by the server and should be re-issued later at
the client's discretion.</td>

<td>NOTE: This control frame may ultimately be removed. It has been implemented
for experimentation purposes.</td>

<td>NOTE: (mnot@mnot.org suggests that mandatory GOAWAY could be useful to
ensure graceful closure of sessions. This would help if the last request on a
session was a POST, and the server closes, the client can't know whether the
request was sent or not. Requiring a GOAWAY before closing would notify the
client exactly which requests had been processed and which had not.)</td>

<td>GOAWAY control message:</td>

<td> +----------------------------------+</td>

<td> |1| 1 | 7 |</td>

<td> +----------------------------------+</td>

<td> | 0 (flags) | 4 (length) |</td>

<td> +----------------------------------|</td>

<td> | Last-good-stream-ID |</td>

<td> +----------------------------------|</td>

<td> Control bit: The control bit is always 1 for this message.</td>
<td> Version: The SPDY version number.</td>
<td> Type: The message type for a GOAWAY message is 7.</td>
<td> Length: This frame is always 4 bytes long.</td>
<td> Last-good-stream-Id: The last stream id which was accepted by the sender of
the GOAWAY message.</td>
<td><b>SUBRESOURCE</b></td>
<td>The subresource control frame is an optional control frame to advise the
receiver of resources that will be needed. If the url and method in the name
value pairs do not match those that are associated with the stream id then this
control message must be ignored.</td>
<td>Control frame</td>

<td> +----------------------------------+</td>

<td> |C| Version(15bits) | 8 |</td>

<td> +----------------------------------+</td>

<td> | Flags (8) | Length (24 bits) |</td>

<td> +----------------------------------+</td>
<td> |0| Stream id (31 bits) |</td>
<td> |----------------------------------|</td>
<td> | Unused (16 bits) | NV Entries |</td>
<td> |----------------------------------|</td>

<td> | Key value pairs |</td>

<td> +----------------------------------+</td>

<td>Length: An unsigned 24 bit value representing the number of bytes after the
length field. The total size of a SUBRESOURCE frame is 8 bytes + length. The
length for this frame must be greater than or equal to 8.</td>
<td>Stream-id is the stream-id for the stream that the subresources are
associated with</td>

<td>NV entries: The number of name/value pairs that follow.</td>

<td>The <a href="/spdy/spdy-protoco#nameheaderblock">Name/value block</a> is the
same as a SYN_REPLY, but url and method are mandatory fields.</td>

<td>If a sender is using the SUBRESOURCE control frame to inform the client of
streams that the will be created (X-Associated-Content)</td>

<td> (e.g. a server creating a stream to send an image used on a webpage) then
the SUBRESOURCE message must be sent before the data frame where the receiver
could discover the additional resource.</td>

<td>## Server Push</td>

<td>This section needs much work; it currently documents the prototyped
protocol, but is not ready for broad implementation.</td>

<td>Because SPDY enables both clients and servers to create streams, a server
may decide to initiate a stream to the client. Generally, this should be done if
the server has knowledge that the client will need a resource before the client
has requested it. Servers should use this feature with care, because if the
resource is cacheable, and the client already has a cached copy of the resource,
this stream may be wasteful. Further, servers should inform clients of the
pending pushed resource as early as possible to avoid races between the server
pushing the stream and the client requesting it (based on having discovered need
for the resource from other downloaded content). Not all content is push-able
from the server. Only resources fetched via the HTTP GET method can be
server-pushed.</td>

<td>Note: Race conditions can only be avoided with the client if the client uses
a single SPDY session to the server.</td>

<td><b>Server Implementation</b></td>

<td>When the server intends to push resources to the client, it should append a
header, "X-Associated-Content" to the SYN_REPLY for the resource which will also
contain pushed content. This SYN_REPLY MUST be sent prior to initiating the
server-pushed streams.</td>

<td>For each pushed resource listed in the X-Associated-Content header, the
server may create streams by sending a SYN_STREAM frame. This is the same as
client-side stream initiation, except for that the server will append a header
called "Path" which contains the path of the resource requested.</td>

<td><b>Client Implementation</b></td>

<td>When fetching a resource the client has 3 possibilities:</td>

1.  <td>the resource is not being pushed</td>
2.  <td>the resource is being pushed, but the data has not yet
            arrived</td>
3.  <td>the resource is being pushed, and the data has started to
            arrive</td>

<td>When a SYN_REPLY frame which contains a X-Associated-Content header is
received, the client MUST NOT issue GET requests for those resources and instead
wait for the pushed content from the server. Similarly, if the client would have
made a request for a resource but a X-Associated-Content header has been
received for that resource, and yet the pushed stream has not arrived, the
client MUST wait for the data to be pushed.</td>

<td>When a SYN frame is received which contains a path previously specified in
an X-Associated-Content header, the client SHOULD buffer the received data from
the stream as is appropriate for the client.</td>

<td><b>Flaws in this specification:</b></td>

*   <td>The syntax of the X-Associated-Content header needs to be
            redone.</td>
*   <td>Incorporate the SUBRESOURCE control frame into this
            specification.</td>
*   <td>The "Path" header in the SYN_STREAM should be changed to "Url"
            and be a fully qualified path.</td>
*   <td>Need to implement flow control on pushes</td>
*   <td>Need to specify error case when the client cannot buffer.</td>

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
server its approximate bandwidth. The HELLO message is part of this, but
measurement, reporting and all of the other infrastructure and behavioral
modifications are lacking.</td>

*   <td>Server-controlled connection parameters.</td>

<td>The "server" -- since either side may be considered a "server" in SPDY,
"server" refers here to the side which receives requests for new sessions -- can
inform the other side of reasonable connection or stream limits which should be
respected. As with stream capacity, the HELLO message provides a container for
signaling this, but no work has yet been done to figure out what values should
be set, when, and what behavioral changes should be expected when the value does
change.</td>

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

*   <td>Uni-directional (but associated) streams</td>

<td>It may be a mistake to make a connection-analogue in the stream. It is
perhaps better to allow each side to make a unidirectional stream, and provide
data about which stream it may be associated with. This offers a much
easier-to-comprehend association mechanism for server push.</td>

*   <td>Flow control</td>

<td>Each side can announce how much data or bandwidth it will accept for each
class of streams. If this is done, then speculative operations such as server
push can soak up a limited amount of the pipe (especially important if the pipe
is long and thin). This may allow for the elimination of more complex "is this
already in the cache" or "announce what I have in my cache" schemes which are
likely costly and complex.</td>

*   <td>Use of gzip compression</td>

<td>The use of gzip compression should be stateful only across headers in the
channel. Essentially, separate gzip contexts should be maintained for headers
versus data.</td>

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

</tr>
</table>
