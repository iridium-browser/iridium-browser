---
breadcrumbs:
- - /developers
  - For Developers
page_name: md5-certificate-statistics
title: MD5 Certificate Statistics
---

Thursday, February 19, 2009

Last December, a group of security researchers published a practical attack on
CAs that issue certificates signed with MD5-based signatures
(<http://www.win.tue.nl/hashclash/rogue-ca/>). As a result, some browser
developers are planning to drop support of MD5 certificates at some point. The
exact time frame depends in part on how many HTTPS websites are still using MD5
certificates. Johnathan Nightingale of Mozilla crawled the top 1 million HTTPS
sites and published his findings
(<http://blog.johnath.com/2009/01/21/ssl-information-wants-to-be-free/>). With
Google Chrome's usage statistics collection service, we can help answer this
usage question by measuring how often Google Chrome users are encountering MD5
certificates. We'd like to share these measurements with other browser
developers.

The following statistics were gathered on our Dev channel releases, which are
weekly releases based on the tip of the Chromium source tree. Also note that
only Google Chrome users who opted in to usage statistics collection send their
numbers to us.

The counts shown below reflect periods of time on the order of a week, but
didn't seem to vary significantly across over a month of investigating. For each
kind of certificate, we collected two counts. One count reflects the number of
SSL connections made to sites using that kind of certificate, while the other
count reflects the number of Google Chrome sessions involving that kind of
certificate. The latter can provide an estimator of the percentage of users that
would be impacted by dropping MD5 certificate support. Note that we ignore the
root CA certificates because the signatures in the root CA certificates are
irrelevant to an attack of this sort.

1. The number of SSL connections with certain kinds of certificates. Note that
an HTTP keep-alive connection is counted once even though multiple HTTP requests
may be sent over it.

*   The number of all SSL connections: 16408141
*   The number of SSL connections with an MD5 (end entity or
            intermediate CA) certificate in the certificate chain: 365152
*   The number of SSL connections with an MD2 (end entity or
            intermediate CA) certificate in the certificate chain: 47
*   The number of SSL connections with an MD4 (end entity or
            intermediate CA) certificate in the certificate chain: 0
*   The number of SSL connections with an MD5 intermediate CA
            certificate in the certificate chain: 480
*   The number of SSL connections with an MD2 intermediate CA
            certificate in the certificate chain: 25

2. The number of Google Chrome sessions in which the user has seen certain kinds
of certificates.

*   The number of all Google Chrome sessions: 542846
*   The number of Google Chrome sessions that used SSL: 496090
*   The number of Google Chrome sessions that saw an MD5 (end entity or
            intermediate CA) certificate: 33134
*   The number of Google Chrome sessions that saw an MD2 (end entity or
            intermediate CA) certificate: 14
*   The number of Google Chrome sessions that saw an MD4 (end entity or
            intermediate CA) certificate: 0
*   The number of Google Chrome sessions that saw an MD5 intermediate CA
            certificate: 58
*   The number of Google Chrome sessions that saw an MD2 intermediate CA
            certificate: 10

The percentage of MD5 certificates of the secure Web, weighted by site
popularity and reported by users of Google Chrome's Dev channel releases who
opted in to usage statistics collection, is 365152/16408141 = 2.23%. Looking at
the per session statistics, we see that the number of user sessions that would
be impacted by no longer supporting MD5 certificates would be 33134/542846 =
6.1%. Either of these percentages, though significantly smaller than reported by
Nightingale, suggests that there would be a large user impact if MD5 certificate
support were suddenly removed.

A second alternative being considered is to remove support for MD5 intermediate
CA certificates. Although this would not completely preclude an attack, it would
significantly raise the cost of attacks. The percentage of MD5 intermediate CA
certificates is notably lower, at 480/16408141 &lt; 0.003%. Looking at a per
session statistics, we can see that only 58/542846 = .011% would be impacted by
such a change. As a result, it should be possible for browsers to drop support
of MD5 intermediate CA certificates sooner, which will make the MD5 certificate
attack much more expensive.

In the future we hope to use the statistics collection service to gather other
information of interest to crypto practitioners, such as which SSL cipher suites
are being used in practice and how many HTTPS sites use certificates with
certification path validation errors.

Posted by Wan-Teh Chang and Jim Roskind, Software Engineers
