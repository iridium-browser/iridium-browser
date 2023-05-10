---
breadcrumbs:
- - /nativeclient
  - Native Client
page_name: life-of-sel_ldr
title: The life of sel_ldr
---

sel_ldr stands for Secure ELF Loader and is the heart of Native Client. The
plugin (or other supervisor) starts sel_ldr, loads nacl module, opens SRPC
channels, communicates and terminates sel_ldr at the end.

**Starting sel_ldr**

In order to start sel_ldr, we need to create a socketpair(AF_UNIX, SOCK_DGRAM,
0, fds) and pass the one side of it to sel_ldr.

The minimal set of sel_ldr arguments is:

sel_ldr -i D:5 -X 5 -- nacl_module &lt;nacl_module_args&gt;

where D=fds\[0\] is the file descriptor of the above socketpair half. This
socketpair is the service socket which will be used to obtain some descriptors
from sel_ldr.

**Receiving a bound socket**

The first action we need to do is to receive a bound socket that will be used to
open SRPC channels. It's very simple, we need just to call recvmsg on the
plugin's half of the above socketpair. The simplification of
src/nonnacl_util/sel_ldr_launcher.cc::GetSockAddr method:

int\* control = (int\*)calloc(100, sizeof(int));

struct iovec iov;

iov.iov_base = calloc(100, 1);

iov.iov_len = 100;

struct msghdr msg;

msg.msg_name = NULL;

msg.msg_namelen = 0;

msg.msg_iov = &iov;

msg.msg_iovlen = 1;

msg.msg_control = control;

msg.msg_controllen = 100 \* sizeof(int);

msg.msg_flags = 0;

recvmsg(fds\[1\], &msg, 0)

int bound_socket = control\[4\]; // This is the socket we wanted to receive

// NOTE: this is a hacky code, it does not check for errors, so it should not be
used in production.

See man recvmsg for more details.

