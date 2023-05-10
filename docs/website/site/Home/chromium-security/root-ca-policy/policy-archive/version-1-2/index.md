---
breadcrumbs:
- - /Home
  - Chromium
- - /Home/chromium-security
  - Chromium Security
- - /Home/chromium-security/root-ca-policy
  - Root Program Policy
page_name: root-ca-policy
title: Chrome Root Program Policy Archive - Version 1.2
---

## Archive Notice 

<p><strong><span style="color:#FF0000">IMPORTANT:</span></strong> This page is
 retained for historical purposes only. 
 
Version 1.2 of the Chrome Root Program Policy was superseded by Version <a href=/Home/chromium-security/root-ca-policy/policy-archive/version-1-3/>1.3</a> on January 6, 2023.
 
For the latest version of the Chrome Root Program Policy, see <a href="https://g.co/chrome/root-policy">https://g.co/chrome/root-policy</a>.</p>

## Introduction
Google Chrome relies on Certification Authority systems (herein referred to as “CAs”) to issue certificates to websites. Chrome uses these certificates to help ensure the connections it makes on behalf of its users are properly secured. Chrome accomplishes this by verifying that a website’s certificate was issued by a recognized CA, while also performing additional evaluations of the HTTPS connection's security properties. Certificates that are not issued by a CA recognized by Chrome, or by a user's local settings, can cause users to see warnings and error pages.

Because of the CA's critical role in facilitating secure HTTPS connections, Chrome must ensure the CAs who issue certificates are operated in a consistent and trustworthy manner. Chrome refers to a list of root certificates from CAs that have demonstrated why continued trust in them is justified. This list is known as a “Root Store”. 

Historically, Chrome has integrated with the Root Store provided by the platform on which it is running. Beginning in Chrome 105, Chrome will begin a platform-by-platform transition from relying on the host operating system’s Root Store to its own on Android, Chrome OS, Linux, Windows, and macOS. Apple policies prevent the Chrome Root Store and corresponding Chrome Certificate Verifier from being used on Chrome for iOS. This change will allow Chrome to become more secure and promote consistent user and developer experiences across platforms. This page will be updated in the future with the specific order and timing of the transition to the Chrome Root Store. 

The sections below describe the Chrome Root Program: the policies and requirements for CAs to have their certificates included in a default installation of Chrome, and outlines the initial transition to the Chrome Root Store.

