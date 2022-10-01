---
breadcrumbs:
- - /Home
  - Chromium
- - /Home/chromium-security
  - Chromium Security
- - /Home/chromium-security/education
  - Education
page_name: tls
title: TLS / SSL
---

Data delivered over an unencrypted channel (e.g. HTTP) is insecure,
untrustworthy, and trivially intercepted. **TLS can help!**

[TOC]

## What's TLS?

TLS (also known as SSL) is the industry standard for providing communication
security over the Internet.

### **What security properties does TLS give me?**

TLS guarantees identification, confidentiality, and integrity between a client
(a computer) and a server.

*   Server identification means that the user is talking to the right
            server — i.e., your bank's server, and not someone on the network
            pretending to be your bank's server.
*   Confidentiality (via encryption) ensures that no one with access to
            the data going over the connection can understand the contents of
            the communication.
*   Integrity means that no one can tamper with the data in transit.

In other words, TLS ensures that a [Man-in-the-Middle
(MitM)](/Home/chromium-security/education/tls) can't snoop or tamper with an
Internet connection between a user and website. A [man-in-the-middle
(MiTM)](http://en.wikipedia.org/wiki/Man-in-the-middle_attack) is a term used to
describe a third party that can passively monitor and/or actively tamper with a
connection between two unknowing parties. A MiTM attacker relays messages
between two parties, making them believe that they are talking directly to each
other, when in fact the entire conversation is controlled by the attacker.

MiTM attacks happen in real life! Here are some recent examples:

*   Opportunistic attacks on wifi networks are easy to conduct via
            freely available tools like
            [sslstrip](http://www.thoughtcrime.org/software/sslstrip/) or
            [firesheep](http://codebutler.com/firesheep/)
*   [NSA MiTM monitoring](https://www.eff.org/nsa-spying) of email,
            chat, and other Internet communication
*   [ISPs collecting
            data](http://www.dslreports.com/shownews/Bell-Dramatically-Ramps-Up-Consumer-Data-Collection-Efforts-126375)
            via MiTM sniffing for marketing purposes
*   [ISPs modifying web
            pages](http://arstechnica.com/tech-policy/2013/04/how-a-banner-ad-for-hs-ok/)
            (adding ads) via MiTM tampering
*   Chinese hackers [targeting
            github](https://en.greatfire.org/blog/2013/jan/china-github-and-man-middle)

### **What security properties does TLS *not* give me?**

TLS only protects the connection between your computer and the server. It does
not protect data on the client or data on the server. This means:

*   If malware is installed on your computer, it will be able to see and
            modify your web traffic.
*   If your system administrator has installed local trust anchors or a
            local proxy (for example, on a company computer), then the system
            administrator may be able to see and modify your web traffic.
*   If malware is installed on the server, your data on the server may
            be at risk.
*   TLS does not stop compromised or rogue servers from trying to
            install malware on your computer. Instead, [Google Safe
            Browsing](https://www.google.com/transparencyreport/safebrowsing/)
            scans websites and files for signs of malware. If Google Safe
            Browsing flags a website or file as malicious, you will see a
            separate malware warning for the website or file. This is unrelated
            to TLS.

## **TLS in Chrome**

### **HTTP Strict Transport Security (HSTS)**

HSTS is a mechanism enabling web sites to declare themselves accessible only via
secure connections and/or for users to be able to direct their user agent to
interact with given sites only over secure connections. Chrome supports HSTS and
comes preloaded with a set of domains that use HSTS by default. More details,
including how to add a site to Chrome's preloaded HSTS list, [here](/hsts).

### **Certificates**

TLS relies on websites serving authenticated (X.509) certificates to prove their
identities, which prevents an attacker from pretending to be the website.
Certificates bind a public key and an identity (commonly a DNS name) together
and are typically issued for a period of several years. Ensure that your CA
gives you a SHA-256 certificate, as SHA-1 certificates are deprecated (see
below).

#### **Certificate Pinning**

Chrome has HTTPS "pins" for most Google properties — i.e. certificate chains for
Google properties must have a explicitly listed public key, or it will result in
a fatal error. This feature helped Google [detect a widespread MITM attack to
Gmail users in
2011](http://googleonlinesecurity.blogspot.com/2011/08/update-on-attempted-man-in-middle.html).
You can read more about pinning
[here](https://www.imperialviolet.org/2011/05/04/pinning.html). There's also an
[Internet-Draft for HTTP-based public key
pinning](http://tools.ietf.org/html/draft-ietf-websec-key-pinning).

#### **Certificate Revocation**

Sometimes events occur that invalidate the binding of public key and name, and
the certificate needs to be revoked. For example, a [major flaw in the
implementation of OpenSSL](http://heartbleed.com/) left site operators' private
key vulnerable to theft, so operators needed to invalidate their certificates.
Revocation is the process of invalidating a certificate before its expiry date.
Chrome uses [CRLSets](https://www.imperialviolet.org/2012/02/05/crlsets.html) to
implement certificate revocation. You can read about the how and why of Chrome's
certificate revocation in our [Security
FAQ](/Home/chromium-security/security-faq).

#### **Certificate Errors**

If there is an error in the certificate, Chrome can’t distinguish between a
configuration bug in the site and a real MiTM attack, so Chrome takes proactive
steps to protect users.

If a site has elected to use HSTS, all certificate errors are fatal. Certificate
pinning errors are also fatal. Otherwise, users are shown a full-screen warning
interstitial they can elect to bypass.

### Cipher Suites

TLS connections negotiate a *cipher suite* which determines how data is
encrypted and authenticated. Server products typically leave configuring this to
the administrator. Many cipher suites available in TLS are obsolete and, while
currently supported by Chrome, are not recommended. If an obsolete cipher suite
is used, Chrome may display this message when clicking the lock icon:

> *“Your connection to example.com is encrypted with obsolete cryptography.”*

To avoid this message, use TLS 1.2 and prioritize an ECDHE cipher suite with
AES_128_GCM or CHACHA20_POLY1305. Most servers will wish to negotiate
TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256.

### **Deprecated and Removed Features**

#### SHA-1 Certificate Signatures

You may see this message when you click on the lock icon in Chrome:

> *"This site uses a weak security configuration (SHA-1 signatures), so your
> connection may not be private."*

This means that the certificate chain for the current page is contains a
certificate using a SHA-1-based signature, which is
[outdated](https://www.schneier.com/blog/archives/2012/10/when_will_we_se.html)
and [deprecated in
Chrome](https://blog.chromium.org/2014/09/gradually-sunsetting-sha-1.html).
There are two criteria that determine which lock icon is shown in Chrome:

*   the **expiration date of the leaf certificate** and
*   whether there is a **SHA-1-based signature in the certificate
            chain** (leaf certificate OR intermediate certificate).

Note that the expiration dates of the intermediate certificate do not matter.
Also, SHA-1-based signatures for root certificates are not a problem because
Chrome trusts them by their identity, rather than by the signature of their
hash.
Starting in Chrome 42, the following logic applies:

*   If a leaf certificate **expires in 2016** and the chain **contains a
            SHA-1 signature**, the page will be marked as "secure, but with
            minor errors" (yellow icon).
*   If a leaf certificate **expires in 2017 or later** and the chain
            **contains a SHA-1 signature**, the page will be treated as
            "affirmatively insecure" (red icon).

Note that this use of SHA-1 is not related to TLS message authentication (*“The
connection is using \[cipher\], with HMAC-SHA1 for message authentication.”*).

Please see [this page](/Home/chromium-security/education/tls/sha-1) for more
details on deprecation dates and private enterprise PKIs.

#### Weak Ephemeral Diffie-Hellman Key Exchange (ERR_SSL_WEAK_SERVER_EPHEMERAL_DH_KEY)

Chrome requires a minimum DHE group size of 1024-bits. See [this
announcement](https://groups.google.com/a/chromium.org/forum/#!topic/security-dev/WyGIpevBV1s)
and [this
page](https://support.google.com/chrome/answer/6098869?p=dh_error&rd=1#DHkey)
for more details. Affected sites will fail to load with
ERR_SSL_WEAK_SERVER_EPHEMERAL_DH_KEY.

#### SSLv3

SSLv3 is no longer supported in Chrome. See [this
announcement](https://groups.google.com/a/chromium.org/forum/#!topic/security-dev/Vnhy9aKM_l4)
for more details.

#### RC4

Chrome will remove support for the RC4 cipher in a future release around January
or February 2016. Server operations should tweak their configuration to support
other cipher suites. If available, TLS 1.2 with
TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256 is recommended. See [this
announcement](https://groups.google.com/a/chromium.org/forum/#!topic/security-dev/kVfCywocUO8)
for more details.

#### Insecure Version Fallback (ERR_SSL_FALLBACK_BEYOND_MINIMUM_VERSION)

Historically, some buggy servers have required an insecure version fallback to
function. This has been partially removed in Chrome and will be fully removed at
a later date. See [this
announcement](https://groups.google.com/a/chromium.org/forum/#!msg/security-dev/F6ZjP6FnyRE/bK7TKtvnHYsJ)
for more details. Affected sites will fail to load with
ERR_SSL_FALLBACK_BEYOND_MINIMUM_VERSION.

### **TLS Resources for Developers and Site Operators**

#### TLS Myths

#### **The only security guarantee TLS provides is confidentiality.**

When properly deployed, TLS provides three guarantees: **authentication of the
server**, **data integrity** (tamper-evidence), and **data confidentiality**.
People often think TLS and HTTPS only apply in threat scenarios where data
confidentiality is needed, but in fact they apply when any (or, most often, all
3) guarantees are beneficial.

*   Authentication and integrity: The authors and readers of a news site
            want the news to be the true news that the authors intended. (See
            [the *New York Times*’ statement about
            this](http://open.blogs.nytimes.com/2014/11/13/embracing-https/).)
*   Authentication and integrity: The users of a financial information
            site very much need the facts and figures to be true.
*   Confidentiality: You could be just browsing the web, and a pervasive
            passive monitor could use this to build a profile of you, to track
            your related experiences on a variety of sites, to cross-reference
            your interactions, and then declare you a “threat” for otherwise
            benign interactions.
*   Confidentiality: You might be reading an article on reproductive
            health or religious beliefs that are contrary to local norms.
            Revealing this information could get you in “trouble”, for some
            definition of trouble.
*   Integrity: You could be reading a website supported by advertising,
            but that advertising might be rewritten to credit the attacker,
            rather than the site you're reading. Over time, the site you're
            reading may need to shut down, because all of their revenue has been
            stolen by attackers.
*   Integrity: You could be reading a blog, but which an attacker
            changes the content to suggest the blogger is endorsing or holding
            views contrary to what they really hold.

**My site doesn't need TLS. I'm not a bank.**

More people are connected to the web than ever before and from more places and
more devices (laptops, phones, tablets, and [other
things](http://en.wikipedia.org/wiki/Internet_of_Things)). Very often, this
access is over untrusted or hostile networks. Data delivered over a clear text
protocol, like HTTP, is insecure, untrustworthy, and trivially intercepted.
Neither the user / user-agent nor the web server / application can trust that
the data was not tampered with or snooped by some third party - that's a
terrible situation for both users and web site operators!

With so much of people's lives moving online, it’s imperative developers take
steps to protect their sites' and users' data, which can even include the *mere
usage* of a web site. By analyzing and correlating the sites and pages a user
visits, observers like schools, ISPs, and governments can learn quite a bit
about a user that the user would wish to keep confidential, such as a users'
sexual orientation
(<http://blogs.wsj.com/digits/2010/03/12/ftcs-privacy-worries-prompt-netflix-to-cancel-contest/>)
or physical location
(<http://www.theregister.co.uk/2009/05/21/geo_location_data/>).

#### **TLS is too slow.**

Historically, TLS used to have a significant performance on web applications.
So, [istlsfastyet.com](http://istlsfastyet.com/)? (Spoiler: Yes!) Check out
<https://istlsfastyet.com/> for more details and a performance checklist.

#### TLS is too expensive.

[The Let’s Encrypt project](https://letsencrypt.org/) offers free certificates.

[SSLs.com offers certificates for a very low price](https://www.ssls.com/), as
low as $5.

[SSLmate.com is cheap and easy to use](https://sslmate.com/) — you can buy
certificates from the command line.

#### TLS is a privacy / security silver bullet.

TLS does ==not== guarantee perfect privacy or solve all security problems.

For example, when used to secure HTTP traffic (i.e. HTTPS), we’re piggybacking
HTTP entirely on top of TLS. This means the entirety of the HTTP protocol can be
encrypted (request URL, query parameters, headers, and cookies), however,
because host IP addresses and port numbers are necessarily part of the
underlying TCP/IP protocols, a third party can still infer these. Also, while
you can’t infer the contents of the communication, you can infer the amount and
duration of the communication. For specific applications, it’s been demonstrated
that this can leak useful information for an attacker, and services have added
padding to counter the timing or pattern analysis.

The identity of the site you are visiting is still (unfortunately) pretty
visible to passive eavesdroppers. For example, the IP addresses of client and
server are shown in the clear on the network, and the hostname(s) of the sites
you are visiting are transmitted in the clear in DNS requests, in the [Server
Name Indication portion of a TLS
handshake](http://en.wikipedia.org/wiki/Server_Name_Indication), and in the
server's certificate(s).
Also, since TLS is a transport protocol, attacks at other layers of the network
stack remain. In particular, IP-level threats (e.g. spoofing, SYD floods, DDoS
by data flood) are not protected and TLS doesn’t address common web application
vulnerabilities, like cross-site scripting or cross-site request forgery.

### Common Pitfalls

#### Deploying TLS

SSL Labs puts out a great [Deployment Best Practice
Guide](https://github.com/ssllabs/research/wiki/SSL-and-TLS-Deployment-Best-Practices)
that should help site operators avoid the most common deployment mistakes. You
can also test your setup via <https://www.ssllabs.com/ssltest/>.

#### Mixed Content (HTTP / HTTPS) Vulnerabilities

A mixed content vulnerability refers to a page served over HTTPS that includes
content served over HTTP, making the page vulnerable to MitM attacks. This is
especially problematic when the HTTP resources are active content (e.g.
Javascript, plug-in content, CSS, or iframes). To protect users, Chrome will
block mixed-content iframes, Javascript, CSS and plug-in loads by default. So
beyond the security risk, mixed content bugs may degrade your page for users in
unintended ways.
To fix mixed content issues, make sure that all the resources loaded by an HTTPS
page are also sent over HTTPS. If the resources are available on the same
domain, you can use hostname-relative URLs (e.g. &lt;img
src="something.png"&gt;). You can also use scheme-relative URLs (e.g. &lt;img
src="//example.com/something.png"&gt;) — the browser will use the same scheme as
the enclosing page to load these subresources. If the server does not serve
these resources over HTTPS, you may have to serve them from elsewhere or enable
HTTPS on that server.

You may also want to consider the
[upgrade-insecure-requests](http://www.w3.org/TR/upgrade-insecure-requests/) CSP
directive.