Also, you can look into [fuse
tutorial](http://ptspts.blogspot.com/2009/11/fuse-protocol-tutorial-for-linux-26.html).
They also start fusermount with a socketpair like above and receive the file
descriptor to communicate with the kernel. That documentation could be a good
explanation why does it work this way.

**Opening SRPC channels**

The plugin opens 2 SRPC channels: trusted_command, and untrusted. The first one
is used to pass commands to the trusted part of sel_ldr. It is opened first and
we need to do the following:

1. Create a DGRAM socket pair like we did in the first paragraph

2. Send a half of the socket pair to bound_socket via sendmsg (see below for a
code snippet)

3. Use the other half of the socket pair as the descriptor for SRPC channel

Sending a file descriptor is very similar to receiving:

int srpc_fds\[2\];

socketpair(AF_UNIX, SOCK_DGRAM, 0, srpc_fds)

int\* control = (int\*)calloc(100, sizeof(int));

control\[0\] = 20;

control\[1\] = 0;

control\[2\] = 1;

control\[3\] = 1;

control\[4\] = srpc_fds\[0\]; // Sending the half of the socket pair

struct iovec iov;

char\* base = (char\*) calloc(100, 1);

base\[0\] = 'c';

iov.iov_base = base

iov.iov_len = 1;

struct msghdr msg;

msg.msg_name = NULL;

msg.msg_namelen = 0;

msg.msg_iov = &iov;

msg.msg_iovlen = 1;

msg.msg_control = control;

msg.msg_controllen = 20;

msg.msg_flags = 0;

sendmsg(bound_socket, &msg, 0)

// srpc_fds\[1\] is the fd to use for SRPC communication

So, the first SRPC channel is opened and we will call it trusted_command
channel. The first what we should do with it is to call SRPC method
service_discovery::C to get the list of available SRPC methods for this channel.
service_discovery method always has index 0, so we can do it w/o the knowledge
of the available methods.

To read about SRPC, go to [Using Simple RPC to Implement Native Client
Services](/system/errors/NodeNotFound), the
[reference](http://nativeclient.googlecode.com/svn/data/docs_tarball/nacl/googleclient/native_client/scons-out/doc/html/group___s_r_p_c.html)
or native_client/src/shared/srpc in the Native Client sources. Take a look into
native_client/src/shared/srpc/rpc_serialize.c to get understanding of SRPC
marshalling.

To call SRPC method we need to sendmsg with the serialized SRPC call and after
that recvmsg to get the answer. In the simplest case of service_discovery::C we
need to send the following:

iov = \[{"\\1\\336\\300\\323\\0\\0\\0\\0\\0\\0\\0\\0\\0\\0\\0\\0", 16},
{"\\2\\0\\332\\300\\0\\0\\0\\0\\0\\0\\0\\0\\1\\0\\0\\0\\0\\0\\0\\0\\0\\1\\0\\0\\0C\\240\\17\\0\\0",
30}\]

The first string is an SRPC banner that is constant.

The second string has the following format (according to
native_client/src/shared/srpc/rpc_serialize.c):

/\*

\* Message formats:

\* SRPC communicates using two main message types, requests and responses.

\* Both are communicated with an rpc (header) prepended.

\*

\* rpc:

\* protocol - (uint32_t) 4 bytes

\* message id - (uint64_t) 8 bytes

\* request/response - (uint8_t) 1 byte

\* rpc method index - (uint32_t) 4 bytes

\* return code - (uint32_t) 4 bytes (only sent for responses)

\*

\* request:

\* #args - (uint32_t) 4 bytes

\* #args \* (arg value) - varying size defined by interface below

\* #rets - (uint32_t) 4 bytes

\* #rets \* (arg template) - varying size defined by interface below

\*

\* response:

\* #rets - (uint32_t) 4 bytes

\* #rets \* (arg value) - varying size defined by interface below

\*

\*/

When we sent service_discover::C message, sel_ldr will reply with something
like:

msg_iov(2)=\[

{"\\1\\336\\300\\323\\0\\0\\0\\0\\0\\0\\0\\0\\0\\0\\0\\0", 16},

{"\\2\\0\\332\\300\\0\\0\\0\\0\\0\\0\\0\\0\\0\\0\\0\\0\\0\\0\\1\\0\\0\\1\\0\\0\\0CM\\0\\0\\0service_discovery::C\\nhard_shutdown::\\nstart_module::i\\nlog:is:\\nload_module:h:\\n\\0",
107}

\]

As we can see, the first 16 bytes are still the SRPC banner. The second string
contains the list of methods:

service_discovery::C

hard_shutdown::

start_module::i

log:is:

load_module:h:

Before we can open the untrusted SRPC channel, we need to start a NaCl module.
Currently, sel_ldr has been validated and initialized nacl_module but not
started it.

As we can see, start_module has the index 2, so the corresponding message
(omitting the banner) is:
"\\2\\0\\332\\300\\0\\0\\0\\0\\0\\0\\0\\0\\1\\2\\0\\0\\0\\0\\0\\0\\0\\1\\0\\0\\0i"

Sel_ldr will reply with:

"\\2\\0\\332\\300\\0\\0\\0\\0\\0\\0\\0\\0\\0\\2\\0\\0\\0\\0\\1\\0\\0\\1\\0\\0\\0i\\0\\0\\0\\0"

Now we can open the untrusted SRPC channel. It's completely the same procedure
as opening the trusted_command channel: make a socket pair, sent it to the bound
socket, obtain the list of methods via service_discovery::C. When it's done,
everything is initialized. NaCl module has been started to work and sel_ldr will
be alive until the termination of untrusted program (represented by a NaCl
module) or until the supervisor (e.g. the plugin) tells to sel_ldr that it needs
to shutdown.

Hopefully, this information will be needed to just a few developers which are
working on implementing alternative plugins for Native Client or other
sandboxing libraries (for example, for server-side NaCl).

Note, that this is a low level information which were obtained with strace. All
the process could be changed over time and there were the changes last three
months. So, it's more likely an overview of what's happening, nothing more.
