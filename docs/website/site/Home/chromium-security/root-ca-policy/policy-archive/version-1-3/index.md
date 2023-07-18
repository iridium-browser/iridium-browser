---
breadcrumbs:
- - /Home
  - Chromium
- - /Home/chromium-security
  - Chromium Security
- - /Home/chromium-security/root-ca-policy
  - Root Program Policy
title: Chrome Root Program Policy Archive - Version 1.3
---

## Archive Notice 

<p><strong><span style="color:#FF0000">IMPORTANT:</span></strong> This page is
 retained for historical purposes only. 
 
Version 1.3 of the Chrome Root Program Policy was superseded by Version <a href=/Home/chromium-security/root-ca-policy/>1.4</a> on March 3, 2023.
 
For the latest version of the Chrome Root Program Policy, see <a href="https://g.co/chrome/root-policy">https://g.co/chrome/root-policy</a>.</p>

## Introduction
Google Chrome relies on Certification Authority systems (herein referred to as “CAs”) to issue certificates to websites. Chrome uses these certificates to help ensure the connections it makes on behalf of its users are properly secured. Chrome accomplishes this by verifying that a website’s certificate was issued by a recognized CA, while also performing additional evaluations of the HTTPS connection's security properties. Certificates not issued by a CA recognized by Chrome or a user’s local settings can cause users to see warnings and error pages.

