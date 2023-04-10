---
breadcrumbs: []
page_name: security-keys
title: Security Keys
---

Security Keys Policy

This section documents Chrome's policy around Security Keys. Questions may be
directed to security@chromium.org.

**Registration and authentication attempts by sites**

Web sites can attempt to register or authenticate Security Keys at any time.
This ability should not be used in an abusive manner, in a way designed to
compromise the privacy of the user, or in a way that triggers excessive
notifications in Chrome or by the devices themselves.

If Chrome becomes aware of abuses of this ability we may withdraw Security Key
privileges from certain websites, or alter our behaviour in other ways to
ameliorate the problem.

**Site attestation requirements**

Chrome’s users have an interest in ensuring a healthy and interoperable
ecosystem of Security Keys. To this end, public websites that restrict the set
of allowed Security Keys should do so based on articulable, technical
considerations. They should regularly update their set of trusted attestation
roots that meet their policies (for example, from the FIDO Metadata Service) to
ensure that new Security Keys that meet their requirements will function.

Chrome expects Security Key manufacturers who have passed FIDO certification to
keep their attestation metadata updated in the FIDO Metadata Service.

If Chrome becomes aware that websites are not meeting these expectations, we may
withdraw Security Key privileges from those sites. If Chrome becomes aware that
manufacturers are not maintaining their attestation metadata we may choose to
disable attestation for those devices in order to ensure a healthy ecosystem.

**Attestation Batches**

Security Keys are able to present an attestation certificate when registering
with websites. These attestation certificates are intended to provide assurance
to the site about the level of protection provided by the Security Key. However,
they are also a privacy concern if the certificate is unique to a small number
of Security Keys.

Google Chrome expects Security Keys to meet or exceed the FIDO guidance in this
respect and thus have attestation certificates that cover at least 100,000
devices. When starting a new batch, manufacturers may temporarily have fewer
than 100,000 devices using a given certificate. But, in the long run, Chrome
expects at least 100,000 to share a certificate. However, Chrome expects at
least 100,000 devices to share each new certificate before a new batch is begun.

If Chrome becomes aware that certain Security Keys are not meeting these
requirements we may choose to disable attestation for those devices in order to
protect the privacy expectations of our users.

(This does not apply to registrations where Chrome has signaled to a Security
Key that it is operating in an enterprise context as detailed above—certificates
may be individually identifying in this case.)

If Chrome becomes aware that devices are overly identifiable via other aspects
of the protocol we may choose to cease allowing registrations, or to cease all
interoperation with these devices for the same reason.

**User presence tests**

Some Security Keys are required to confirm user presence before performing
certain operations. This is expected to prevent silent registration and silent
authentication, both of which could violate our users’ privacy expectations.
Chrome requires Security Keys to conform to the relevant specifications and to
effectively implement these checks when required.

If Chrome becomes aware that certain Security Keys can be abused to allow silent
registration or silent authentication we may choose to cease interoperating with
these devices in order to protect the privacy expectations of our users.

**User verification tests**

Some Security Keys are capable of verifying the presence of a specific person,
for example with a fingerprint sensor or PIN entry. Chrome expects these
Security Keys to effectively protect this personal information.

If Chrome becomes aware that certain Security Keys are disclosing personal
information, such as PINs or fingerprint hashes, then we may choose to cease
interoperating with these devices in order to protect user privacy.

Attestation Behavior

