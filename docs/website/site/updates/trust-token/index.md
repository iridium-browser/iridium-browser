---
breadcrumbs:
- - /updates
  - updates
page_name: private-state-token
title: Private State Token API
---

*   [Getting Started with Private State Tokens](https://web.dev/trust-tokens/) - Developer focused explanation of Private State Tokens (formerly known as Trust Tokens).
*   **[API Explainer](https://github.com/WICG/trust-token-api/blob/master/README.md)** - high-level description of the problem and initial sketch of the interface.
*   [Chrome Trust Token Design Doc](https://docs.google.com/document/d/1TNnya6B8pyomDK2F1R9CL3dY10OAmqWlnCxsWyOBDVQ/edit) - currently the normative source for the HTTP API.
*   [Using the prototype Private State Tokens API](https://docs.google.com/document/u/1/d/1qUjtKgA7nMv9YGMhi0xWKEojkSITKzGLdIcZgoz6ZkI/edit) - describes the interface as implemented, with examples and error conditions.
*   [Prototype Issuer Library](https://github.com/google/libtrusttoken) - C library for Private State Tokens (PrivateTokenV2+).

**Experimenting**

In order to execute Private State Token operations, you'll need an origin trial token present *or* to have provided the additional command-line flag `--enable-blink-features=PrivateStateTokensAlwaysAllowIssuance --enable-features=PrivateStateTokens,PrivacySandboxSettings3`.
If you are experimenting with a new issuer, you can manually provide the Private State Token key commitments via the `--additional-private-state-token-key-commitments='{ "<issuer
origin>": <key commitment response> }'` flag.

If you have questions/suggestions related to the web API or protocol that needs clarification, please file an issue at: <https://github.com/WICG/trust-token-api/issues/>

If you have questions/suggestions related to the Chrome origin trial or implementation, please file a bug at: [Chromium Bug Tracker](https://bugs.chromium.org/p/chromium/issues/entry?components=Internals%3ENetwork%3ETrustTokens)