When making HTTPS connections, Chrome refers to a list of root certificates from CAs that have demonstrated why continued trust in them is justified. This list is known as a “Root Store.” CA certificates included in the [Chrome Root Store](https://g.co/chrome/root-store) are selected on the basis of publicly available and verified information, such as that within the Common CA Database ([CCADB](https://ccadb.org/)), and ongoing reviews by the Chrome Root Program. CCADB is a datastore run by Mozilla and used by various operating systems, browser vendors, and CA owners to share and disclose information regarding the ownership, historical operation, and audit history of CAs and corresponding certificates and key material.

Historically, Chrome has integrated with the Root Store provided by the platform on which it is running. In Chrome 105, Chrome began a platform-by-platform transition from relying on the host operating system’s Root Store to its own on Windows, macOS, ChromeOS, Linux, and Android. This change makes Chrome more secure and promotes consistent user and developer experiences across platforms. Apple policies prevent the Chrome Root Store and corresponding Chrome Certificate Verifier from being used on Chrome for iOS. 

The Chrome Root Program policy below establishes the minimum requirements for CA certificates to be included in a default installation of Chrome. Learn more about priorities and initiatives that may influence future versions of this policy [here](/Home/chromium-security/root-ca-policy/moving-forward-together/). 

CA owners that satisfy the requirements defined in the policy below may apply for certificate inclusion in the Chrome Root Store using [these](/Home/chromium-security/root-ca-policy/apply-for-inclusion/) instructions.

### Additional Information
If you’re a Chrome user experiencing a certificate error and need help, please see [this support article](https://support.google.com/chrome/answer/6098869?hl=en).

If you’re a website operator, you can learn more about [why HTTPS matters](https://web.dev/why-https-matters/) and how to [secure your site with HTTPS](https://support.google.com/webmasters/answer/6073543). If you’ve got a question about a certificate you’ve been issued, please contact the CA that issued it.

If you're responsible for a CA that only issues certificates to your enterprise organization, sometimes called a "private" or "locally trusted" CA, the Chrome Root Program policy does not apply to or impact your organization’s use cases. Enterprise CAs are intended for use cases exclusively internal to an organization (e.g., a TLS certificate issued to a corporate intranet site).

Though uncommon, websites can also use certificates to identify clients (e.g., users) connecting to them. Besides ensuring it is well-formed, Chrome passes this type of certificate to the server, which then evaluates and enforces its chosen policy. The policies on this page do not apply to client authentication certificates.

## Change History 

<style type="text/css">.tg  {border-collapse:collapse;border-spacing:0;}
.tg td{border-color:black;border-style:solid;border-width:1px;overflow:hidden;padding:10px 5px;word-break:normal;}
.tg th{border-color:black;border-style:solid;border-width:1px;font-weight:normal;overflow:hidden;padding:10px 5px;word-break:normal;}
.tg .tg-h1{background-color:#efefef;font-weight:bold;text-align:center;vertical-align:top}
.tg .tg-center{text-align:center;vertical-align:top}
.tg .tg-left{text-align:left;vertical-align:top}
</style>
<table class="tg">
<thead>
<tr>
<th class="tg-h1">Version</th>
<th class="tg-h1">Date</th>
<th class="tg-h1">Note</th>
		</tr>
	</thead>
	<tbody>
		<tr>
			<td class="tg-center">1.0</td>
			<td class="tg-center">2020-12-20</td>
			<td class="tg-left">Initial release</td>
		</tr>
		<tr>
			<td class="tg-center">1.1</td>
			<td class="tg-center">2022-06-01</td>
			<td class="tg-left">Updated in anticipation of the future Chrome Root Program launch.<br><br>Updates include, but are not limited to:
			<li>future-dated applicant requirements for dedicated TLS-hierarchies and key-pair freshness</li>
			<li>clarification of audit expectations</li>
			<li>requirements for cross-certificate issuance notification</li>
			<li>description of and requirements related to an annual self-assessment process</li>
			<li>an outline of priority Chrome Root Program initiatives</li>
			</td>
		</tr>
		<tr>
			<td class="tg-center">1.2</td>
			<td class="tg-center">2022-09-01</td>
			<td class="tg-left">Updated to reflect the launch of the Chrome Root Program. <br><br>Updates include, but are not limited to:			
			<li>removal of pre-launch discussion</li>
			<li>clarifications resulting from the June 2022 Chrome CCADB survey</li>
			<li>minor reorganization of normative and non-normative requirements</li>
			</td>
		</tr>
		<tr>
			<td class="tg-center">1.3</td>
			<td class="tg-center">2023-01-06</td>
			<td class="tg-left">Updated to include the CCADB Self-Assessment
			</td>
	</tbody>
</table>


## Minimum Requirements for CAs 

This policy considers a CA owner to be an organization or legal entity that is represented in the subject DN of a CA certificate that is in possession or control of the corresponding private key capable of issuing new certificates. CA owners with certificates included in the Chrome Root Store must satisfy the requirements defined in this policy, including taking responsibility for ensuring the continued compliance of all corresponding subordinate CAs and delegated third parties participating in the Public Key Infrastructure (PKI).

A CA owner’s failure to follow the requirements defined in this policy may result in a CA certificate’s removal from the Chrome Root Store, limitations on Chrome's acceptance of the certificates they issue, or other technical or policy restrictions.

Any questions regarding this policy can be directed to [chrome-root-program@google.com](mailto:chrome-root-program@google.com).

### 1. Baseline Requirements
Applicants and CA owners with certificates included in the Chrome Root Store must adhere to the latest version of the CA/Browser Forum [“Baseline Requirements for the Issuance and Management of Publicly-Trusted Certificates”](https://cabforum.org/baseline-requirements-documents/) (“Baseline Requirements”).

### 2. CA Owner Policies
Applicants and CA owners with certificates included in the Chrome Root Store must accurately describe the policies and practices of their CA(s) within a Certificate Policy (CP) and corresponding Certification Practice Statement (CPS). These documents must provide sufficient detail to assess the operations of the CA(s) and the compliance with these expectations and those of the Baseline Requirements, and must not conflict with either of these requirements. CA owners may combine their CP and CPS into a single document as a CP/CPS. These documents must be freely publicly available for examination, and include at least an authoritative English language version.

CA owners must upload updated versions of a CA’s CP, CPS, or CP/CPS to its own publicly accessible policy repository and update CCADB within seven days of the update being completed.

### 3. Modern Infrastructures
The Chrome Root Store will only accept applicant root CA certificates with corresponding key material generated within five years of application to the Chrome Root Store.

### 4. Dedicated TLS PKI Hierarchies
The Chrome Root Store will only accept applicant root CA certificates that are part of PKI hierarchies dedicated to TLS certificate issuance.

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

It is expected that a future version of this policy will identify a phase-out plan for existing root CA certificates included in the Chrome Root Store that do not satisfy the requirements above to align all included CAs on these principles.

### 5. Audits

Both applicant CAs and those with certificates already included in the Chrome Root Store, along with all corresponding subordinate CAs, must be audited in accordance with the table below. 

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

\* while existing CA certificates trusted by Chrome *may* have EKU values as described in this table, applicant CAs **must** be a part of a [dedicated TLS PKI hierarchy](#4-dedicated-tls-pki-hierarchies)<br>
\** accepted on a discretionary basis

#### Annual Audits
CAs must retain an unbroken, contiguous audit coverage.

Recurring complete (i.e., “full”, “full system” or “full re-assessment”) annual audits must begin once a CA’s key material has been generated and must continue until the corresponding root CA’s key material has been destroyed or is no longer included in the Chrome Root Store.

#### Ad-Hoc Audits
Root CA certificate key material generation must be observed and audited by a Qualified Auditor who is unaffiliated with the CA owner, as described and required by Section 6.1.1.1 of the Baseline Requirements. 

When deemed necessary, the Chrome Root Program may require CAs undergo additional ad-hoc audits, including, but not limited to, instances of CA private key destruction or verification of incident remediation.

#### Reporting Requirements
Reports for both [annual](#annual-audits) and [ad-hoc](#ad-hoc-audits) audits must satisfy the requirements in Section 8.6 of the Baseline Requirements and be uploaded to the CCADB within 90 days from the ending date specified in the audit letter. See CCADB instructions for [formatting](https://www.ccadb.org/policy#51-audit-statement-content) and [uploading](https://ccadb.org/cas/updates) audit reports.

### 6. Annual Self-Assessments
Beginning in 2023, on an annual basis, CA owners with certificates included in the Chrome Root Store must complete and submit to the CCADB a [self-assessment](https://www.ccadb.org/cas/self-assessment) evaluating the conformance of their policies and practices against this policy and the Baseline Requirements for:
- each root CA certificate included in the Chrome Root Store.
- any corresponding subordinate CA certificate technically capable of issuing TLS certificates (i.e., explicitly contains an extendedKeyUsage value of id-kp-serverAuth or the functional equivalent, for example, due to the absence of any extendedKeyUsage values).

A single self-assessment may cover multiple CAs operating under both the same CP and CPS, or combined CP/CPS. If a consolidated self-assessment is submitted, the CA owner must enumerate the CAs in the scope of the self-assessment.

Completed annual self-assessments must be submitted to the CCADB within 90 days from the “BR Audit Period End Date” field specified in the root CA’s “CA Owner/Certificate” CCADB record.

All applicants must submit a completed self-assessment to the CCADB before requesting a CA certificate’s inclusion in the Chrome Root Store.

### 7. Responding to Incidents
All applicants and CA owners with certificates included in the Chrome Root Store must abide by their CP and/or CPS, where they describe how the CA is operated, and must incorporate the Baseline Requirements. Failure of a CA owner to meet their commitments, as outlined in the CA’s CP/CPS and the Baseline Requirements, is considered an incident, as is any other situation that may impact the CA’s integrity, trustworthiness, or compatibility.

Any suspected or actual incident must be reported and publicly disclosed at the time of discovery by the CA owner, along with a timeline for an [an analysis of root causes](https://sre.google/sre-book/postmortem-culture/), regardless of perceived impact on end users. Chrome will evaluate every incident on a case-by-case basis, and will work with the CA owner to identify ecosystem-wide risks or potential improvements to be made in the CA’s operations or in root program requirements that can help prevent future incidents.

When evaluating an incident response, Chrome’s primary concern is ensuring that browsers, other CA owners, users, and website developers have the necessary information to identify improvements, and that the CA owner is responsive to addressing identified issues.

Factors that are significant to Chrome when evaluating incidents include (but are not limited to): 
- a demonstration of understanding of the root causes of an incident, 
- a substantive commitment and timeline to changes that clearly and persuasively address the root cause, 
- past history by the CA owner in its incident handling and its follow through on commitments, and, 
- the severity of the security impact of the incident.

In general, a single incident considered alone is unlikely to result in removal of a CA certificate from the Chrome Root Store.

CA owners must be detailed, candid, timely, and transparent in describing their architecture, implementation, operations, and external dependencies as necessary for the Chrome Root Program and the public to evaluate the nature of the incident and the CA owner’s response.

CA owners must file an incident report if they fail to meet the commitments made in their CP, CPS, or CP/CPS. Due to the incorporation of the Baseline Requirements into the CP and CPS, incidents may include a prescribed follow-up action, such as revoking impacted certificates within a certain timeframe. If the CA owner does not perform the required follow-up actions, or does not perform them in the expected timeframe, the CA owner should file a secondary incident report describing any certificates involved, the expected timeline to complete any follow-up actions, and what changes they are making to ensure they can meet these requirements consistently in the future.

When a CA owner becomes aware of or suspects an incident, they must notify [chrome-root-program@google.com](mailto:chrome-root-program@google.com) with a description of the incident. If the CA owner has publicly disclosed this incident, this notification must include a link to the disclosure. If the CA owner has not yet disclosed this incident, this notification should include an initial timeline for public disclosure. Chrome uses the information in the public disclosure as the basis for evaluating incidents.

### 8. Common CA Database
All applicants and CA owners with certificates included in the Chrome Root Store must follow the requirements defined in the [CCADB policy](https://www.ccadb.org/policy). In instances where the CCADB policy conflicts with this policy, this policy must take precedence.

Minimally, CA owners with certificates included in the Chrome Root Store must ensure information stored in the CCADB is reviewed monthly and updated when changes occur.

### 9. Timely and Transparent Communications
At any time, the Chrome Root Program may request additional information from a CA owner using email or CCADB communications to verify the commitments and obligations outlined in this policy are being met. CA owners must provide the requested information within 14 days unless specified otherwise.

#### Notification of Cross-Certificate Issuance
CA owners must notify [chrome-root-program@google.com](mailto:chrome-root-program@google.com) at least three weeks before issuing a certificate to any CA owned or operated by a different CA owner that can validate to a corresponding root CA certificate included in the Chrome Root Store.

The notification must include:
- the intended issuing CA certificate’s SHA-256 hash,
- the intended subject CA certificate’s DN,
- the intended date of certificate issuance,
- the organization responsible for operating the intended subject CA,
- the organization that owns the intended subject CA’s key material,
- the organization providing RA services for the intended subject CA,
- all relevant audit reports related to the intended subject CA,
- a detailed description of the process the intended issuing CA owner used to evaluate the conformance of the intended subject CA and subject CA owner against its own policies, the Baseline Requirements, and this policy,
- all relevant artifacts related to the evaluation of the intended subject CA and subject CA owner (including CP, CPS, subscriber agreements, assessment methodology, outputs from the assessment process, assessor qualifications, etc.),
- a detailed description of the intended issuing CA owner’s planned compliance monitoring activities and oversight functions for the intended subject CA and subject CA owner, and
- an explicit acknowledgment that the intended subject CA and subject CA owner satisfy the requirements defined in this policy.

Cross-certificates must not be issued without the expressed approval of the Chrome Root Program.

#### Notification of Procurement, Sale, or other Change Control Events
CA owners with CAs included in the Chrome Root Store must not assume trust is transferable.

Where permissible by law, CA owners must notify [chrome-root-program@google.com](mailto:chrome-root-program@google.com) at least 30 days before any impending:
- procurements,
- sales,
- changes of ownership or operating control,
- cessations of operations, or
- other change control events involving PKI components that would materially affect the ongoing operations or perceived trustworthiness of a CA whose certificate is included in the Chrome Root Store (e.g., changes to operational location(s)).

Not limited to the circumstances above, the Chrome Root Program reserves the right to require a CA certificate’s re-application to the Chrome Root Store.
