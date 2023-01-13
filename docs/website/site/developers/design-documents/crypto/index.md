---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: crypto
title: Crypto
---

&lt;&lt; UNDER CONSTRUCTION &gt;&gt;

This page covers crypto and related technologies such as SSL and certificates in
Chromium. Most of the code is in the "net" module, with some crypto classes in
the "base" module.

## Top priorities

1.  Port SSLClientSocketNSS to use native crypto APIs for SSL client
            authentication on [Mac OS
            X](http://code.google.com/p/chromium/issues/detail?id=45369) and
            [Windows](http://code.google.com/p/chromium/issues/detail?id=37560).
    *   Pending changelists:
                [4670004](http://codereview.chromium.org/4670004/).
    *   Remaining work:
        *   Remove support for ssl_PlatformAuthTokenPresent and make it
                    always return PR_TRUE.
        *   Generate an NSS patch.
2.  [Implement a password callback for NSS on
            Linux](http://code.google.com/p/chromium/issues/detail?id=42073).
            This allows us to protect the private keys in the NSS key database
            with a password, and support smart cards.
    *   Pending changelists:
                [5686002](http://codereview.chromium.org/5686002/)
3.  [Load the test root CA certificate temporarily on
            Windows](http://code.google.com/p/chromium/issues/detail?id=8470).
            This eliminates the need to install the test root CA certificate on
            Windows to run the SSL unit tests.
    *   Pending changelists:
                [4646001](http://codereview.chromium.org/4646001/).
4.  [Regenerate the root CA and test certificates to have a long
            validity
            period](http://code.google.com/p/chromium/issues/detail?id=5552).
    *   Pending changelists:
                [5535006](http://codereview.chromium.org/5535006/)
5.  [Cache certificate verification results in
            memory](http://code.google.com/p/chromium/issues/detail?id=63357).
    *   Pending changelists:
                [5386001](http://codereview.chromium.org/5386001/)
6.  Complete &lt;keygen&gt; implementation.
    *   [Support for intermediate CAs as part of
                application/x-x509-user-cert](http://code.google.com/p/chromium/issues/detail?id=37142).
    *   [Confirmation UI for importing
                certificates](http://code.google.com/p/chromium/issues/detail?id=65541).
    *   [Error UI for import
                failures](http://code.google.com/p/chromium/issues/detail?id=65543).
7.  [Cache complete certificate chains in the HTTP
            cache](http://code.google.com/p/chromium/issues/detail?id=7065).
    *   Pending changelists:
                [4645001](http://codereview.chromium.org/4645001/).

## Work plan

&lt;&lt; A nice dependency diagram to be added by Ryan Sleevi. &gt;&gt;

1.  [SSL client authentication](/system/errors/NodeNotFound)
    *   [Improve the SSL client authentication
                UI](http://code.google.com/p/chromium/issues/detail?id=29784).
    *   [Remember client certificate selection
                persistently](http://code.google.com/p/chromium/issues/detail?id=37314).
    *   [Support European national ID
                cards](http://code.google.com/p/chromium/issues/detail?id=44075).
    *   Improve test coverage.
2.  A strategy towards FIPS 140-2 compliance.
3.  Clean up the crypto classes/API in base. The main issue is to
            standardize on one or two ways to represent a data buffer.
4.  Combine regular certificate verification and EV certificate
            verification into one for NSS. Not sure if this is possible.
5.  Have the NSS CERT_PKIXVerifyCert function report all certificate
            errors using the cert_po_errorLog output parameter.
