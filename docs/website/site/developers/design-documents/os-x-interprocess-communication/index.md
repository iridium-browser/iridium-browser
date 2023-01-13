---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: os-x-interprocess-communication
title: Mach based OS X Interprocess Communication (Obsolete)
---

## ==Mach based IPC Design==

==Current status of this design:==
The design described here is currently not used on OS X. Please see the
[Interprocess
Communication](/developers/design-documents/inter-process-communication) design
doc for coverage of the current implementation for all platforms.
A reference implementation of Mach based IPC including a kqueue bridge can be
found in [issue 5308](http://code.google.com/p/chromium/issues/detail?id=5308).

In 2015, another consideration was given to using Mach IPC, and [the results of
that survey are
here](https://docs.google.com/a/chromium.org/document/d/14-twSrkEhgPI87Pi757auoRhzFYRPqbzdHorHhJn0Kk/edit#).
==Rationale for not using Mach based IPC:==
Chrome handles network communication and IPC messages on the same thread.
Sockets are waited on using kqueues via libevent.
Although there is a constant defined in the kqueue headers on OS X
(EVFILTER_MACH in sys/event.h), there is currently no way to block on both a
socket and a mach port at once, this means that our only option is to spawn
another thread to bridge Mach messages to kqueue. Our reference implementation
does this by opening a pipe between both threads and writing a byte each time a
Mach message is received.
Because of this extra step, we now need to pay the price both of receiving a
Mach message and communicating via a pipe between threads. We've timed this
approach and found it to be 10uSec slower on Desktops & 20uSecs faster on
laptops than a pure pipe based implementation.
If you look at the measurements at the bottom of this document, you can see that
most of the messages Chrome sends are very small. So the performance benefits of
Mach messages over pipes are negligible.
Thus our decision at this time is to use the same approach as Linux. If we run
into problems at a later date with a pipe-based implementation we can revert to
the Mach-based one.

### ==Mach based IPC:==

Summary of departures from current Windows architecture:

==PC::Channel:==

This class basically needs to be rewritten to use Mach ports, we require a
bidirectional communication mechanism able to transfer arbitrarily sized
messages. This means we need to create two Mach ports (Server-&gt;Client &
Client-&gt;Server).

==Establishing Communications:==

(From chrome/common/ipc_channel.h)

enum Mode {

MODE_SERVER,

MODE_CLIENT

};

IPC::Channel::Channel(const std::wstring& channel_id, Mode mode, Listener\*
listener);

bool Connect();

When a Channel is created in Server mode it creates a Mach port and registers it
with the Bootstrap server using the channel_id prefixed with the string
'chrome_'. It then sets waiting_connect_ to false.

When a Child process is started, it's passed the channel id and an authorization
token via stdin (since that's not visible to other processes on the system).

When a Channel is opened in Client mode, the Channel ID is looked up on the
Bootstrap server. The client then creates a Mach port for incoming messages, it
sends a Hello message to the server containing port rights to its incoming port
and the authorization token sent over the pipe.

Upon receiving a Hello message, the Server verifies the token, if it's valid, it
stores the send port rights and sets waiting_connect_ to false.

Server-&gt;Client.

==Sending Messages:==

(From chrome/common/ipc_channel.h)

bool Send(Message\* message);

Mach ports have a fixed queue size, we want to be able to send arbitrary numbers
of messages without blocking.

Messages to send over the wire are queued up on the Server side in an
std::vector&lt;Message\*&gt;, we specify a timeout value when sending a Mach
message, if the send times out then we set a delayed task to attempt to resend
the message after a delta.

When IPC::Channel::Send() is called, we attempt to send out all the messages in
our outgoing queue until we block.

==Receiving messages:==

Messages can be an size up to 256MB, this presents a problems since we don't
want to allocate a 256MB buffer to receive messages into.

We can allocate an input buffer of a reasonable size, and specify the
MACH_RCV_LARGE flag when receiving messages, if the message doesn't fit into our
buffer then we get a chance to dynamically allocate a new receive buffer and
stick our data in there.

We will probably also want to look at large messages and send those over as OOL
transfer (OS X Internals 9.5.5) so that they're transferred with copy-on-write
semantics.

==Security:==
The initial security token provides security in the face of rogue client
processes trying to connect back to a server, we can use the OS X Authorization
Services API & AuthorizationMakeExternalForm() to generate the token.

We can make use of Mach's sender security token (OS X Internals 9.2.2.3) to
prevent processes not owned by the user from communicating with the server.

## ==Rationale for using Mach ports:==

==Current Windows implementation:==

The IPC::Channel object (chrome/common/ipc_channel.h) sends/receives discrete
messages \[length/byte array\] over a bidirectional named pipe.
Messages are limited to be less than 256MB, but can otherwise be of arbitrary
size.
The pipe name is passed as a parameter to new rendering processes. This is
useful for debugging purposes since you can connect an arbitrary rendering
process to a browser instance.
==Sharing resources between processes:==
Windows OS Handles can be shared between processes by calling DuplicateHandle(),
this duplicates the handle into the target process and returns an ID valid in
that process. This ID can then be sent as POD over the IPC Channel . This is
very convenient since it means that they can just be wrapped in an arbitrary
messagen and send over the wire, but it's a very Windows-specific capability.
There are currently 45 calls to DuplicateHandle() in the code (Not all of these
are necessarily used for IPC).
==OS X Implementation:==
We basically have two options for implementation here worth discussing:
==FIFO:==
==Advantages:==

*   Very close to the current Windows implementation.
*   Might be shareable with Linux port.
*   Access control via full file system owner/permissions/ACL semantics

==Disadvantages:==

*   Provides no mechanism to transmit mach semaphores and other system
            resources over the connection.
*   Messy, lives in the file system.
*   OS X may have quirks that prevent us from sharing the implementation
            with Linux.

==Mach ports:==
==Advantages:==

*   Fast:
    *   pretty much any other IPC API we might use is already layered on
                top of this.
    *   Does everything it can to remap memory rather than copy data.
    *   Facilities to send over mem. buffers by remapping them via
                copy-on-write (OOL).
*   Secure - bunch of security primitives.
*   Allows us to send unnamed system resources such as semaphores and
            shared memory regions to another process.

==Disadvantages:==

*   OS X Specific.
*   Message based rather than stream based so if we get large messages
            we potentially need to copy them multiple times.

## ==Performance Considerations==

What follows are the results of some benchmarks we ran contrasting Mach
messaging and FIFO's.
We tested Mach ports using both inline & out of line (OOL) data transfer. Inline
transfer means the payload is transferred as part of the message and copied into
the receiving process. OOL remaps the memory area using copy-on-write semantics.

The executive summary is that Mach messages are faster than FIFOs on OS X,
especially if we transfer messages larger than a certain threshold using OOL.

==Technical Detail:==

The tables below contain the results from the following test programs:

*   Mach - spawn two processes and send mach messages containing an
            inline buffer of the requisite size between them.
*   Mach OOL - Same as MachPerf but sends the data OOL.
*   FIFO - spawn a new process and read/write data over a FIFO.

Our testing methodology was to send over 2000 messages, times are in uSec and
the ones shown represent the 98th percentile of the measured data. Variance of
all values is ~10uSec, possibly higher.
==Discussion:==

Inline Mach messages take a performance hit for message sizes &gt;5K, below that
they are ~1.5X FIFOS on a 4 core Mac desktop and 280%-1000% faster than FIFOs a
Laptop.

The desktop/laptop difference is something we see consistently.
OOL transfer has a constant overhead of ~30uSec which appears to be a clear win over any method that copies data between processes.

==The data (times in uSec +/- 10uSec):==
Laptop:
Packet Size (bytes) Mach Mach OOL FIFO % min(Mach,Mach OOL) better than FIFO
100 29 35 112 386
200 10 37 121 1210
500 11 36 124 1127
1024 9 36 115 1277
2048 28 37 131 467
3072 11 39 129 1172
4096 11 29 128 1163
5120 13 31 127 976
6144 51 30 134 446
7168 46 30 133 443
8192 51 32 215 671
9216 57 30 218 726
1048576 1477 29 2873 9906
5242880 11079 39 10924 28010
7340032 15144 38 14184 37326
Desktop:

Packet Size (bytes) Mach Mach OOL FIFO % min(Mach,Mach OOL) better than FIFO
100 10 26 29 290
200 11 26 30 272
500 12 29 30 250
1024 11 34 30 272
2048 15 36 31 206
3072 16 39 32 200
4096 14 29 36 257
5120 19 25 37 194
6144 66 27 34 125
7168 53 26 38 146
8192 70 26 59 226
9216 81 25 66 264
1048576 1822 33 2623 7948
5242880 11536 37 13125 35472
7340032 15693 41 17960 43804

Distribution of IPC Message Sizes In The Current Windows Implementation

The following histogram was added to IPC::Channel::Send(), and measures an hour
long browsing session including Gmail, YouTube, Hulu, CNN and other random
sites. We see that:

*   90% of messages are less than 65 bytes
*   99.9% of messages are less than 2400 bytes

Histogram: IPC.MessageSize recorded 37573 samples, average = 86.4, standard
deviation = 329.5

0 ...

12 ---------------------O (4387 = 11.7%) {0.0%}

16 ----O (990 = 2.6%) {11.7%}

21 -----------------------------------O (9307 = 24.8%) {14.3%}

28 O (10 = 0.0%) {39.1%}

37 O (8 = 0.0%) {39.1%}

49 ------------------------------------------------------------------------O
(19123 = 50.9%) {39.1%}

65 --O (614 = 1.6%) {90.0%}

86 O (38 = 0.1%) {91.7%}

113 O (113 = 0.3%) {91.8%}

149 O (34 = 0.1%) {92.1%}

196 O (110 = 0.3%) {92.2%}

258 --O (663 = 1.8%) {92.4%}

340 ---O (783 = 2.1%) {94.2%}

448 --O (653 = 1.7%) {96.3%}

590 --O (443 = 1.2%) {98.0%}

777 -O (198 = 0.5%) {99.2%}

1023 O (72 = 0.2%) {99.7%}

1347 O (6 = 0.0%) {99.9%}

1774 O (5 = 0.0%) {99.9%}

2336 O (1 = 0.0%) {100.0%}

3077 ...

12196 O (15 = 0.0%) {100.0%}

16063 ...
