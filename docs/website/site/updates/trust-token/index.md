---
breadcrumbs:
- - /updates
  - updates
page_name: trust-token
title: Trust Token API
---

*   [Getting Started with Trust Tokens](https://web.dev/trust-tokens/) -
            Developer focused explanation of Trust Token.
*   **[API
            Explainer](https://github.com/WICG/trust-token-api/blob/master/README.md)**
            - high-level description of the problem and initial sketch of the
            interface
*   [Chrome Trust Token Design
            Doc](https://docs.google.com/document/d/1TNnya6B8pyomDK2F1R9CL3dY10OAmqWlnCxsWyOBDVQ/edit)
            - currently the normative source for the HTTP API
*   [Using the prototype Trust Token
            API](https://docs.google.com/document/u/1/d/1qUjtKgA7nMv9YGMhi0xWKEojkSITKzGLdIcZgoz6ZkI/edit)
            - describes the interface as implemented, with examples and error
            conditions
*   [Prototype Issuer Library](https://github.com/google/libtrusttoken)
            - C library for TrustTokenV2.

**Experimenting**

You can manually enable this feature on your build of Chrome by using the
"--enable-features=TrustTokens" command line flag (or setting
chrome://flags/#trust-tokens to enabled); in order to execute Trust Tokens
operations, you'll need an origin trial token present *or* to have provided the
additional (...and unfortunately wordy) command-line flag
--enable-blink-features=TrustTokens,TrustTokensAlwaysAllowIssuance. If you are
experimenting with a new issuer, you can manually provide the Trust Token key
commitments via the “--additional-trust-token-key-commitments=’{ "&lt;issuer
origin&gt;": &lt;key commitment response&gt; }’” flag.

Process for registering as an issuer:
<https://docs.google.com/document/d/1cvUdAmcstH6khLL7OrLde4TnaPaMF1qPp3i-2XR46kU/>

If you are trying to register as a developer to use the Trust Token APIs to
issue/redeem, please follow the standard Origin Trial registration process:
<https://github.com/GoogleChrome/OriginTrials/blob/gh-pages/developer-guide.md>

If you have questions/suggestions related to the web API or protocol that needs
clarification, please file an issue at:
<https://github.com/WICG/trust-token-api/issues/>

If you have questions/suggestions related to the Chrome origin trial or
implementation, please file a bug at: [Chromium Bug
Tracker](https://bugs.chromium.org/p/chromium/issues/entry?components=Internals%3ENetwork%3ETrustTokens)

**Launch Timeline**

Last updated November 11, 2021.

The Trust Token API has been running in Origin Trial since Chrome 84, running at
50% on Dev/Canary/Beta and 10% on Stable. It is currently running through to
Chrome 94.

Chrome 84-88 supports
[TrustTokenV1](https://github.com/WICG/trust-token-api/tree/36da1948de580fa4efb61a3ec324a608edca8c68)
which includes verification of the Redemption Record.

Starting in Chrome 88, the Trust Token API in Chrome will will support
[TrustTokenV2](https://github.com/WICG/trust-token-api/) which renames a few
APIs and no longer verifies the Redemption Record, allowing issuers to use it as
a free-form record. Additionally, TrustTokenV2 supports two operating modes, a
faster variant based on VOPRFs that supports 6 public buckets (using 6 keys in
the key commitment) and the slower private metadata variant using PMBTokens that
supports 3 public buckets and 1 private biut (using 3 keys in the key
commitments).

Key commitments for Chrome 88 onward can omit the 'srrkey' field in the key
commitment.

Starting in Chrome 92, the Trust Token API in Chrome will partially support
TrustTokenV3 which changes the format of the key commitment to allow for better
support across Trust Token versions. Starting in Chrome 93, the Trust Token API
in Chrome will fully support TrustTokenV3 including the switch to using P256 for
the signing algorithm. To support migration between TrustTokenV2 and
TrustTokenV3, Chrome's commitment fetcher will support parsing a 'hybrid'
commitment format that contains both the V2 and V3 key commitments until Chrome
92 reaches stable:

> {

> "TrustTokenV3...": { ... }, // V3 commitment (parsed by Chrome 92 and above)

> // V2 commitment (parsed by Chrome 91 and below)

> "protocol_version": "TrustTokenV2...",

> "id": ...,

> ...

> }

To prevent ecosystem burn-in of the long origin trial, we will be temporarily
disabling the API from November 18th to December 2nd.

For the full Chrome release schedule, [see
here](https://chromiumdash.appspot.com/schedule).