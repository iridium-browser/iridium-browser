---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: network-settings
title: Network Settings
---

[TOC]

### System network settings

The Chromium network stack uses the system network settings so that users and
administrators can control the network settings of all applications easily. The
network settings include:

*   proxy settings
*   SSL/TLS settings
*   certificate revocation check settings
*   certificate and private key stores

So far this design decision has worked well. The only network settings that some
users ask for an alternative to system settings are proxy settings. For this we
recently added some command-line options that allow you to run Chromium with
custom proxy settings.

### Preference service for network settings

Although the system network settings have been sufficient for our network stack,
eventually there will be some configuration settings specific to our network
stack, so we need to have our own preference service for those settings. See
also issue 266, in which some Firefox users demand that we not use the WinInet
proxy settings (the de facto system proxy settings on Windows).

### Command-line options for proxy settings

Chrome supports the following proxy-related command line arguments:

**--no-proxy-server**

This tells Chrome not to use a Proxy. It overrides any other proxy settings
provided.

**--proxy-auto-detect**

This tells Chrome to try and automatically detect your proxy configuration. This
flag is ignored if --proxy-server is also provided.

**--proxy-server=&lt;scheme&gt;=&lt;uri&gt;\[:&lt;port&gt;\]\[;...\] |
&lt;uri&gt;\[:&lt;port&gt;\] | "direct://"**

This tells Chrome to use a custom proxy configuration. You can specify a custom
proxy configuration in three ways:

1) By providing a semi-colon-separated mapping of list scheme to url/port pairs.

For example, you can specify:

--proxy-server="http=foopy:80;ftp=foopy2"

to use HTTP proxy "foopy:80" for http URLs and HTTP proxy "foopy2:80" for ftp
URLs.

2) By providing a single uri with optional port to use for all URLs.

For example:

--proxy-server="foopy:8080"

will use the proxy at foopy:8080 for all traffic.

3) By using the special "direct://" value.

--proxy-server="direct://" will cause all connections to not use a proxy.

**--proxy-bypass-list=(&lt;trailing_domain&gt;|&lt;ip-address&gt;)\[:&lt;port&gt;\]\[;...\]**

This tells chrome to bypass any specified proxy for the given
semi-colon-separated list of hosts. This flag must be used (or rather, only has
an effect) in tandem with --proxy-server.

Note that trailing-domain matching doesn't require "." separators so
"\*[google.com](http://google.com/)" will match
"[igoogle.com](http://igoogle.com/)" for example.

For example,

--proxy-server="foopy:8080"
--proxy-bypass-list="\*.[google.com](http://google.com/);\*[foo.com](http://foo.com/);[127.0.0.1:8080](http://127.0.0.1:8080/)"

will use the proxy server "foopy" on port 8080 for all hosts except those
pointing to \*.[google.com](http://google.com/), those pointing to
\*[foo.com](http://foo.com/) and those pointing to localhost on port 8080.

[igoogle.com](http://igoogle.com/) requests would still be proxied.
[ifoo.com](http://ifoo.com/) requests would not be proxied since \*foo, not
\*.foo was specified.

**--proxy-pac-url=&lt;pac-file-url&gt;**

This tells Chrome to use the PAC file at the specified URL.

For example,

--proxy-pac-url="<http://wpad/windows.pac>"

will tell Chrome to resolve proxy information for URL requests using the
windows.pac file.
