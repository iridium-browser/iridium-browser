# Discovery

This discovery module is an implementation of the mDNS and DNS-SD protocols as
defined in [RFC 6762](https://tools.ietf.org/html/rfc6762) and
[RFC 6763](https://tools.ietf.org/html/rfc6763). The protocols have been fully
implemented, with the exceptions called out below.

Other modules in Open Screen use this module to advertise and discover
presentation displays. If you wish to provide your own implementation of DNS-SD,
implement the
[DNS-SD public interfaces](https://chromium.googlesource.com/openscreen/+/refs/heads/master/discovery/dnssd/public)
and updating the linker rules. In this case, the
[public discovery layer](https://chromium.googlesource.com/openscreen/+/refs/heads/master/discovery/public)
will continue to function as described below.

Note that this DNS-SD implementation is fully generic and can be used for
advertisement and discovery of any RFC 6763 compliant services.


## How To Use This Module

In order to use the discovery module, embedders should only need to reference
the
[public](https://chromium.googlesource.com/openscreen/+/refs/heads/master/discovery/public)
and
[common](https://chromium.googlesource.com/openscreen/+/refs/heads/master/discovery/common)
directories. The
[DNS-SD](https://chromium.googlesource.com/openscreen/+/refs/heads/master/discovery/dnssd)
and
[mDNS](https://chromium.googlesource.com/openscreen/+/refs/heads/master/discovery/mdns)
layers should not be accessed. In order to encourage this behavior, DEPS rules
have been defined.

The config directory provides parameters needed to configure the discovery
module. Specifically:

* The
[openscreen::discovery::Config struct](https://chromium.googlesource.com/openscreen/+/refs/heads/master/discovery/common/config.h)
provides the required data to publish or discover service
instances, such as a list of valid network interfaces or constants used to
configure the underlying stack.
* The
[openscreen::discovery::ReportingClient](https://chromium.googlesource.com/openscreen/+/refs/heads/master/discovery/common/reporting_client.h)
provides a way for embedders to receive callbacks for errors which occur inside
the discovery implementation.

The public directory provides wrappers around the DNS-SD protocol to simplify
interacting with the internals of this module:

* The
[openscreen::discovery::DnsSdServicePublisher](https://chromium.googlesource.com/openscreen/+/refs/heads/master/discovery/public/dns_sd_service_publisher.h)
provides a simple interface for registering, updating, and de-registering DNS-SD
service instances.
* The
[openscreen::discovery::DnsSdServiceWatcher](https://chromium.googlesource.com/openscreen/+/refs/heads/master/discovery/public/dns_sd_service_watcher.h)
provides an interface to begin and end querying for service types, providing
callbacks for the embedder to be notified whenever an instance is discovered,
changed, or deleted.

For an example use case of this module, see the
[standalone sender](https://chromium.googlesource.com/openscreen/+/refs/heads/master/cast/standalone_sender/main.cc)
and
[standalone receiver](https://chromium.googlesource.com/openscreen/+/refs/heads/master/cast/standalone_receiver/main.cc)
code.


## Limitations

This implementation has been created to fulfill the role of discovery for the
CastV2 protocol. For this reason, some details or optimizations called out in
the RFC documents had cost or additional required complexity that outweighed the
added benefit they provided. As such, the following have not been fully
implemented:

* [Selective Instance Enumeration](https://tools.ietf.org/html/rfc6763#section-7.1)
is not supported.
* [Multiple SRV records](https://tools.ietf.org/html/rfc6763#section-5) for a
single service instance are not supported.
* [Multiple TXT records](https://tools.ietf.org/html/rfc6763#section-6.8) for a
single service instance are not supported.
* Hosts which publish [no TXT record](https://tools.ietf.org/html/rfc6763#section-6.1)
are treated as invalid. However, hosts which publish
[an empty TXT record](https://tools.ietf.org/html/rfc6763#section-6.1) are
valid.
* [Duplicate Question Suppression](https://tools.ietf.org/html/rfc6762#section-7.3)
has not been implemented.

Additionally, the following limitations should be noted:

* If network interface information is changed, the discovery stack must be
re-initialized.
* If a
[fatal error](https://chromium.googlesource.com/openscreen/+/refs/heads/master/discovery/common/reporting_client.h#25)
occurs, the discovery stack must be re-initialized.