For CA operators that already participate in other public Root Programs, such as the [Mozilla Root Program](https://www.mozilla.org/en-US/about/governance/policies/security-group/certs/), many of these requirements and processes should be familiar. 

### Additional Information
If you’re a Chrome user experiencing a certificate error and need help, please see [this support article](https://support.google.com/chrome/answer/6098869?hl=en).

If you’re a website operator, you can learn more about [why HTTPS matters](https://web.dev/why-https-matters/) and how to [secure your site with HTTPS](https://support.google.com/webmasters/answer/6073543). If you’ve got a question about a certificate you’ve been issued, please contact the CA operator that issued it.

If you're responsible for a CA that only issues certificates to your enterprise organization, sometimes called a "private" or "locally trusted" CA, the Chrome Root Program policy does not apply to or impact your organization’s use cases. Enterprise CAs are intended for use cases exclusively internal to an organization (e.g., a TLS certificate issued to a corporate intranet site). 

Though uncommon, websites can also use certificates to identify clients (e.g., users) connecting to them. Besides ensuring it is well-formed, Chrome passes this type of certificate to the server, which then evaluates and enforces its chosen policy. The policies on this page do not apply to client authentication certificates.

### Chrome Root Store
The initial [Chrome Root Store](https://g.co/chrome/root-store) contains a variety of existing CA certificates that have historically been distributed as trusted on the majority of Chrome’s supported platforms. This promotes interoperability on different devices and platforms, and minimizes compatibility issues as the transition to the Chrome Root Store takes place. This should ensure as seamless a transition as possible for users.

In addition to compatibility considerations, CA certificates have been selected on the basis of past and current publicly available and verified information, such as that within the Common CA Database ([CCADB](https://ccadb.org)). CCADB is a datastore run by Mozilla and used by a variety of operating systems, browser vendors, and CA operators to share and disclose information regarding the ownership, historical operation, and audit history of CAs and corresponding certificates and key material.

### Requesting Inclusion
For CA certificates that have not been included in the initial [Chrome Root Store](https://g.co/chrome/root-store) that will be used during the rollout, questions can be directed to [chrome-root-program@google.com](mailto:chrome-root-program@google.com). 

The Chrome Root Program and corresponding Root Store will process applications and requests for changes through CCADB. This integration is ongoing, and is expected to be completed before September 2022. Before accepting applications, this policy will be updated with the steps for applying or updating existing included certificates, and CA operators will be contacted with more information using a CCADB message. 

Much of the application process and information requested will be familiar to CA operators that participate in other public Root Programs; for example CA owner and operating organization details, and artifacts related to the CA and its operator’s policies, practices, and audit history. The Chrome Root Program application will require additional information, such as (but not limited to):
- an architectural overview of the applicant PKI.
- the dates and related audit artifacts pertaining to CA key material generation.
- the collection of domain validation methods used during TLS certificate issuance.
- the collection of a self-assessment concerning compliance with this policy and the Baseline Requirements.

Until CCADB integration is complete, the Chrome Root Store may be updated at Google’s discretion to add CA certificates that satisfy the below criteria:

- CA certificates that are widely trusted on Chrome’s existing platforms, and which are replacing older certificates with certificates and key material created within the past five years, and have an unbroken sequence of annual audits where these certificates and key material are explicitly listed in scope.
- CAs whose corresponding certificate hierarchy is used to issue TLS server certificates, and do not issue other forms of subscriber certificates.
- CAs and their corresponding operators that have undergone a widely-recognized public discussion process regarding their Certificate Policy (CP), Certification Practice Statement (CPS), audits, and practices. At this time, the only discussion process that meets these requirements is the one run by Mozilla on behalf of the open-source community at [mozilla.dev.security.policy](https://www.mozilla.org/en-US/about/forums/#dev-security-policy).
- CAs whose operators maintain sole control over all CA key material within their CA certificate hierarchy, and include their entire certificate hierarchy within a single audit scope.
- CAs and their corresponding operators that have been annually audited according to both of the “WebTrust Principles and Criteria for Certification Authorities” and the “WebTrust Principles and Criteria for Certification Authorities - SSL Baseline With Network Security”. Other audit criteria may be accepted on a discretionary basis, but Chrome will prioritize audits conducted according to both of these criteria.

Note that the above requirements are illustrative only; Google includes or removes CA certificates within its Root Store as it deems appropriate for user safety. The selection and ongoing inclusion of CA certificates is done to enhance the security of Chrome and promote interoperability; CA certificates that do not provide a broad service to all browser users will not be added to the Chrome Root Store.

As the transition to the Chrome Root Store occurs, CA operators should continue to work with the relevant vendors of operating systems where Chrome is supported to also request certificate inclusion within their root programs as appropriate. This will help minimize any disruption or incompatibilities for end users, by ensuring that Chrome can validate certificates from the CA regardless of whether it is using the Chrome Root Store or existing platform integrations.

Any questions regarding this policy can be directed to [chrome-root-program@google.com](mailto:chrome-root-program@google.com). 

## Minimum Requirements for CA Operators
CA operators with certificates included in the Chrome Root Store must satisfy the requirements defined in this policy, including taking responsibility for ensuring the continued compliance of all corresponding subordinate CAs and third parties participating in the Public Key Infrastructure (PKI).

A CA operator’s failure to follow the requirements defined in this policy may result in a CA certificate’s removal from the Chrome Root Store, limitations on Chrome's acceptance of the certificates they issue, or other technical or policy restrictions.

### 1. Baseline Requirements
CA operators with certificates included in the Chrome Root Store are expected to adhere to the latest version of the CA/Browser Forum [“Baseline Requirements for the Issuance and Management of Publicly-Trusted Certificates”](https://cabforum.org/baseline-requirements-documents/) (“Baseline Requirements”).

### 2. CA Operator Policies
CA operators must sufficiently describe the policies and practices of their CA(s) within a Certificate Policy (CP) and corresponding Certification Practice Statement (CPS). These documents must provide sufficient detail to assess the operations of the CA(s) and the compliance with these expectations and those of the Baseline Requirements, and must not conflict with either of these requirements. CA operators may elect to combine their CP and CPS into a single document as a CP/CPS. These documents must be freely publicly available for examination, and include at least an authoritative English language version.

CA operators must upload updated versions of a CA’s CP, CPS, or CP/CPS to its own publicly accessible policy repository and the CCADB within 7 days of being updated. 

### 3. Modern Infrastructures
As part of Chrome's commitment to promoting modern infrastructures and cryptographic agility, effective September 1, 2022:
- An applicant root CA certificate’s corresponding key material must have been generated within five years from application to the Chrome Root Store. 

### 4. Dedicated TLS PKI Hierarchies
A dedicated PKI hierarchy is intended to serve one specific use case, for example, the issuance of TLS certificates. 

To qualify as a dedicated TLS PKI hierarchy under this policy:

1. All corresponding subordinate CA certificates operated beneath a root CA must:
    - <u>include</u> the extendedKeyUsage extension and <u>only</u> assert an extendedKeyUsage purpose of either:
        1. id-kp-serverAuth, or 
        2. id-kp-serverAuth and id-kp-clientAuth
    - not contain a public key corresponding to any other unexpired or non-revoked certificate that asserts different extendedKeyUsage values.
<br><br>
2. All corresponding subscriber certificates must:
    - <u>include</u> the extendedKeyUsage extension and <u>only</u> assert an extendedKeyUsage purpose of either:
        1. id-kp-serverAuth, or 
        2. id-kp-serverAuth and id-kp-clientAuth

Effective September 1, 2022, the Chrome Root Store will only accept applicant root CA certificates that are part of PKI hierarchies dedicated to TLS certificate issuance. 

It is expected that a future version of this policy will identify a phase-out plan for existing root CA certificates included in the Chrome Root Store that do not satisfy the requirements above to align all included CAs on these principles.

### 5. Audits
Root CAs with certificates included in the Chrome Root Store and all corresponding subordinate CAs must be audited according to an approved audit scheme.

#### Reporting Requirements
Reports for both annual and ad-hoc audits, described below, must satisfy the requirements in Section 8.6 of the Baseline Requirements and be uploaded to the CCADB within 90 days from the ending date specified in the letter. See [https://ccadb.org/cas/updates](https://ccadb.org/cas/updates) for instructions on uploading audit reports and [https://www.ccadb.org/policy#51-audit-statement-content](https://www.ccadb.org/policy#51-audit-statement-content) for formatting requirements.

#### Annual Audit Requirements
CAs must retain an unbroken, contiguous audit coverage. 

Recurring complete (i.e., “full”, “full system” or “full re-assessment”) annual audits must begin once a CA’s key material has been established and continue until the corresponding root CA’s key material has been destroyed or is no longer included in the Chrome Root Store. 

##### Accepted Annual Audit Criteria
The following table presents the annual audit criteria accepted by the Chrome Root Program. The Chrome Root Program considers a subordinate CA as “Technically Constrained” (“Constrained”, below) if it satisfies the criteria defined in Section 7.1.5 of the Baseline Requirements. 

<style type="text/css">
.tg  {border-collapse:collapse;border-spacing:0;}
.tg td{border-color:black;border-style:solid;border-width:1px;overflow:hidden;padding:10px 5px;word-break:normal;}
.tg th{border-color:black;border-style:solid;border-width:1px;font-weight:normal;overflow:hidden;padding:10px 5px;word-break:normal;}
.tg .tg-h1-r{background-color:#efefef;text-align:center;vertical-align:top}
.tg .tg-h1-l{background-color:#efefef;text-align:center;vertical-align:middle}
.tg .tg-h2{background-color:#efefef;font-weight:bold;text-align:center;vertical-align:top}
.tg .tg-h3{text-align:left;vertical-align:top}
.tg .tg-req{text-align:center;vertical-align:top}
</style>
<table class="tg">
<thead>
  <tr>
    <th class="tg-h1-l" rowspan="2"><span style="font-weight:bold">Audit Criteria</span></th>
    <th class="tg-h1-r" colspan="3"><span style="font-weight:700;font-style:normal;text-decoration:none">Certification Authority Type</span></th>
  </tr>
  <tr>
    <th class="tg-h2">Root</th>
    <th class="tg-h2">Constrained</th>
    <th class="tg-h2">All others</th>
  </tr>
</thead>
<tbody>
  <tr>
    <td class="tg-h3"><span style="font-weight:400;font-style:normal;text-decoration:underline">WebTrust</span><br><span style="font-weight:400;font-style:normal;text-decoration:none"><li> WebTrust Principles and Criteria for Certification Authorities; <b>and</b></li><li> WebTrust Principles and Criteria for Certification Authorities – SSL Baseline with Network Security</li></span></td>
    <td class="tg-req">Yes</td>
    <td class="tg-req">Yes</td>
    <td class="tg-req">Yes</td>
  </tr>
  <tr>
    <td class="tg-h3"><span style="font-weight:400;font-style:normal;text-decoration:underline">ETSI </span><br><span style="font-weight:400;font-style:normal;text-decoration:none"><li>ETSI EN 319 411-1 LCP and [DVCP or OVCP]; <b>or</b></li><li> ETSI EN 319 411-1 [NCP or NCP+] and EVCP</li></span></td>
    <td class="tg-req">Yes*</td>
    <td class="tg-req">Yes*</td>
    <td class="tg-req">Yes*</td>
  </tr>
  <tr>
    <td class="tg-h3"><span style="font-weight:400;font-style:normal;text-decoration:underline">Self-Audits </span><br><span style="font-weight:400;font-style:normal;text-decoration:none"><li> As permitted in Section 8.7 of the Baseline Requirements</li></span></td>
    <td class="tg-req">Not Permitted</td>
    <td class="tg-req">Yes*</td>
    <td class="tg-req">Not Permitted</td>
  </tr>
</tbody>
</table>

\* accepted on a discretionary basis

The Chrome Root Program gives priority to WebTrust audits.

It is expected that a future version of this policy will prohibit use of self-audits for Technically Constrained subordinate CAs.

#### Ad-Hoc Audit Requirements
Root CA certificate key material generation must be observed and audited by an independent third party auditor as described and required by Section 6.1.1.1 of the Baseline Requirements. The corresponding audit report is required as part of the application process to the Chrome Root Store.

When deemed necessary, the Chrome Root Program may require CAs and corresponding CA operators to undergo additional ad-hoc audits, including, but not limited to, instances of CA private key destruction or verification of incident remediation.

#### Annual Self-Assessments
Beginning in 2023, on an annual basis, CA operators with certificates included in the Chrome Root Store must complete and submit to the CCADB a self-assessment evaluating the conformance of their policies and practices against this policy and the Baseline Requirements for:
- each root CA certificate included in the Chrome Root Store.
- any corresponding subordinate CA certificate technically capable of issuing TLS certificates (i.e., explicitly contains an extendedKeyUsage value of id-kp-serverAuth or the functional equivalent, for example, due to the absence of any extendedKeyUsage values).

A single self-assessment may cover multiple CAs operating under both the same CP and CPS, or combined CP/CPS. If a consolidated self-assessment is submitted, the CA operator must enumerate the CAs in the scope of the assessment on the provided cover sheet.

Completed self-assessments must be submitted to the CCADB within 90 days from the “BR Audit Period End Date” field specified in the root CA’s “CA Owner/Certificate” CCADB record.

The self-assessment template will be added to this policy before September 2022, and is expected to closely align with the [self-assessment](https://docs.google.com/spreadsheets/d/1ExZE6PWIBM8rV9c6p6fFxOWmZyvf6X4ucMQRv7usHEk/edit#gid=0) required by the Mozilla Root Program’s application process.

### 6. Responding to Incidents
Chrome requires that CAs with certificates included in the Chrome Root Store abide by their Certificate Policy and/or Certification Practices Statement, where the CA operator describes how the CA is operated, and must incorporate the Baseline Requirements. Failure of a CA operator to meet their commitments, as outlined in the CA’s CP/CPS and the Baseline Requirements, is considered an incident, as is any other situation that may impact the CA’s integrity, trustworthiness, or compatibility.

Chrome requires that any suspected or actual compliance incident be promptly reported and publicly disclosed upon discovery by the CA operator, along with a timeline for [an analysis of root causes](https://sre.google/sre-book/postmortem-culture/), regardless of whether the non-compliance appears to the CA operator to have a serious or immediate security impact on end users. Chrome will evaluate every compliance incident on a case-by-case basis, and will work with the CA operator to identify ecosystem-wide risks or potential improvements to be made in the CA’s operations or in root program requirements that can help prevent future compliance incidents.

When evaluating a CA operator’s incident response, Chrome’s primary concern is ensuring that browsers, other CA operators, users, and website developers have the necessary information to identify improvements, and that the CA operator is responsive to addressing identified issues.

Factors that are significant to Chrome when evaluating incidents include (but are not limited to): a demonstration of understanding of the root causes of an incident, a substantive commitment and timeline to changes that clearly and persuasively address the root cause, past history by the CA operator in its incident handling and its follow through on commitments, and the severity of the security impact of the incident. In general, a single compliance incident considered alone is unlikely to result in removal of a CA certificate from the Chrome Root Store.

Chrome expects CA operators to be detailed, candid, timely, and transparent in describing their architecture, implementation, operations, and external dependencies as necessary for Chrome and the public to evaluate the nature of the incident and depth of the CA operator’s response.

When a CA operator fails to meet the commitments made in their CP/CPS, Chrome expects them to file an incident report. Due to the incorporation of the Baseline Requirements into the CP and CPS, incidents may include a prescribed follow-up action, such as revoking impacted certificates within a certain timeframe. If the CA operator doesn’t perform the required follow-up actions, or doesn’t perform them in the expected time, the CA operator should file a secondary incident report describing any certificates involved, the CA operator’s expected timeline to complete any follow-up actions, and what changes the CA operator is making to ensure they can meet these requirements consistently in the future.

When a CA operator becomes aware of or suspects an incident, they should notify [chrome-root-program@google.com](mailto:chrome-root-program@google.com) with a description of the incident. If the CA operator has publicly disclosed this incident, this notification should include a link to the disclosure. If the CA operator has not yet disclosed this incident, this notification should include an initial timeline for public disclosure. Chrome uses the information on the public disclosure as the basis for evaluating incidents.

### 7. Common CA Database
CA operators must follow the requirements defined in the [CCADB Policy](https://www.ccadb.org/policy). In instances where the CCADB Policy conflicts with this policy, this policy must take precedence.

Minimally, CA operators must ensure information stored in the CCADB is reviewed monthly and updated as needed.

### 8. Timely and Transparent Communications
At any time, the Chrome Root Program may request additional information from a CA operator using email or CCADB communications to verify the commitments and obligations outlined in this policy are being met. CA operators must provide the requested information within 10 business days unless specified otherwise. 

#### Notification of Cross-Certificate Issuance
CA operators must notify [chrome-root-program@google.com](mailto:chrome-root-program@google.com) at least 3 weeks before issuing a certificate to any CA owned or operated by an external organization that can validate to a corresponding root CA certificate included in the Chrome Root Store.

The notification must include:
- the intended issuing CA certificate’s SHA-256 hash,
- the intended subject CA certificate’s DN,
- the intended date of certificate issuance,
- the organization responsible for operating the intended subject CA,
- the organization that owns the intended subject CA’s key material, 
- the organization providing RA services for the intended subject CA, 
- all relevant audit reports related to the intended subject CA,
- a detailed description of the process the intended issuing CA operator used to evaluate the conformance of the intended subject CA and subject CA operator against its own policies, the Baseline Requirements, and this policy
- all relevant artifacts related to the evaluation of the intended subject CA and subject CA operator (including CP, CPS, subscriber agreements, assessment methodology, outputs from the assessment process, assessor qualifications, etc.)
- a detailed description of the intended issuing CA operator’s planned compliance monitoring activities and oversight functions for the intended subject CA and subject CA operator, and
- an explicit acknowledgment that the intended subject CA and subject CA operator satisfy the requirements defined in this policy.

Certificates must not be issued without the expressed approval of the Chrome Root Program.

#### Notification of Procurement, Sale, or other Change Control Events
CA operators with CAs included in the Chrome Root Store must not assume trust is transferable. 

Where permissible by law, CA operators must notify [chrome-root-program@google.com](mailto:chrome-root-program@google.com) at least 30 days before any impending:
- procurements, 
- sales, 
- changes of ownership or operating control, 
- cessations of operations, or
- other change control events involving PKI components that would materially affect the ongoing operations or perceived trustworthiness of a CA whose certificate is included in the Chrome Root Store (e.g., changes to operational location(s)).

Not limited to the circumstances above, the Chrome Root Program reserves the right to require a CA certificate’s re-application to the Chrome Root Store. 

## Moving Forward, Together
For more than the last decade, Web PKI community members have tirelessly worked together to make the Internet a safer place. However, there’s still more work to be done. While we don’t know exactly what the future looks like, we remain focused on promoting changes that increase speed, security, stability, and simplicity throughout the ecosystem.

With that in mind, the Chrome team continues to explore introducing future policy requirements related to the following initiatives:
- Encouraging modern infrastructures and agility
- Focusing on simplicity 
- Promoting automation 
- Reducing mis-issuance
- Increasing accountability and ecosystem integrity 
- Streamlining and improving domain validation practices
- Preparing for a “post-quantum” world

We hope to make progress against many of these initiatives in the next version of our policy and welcome feedback on the proposals below at [chrome-root-program@google.com](mailto:chrome-root-program@google.com). We also intend to share a CCADB survey to more easily collect targeted CA operator feedback. We want to hear from CA operators about what challenges they anticipate with the proposed changes below and how we can help in addressing them.

### Encouraging modern infrastructures and agility
We think it’s time to revisit the notion that root CAs and their corresponding certificates should be trusted for 30+ years. While we do not intend to require a reduced root CA certificate validity period, we think it’s critically important to promote modern infrastructures by requiring operators to rotate aging root CAs with newer ones.

In a future policy update, we intend to introduce a maximum “term limit” for root CAs whose certificates are included in the Chrome Root Store. The proposed term duration is 7 years, measured from the initial date of certificate inclusion. For CA certificates already included in the Chrome Root Store, the term would begin when the policy introducing the requirement took effect. CA operators would be encouraged to apply with a replacement CA certificate after 5 years of inclusion, which must contain a subject public key that the Chrome Root Store has not previously distributed. For compatibility reasons, CAs transitioning out of Chrome Root Store due to the term limit may issue a certificate to the replacement CA.

### Focusing on simplicity 
One of the [10 things we know to be true](https://about.google/philosophy/) is that “it’s best to do one thing really, really well.” Though multipurpose root CAs have offered flexibility in addressing subscriber use cases over the last few decades, they are now faced with satisfying the demands of multiple sets of increasingly rigorous expectations and requirements that do not always align. We believe in the value of purpose-built, dedicated infrastructures and the increased opportunity for innovation that comes along with them. We also think there’s value in reducing the complexity of the Web PKI. By promoting dedicated-use hierarchies, we can better align the related policies and processes that CAs must conform to with the expectations of subscribers, relying parties, and root program operators. 

This policy introduces a requirement for all new applicant CA certificates to be part of dedicated TLS PKI hierarchies. We intend to identify a phase-out plan for existing multipurpose root CA certificates included in the Chrome Root Store in a future policy update. A future update may also modify the definition of a dedicated TLS PKI hierarchy to prohibit using the extendedKeyUsage value of id-kp-clientAuth in new subordinate CA and TLS certificates chaining to a root included in the Chrome Root Store.

### Promoting automation
One of the most prominent classes of incidents we continue to observe in the Web PKI is a failure to revoke certificates in a timely manner. Justification for the delays is often cited due to the inability of affected subscribers to obtain new certificates before the required revocation window. The most reliable approach to addressing this concern is to replace manual issuance with robust, standardized automated issuance processes such as Automatic Certificate Management Environment (ACME, [RFC 8555](https://datatracker.ietf.org/doc/html/rfc8555)).

In a future policy update, we intend to introduce a requirement that all new subordinate CAs established after a specific date must support ACME for certificate issuance and revocation. 

### Increasing accountability and ecosystem integrity
We value high-quality, independent audit criteria that result in accurate, detailed, and consistent evaluations of CA operators’ policies and practices, regardless of their intended scope of certificate issuance or geographic location. 

In a future policy update, we intend to introduce two changes related to the annual audit criteria accepted by the Chrome Root Program for Technically Constrained subordinate CAs:
1. Effective at a future date, the Chrome Root Program will no longer accept self-audits as permitted by the Baseline Requirements.
2. Effective at a future date, all Technically Constrained subordinate CAs assessed against the WebTrust audit scheme must be assessed against both the WebTrust Principles and Criteria for Certification Authorities and the WebTrust Principles and Criteria for Certification Authorities – SSL Baseline with Network Security. Corresponding audit reports for CAs whose certificate does not assert an extendedKeyUsage value of id-kp-serverAuth must express non-performance of TLS certificate issuance and include an evaluation of the Network and Certificate System Security Requirements.
