---
breadcrumbs:
- - /Home
  - Chromium
- - /Home/chromium-security
  - Chromium Security
page_name: root-ca-policy
title: Chrome Root Program Policy, Version 1.4
---

## Last updated: 2023-03-03
Bookmark this page as <https://g.co/chrome/root-policy>

## Table of Contents
- [Introduction](#introduction)
     - [Apply for Inclusion](#apply-for-inclusion)
     - [Moving Forward, Together](#moving-forward-together)
     - [Additional Information](#additional-information)
- [Change History](#change-history)
- [Minimum Requirements for CAs](#minimum-requirements-for-cas)
     - [1. Baseline Requirements](#1-baseline-requirements)
     - [2. Chrome Root Program Participant Policies](#2-chrome-root-program-participant-policies)
     - [3. Modern Infrastructures](#3-modern-infrastructures)
     - [4. Dedicated TLS Server Authentication PKI Hierarchies](#4-dedicated-tls-server-authentication-pki-hierarchies)
     - [5. Audits](#5-audits)
     - [6. Annual Self Assessments](#6-annual-self-assessments)
     - [7. Responding to Incidents](#7-responding-to-incidents)
     - [8. Common CA Database](#8-common-ca-database)
     - [9. Timely and Transparent Communications](#9-timely-and-transparent-communications)

## Introduction

Google Chrome relies on Certification Authority systems (herein referred to as “CAs”) to issue certificates to websites. Chrome uses these certificates to help ensure the connections it makes on behalf of its users are properly secured. Chrome accomplishes this by verifying that a website’s certificate was issued by a recognized CA, while also performing additional evaluations of the HTTPS connection's security properties. Certificates not issued by a CA recognized by Chrome or a user’s local settings can cause users to see warnings and error pages.

When making HTTPS connections, Chrome refers to a list of root certificates from CAs that have demonstrated why continued trust in them is justified. This list is known as a “Root Store.” CA certificates included in the [Chrome Root Store](https://g.co/chrome/root-store) are selected on the basis of publicly available and verified information, such as that within the Common CA Database ([CCADB](https://ccadb.org/)), and ongoing reviews by the Chrome Root Program. CCADB is a datastore run by Mozilla and used by various operating systems, browser vendors, and CA owners to share and disclose information regarding the ownership, historical operation, and audit history of CAs and corresponding certificates and key material.

Historically, Chrome has integrated with the Root Store provided by the platform on which it is running. In Chrome 105, Chrome began a platform-by-platform transition from relying on the host operating system’s Root Store to its own on Windows, macOS, ChromeOS, Linux, and Android. This change makes Chrome more secure and promotes consistent user and developer experiences across platforms. Apple policies prevent the Chrome Root Store and corresponding Chrome Certificate Verifier from being used on Chrome for iOS. 

The Chrome Root Program Policy below establishes the minimum requirements for root CA certificates to be included as trusted in a default installation of Chrome. 

### Apply for Inclusion

CA owners that satisfy the requirements defined in the policy below may apply for self-signed root CA certificate inclusion in the Chrome Root Store using [these](/Home/chromium-security/root-ca-policy/apply-for-inclusion/) instructions.

### Moving Forward, Together

The June 2022 release ([Version 1.1](/Home/chromium-security/root-ca-policy/policy-archive/version-1-1/)) of the Chrome Root Program Policy introduced the Chrome Root Program’s “Moving Forward, Together” initiative that set out to share our vision of the future that includes modern, reliable, highly agile, purpose-driven PKIs with a focus on automation, simplicity, and security.

Learn more about priorities and initiatives that may influence future versions of this policy [here](/Home/chromium-security/root-ca-policy/moving-forward-together/). 

### Additional Information

If you’re a Chrome user experiencing a certificate error and need help, please see [this support article](https://support.google.com/chrome/answer/6098869?hl=en).

If you’re a website operator, you can learn more about [why HTTPS matters](https://web.dev/why-https-matters/) and how to [secure your site with HTTPS](https://support.google.com/webmasters/answer/6073543). If you’ve got a question about a certificate you’ve been issued, please contact the CA that issued it.

If you're responsible for a CA that only issues certificates to your enterprise organization, sometimes called a "private" or "locally trusted" CA, the Chrome Root Program Policy does not apply to or impact your organization’s use cases. Enterprise CAs are intended for use cases exclusively internal to an organization (e.g., a TLS server authentication certificate issued to a corporate intranet site).

Though uncommon, websites can also use certificates to identify clients (e.g., users) connecting to them. Besides ensuring it is well-formed, Chrome passes this type of certificate to the server, which then evaluates and enforces its chosen policy. The policies on this page do not apply to client authentication certificates.

## Change History

<table>
  <tr>
   <td><strong>Version</strong>
   </td>
   <td><strong>Date</strong>
   </td>
   <td><strong>Note</strong>
   </td>
  </tr>
  <tr>
   <td><a href=/Home/chromium-security/root-ca-policy/policy-archive/version-1-0/>1.0</a>
      </td>
   <td>2020-12-20
   </td>
   <td>Initial release
   </td>
  </tr>
  <tr>
   <td><a href=/Home/chromium-security/root-ca-policy/policy-archive/version-1-1/>1.1</a>
   </td>
   <td>2022-06-01
   </td>
   <td>Updated in anticipation of the future Chrome Root Program launch. 
<p>
Updates include, but are not limited to:<ul>

<li>future-dated applicant requirements for dedicated TLS-hierarchies and key-pair freshness
<li>clarification of audit expectations 
<li>requirements for cross-certificate issuance notification
<li>description of and requirements related to an annual self-assessment process
<li>an outline of priority Chrome Root Program initiatives </li></ul>

   </td>
  </tr>
  <tr>
   <td><a href=/Home/chromium-security/root-ca-policy/policy-archive/version-1-2/>1.2</a>
   </td>
   <td>2022-09-01
   </td>
   <td>Updated to reflect the launch of the Chrome Root Program.
<p>
Updates include, but are not limited to:<ul>

<li>removal of pre-launch discussion
<li>clarifications resulting from the June 2022 Chrome CCADB survey
<li>minor reorganization of normative and non-normative requirements</li></ul>

   </td>
  </tr>
  <tr>
   <td><a href=/Home/chromium-security/root-ca-policy/policy-archive/version-1-3/>1.3</a>
   </td>
   <td>2023-01-06
   </td>
   <td>Updated to include the CCADB Self-Assessment
   </td>
  </tr>
  <tr>
   <td>1.4 (current)
   </td>
   <td>2023-03-03
   </td>
   <td>Updates include, but are not limited to:<ul>

<li>alignment with CCADB Policy Version 1.2 and the Baseline Requirements
<li>clarify requirements related to the submission of annual self assessments
<li>clarify requirements to better align with program intent (e.g., CA owner policy document freshness)
<li>updated audit and incident reporting requirements to promote increased transparency
<li>require subordinate CA disclosures in CCADB
<li>clarify CA certificate issuance notification requirements</li></ul>

   </td>
  </tr>
</table>

## Minimum Requirements for CAs
This policy considers a CA owner to be the organization or legal entity that is either:
* represented in the subject DN of the CA certificate, or 
* in possession or control of the corresponding private key capable of issuing new certificates, if not the same organization or legal entity directly represented in the subject DN of the certificate.

The Chrome Root Program relies on the [CCADB](https://ccadb.org/) to identify and maintain up-to-date information for the CA owners whose certificates are trusted by default in Chrome.

CA owners with self-signed root CA certificates included in the Chrome Root Store must satisfy the requirements defined in this policy, including taking responsibility for ensuring the continued compliance of all corresponding subordinate CAs and delegated third parties participating in the Public Key Infrastructure (PKI).

This policy considers an “applicant” to be an organization or legal entity that has an active self-signed root CA certificate inclusion request submitted to the Chrome Root Store in the CCADB. 

This policy uses the term “Chrome Root Program Participants” to describe applicants and CA owners with:
*   CA certificates included in the Chrome Root Store; or 
*   CA certificates that validate to certificates included in the Chrome Root Store.

Google includes or removes self-signed root CA certificates in the Chrome Root Store as it deems appropriate at its sole discretion. The selection and ongoing inclusion of CA certificates is done to enhance the security of Chrome and promote interoperability. CA certificates that do not provide a broad service to all browser users will not be added to, or may be removed from the Chrome Root Store. CA certificates included in the Chrome Root Store must provide value to Chrome end users that exceeds the risk of their continued inclusion. 

Chrome maintains a variety of mechanisms to protect its users from certificates that put their safety and privacy at risk, and is prepared to use them as necessary. A Chrome Root Program Participant’s failure to follow the minimum requirements defined in this policy may result in the corresponding CA certificate’s removal from the Chrome Root Store, limitations on Chrome's acceptance of the certificates they issue, or other technical or policy restrictions. 

The requirements included in this policy are effective immediately, unless explicitly stated as otherwise.

Any questions regarding this policy can be directed to _chrome-root-program_ _[at]_ _google_ _[dot]_ _com_.

### 1. Baseline Requirements
Chrome Root Program Participants that issue TLS server authentication certificates trusted by Chrome must adhere to the latest version of the CA/Browser Forum [“Baseline Requirements for the Issuance and Management of Publicly-Trusted Certificates”](https://cabforum.org/baseline-requirements-documents/) (“Baseline Requirements”).

This policy describes TLS server authentication certificates in [Section 4](#4-dedicated-tls-server-authentication-pki-hierarchies) (“Dedicated TLS Server Authentication PKI Hierarchies”).

### 2. Chrome Root Program Participant Policies
Chrome Root Program Participants must accurately describe the policies and practices of their CA(s) within a Certificate Policy (CP) and corresponding Certification Practice Statement (CPS). 

These documents must provide sufficient detail to assess the operations of the CA(s) and the compliance with these expectations and those of the Baseline Requirements, and must not conflict with either of these requirements. Chrome Root Program Participants may combine their CP and CPS into a single document as a CP/CPS. These documents must be freely publicly available for examination, and include at least an authoritative English language version.

Chrome Root Program Participants must ensure updated versions of a CA’s CP, CPS, or CP/CPS are uploaded to their own publicly accessible policy repository and the CCADB before the corresponding changes are put into practice. 

The Chrome Root Program considers policy documentation in the CCADB to be authoritative. Chrome Root Program Participant operational practices must comply with those defined in the corresponding policy documents stored in the CCADB.

### 3. Modern Infrastructures
The Chrome Root Store will only accept applicant root CA certificates with corresponding key material generated within five years of application to the Chrome Root Store.

### 4. Dedicated TLS Server Authentication PKI Hierarchies
The Chrome Root Store will only accept applicant root CA certificates that are part of PKI hierarchies dedicated to TLS server authentication certificate issuance.

A dedicated PKI hierarchy is intended to serve one specific use case, for example, the issuance of TLS server authentication certificates.

To qualify as a dedicated TLS server authentication PKI hierarchy under this policy:

1. All corresponding subordinate CA certificates operated beneath a root CA must:
    *   include the extendedKeyUsage extension and only assert an extendedKeyUsage purpose of either:
        1. id-kp-serverAuth, or
        2. id-kp-serverAuth and id-kp-clientAuth
    *   not contain a public key corresponding to any other unexpired or non-revoked certificate that asserts different extendedKeyUsage values.
<br><br>
2. All corresponding subscriber (i.e., server) certificates must:
    *   include the extendedKeyUsage extension and only assert an extendedKeyUsage purpose of either:
        1. id-kp-serverAuth, or
        2. id-kp-serverAuth and id-kp-clientAuth

It is expected that a future version of this policy will identify a phase-out plan for existing root CA certificates included in the Chrome Root Store that do not satisfy the requirements above to align all included CAs on these principles.

### 5. Audits
Chrome Root Program Participant CAs must be audited in accordance with the table below. 

<style type="text/css">
.tg  {border-collapse:collapse;border-spacing:0;}
.tg td{border-color:black;border-style:solid;border-width:1px;font-family:Arial, sans-serif;font-size:14px;
  overflow:hidden;padding:10px 5px;word-break:normal;}
.tg th{border-color:black;border-style:solid;border-width:1px;font-family:Arial, sans-serif;font-size:14px;
  font-weight:normal;overflow:hidden;padding:10px 5px;word-break:normal;}
.tg .tg-h1{background-color:#CCC;border-color:inherit;font-weight:bold;text-align:center;vertical-align:top}
.tg .tg-3paudit{border-color:inherit;text-align:left;vertical-align:top}
.tg .tg-self{text-align:left;vertical-align:top}
</style>
<table class="tg">
<thead>
  <tr>
    <th class="tg-h1">CA Type</th>
    <th class="tg-h1">EKU Characteristics*</th>
    <th class="tg-h1">Audit Criteria</th>
  </tr>
</thead>
<tbody>
  <tr>
    <td class="tg-3paudit">Root CA</td>
    <td class="tg-3paudit">N/A</td>
    <td class="tg-3paudit" rowspan="3"><span style="text-decoration:underline">If WebTrust…</span>
	<li>WebTrust Principles and Criteria for Certification Authorities; <b>and</b></li>
	<li>WebTrust Principles and Criteria for Certification Authorities – SSL Baseline with Network Security</li>
	<li>[<b>and</b> WebTrust for CAs - EV SSL, if issuing EV]</li>
	<br>or…<br>
	<br><span style="text-decoration:underline">If ETSI**…</span>
	<li>ETSI EN 319 411-1 LCP and [DVCP or OVCP]; <b>or</b></li>
	<li>ETSI EN 319 411-1 [NCP or NCP+] and EVCP (if issuing EV)</li></td>
  </tr>
  <tr>
    <td class="tg-3paudit">Cross-Certified Subordinate CA</td>
    <td class="tg-3paudit"><span style="text-decoration:underline">Either</span>:
	<li>Certificate does not include an EKU; <b>or</b></li>
	<li>EKU is present and includes id-kp-serverAuth <span style="text-decoration:underline">or</span> anyExtendedKeyUsage</li></td>
  </tr>
  <tr>
    <td class="tg-3paudit">TLS Subordinate CA or Technically Constrained TLS Subordinate CA</td>
    <td class="tg-3paudit"><span style="text-decoration:underline">Either</span>:
	<li>Certificate does not include an EKU; <b>or</b></li>
	<li>EKU is present and includes id-kp-serverAuth <span style="text-decoration:underline">or</span> anyExtendedKeyUsage</li></td>
  </tr>
  <tr>
    <td class="tg-self">Technically Constrained Non-TLS Subordinate CA</td>
    <td class="tg-self">EKU is present and does not include id-kp-serverAuth or anyExtendedKeyUsage.</td>
    <td class="tg-self" rowspan="2">Minimally expected to be audited as defined in Section 8.7 of the BRs (self-audit).</td>
  </tr>
  <tr>
    <td class="tg-self">All others</td>
    <td class="tg-self">N/A</td>
  </tr>
</tbody>
</table>

\* while existing CA certificates trusted by Chrome may have EKU values as described in this table, applicant CAs must be a part of a [dedicated TLS PKI hierarchy](#4-dedicated-tls-server-authentication-pki-hierarchies)<br>
\** accepted on a discretionary basis

#### Annual Audits

Chrome Root Program Participant CAs must retain an unbroken, contiguous audit coverage.

Recurring complete (i.e., “full”, “full system” or “full re-assessment”) audits must occur at least once every 365 calendar days. These audits must begin once a CA’s key material has been generated and must continue until the corresponding root CA’s key material has been destroyed or is no longer included in the Chrome Root Store.

#### Ad-Hoc Audits

Root CA certificate key material generation must be observed and audited by a Qualified Auditor who is unaffiliated with the CA owner, as described and required by Section 6.1.1.1 of the Baseline Requirements. 

When deemed necessary, the Chrome Root Program may require CAs undergo additional ad-hoc audits, including, but not limited to, instances of CA private key destruction or verification of incident remediation.

#### Reporting Requirements
[Annual](#annual-audits) or [ad-hoc](#ad-hoc-audits) audit reports must satisfy the requirements in [Section 5.1](https://www.ccadb.org/policy#51-audit-statement-content) (“Audit Statement Content”) of the CCADB Policy.

All audit reports must be uploaded to the CCADB within 90 calendar days from the audit period ending date specified in the audit letter. See CCADB.org instructions for formatting and uploading audit reports.

### 6. Annual Self Assessments
CA owners with certificates included in the Chrome Root Store must complete and submit an annual self assessment to the CCADB. Instructions for completing the self assessment are included in the required assessment [template](https://www.ccadb.org/cas/self-assessment). 

A single self assessment may cover multiple CAs operating under both the same CP and CPS(s), or combined CP/CPS. CAs not operated under the same CP and CPS(s) or combined CP/CPS must be covered in a separate self assessment.

The scope of the self assessment submission must include:
* each root CA certificate included in the Chrome Root Store.
* any corresponding subordinate CA technically capable of issuing TLS certificates (i.e., explicitly contains an extendedKeyUsage value of id-kp-serverAuth or the functional equivalent, for example, due to the absence of any extendedKeyUsage values).

The self assessment submission date is determined by the earliest appearing “BR Audit Period End Date” field specified in any of the CA owner’s “CA Owner/Certificate” CCADB root records.
* The initial annual self assessment must be completed and submitted to the CCADB within 90 calendar days from the CA owner's earliest appearing root record “BR Audit Period End Date” that is after December 31, 2022. 
* Subsequent annual submissions must be made no later than 365 calendar days after the previous submission to the CCADB.

CA owners should always use the latest available version of the self assessment template. CA owners must not use a version of the self assessment template that has been superseded by more than 90 calendar days before their submission.

### 7. Responding to Incidents
The failure of a CA owner to meet the commitments of this policy is considered an incident, as is any other situation that may impact the CA’s integrity, trustworthiness, or compatibility.

Chrome Root Program Participants must publicly disclose incidents in [Bugzilla](https://bugzilla.mozilla.org/enter_bug.cgi?product=CA%20Program&component=CA%20Certificate%20Compliance), regardless of perceived impact on end users.

If the CA owner has not yet publicly disclosed an incident, they must notify _chrome-root-program_ _[at]_ _google_ _[dot]_ _com_ and include an initial timeline for public disclosure. Chrome uses the information in the public disclosure as the basis for evaluating incidents.

Reports must include a description of the incident using [this](https://www.ccadb.org/cas/incident-report#incident-reports) format and be submitted within the following timelines:
* incidents involving the loss of confidentiality, integrity, or unplanned availability of a CA system to include components with access to CA private key material, the ability to issue and/or manage certificates, or provide certificate status information, that prevents it from satisfying the commitments required by this policy, must be reported within 24 hours of initial identification or notification by an external party.
* all other incidents must be reported within seven (7) calendar days of initial identification or notification by an external party, whichever comes first, but preferably as soon as possible.

Once the report is posted, CA owners must respond promptly to questions that are asked, and in no circumstances should a question linger without a response for more than seven calendar days, even if the response is only to acknowledge the question and provide a later date when an answer will be delivered.

The Chrome Root Program will evaluate every incident on a case-by-case basis, and will work with the CA owner to identify ecosystem-wide risks or potential improvements to be made in the CA’s operations or in root program requirements that can help prevent future incidents.

CA owners must be detailed, candid, timely, and transparent in describing their architecture, implementation, operations, and external dependencies as necessary for the Chrome Root Program and the public to evaluate the nature of the incident and the CA owner’s response. When evaluating an incident response, the Chrome Root Program’s primary concern is ensuring that browsers, other CA owners, users, and website developers have the necessary information to identify improvements, and that the CA owner is responsive to addressing identified issues.

Factors that are significant to the Chrome Root Program when evaluating incidents include (but are not limited to): 
* a demonstration of understanding of the [root causes](https://sre.google/sre-book/postmortem-culture/) of an incident, 
* a substantive commitment and timeline to changes that clearly and persuasively address the root cause, 
* past history by the CA owner in its incident handling and its follow through on commitments, and, 
* the severity of the security impact of the incident.

Due to the incorporation of the Baseline Requirements into the CP and/or CPS, incidents may include a prescribed follow-up action, such as revoking impacted certificates within a certain timeframe. If the CA owner does not perform the required follow-up actions, or does not perform them in the expected timeframe, the CA owner should file a secondary incident report describing any certificates involved, the expected timeline to complete any follow-up actions, and what changes they are making to ensure they can meet these requirements consistently in the future.

#### Audit Incident Reports

For ETSI audits, any non-conformity and/or problem identified over the course of the audit is considered an incident and must have an [Audit Incident Report](https://www.ccadb.org/cas/incident-report#audit-incident-reports) created in Bugzilla by the CA owner prior to or within seven calendar days of the AAL’s issuance date.

For WebTrust audits, any qualification and/or modified opinion is considered an incident and must have an [Audit Incident Report](https://www.ccadb.org/cas/incident-report#audit-incident-reports) created in Bugzilla by the CA owner prior to or within seven calendar days of the Assurance Report’s issuance date. 

### 8. Common CA Database
Chrome Root Program Participants must:
1. Follow the requirements defined in the [CCADB policy](https://www.ccadb.org/policy).
    * In instances where the CCADB policy conflicts with this policy, this policy must take precedence. 
    * When a timeline is not defined for a requirement specified within the CCADB policy, updates must be submitted to the CCADB within 14 calendar days of being completed.
2. Ensure information stored in the CCADB is reviewed monthly and updated when changes occur.
3. Disclose all subordinate CA certificates capable of validating to a certificate included in the Chrome Root Store to the CCADB. 
    * All unrevoked and unexpired subordinate CA certificates must be disclosed no later than April 1, 2023.
    * For new subordinate CA certificates, CCADB disclosure must take place within seven calendar days of issuance and before the subject CA represented in the certificate begins issuing publicly-trusted certificates. 
4. Disclose revocation of all subordinate CA certificates capable of validating to a certificate included in the Chrome Root Store to the CCADB within seven calendar days of revocation. 

### 9. Timely and Transparent Communications
At any time, the Chrome Root Program may request additional information from a Chrome Root Program Participant using email or CCADB communications to verify the commitments and obligations outlined in this policy are being met, or as updates to policy requirements are being considered. Chrome Root Program Participants must provide the requested information within 14 calendar days unless specified otherwise.

#### Notification of CA Certificate Issuance

CA owners included in the Chrome Root Store must complete and submit [this](https://forms.gle/BxpZ4eyseAdo9Yn48) form at least three weeks before a CA in the corresponding hierarchy issues a CA certificate that:

* extends the Chrome Root Store’s trust boundary (i.e., the subject certificate CA owner is not explicitly included in the Chrome Root Store at the time of issuance), or
* replaces an unrevoked and unexpired CA certificate whose subject certificate CA owner is not explicitly included in the Chrome Root Store.

Such CA certificates must not be issued without the expressed approval of the Chrome Root Program.

No other notification or approval is required.

#### Notification of Procurement, Sale, or other Change Control Events

CA owners with CAs included in the Chrome Root Store must not assume trust is transferable.

Where permissible by law, CA owners must notify _chrome-root-program_ _[at]_ _google_ _[dot]_ _com_ at least 30 calendar days before any impending:

* procurements,
* sales,
* changes of ownership or operating control,
* cessations of operations, or
* other change control events involving PKI components that would materially affect the ongoing operations or perceived trustworthiness of a CA certificate included in the Chrome Root Store (e.g., changes to operational location(s)).

Not limited to the circumstances above, the Chrome Root Program reserves the right to require re-application to the Chrome Root Store.
