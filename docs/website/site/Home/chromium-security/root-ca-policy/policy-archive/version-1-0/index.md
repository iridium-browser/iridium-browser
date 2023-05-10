---
breadcrumbs:
- - /Home
  - Chromium
- - /Home/chromium-security
  - Chromium Security
- - /Home/chromium-security/root-ca-policy
  - Root Program Policy
page_name: root-ca-policy-version-1-0
title: Chrome Root Program Policy Archive - Version 1.0
---

## Archive Notice 

<p><strong><span style="color:#FF0000">IMPORTANT:</span></strong> This page is
 retained for historical purposes only. 
 
Version 1.0 of the Chrome Root Program Policy was superseded by Version <a href=/Home/chromium-security/root-ca-policy/policy-archive/version-1-1/>1.1</a> on June 1, 2022.
 
For the latest version of the Chrome Root Program Policy, see <a href="https://g.co/chrome/root-policy">https://g.co/chrome/root-policy</a>.</p>

## Introduction

Google Chrome relies on Certification Authorities (CAs) to issue certificates to
websites. Chrome uses these certificates to ensure the HTTPS connections it
makes on behalf of its users are secure and private.

As part of establishing a secure connection, Chrome cryptographically verifies
that the website's certificate was issued by a recognized CA. Certificates that
are not issued by a CA recognized by Chrome, or by a user's local settings, can
cause users to see warnings and error pages.

If you’re an enterprise managing trusted CAs for your organization, including
locally installed enterprise CAs, the policies described in this document do not
apply to your CA. No changes are currently planned for how enterprise
administrators manage those CAs within Chrome. CAs that have been installed by
the device owner or administrator into the operating system trust store are
expected to continue to work as they do today.

