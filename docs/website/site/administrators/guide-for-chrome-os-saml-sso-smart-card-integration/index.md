---
breadcrumbs:
- - /administrators
  - Documentation for Administrators
page_name: guide-for-chrome-os-saml-sso-smart-card-integration
title: Guide for Chrome OS SAML SSO smart card integration
---

## Objective

Document the requirements for third-party Identity Providers in order to
integrate with Chrome OS smart card based user login.

## Background

Starting from version 83, Chrome OS supports authenticating OS users using smart
cards (instead of passwords). A smart card is a physical device that can
securely store private keys and certificates, and, when inserted into a smart
card reader, can be used in order to perform private key operations and
authenticate the user.

The Chrome OS smart card based user authentication is based on the SAML SSO
functionality. This means that the smart card based authentication has to be set
up by the administrator on the side of the third-party identity provider (IdP)
that is used in the given Chrome OS deployment.

After the user successfully authenticated using a smart card at the IdP website
on Chrome OS Login Screen, the user profile associated with the certificate will
be created on the Chromebook. Subsequent logins of this user may then be handled
by Chrome OS in the “offline” mode, without reaching out to the IdP (note that
this can be customized using the
[SAMLOfflineSigninTimeLimit](https://cloud.google.com/docs/chrome-enterprise/policies/?policy=SAMLOfflineSigninTimeLimit)
policy).

Note that the smart card based authentication is NOT implemented for regular
Gaia users.

## Overview

Requirements for the Identity Provider in order to be compatible with Chrome OS
smart card based user authentication:

    The authentication should be performed using the standard TLS client
    authentication.
    Exactly one certificate from the smart card has to be used during the
    authentication.
    (Currently, only TLS 1.2 is supported; in the future, the support of TLS 1.3
    will be added into Chrome OS as well.
    Note that using multiple client certificates during a single authentication
    session is NOT supported.)

    The key on the smart card should be an RSA key.
    The key size should be 2048 bits (recommended) or 1024 bits (NOT
    recommended).
    (I.e., the elliptic-curve cryptography is currently NOT supported.)

    The client certificate must allow signature operations using the
    RSASSA-PKCS1-v1_5 signature scheme.
    At least SHA-1 should be supported if the customer is going to use this on
    Chromebooks equipped with the TPM 1.2 chips; it’s also recommended to
    additionally support SHA-256/SHA-384/SHA-512.
    (I.e., the certificates that only allow decryption are NOT supported, and
    the RSA-PSS signature algorithm is NOT supported too.)

    The smart card should be a PIV or a CAC contact card; some other types of
    cards are also supported.
    (There are many various types of cards and card profiles; the best way of
    checking for compatibility with Chrome OS is to try using the card on a
    Chromebook for visiting a website inside a user session, using the CSSI
    smart card middleware according to this Help Article:
    [support.google.com/chrome/a/answer/7014689](https://support.google.com/chrome/a/answer/7014689).)

Other notes:

    The user’s Chromebook must be managed (“enrolled”).
    The Chrome OS administrator has to configure several special device-level
    policies in order to enable the smart card support (the details of the admin
    configuration will be described in a separate document).

    The certificate expiration or revocation are NOT automatically checked by
    Chrome OS. Instead, the administrator should enforce the user to
    periodically go through the online login process, allowing the IdP to
    perform all necessary checks.

    When the certificate on the user’s smart card is changed, the users profile
    on the Chromebook will have to be recreated, wiping out all their locally
    cached data.
    (That’s caused by the fact that the user’s profile is cryptographically
    bound to the key that is stored on the card. However, in the future Chrome
    OS will support re-binding the user’s profile to new keys, allowing to
    update the smart card without losing locally cached data.)

    The set of supported smart card readers is documented on the page of the
    CCID free software driver:
    [ccid.apdu.fr/ccid/section.html](https://ccid.apdu.fr/ccid/section.html)
