---
breadcrumbs:
- - /Home
  - Chromium
- - /Home/chromium-security
  - Chromium Security
- - /Home/chromium-security/root-ca-policy
  - Root Program Policy
page_name: apply-for-inclusion
title: Apply for Inclusion
---

## Last updated: 2023-01-25

The Chrome Root Program policy defines the [minimum requirements](/Home/chromium-security/root-ca-policy/) that must be met by Certification Authority (CA) owners for both initial and continued inclusion in the Chrome Root Store.

Google includes or removes self-signed root CA certificates in the Chrome Root Store as it deems appropriate at its sole discretion. The selection and ongoing inclusion of CA certificates is done to enhance the security of Chrome and promote interoperability. CA certificates that do not provide a broad service to all browser users will not be added to, or may be removed from the Chrome Root Store. CA certificates included in the Chrome Root Store must provide value to Chrome end users that exceeds the risk of their continued inclusion.

### Inclusion Processing

The Chrome Root Program and corresponding Root Store processes inclusion requests and requests for changes through the Common CA Database (CCADB). CA owners who satisfy all of the requirements in the Chrome Root Program [policy](/Home/chromium-security/root-ca-policy/) may apply.

The application process includes:

1.  A CA owner [requests](https://www.ccadb.org/cas/request-access) and gains access to CCADB (if not already granted access).
2.  A CA owner adds a root CA certificate to CCADB and completes an “[Add/Update Root Request](https://www.ccadb.org/cas/updates)” in CCADB, with all tabs populated with information.
3.  A CA owner submits a “[Root Inclusion Request](https://www.ccadb.org/cas/inclusion)” in CCADB.
4.  The Chrome Root Program performs an initial review of the information included in CCADB to ensure completeness and compliance with the minimum requirements.
5.  A [CCADB public discussion](https://www.ccadb.org/cas/public-group) period ensues.
6.  The Chrome Root Program performs a detailed review of all information provided in CCADB and publicly available (to include output from the CCADB public discussion).
7.  The Chrome Root Program makes a final determination and communicates it to the CA owner.

Typically, applications are processed on a first-in, first-out basis, with priority given to those replacing an existing root CA certificate already included in the Chrome Root Store. The Chrome Root Program takes as much time to process applications as needed to ensure user security, and makes no guarantees on application processing time. The Chrome Root Program may apply additional application review weighting criteria as it sees necessary or valuable to Chrome user security. At any point, the Chrome Root Program may contact the applicant during its review seeking additional or clarifying information. Applicants are expected to provide the requested information in a timely manner.

Ultimately, in order for a CA owner’s inclusion request to be accepted, it must clearly demonstrate the value proposition for the security and privacy of Chrome’s end users exceeds the corresponding risk of inclusion.

Root CA certificates approved for distribution will be added to the Chrome Root Store on approximately, but not limited to, a quarterly basis. However, the Chrome Root Program offers no guarantees related to the timeliness of CA certificate distribution.

### Inclusion Rejection

The Chrome Root Program will reject inclusion requests where an applicant does not meet the minimum requirements defined by the Chrome Root Program [policy](/Home/chromium-security/root-ca-policy/).

The Chrome Root Program may reject requests for inclusion into the Chrome Root Store as deemed appropriate, and is not obligated to justify any inclusion decision.

Illustrative factors for application rejection may include:

-   a failure to demonstrate broad value for Chrome users and why the benefits of inclusion outweigh the risks to user safety and privacy.
-   a corresponding Public Key Infrastructure (PKI) certificate hierarchy where leaf certificates are not primarily intended to be used for server authentication facilitating a secure connection between a web browser and a corresponding website (e.g., client authentication certificates, Internet of Things (IoT) device certificates, smart cities, transportation, medical devices, etc.).
-   a corresponding PKI hierarchy that currently or previously allowed, facilitated, or enabled “Monster in the Middle” (MITM) attacks (either successful or attempted) where a certificate was issued for the purposes of impersonation, interception, or to alter communications.
-   where the corresponding CA owner has ever been:
    -   determined to have acted in an untrustworthy manner or created unnecessary ecosystem risk, or
    -   associated with a certificate that was previously distrusted by Chrome or any other public root program.
-   has an incident history that does not convey the [factors](https://www.chromium.org/Home/chromium-security/root-ca-policy/#7-responding-to-incidents:~:text=7.%20Responding%20to%20Incidents) significant to Chrome.
-   completion of a CCADB root inclusion public discussion that casts doubt over the CA owners security, honesty or reliability.
-   discovery of false or misleading information provided by the CA owner.
-   significant delays in response from the CA owner when seeking additional or clarifying information.

Actions in this list are only illustrative and considerations for rejection are not limited to this list.

Depending on the reason for application rejection, the Chrome Root Program, at its sole discretion, may:

-   require a period of time to elapse before the CA owner may re-apply, or    
-   reject all future applications from the CA owner.