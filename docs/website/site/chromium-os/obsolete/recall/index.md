---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/obsolete
  - Obsolete Documentation
page_name: recall
title: Recall
---

**Note: the recall code has been deleted from autotest and is no longer
available.**

Recall is a server-side system that intercepts DNS, HTTP and HTTPS traffic from
clients for either recording or playback. This allows tests to be run in labs
without Internet access by playing back pre-recorded copies of websites, allows
tests to be run that use recorded copies of sites known to exhibit errors or
many iterations of the same test to be run with the same data.

Recall is intended to be completely transparent to client tests.

It runs as an autotest server test which makes some configuration changes to the
server machine, and then runs the desired client test on the client wrapped with
a Recall context manager that takes care of adjusting the client's configuration
to redirect the traffic to the recall server (in the case it's not the network's
default gateway already) and install a root certificate for HTTPS
man-in-the-middling.

**Prerequisites:**

At the time of this writing you **must** install the following packages, which
are not installed by default.

1.  the **dnspython** package in the chroot: sudo emerge dnspython
2.  the **iproute2** package on the chroot: sudo emerge iproute2
3.  there may be other needed packages which I am now forgetting
            (iptables?)

In addition, the server-side iptable command to set up the routing tables fails
for me with this error:

FATAL: Could not load /lib/modules/2.6.38.8-gg836/modules.dep: No such file or
directory

I worked around this by copying that file from my root file system into the
chroot. (For testing, I also ran the same iptables command manually outside the
chroot, so I am not sure if you need to do that so that some module is loaded
correctly.)

**From the command line:**

The highest level way in which to run Recall is simply to use the
test_RecallServer test, which handles all of the heavy-lifting for you, and can
be run using run_remote_tests.

Run desktopui_UrlFetch on the client, record results which can be found as
&lt;RESULTS DIR&gt;/pickle:

> ./run_remote_tests.sh --remote=&lt;DEVICE IP&gt; test_RecallServer -a
> 'desktopui_UrlFetch'

Run desktopui_UrlFetch on the client, only returning results found in the given
pickle file created from a previous recording:

> ./run_remote_tests.sh --remote=&lt;DEVICE IP&gt; test_RecallServer -a
> 'desktopui_UrlFetch /path/to/pickle'

Run desktopui_UrlFetch on the client 100 times, the first time will be recorded,
the subsequent 99 will use the playback only

> ./run_remote_tests.sh --remote=&lt;DEVICE IP&gt; test_RecallServer -a
> 'desktopui_UrlFetch num_iterations=100'

Run desktopui_UrlFetch on the client using a simple proxy server that doesn't
record (mostly for infrastructure testing)

> ./run_remote_tests.sh --remote=&lt;DEVICE IP&gt; test_RecallServer -a
> 'desktopui_UrlFetch proxy_only=1'

Infrastructure restrictions mean that these test must always be run as root (or
with sudo), and may not be run against the 127.0.0.1 address. This includes
running them against VMs on your workstation, unless you set up a bridge network
for KVM instead of using its userspace networking.

**From a control file:**

You can also write a server suite that performs the same actions, perhaps on
multiple tests, for example:

> def run(machine):

> host = hosts.create_host(machine)

> job.run_test('test_RecallServer', host=host, test='desktopui_UrlFetch',
> pickle_file='/path/to/pickle')

> parallel_simple(run, machines)

**Customizing server behaviour:**

The test_RecallServer test takes care of many of the common cases, but in some
situations it may not be appropriate to modify it to cover yours and instead
create a new server test that uses the Recall infrastructure. This can be
accomplished by deriving your test from recall_test.RecallServerTest; the
documentation for that class goes into greater detail, here's a very brief
example:

> from autotest_lib.server.cros import recall_test, recall

> class test_MyTest(recall_test.RecallServerTest):

> def initialize(self, host):

> # minimum set up required

> self.certificate_authority = recall.CertificateAuthority("/O=Test")

> self.dns_client = recall.DNSClient()

> self.http_client = recall.HTTPClient()

> # takes care of the rest of the heavy lifting

> # can be overidden a lot, see documentation

> recall_test.RecallServerTest.initialize(self, host)

> def run_once(self, host):

> self.RunTestOnHost('test_MyClientTest', host)

**Static server, custom client:**

For even deeper diving, the Recall infrastructure can be run in a standalone
manner. For example a server can be installed on the network which already has
clients use it as a DNS server and Default Gateway via DHCP and redirects all
traffic to itself with static iptables rules. The recall server itself can be
run by creating instances of recall.DNSServer, recall.HTTPServer,
recall.HTTPSServer, etc. by hand with fixed ports. See the documentation for the
recall package for more details, here's a very quick example:

> from autotest_lib.server.cros import recall

> ca = recall.CertificateAuthority("/O=Test")

> dns = recall.DNSServer(('', 5000), dns_client=DNSClient())

> client = recall.DeterministicScriptInjector(recall.HTTPClient())

> http = recall.HTTPServer(('', 8000), http_client=client)

> https = recall.HTTPSServer(('', 4430), http_client=client,
> dns_client=dns.dns_client, certificate_authority=ca)

> # remain running as long as needed

> dns.shutdown()

> http.shutdown()

> https.shutdown()

A client test can now be written that uses this fixed infrastructure, though
note that this test will not function without it. Client tests simply wrap the
call in the control file with the context manager:

> from autotest_lib.client.cros import recall

> with recall.RecallServer(IP_OF_SERVER):

> job.run_test(...)

The context manager takes care of setting up the root certificate used for HTTPS
recording, it also takes care of the DNS Server and Default Route if those
aren't already correct.