Chrome supports two APIs for interacting with Security Keys: U2F (based on the
[FIDO U2F
API](https://fidoalliance.org/specs/fido-u2f-v1.2-ps-20170411/fido-u2f-javascript-api-v1.2-ps-20170411.html),
but supported via postMessage and an extension) and
[webauthn](https://github.com/w3c/webauthn). (The latter may need to be enabled
with [a flag](javascript:void(0);).)

Prior to Chrome 66, the U2F API always returned attestation information directly
from security keys without obtaining user consent, while the webauthn API was
unavailable.

Starting with Chrome 66, the U2F API has been altered to align it with webauthn:
an additional member of the
[RegisterRequest](https://fidoalliance.org/specs/fido-u2f-v1.2-ps-20170411/fido-u2f-javascript-api-v1.2-ps-20170411.html#dictionary-registerrequest-members)
object is supported that mirrors
[AttestationConveyancePreference](https://w3c.github.io/webauthn/#attestation-convey)
from webauthn. Sites that have been using the U2F API will experience a change
in behavior as the default will no longer cause the device's attestation
information to be returned. To get the old behavior, sites should add an
"attestation" member to the
[RegisterRequest](https://fidoalliance.org/specs/fido-u2f-v1.2-ps-20170411/fido-u2f-javascript-api-v1.2-ps-20170411.html#dictionary-registerrequest-members)
object with the value "direct". However, they should note that this will trigger
a permission prompt.

Additionally, the attestation behavior is affected by the
[SecurityKeyPermitAttestation](/administrators/policy-list-3#SecurityKeyPermitAttestation)
enterprise policy. This list can contain either U2F AppIDs (which are full URLs)
or webauthn RP IDs (which are domains). Its affect on attestation is detailed in
the following tables:

**U2F**

<table>
<tr>
<td><b>"attestation" value</b></td>
<td><b>AppID not listed in policy</b></td>
<td><b>AppID listed in policy</b></td>
</tr>
<tr>
<td>"none" / not given</td>
<td>Fresh, self-signed certificate returned</td>
<td>Fresh, self-signed certificate returned</td>
</tr>
<tr>
<td>"indirect" / "direct"</td>
<td>User prompted for consent. If granted, attestation from device is returned. Otherwise, fresh, self-signed certificate is returned.</td>
<td>Attestation from device is returned (with "individual attestation" hint sent to the device).</td>
</tr>
</table>

**webauthn**

**<table>**
**<tr>**
**<td>"attestation" value</td>**
**<td>RP ID not listed in policy</td>**
**<td>RP ID listed in policy</td>**
**</tr>**
**<tr>**
**<td>"none" / not given</td>**
**<td>Empty, "none" attestation returned</td>**
**<td>Empty, "none" attestation returned</td>**
**</tr>**
**<tr>**
**<td>"indirect" / "direct"</td>**
**<td>User prompted for consent. If granted, attestation from device is returned. Otherwise a permission error is generated.</td>**
**<td>Attestation from device is returned.</td>**
**</tr>**
**<tr>**
**<td>"enterprise"</td>**
**<td>Same as for "direct".</td>**
**<td>Attestation from device is returned (with "individual attestation" hint sent to the device).</td>**
**</tr>**
**</table>**

(The behavior of "indirect" attestation in webauthn may change in the future
but, for now, it is identical to "direct".)

Individual Attestation

Security Keys are required to batch attestation information across at least
100 000 devices to preserve privacy (see below). However, in some enterprise
contexts it is desirable to be able to track individual security keys. To enable
this the
[SecurityKeyPermitAttestation](/administrators/policy-list-3#SecurityKeyPermitAttestation)
enterprise policy can cause a signal to be sent to security keys to indicate
that an individual attestation certificate may be returned for some
registrations.

For U2F, the most-significant bit of the P1 byte in a
[U2F_REGISTER](https://fidoalliance.org/specs/fido-u2f-v1.2-ps-20170411/fido-u2f-raw-message-formats-v1.2-ps-20170411.html#command-and-parameter-values)
message will be set when an AppID is listed in the policy. It is up to the
Security Key to act on this if it wishes.

For webauthn, that bit will be set if the RP ID is listed in the policy and
"enterprise" attestation was requested. This is because RP IDs are coarser than
AppIDs and so enumeration in the policy may not offer enough control.

Inadequately batched attestation certificates

Some security keys have been found to contain attestation certificates that do
not meet the 100 000-batch requirement. Specifically, older models of the
following products contain individually unique certificates:

*   KEY-ID FIDO U2F security key
*   HyperFIDO K5
*   HyperFIDO Mini

Because of this, attestation certificates with an issuer of "FT FIDO 0100" will
be considered to be individually identifying certificates in Google Chrome 67
and later. Such certificates will be replaced with a fresh, self-signed
certificate in U2F and will result in an empty, "none" attestation in
webauthn—even when direct attestation is requested.

This measure is expected to also affect the following products, which we suspect
have the same issue but which are not clearly commercially available and thus we
have not tested:

*   Feitian ePass FIDO USB

Users of all these products could, potentially, find that they are unable to
register these security keys when using Chrome 67 or later. However, at the time
of writing, testing shows no impact in practice.

The SecurityKeyPermitAttestation enterprise policy will override this behaviour
and cause the individually identifying certificates to be returned for the
enumerated AppIDs and RP IDs. (See Individual Attestation, above.)