If you’re a Chrome user experiencing a certificate error, please see [this
support article](https://support.google.com/chrome/answer/6098869?hl=en) for
more information.

If you're a website operator, you can learn more about [why HTTPS
matters](https://web.dev/why-https-matters/) and how to [secure your site with
HTTPS](https://support.google.com/webmasters/answer/6073543). If you've got a
question about a certificate you've been issued, including questions about
validity and revocation, please contact the CA that issued the certificate.

Though uncommon, certificates can also be used by websites to identify clients
connecting to them. Other than ensuring it is well formed, Chrome simply passes
the certificate to the server, which performs the evaluation and enforces its
chosen policy. The policies on this page do not apply to client certs.

## Chrome Root Program

When Chrome presents the connection to a website as secure, Chrome is making a
statement to its users about the security properties of that connection. Because
of the CA's critical role in upholding those properties, Chrome must ensure the
CAs who issue certificates are operated in a consistent and trustworthy manner.
This is achieved by referring to a list of root certificates from CAs that have
demonstrated why continued trust in them is justified. This list is referred to
as a Root Store. The policies and requirements for participating and being
included in a Root Store are known as a Root Program.

## Root Program Transition

The sections below describe the Chrome Root Program, and policies and
requirements for CAs to have their certificates included in a default
installation of Chrome, as part of the transition to the Chrome Root Store.

Historically, Chrome has integrated with the Root Store provided by the platform
on which it is running. Chrome is in the process of transitioning certificate
verification to use a common implementation on all platforms where it's under
application control, namely Android, Chrome OS, Linux, Windows, and macOS. Apple
policies prevent the Chrome Root Store and verifier from being used on Chrome
for iOS. This will ensure users have a consistent experience across platforms,
that developers have a consistent understanding of Chrome's behavior, and that
Chrome will be better able to protect the security and privacy of users'
connections to websites.

For CAs that already participate in other public Root Programs, such as the
[Mozilla Root
Program](https://www.mozilla.org/en-US/about/governance/policies/security-group/certs/),
many of these requirements and processes should be familiar.

### Transitional Root Store

During this transition, the Chrome Root Store contains a variety of existing
Certification Authorities' certificates that have historically worked in Chrome
on the majority of supported platforms. This promotes interoperability on
different devices and platforms, and minimizes compatibility issues. This should
ensure as seamless a transition as possible for users.

In addition to compatibility considerations, CAs have been selected on the basis
of past and current publicly available and verified information, such as that
within the Common CA Certificate Database ([CCADB](https://ccadb.org)). CCADB is
a datastore run by Mozilla and used by a variety of operating systems, browser
vendors, and Certification Authorities to share and disclose information
regarding the ownership, historical operation, and audit history of CA
certificates and key material.

### Requesting Inclusion

For Certification Authorities that have not been included as part of this
initial [Chrome Root Store](https://g.co/chrome/root-store), questions can be
directed to
[chrome-root-authority-program@google.com](mailto:chrome-root-authority-program@google.com).
Priority is given to CAs that are widely trusted on platforms that Chrome
supports, in order to minimize compatibility issues.

For the inclusion of new CA certificates, priority is given to CAs in the
following order, in order to minimize disruption or risks to Chrome users:

- CAs that are widely trusted, and which are replacing older certificates with certificates and key material created within the past five years, and have an unbroken sequence of annual audits where these certificates and key material are explicitly listed in scope.
- CAs whose certificates and certificate hierarchy are only used to issue TLS server certificates, and do not issue other forms of certificates.
- CAs that have undergone a widely-recognized public discussion process regarding their CP, CPS, audits, and practices. At this time, the only discussion process recognized as acceptable is the discussion process operated by Mozilla on behalf of the open-source community at [mozilla.dev.security.policy](https://www.mozilla.org/en-US/about/forums/#dev-security-policy).
- CAs that maintain sole control over all CA key material within their CA certificate hierarchy, and include their entire certificate hierarchy within a single audit scope.
- CAs that have been annually audited according to both of the “WebTrust Principles and Criteria for Certification Authorities” and the “WebTrust Principles and Criteria for Certification Authorities - SSL Baseline With Network Security”. Other audit criteria may be accepted on a discretionary basis, but Chrome will prioritize audits conducted according to both of these criteria.

Certification Authorities that do not meet all of the above criteria will be
dealt with on a case-by-case basis. Note that the above requirements are
illustrative only; Google includes CAs in its Root Program, and includes or
removes CA certificates within its Root Store as it deems appropriate for user
safety. The selection and ongoing membership of CAs is done to enhance the
security of Chrome and promote interoperability; CAs that do not provide a broad
service to all browser users are unlikely to be suitable.

As this transition occurs, CAs should continue to work with the relevant vendors
of operating systems where Chrome is supported to additionally request inclusion
within their root certificate programs as appropriate. This will help minimize
any disruption or incompatibilities for end users, by ensuring that Chrome is
able to validate certificates from the CA regardless of whether it is using the
Chrome Root Store or existing platform integrations.

The Chrome Root Store Policy will be updated to more fully detail the set of
formal ongoing requirements for working with Google in order to be distributed
and included in a default installation of Chrome, as well as additional steps
for applying or updating existing included certificates. Any questions regarding
this policy can be directed to
[chrome-root-authority-program@google.com](mailto:chrome-root-authority-program@google.com)

## Responding to Incidents

Chrome requires that CAs included in the Chrome Root Program abide by their
Certificate Policy and/or Certification Practices Statement, where the CA
describes how they operate, and must incorporate [CA/Browser Forum’s Baseline
Requirements](https://cabforum.org/baseline-requirements-documents/). Failure of
a CA to meet their commitments, as outlined in their CP/CPS and the Baseline
Requirements, is considered an incident, as is any other situation that may
impact the CA’s integrity, trustworthiness, or compatibility.

Chrome requires that any suspected or actual compliance incident be promptly
reported and publicly disclosed upon discovery by the CA, along with a timeline
for [an analysis of root
causes](https://landing.google.com/sre/sre-book/chapters/postmortem-culture/),
regardless of whether the non-compliance appears to the CA to have a serious or
immediate security impact on end users. Chrome will evaluate every compliance
incident on a case-by-case basis, and will work with the CA to identify
ecosystem-wide risks or potential improvements to be made in the CA’s operations
or in root program requirements that can help prevent future compliance
incidents.

When evaluating a CA’s incident response, Chrome’s primary concern is ensuring
that browsers, CAs, users, and website developers have the necessary information
to identify improvements, and that the CA is responsive to addressing identified
issues.

Factors that are significant to Chrome when evaluating incidents include (but
are not limited to): a demonstration of understanding of the root causes of an
incident, a substantive commitment and timeline to changes that clearly and
persuasively address the root cause, past history by the CA in its incident
handling and its follow through on commitments, and the severity of the security
impact of the incident. In general, a single compliance incident considered
alone is unlikely to result in removal of a CA from the Chrome Root Store.

Chrome expects CAs to be detailed, candid, timely, and transparent in describing
their architecture, implementation, operations, and external dependencies as
necessary for Chrome and the public to evaluate the nature of the incident and
depth of the CA’s response.

When a CA fails to meet their commitments made in their CP/CPS, Chrome expects
them to file an incident report. Due to the incorporation of the Baseline
Requirements into the CP and CPS, incidents may include a prescribed follow-up
action, such as revoking impacted certificates within a certain timeframe. If
the CA doesn’t perform the required follow-up actions, or doesn’t perform them
in the expected time, the CA should file a secondary incident report describing
any certificates involved, the CA’s expected timeline to complete any follow-up
actions, and what changes the CA is making to ensure they can meet these
requirements consistently in the future.

When a CA becomes aware of or suspects an incident, they should notify
[chrome-root-authority-program@google.com](mailto:chrome-root-authority-program@google.com)
with a description of the incident. If the CA has publicly disclosed this
incident, this notification should include a link to the disclosure. If the CA
has not yet disclosed this incident, this notification should include an initial
timeline for public disclosure. Chrome uses the information on the public
disclosure as the basis for evaluating incidents.
