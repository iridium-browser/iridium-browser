---
breadcrumbs:
- - /administrators
  - Documentation for Administrators
page_name: advanced-integration-for-saml-sso-on-chrome-devices
title: Advanced Integration for SAML SSO on Chrome Devices
---

This page describes the advanced integration that **may** be done in enabling
SAML SSO on Chrome Devices. This article is intended for partners, SAML SSO
vendors and IT administrators. The integration described in this document is not
mandatory for the feature to work. For basic information on how SAML SSO can be
enabled for Chrome Devices, please refer to [this
article](https://support.google.com/chrome/a/answer/6060880).

### Chrome Credentials Passing API

API version 1.0 - 3rd June 2014

#### Motivation

Users can sign in to Chrome with their Google accounts. The password is verified
by Google’s authentication servers but is also used locally in Chrome to provide
features such as session lock/unlock, Chrome Device offline sign-in and Chrome
Device user data encryption. For SAML users, authentication is performed by a
third-party identity provider (IdP). This document describes an API that SAML
IdPs can use to securely provide Chrome with the user credentials required to
implement the session lock/unlock, offline sign-in, and data encryption
features.

#### Overview

The Chrome sign-in flow is entirely web-based. Google’s authentication service
redirects to the IdP, the IdP guides the user through the authentication process
and eventually redirects back to a Google URL, reporting success or failure (see
the [Google developer
documentation](https://developers.google.com/google-apps/sso/saml_reference_implementation)).
The IdP’s authentication flow typically consists of at least three steps:

    an HTML login form collects user credentials

    the IdP verifies that the credentials submitted are correct

    an HTML page performs the redirect to the Google URL, indicating
    success/failure

This document introduces a JavaScript API via which the IdP can pass user
credentials to Chrome in step 1 and can inform Chrome when the credentials have
been verified in step 3. The API works locally: API methods are called by a
script that is executed by Chrome as part of the SAML IdP’s web-based
authentication flow and are processed by the same instance of Chrome. They do
not flow through any server. The API methods are provided by a JavaScript file
that handles the lower-level communication with Chrome. This file is hosted by
Google at:

<https://ssl.gstatic.com/accounts/chrome/users-1.0.js>

We recommend that SAML IdPs reference this file at the above location in their
scripts. This way, any improvements and bug fixes to the file will be picked up
automatically. We will make every effort to keep the API provided by this file
stable.

#### Key Types

To support session lock/unlock, offline sign-in and user data encryption, Chrome
must be able to authenticate a returning user without contacting any servers.
This is done by deriving a key from the user’s password via a one-way hash
function during initial authentication. When the user returns and enters his/her
password again, the same one-way hash function is applied to this password. If
the resulting keys match, the password entered was correct and the user is
authenticated.

Chrome uses a built-in one-way hash function to derive keys from passwords for
non-SAML users. The same one-way hash function is used when a SAML IdP passes a
user’s password to Chrome via the API described in this document. Chrome
discards the password after hashing and does not store it anywhere. An
alternative is for the IdP to apply a one-way hash function and pass the derived
key to Chrome. When the user returns and enters his/her password, Chrome must
apply the same one-way hash function to determine whether the password entered
is correct. The IdP must therefore use a one-way hash function supported by
Chrome and must also pass Chrome any additional metadata required to repeat the
hashing, such as a salt.

Chrome provides the SAML IdP with a list of supported key derivation mechanisms
when the IdP initializes the API. KEY_TYPE_PASSWORD_PLAIN is always supported.
For this key type, the IdP passes the password to Chrome and Chrome applies its
built-in one-way hash function to it. Additional key types, in which the IdP
applies a one-way hash function and passes the key and metadata to Chrome, will
be added in the future.

#### API Methods

The API consists of three JavaScript methods:

    initialize(callback)

    add(details, callback)

    complete(details, callback)

All methods are asynchronous. Each method will invoke the callback passed to it
when its work is done. The initialize method must be called first to initialize
the API. The add method is then used to pass credentials to Chrome and the
complete method is used to inform Chrome that the passed credentials have been
verified. IdP login flows typically span multiple HTML login pages. The API
supports this by allowing its methods to be called from different pages.

However, the API must correlate the complete call that indicates authentication
success with the add call that passed the corresponding credentials. This
correlation is established using the token argument: The SAML IdP passes a token
of its choosing in the details provided to the add call and must then pass the
same token in the details of the corresponding complete call. The API uses the
token to correlate these method calls only. The token is treated as opaque, is
not interpreted in any way and is discarded at the end of the sign-in process.

If authentication fails, Chrome may return to the sign-in form, asking the user
to try signing in again. A sign-in process can thus encompass multiple
authentication attempts. To ensure that method calls are correlated correctly,
the SAML IdP should use a different token for each of these authentication
attempts. The IdP must then choose a token when calling add and must keep this
token as part of its internal state until complete is called. The need to
generate and keep track of a suitable token can be avoided by using the
RelayState (see the [Google SAML developer
documentation](https://developers.google.com/google-apps/sso/saml_reference_implementation)
for a definition) as the token. The RelayState meets all requirements that the
token must fulfill:

    The RelayState remains constant throughout an authentication attempt

    The RelayState is different for each authentication attempt

    The RelayState must be passed through from the start to the end of the
    authentication flow so that it will be available in steps 1 and 3 of the
    flow

While the RelayState is an obvious choice for the token, a SAML IdP is free to
choose any other token instead, as long as it allows the add and complete calls
to be correlated.

> ## initialize(callback)

> The initialize method should be invoked once at the start of the
> authentication flow. While the API is currently implemented by Chrome only, a
> SAML IdP is not required to (and should not) interpret the user agent string
> to determine whether the API is available or not. Instead, the initialize
> method should always be called at the start of the authentication flow,
> regardless of user agent.

> If the authentication attempt is part of a sign-in process on a version of
> Chrome that provides the API, Chrome will invoke the callback with a list of
> supported key types. Once the callback has been invoked by Chrome, the IdP may
> use the API by calling the add and complete methods.

> In all other cases (e.g., not Chrome, older version of Chrome, not part of the
> sign-in process), the callback will not be invoked. If the callback is not
> invoked, the API is not available and the IdP must not call any of its other
> methods. If the IdP’s authentication flow spans multiple HTML pages, the IdP
> must keep track of the initialization result across these pages. If the API is
> available, the IdP should call the add and complete methods at the appropriate
> points in the authentication flow. Otherwise, the add and complete methods
> must not be called.

> The callback should be a function that takes a single argument, keyTypes.
> Chrome passes an array of string constants indicating the supported key
> derivation mechanisms via this argument (see the key types section for
> details). The constant ’KEY_TYPE_PASSWORD_PLAIN’ will always be present in
> this array. Additional constants indicating further key derivation mechanisms
> will be added in the future.

> ## add(details, callback)

> This method should be invoked when the user has finished entering his/her
> credentials. The details should be a JavaScript object that contains the
> following fields:

> token
> user
> passwordBytes
> keyType

> The token is used to correlate the add call with a subsequent complete call.
> It is not interpreted by the API in any way and may be freely chosen by the
> IdP (see the API methods section for details). The user field should be set to
> the user’s e-mail address. If the IdP does not know the user’s e-mail address,
> an empty string should be passed.

> The next two fields are used to pass a key to Chrome. keyType indicates the
> key derivation mechanism that the IdP used to derive the key from the user’s
> password. It must be one of the string constants that the API returned in the
> keyTypes array during initialization (see the previous section for details).
> passwordBytes contains the key and any metadata required to repeat the hashing
> process that was used to derive the key.

> When the keyType is ’KEY_TYPE_PASSWORD_PLAIN’, passwordBytes contains the
> password itself. For other key types that will be added in the future,
> passwordBytes will contain the metadata, such as a salt, and the key,
> separated by a delimiter.

> The callback will be invoked when Chrome has received the credentials. Since
> the user has not been authenticated yet, Chrome keeps the credentials in
> memory only and does not store them permanently until the corresponding
> complete method is called.

> The add method will typically be called when the HTML login form is about to
> submitted. It is important to not actually submit the form until after the
> callback has been invoked. If the IdP calls the add method and submits the
> form without waiting for the callback, the HTML page containing the login form
> and any scripts it is running it will be torn down immediately, preventing the
> credentials from reaching Chrome. The recommended way to handle form
> submission is to set the callback to form.submit.bind(form), where form is the
> DOM node representing the login form. This way, the form will be submitted
> automatically when Chrome has received the credentials and invokes the
> callback.

> If the credentials entered are incorrect, the IdP will typically end the
> authentication flow with a SAMLResponse indicating failure. However, an IdP
> may also redirect back to the IdP’s login form, allowing the user to try
> entering his/her credentials again. If the IdP implements this flow, the add
> method should be called whenever new credentials are entered. It is
> permissible to reuse the same token in this case. When the add method is
> called with new credentials, any credentials previously passed with the same
> token are superseded and replaced.

> ## complete(details, callback)

> The complete method should be invoked when the user’s credentials have been
> verified by the SAML IdP and the user is authenticated. The details should be
> a JavaScript object that contains one field, token. The token must match the
> one that was passed to the add call for this authentication attempt, allowing
> the two calls to be correlated. The complete call indicates that the
> credentials which the IdP had passed to the corresponding add call are valid.
> Chrome will use these credentials to provide the session lock/unlock, offline
> sign-in, and data encryption features for this user. The callback will be
> invoked when Chrome has processed the credentials and has stored them
> permanently.

> After verifying the user’s credentials, an IdP needs to redirect to a Google
> URL, passing the RelayState and SAMLResponse. This is typically done by
> serving an HTML page that contains a hidden form which is submitted to the
> Google URL automatically upon page load. It is important to not actually
> submit the form until after the callback has been invoked. The recommended way
> to handle this is to call the complete method upon page load, setting the
> callback to form.submit.bind(form), where form is the DOM node representing
> the hidden form. This way, the form will be submitted automatically when
> Chrome has processed the credential and invokes the callback.

> If the credentials provided are incorrect, the IdP must not call the complete
> method. When the SAML authentication flow completes with a SAMLResponse
> indicating authentication failure, Chrome will inform the user of the
> authentication failure and will allow him/her to try again, starting a new
> authentication flow.

#### Limitations

The API can be used to securely pass a key derived from the user’s password to
Chrome. If the user authenticates using other means (e.g., smart card,
biometrics, single-use codes) and does not have a password, the API cannot be
used as no key can be derived. Support for other authentication types is a
separate problem outside the scope of the API at this time.

The API is available in Chrome 36 and higher. Earlier Chrome versions behave
like other user agents that do not support the API: It is safe to call the
initialize method. Since the API is not supported, the callback passed to
initialize will never be invoked.
