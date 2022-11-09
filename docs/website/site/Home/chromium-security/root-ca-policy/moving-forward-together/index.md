---
breadcrumbs:
- - /Home
  - Chromium
- - /Home/chromium-security
  - Chromium Security
- - /Home/chromium-security/root-ca-policy
  - Root Program Policy
page_name: moving-forward-together
title: Moving Forward, Together
---

## Last updated: 2022-09-01

For more than the last decade, Web PKI community members have tirelessly worked together to make the Internet a safer place. However, there’s still more work to be done. While we don’t know exactly what the future looks like, we remain focused on promoting changes that increase speed, security, stability, and simplicity throughout the ecosystem.

With that in mind, the Chrome team continues to explore introducing future policy requirements related to the following initiatives:
- Encouraging modern infrastructures and agility
- Focusing on simplicity
- Promoting automation
- Reducing mis-issuance
- Increasing accountability and ecosystem integrity
- Streamlining and improving domain validation practices
- Preparing for a “post-quantum” world

We hope to make progress against many of these initiatives in future versions of our [policy](/Home/chromium-security/root-ca-policy) and welcome feedback on the proposals below at [chrome-root-program@google.com](mailto:chrome-root-program@google.com). We also intend to share CCADB surveys to more easily collect targeted CA owner feedback. We want to hear from CA owners about what challenges they anticipate with the proposed changes below and how we can help in addressing them.

## Encouraging modern infrastructures and agility
We think it’s time to revisit the notion that root CAs and their corresponding certificates should be trusted for 30+ years. While we do not intend to require a reduced root CA certificate validity period, we think it’s critically important to promote modern infrastructures by requiring operators to rotate aging root CAs with newer ones.

In a future policy update, we intend to introduce a maximum “term limit” for root CAs whose certificates are included in the Chrome Root Store. The proposed term duration is seven years, measured from the initial date of certificate inclusion. For CA certificates already included in the Chrome Root Store, the term would begin when the policy introducing the requirement took effect. CA owners would be encouraged to apply with a replacement CA certificate after five years of inclusion, which must contain a subject public key that the Chrome Root Store has not previously distributed. For compatibility reasons, CAs transitioning out of Chrome Root Store due to the term limit may issue a certificate to the replacement CA.

## Focusing on simplicity
One of the [10 things we know to be true](https://about.google/philosophy/) is that “it’s best to do one thing really, really well.” Though multipurpose root CAs have offered flexibility in addressing subscriber use cases over the last few decades, they are now faced with satisfying the demands of multiple sets of increasingly rigorous expectations and requirements that do not always align. We believe in the value of purpose-built, dedicated infrastructures and the increased opportunity for innovation that comes along with them. We also think there’s value in reducing the complexity of the Web PKI. By promoting dedicated-use hierarchies, we can better align the related policies and processes that CAs must conform to with the expectations of subscribers, relying parties, and root program operators.

The Chrome Root Program [policy](/Home/chromium-security/root-ca-policy) includes a requirement for all new applicant CA certificates to be part of dedicated TLS PKI hierarchies. We intend to identify a phase-out plan for existing multipurpose root CA certificates included in the Chrome Root Store in a future policy update. A future update may also modify the definition of a dedicated TLS PKI hierarchy to prohibit using the extendedKeyUsage value of id-kp-clientAuth in new subordinate CA and TLS certificates chaining to a root included in the Chrome Root Store.

## Promoting automation
One of the most prominent classes of incidents we continue to observe in the Web PKI is a failure to revoke certificates in a timely manner. Justification for the delays is often cited due to the inability of affected subscribers to obtain new certificates before the required revocation window. The most reliable approach to addressing this concern is to replace manual issuance with robust, standardized automated issuance processes such as Automatic Certificate Management Environment (ACME, [RFC 8555](https://datatracker.ietf.org/doc/html/rfc8555)).

In a future policy update, we intend to introduce a requirement that all new subordinate CAs established after a specific date must support ACME for certificate issuance and revocation.

## Increasing accountability and ecosystem integrity
We value high-quality, independent audit criteria that result in accurate, detailed, and consistent evaluations of CA owners’ policies and practices, regardless of their intended scope of certificate issuance or geographic location.

In a future policy update, we intend to introduce a requirement for all non-[TLS dedicated](/Home/chromium-security/root-ca-policy/#4-dedicated-tls-pki-hierarchies) Technically Constrained subordinate CAs chaining to a certificate included in the Chrome Root Store to provide an audit report that expresses non-performance of TLS certificate issuance and includes an evaluation of the Network and Certificate System Security Requirements.

