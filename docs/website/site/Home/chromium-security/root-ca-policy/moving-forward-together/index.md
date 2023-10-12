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

## Last updated: 2023-03-03

For more than the last decade, Web PKI community members have tirelessly worked together to make the Internet a safer place. However, there’s still more work to be done. While we don’t know exactly what the future looks like, we remain focused on promoting changes that increase speed, security, stability, and simplicity throughout the ecosystem.

With that in mind, the Chrome Root Program continues to explore introducing future policy requirements related to the following initiatives:
*   Encouraging modern infrastructures and agility
*   Focusing on simplicity
*   Promoting automation
*   Reducing mis-issuance
*   Increasing accountability and ecosystem integrity
*   Streamlining and improving domain validation practices
*   Preparing for a “post-quantum” world

We hope to make progress against many of these initiatives in future versions of our [policy](https://g.co/chrome/root-policy) and welcome feedback on the proposals below at _chrome-root-program_ _[at]_ _google_ _[dot]_ _com_. We also intend to share [CCADB](https://www.ccadb.org/) surveys to collect targeted CA owner feedback more easily. We want to hear from CA owners about what challenges they anticipate with the proposed changes below and how we can help address them.

## Encouraging modern infrastructures and agility

We think it’s time to revisit the notion that root CAs and their corresponding certificates should be trusted for 30+ years. While we do not intend to require a reduced root CA certificate validity period, we think it’s critically important to promote modern infrastructures by requiring operators to rotate aging root CAs with newer ones.

<span style="text-decoration:underline;">In a future policy update, we intend to introduce: </span>
*   **a maximum “term limit” for root CAs whose certificates are included in the Chrome Root Store.** Currently, our proposed term duration is seven (7) years, measured from the initial date of certificate inclusion. The term for CA certificates already included in the Chrome Root Store would begin when the policy introducing the requirement took effect. CA owners would be encouraged to apply with a replacement CA certificate after five (5) years of inclusion, which must contain a subject public key that the Chrome Root Store has not previously distributed. For compatibility reasons, CAs transitioning out of the Chrome Root Store due to the term limit may issue a certificate to the replacement CA.

<span style="text-decoration:underline;">In a future policy update or CA/Browser Forum Ballot Proposal, we intend to introduce: </span>
*   **a maximum validity period for subordinate CAs.** Much like how introducing a term limit for root CAs will allow the ecosystem to take advantage of continuous improvement efforts made by the Web PKI community, the same is true for subordinate CAs. Promoting agility in the ecosystem with shorter subordinate CA lifetimes will encourage more robust operational practices, reduce ecosystem reliance on specific subordinate CA certificates that might represent single points of failure, and discourage potentially harmful practices like key-pinning. Currently, our proposed maximum subordinate CA certificate validity is three (3) years.
*   **a reduction of TLS server authentication subscriber certificate maximum validity from 398 days to 90 days.** Reducing certificate lifetime encourages automation and the adoption of practices that will drive the ecosystem away from baroque, time-consuming, and error-prone issuance processes. These changes will allow for faster adoption of emerging security capabilities and best practices, and promote the agility required to transition the ecosystem to quantum-resistant algorithms quickly. Decreasing certificate lifetime will also reduce ecosystem reliance on “[broken](https://scotthelme.co.uk/revocation-is-broken/)” revocation checking solutions that [cannot fail-closed](https://www.imperialviolet.org/2014/04/29/revocationagain.html) and, in turn, offer incomplete protection. Additionally, shorter-lived certificates will decrease the impact of unexpected Certificate Transparency Log disqualifications. 

In hopes of promoting the issuance and use of short-lived certificates, we [presented](https://lists.cabforum.org/pipermail/servercert-wg/2022-November/003420.html) a set of proposed changes to the [Baseline Requirements](https://cabforum.org/baseline-requirements-documents/) that incentivize the security properties described above. These changes are currently under review and consideration by the CA/Browser Forum Server Certificate Working Group members. 

In this same proposal, we introduced the idea of making Online Certificate Status Protocol (OCSP) services optional. OCSP requests reveal details of individuals’ browsing history to the operator of the OCSP responder. These can be exposed accidentally (e.g., via data breach of logs) or intentionally (e.g., via subpoena). Beyond privacy concerns, OCSP use is accompanied by a high volume of routine incidents and issues ([1](https://bugzilla.mozilla.org/buglist.cgi?bug_status=UNCONFIRMED&bug_status=NEW&bug_status=ASSIGNED&bug_status=REOPENED&bug_status=RESOLVED&bug_status=VERIFIED&bug_status=CLOSED&product=CA%20Program&query_format=advanced&component=CA%20Certificate%20Compliance&resolution=---&resolution=FIXED&resolution=INVALID&resolution=WONTFIX&resolution=INACTIVE&resolution=DUPLICATE&resolution=WORKSFORME&resolution=INCOMPLETE&resolution=SUPPORT&resolution=EXPIRED&resolution=MOVED&classification=Client%20Software&classification=Developer%20Infrastructure&classification=Components&classification=Server%20Software&classification=Other&classification=Graveyard&short_desc=OCSP&short_desc_type=allwordssubstr&list_id=16376213) and [2](https://sslmate.com/labs/ocsp_watch/)). Concern surrounding OCSP is further elevated considering the disproportionately high cost of offering these services reliably at the global scale of the Web PKI.

## Focusing on simplicity

One of the [10 things we know to be true](https://about.google/philosophy/) is that “it’s best to do one thing really, really well.” Though multipurpose root CAs have offered flexibility in addressing subscriber use cases over the last few decades, they are now faced with satisfying the demands of multiple sets of increasingly rigorous expectations and requirements that do not always align. We believe in the value of purpose-built, dedicated infrastructures and the increased opportunity for innovation that comes along with them. We also think there’s value in reducing the complexity of the Web PKI to increase the security and stability of the ecosystem. By promoting dedicated-use hierarchies, we can better align the related policies and processes that CAs must conform to with the expectations of subscribers, relying parties, and root program operators. 

In [Version 1.1](/Home/chromium-security/root-ca-policy/policy-archive/version-1-1/) of our policy, we announced the Chrome Root Store will only accept applicant root CA certificates that are part of PKI hierarchies dedicated to TLS server authentication certificate issuance.

<span style="text-decoration:underline;">In a future policy update, we intend to introduce: </span>
*   **a phase-out plan for existing multipurpose root CA certificates included in the Chrome Root Store.** Our planning will include CA owner outreach using a CCADB survey as we continue to study and consider the corresponding ecosystem impacts of this proposed change.

<span style="text-decoration:underline;">In a future policy update or CA/Browser Forum Ballot Proposal, we may introduce: </span>
*   **a requirement that would restrict extendedKeyUsage values in new subordinate CA and TLS server authentication subscriber certificates to only “id-kp-serverAuth.”** Modifying the definition of a dedicated TLS PKI hierarchy to prohibit use of other extendedKeyUsage values (i.e., id-kp-clientAuth) in new CA certificates capable of issuing TLS server certificates more closely aligns CA practices with web browser use cases (i.e., securing connections to websites). Further, it eliminates the risk and unintended consequences of other use cases (e.g., IoT) from negatively affecting website authentication.

## Promoting automation

The Automatic Certificate Management Environment (ACME, [RFC 8555](https://www.rfc-editor.org/rfc/rfc8555)) seamlessly allows for server authentication certificate request, issuance, installation, and ongoing renewal across many web server implementations with an extensive set of [well-documented](https://certbot.eff.org/) client options [spanning](https://letsencrypt.org/docs/client-options/#other-client-options) multiple languages and platforms. Unlike proprietary implementations used to achieve automation goals, ACME is open and benefits from continued innovation and enhancements from a robust set of ecosystem participants.

Although ACME is not the first method of automating certificate issuance and management (e.g., [CMP](https://en.wikipedia.org/wiki/Certificate_Management_Protocol), [EST](https://en.wikipedia.org/wiki/Enrollment_over_Secure_Transport), [CMC](https://en.wikipedia.org/wiki/Certificate_Management_over_CMS), and [SCEP](https://en.wikipedia.org/wiki/Simple_Certificate_Enrollment_Protocol)), it has quickly become the most widely used. Today, [over 50%](https://crt.sh/cert-populations) of the certificates issued by the Web PKI rely on ACME. Furthermore, approximately 95% of the certificates issued by the Web PKI today are issued by a CA owner with some form of existing ACME implementation available for customers. A recent survey performed by the Chrome Root Program indicated that most of these CA owners report increasing customer demand for ACME services, with not a single respondent expressing decreasing demand.

Unifying the Web PKI ecosystem in support of ACME will:
*   promote ecosystem agility, 
*   increase resiliency for CA owners and website owners alike, 
*   help website owners address scale and complexity challenges related to certificate issuance,
*   drive innovation through ongoing enhancements and support from an open community,
*   ease the transition to quantum-resistant algorithms, and
*   better position the Web PKI ecosystem to manage risk

in ways that, historically, other automation technologies have been unable to accomplish.

<span style="text-decoration:underline;">In a future policy update, we intend to introduce requirements that all Chrome Root Store applicants must: </span>
*   **be part of PKI hierarchies that offer ACME services for TLS server authentication certificate issuance and management.** Minimally, our program policy will define expectations related to the availability and up-time of ACME services, URL disclosures required in CCADB, the types of certificate issuance ACME services must support, and the minimum privileges that must be provided to the Chrome Root Program for routine evaluation and monitoring of ACME services.
*   **support ACME Renewal Information (ARI, [Draft RFC](https://datatracker.ietf.org/doc/draft-ietf-acme-ari/01/)) to further improve ecosystem agility**. With ARI enabled, CA owners will be able to reduce adverse customer impacts caused by unplanned revocation events and otherwise improve resource management related to routine certificate renewal activities (e.g., rather than a significantly large portion of a certificate corpus being renewed in a narrow window of time, allow the CA to communicate a preferred schedule and facilitate a phased renewal to avoid overextending its resources). ARI looks promising as a mechanism for providing CA and site owners with a more seamless response to the transition to quantum-resistant algorithms without requiring ecosystem resources and support for interim or possibly insecure capabilities (e.g., composite or hybrid certificates). 

## Increasing accountability and ecosystem integrity

We value high-quality, independent audit criteria that result in accurate, detailed, and consistent evaluations of CA owners’ policies and practices, regardless of their intended scope of certificate issuance or geographic location.

<span style="text-decoration:underline;">In a future policy update, we intend to introduce: </span>
*   **a requirement for all non-[TLS server authentication dedicated](https://www.chromium.org/Home/chromium-security/root-ca-policy/#4-dedicated-tls-pki-hierarchies) Technically Constrained subordinate CAs capable of validating to a certificate included in the Chrome Root Store to provide an audit report that expresses non-performance of TLS server authentication certificate issuance.** This audit report may also include an evaluation of the Network and Certificate System Security Requirements for a more complete perspective of the CA’s security posture and the risk to Chrome end users.

## Streamlining and improving domain validation practices

The Baseline Requirements allow for the reuse of data or documents related to previously completed domain validations for up to 398 days. However, with the existing policy requirements in place, it’s possible for a CA to rely upon “stale” information for much longer than this (i.e., a new 398-day validity certificate can be issued that relies on information that’s 397 days old).

<span style="text-decoration:underline;">In a future policy update or CA/Browser Forum Ballot Proposal, we intend to introduce: </span>
*   **reduced domain validation reuse periods**, along with a reduction of the maximum allowed TLS server authentication certificate validity to 90 days. More timely domain validation will better protect domain owners while also reducing the potential for a CA to mistakenly rely on stale, outdated, or otherwise invalid information resulting in certificate mis-issuance and potential abuse.

Multi-perspective Domain Validation (sometimes called [Multi-Vantage-Point Domain Validation](https://www.usenix.org/conference/usenixsecurity21/presentation/birge-lee)) is a promising technology that enhances domain validation methods by reducing the likelihood that routing attacks (e.g., BGP hijacking) can result in fraudulently issued TLS server authentication certificates. Rather than performing domain validation from a single geographic or routing vantage point, which an adversary could influence, multi-perspective domain validation performs the same validation from multiple geographic locations or Internet Service Providers and has been observed as an effective countermeasure against ethically conducted, real-world BGP hijacks.

<span style="text-decoration:underline;">In a future policy update or CA/Browser Forum Ballot Proposal, we may introduce: </span>
*   **requirements for CA owners to perform domain validation and CAA record checking from multiple vantage points.** Specific requirements will minimally define expectations regarding the number and diversity of perspectives, in addition to the size of the quorum that must be in agreement to allow certificate issuance.  