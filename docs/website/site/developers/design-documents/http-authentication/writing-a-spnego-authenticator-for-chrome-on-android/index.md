---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
- - /developers/design-documents/http-authentication
  - HTTP authentication
page_name: writing-a-spnego-authenticator-for-chrome-on-android
title: Writing a SPNEGO Authenticator for Chrome on Android
---

As described in [Kerberos for Chrome on
Android](https://docs.google.com/document/d/1G7WAaYEKMzj16PTHT_cIYuKXJG6bBcrQ7QQBQ6ihOcQ/edit?usp=sharing),
in Chrome M46 third parties can enable SPNEGO authentication in Chrome for
Android. To do this they must provide a SPNEGO Authenticator. This page
describes the interface between Chrome and the SPNEGO Authenticator.

# Basics

The SPNEGO Authenticator is provided by an Android Service. This must be
incorporated in an app, provided by the third party, installed on the user’s
device. The app is responsible for managing any accounts used for SPNEGO
authentication, and for all communication with the SPNEGO server.

The SPNEGO Authenticator is an Android AccountAuthenticator. As such it must
follow the pattern described in
[AbstractAccountAuthenticator](http://developer.android.com/reference/android/accounts/AbstractAccountAuthenticator.html).
In particular it must implement an authenticator class derived from
AbstractAccountAuthenticator.

The SPNEGO Authenticator must define a new account type. Its name should be
derived from the writer’s domain name (e.g. com.example.spnego). As explained in
[Kerberos for Chrome on
Android](https://docs.google.com/document/d/1G7WAaYEKMzj16PTHT_cIYuKXJG6bBcrQ7QQBQ6ihOcQ/edit?usp=sharing),
the account type must be defined to use
[customTokens](http://developer.android.com/reference/android/R.attr.html#customTokens).
The account type must support the “SPNEGO” feature
(HttpNegotiateConstants.SPNEGO_FEATURE).

# Interface to Chrome

Chrome finds the SPNEGO authenticator through the Android account type it
provides. This is defined by the authenticator, and passed to Chrome through the
AuthAndroidNegotiateAccountType policy.

The interface to Chrome is through the Android account management framework, and
in particular through
[AbstractAccountManager.getAuthToken](http://developer.android.com/reference/android/accounts/AbstractAccountAuthenticator.html#getAuthToken(android.accounts.AccountAuthenticatorResponse,%20android.accounts.Account,%20java.lang.String,%20android.os.Bundle)).
Chrome, in
[org.chromium.net.HttpNegotiateConstants](https://code.google.com/p/chromium/codesearch#chromium/src/net/android/java/src/org/chromium/net/HttpNegotiateConstants.java&q=HttpNeg&sq=package:chromium&l=10),
defines some additional keys and values that are used in the arguments to
getAuthToken, and in the returned result bundle.

## getAuthToken arguments

When
[getAuthToken](http://developer.android.com/reference/android/accounts/AbstractAccountAuthenticator.html#getAuthToken(android.accounts.AccountAuthenticatorResponse,%20android.accounts.Account,%20java.lang.String,%20android.os.Bundle))
is called the authTokenType will be "SPNEGO:HOSTBASED:&lt;spn&gt;" where
&lt;spn&gt; is the principal for the request. This will always be a host based
principal (in the current implementation; future versions may allow other types
of principal, but if they do so they will use a different prefix. SPNEGO
Authenticators should check the prefix).

The options bundle will contain the keys:

    [KEY_CALLER_PID](http://developer.android.com/reference/android/accounts/AccountManager.html#KEY_CALLER_PID)

    [KEY_CALLER_UID](http://developer.android.com/reference/android/accounts/AccountManager.html#KEY_CALLER_UID)

    HttpNegotiateConstants.KEY_CAN_DELEGATE - True if delegation is allowed,
    false otherwise.

If this is the second or later round of multi-round authentication sequence it
will also contain.

    HttpNegotiateConstants.KEY_INCOMING_AUTH_TOKEN - The incoming token from the
    WWW-Authenticate header, Base64 encoded.

    HttpNegotiateConstants.KEY_SPNEGO_CONTEXT - The SPNEGO context provided by
    the authenticator on the previous round. This is a bundle. Chrome treats
    this as an opaque object and simply preservers it between rounds.

## getAuthToken result bundle

The final result bundle of
[getAuthToken](http://developer.android.com/reference/android/accounts/AbstractAccountAuthenticator.html#getAuthToken(android.accounts.AccountAuthenticatorResponse,%20android.accounts.Account,%20java.lang.String,%20android.os.Bundle))
(returned either as the return value of getAuthToken, or through the
[AccountAuthenticatorResponse](http://developer.android.com/reference/android/accounts/AccountAuthenticatorResponse.html))
should contain the account name, account type, and token as defined in the
Android documentation. The token should be Base64 encoded. In addition the
bundle should contain the keys:

    HttpNegotiateConstants.KEY_SPNEGO_RESULT - the SPNEGO result code. This
    should be one of the values defined in
    [HttpNegotiateConstants](https://code.google.com/p/chromium/codesearch#chromium/src/net/android/java/src/org/chromium/net/HttpNegotiateConstants.java&q=HttpNeg&sq=package:chromium&l=10).

    HttpNegotiateConstants.KEY_SPNEGO_CONTEXT - a context to be returned to the
    authenticator in the next authentication round. This is only required if
    authentication is incomplete.

# Implementation recommendations

    Each account provided by an SPNEGO Account Authenticator should correspond
    to a single user principal (account) provided by single key distribution
    center.

    The account authenticator should not store any passwords. Instead it should
    store TGTs for the user principles, and require the users to re-enter their
    passwords (or other authentication data) when their TGT expires.

    The account authenticator should keep a list of authorized applications (or
    application signatures) for each account, and refuse to provide service
    tokens to other applications. The list of authorized applications may be:

        Built into the account authenticator.

        Configurable by the system administrator

        Configurable by the user. In this case the account authenticator might
        choose to allow the user to authorize new applications dynamically when
        they first request access.

The authenticator can get the uid of the calling app using the KEY_CALLER_UID
field of the options bundle, and then identify the requesting application using
context.getPackageManager().getNameForUid() or similar.

This is required to ensure that malicious apps run by the user cannot use the
user’s credentials to access services in unintended ways. This is particularly
important since using the custom tokens option (as described above) disables
Android’s own signature check when getting auth tokens.

    Unless it is built into the account authenticator, the system administrator
    or user will need to be able to configure the location of the key
    distribution center.

Error codes displayed in Chrome

In addition to the error codes that can be forwarded from the authenticator app,
the following errors can be displayed by Chrome when trying to authenticate a
request:

*   ERR_MISSING_AUTH_CREDENTIALS: The account information is not usable.
            It can be raised for example if the user did not log in to the
            authenticator app and no eligible account is found, if the account
            information can't be obtained because the current app does not have
            the required permissions, or if there is more than one eligible
            account and we can't obtain a selection from the user.
*   ERR_UNEXPECTED: An unexpected error happened and the request has
            been terminated.
*   ERR_MISCONFIGURED_AUTH_ENVIRONMENT: The authentication can't be
            completed because of some issues in the configuration of the app.
            Some permissions may be missing.

Please search for the **cr_net_auth** tag in logcat to have more information
about the cause of these errors.

# Use, and testing, with Chrome

Chrome defines [a number of policies for controlling the use of SPNEGO
authentication](/administrators/policy-list-3#HTTPAuthentication). In particular
to enable SPNEGO authentication the
[AuthServerWhitelist](/administrators/policy-list-3#AuthServerWhitelist) must
not be empty, and the
[AuthAndroidNegotiateAccountType](http://www.chromium.org/administrators/policy-list-3#AuthAndroidNegotiateAccountType)
must match the account type provided by the SPNEGO authenticator.

To simplify testing of SPNEGO authentication Chrome on Android supports command
line options corresponding to these policies. These are
“--auth-server-whitelist=*&lt;whitelist&gt;*” and
“--auth-spnego-account-type=*&lt;account type&gt;*”. To set these, [add them to
the command line on the
device](/developers/how-tos/run-chromium-with-flags#TOC-Setting-Flags-for-Chrome-on-Android)
(requires a rooted device).
