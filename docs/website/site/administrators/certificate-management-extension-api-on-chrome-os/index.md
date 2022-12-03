---
breadcrumbs:
- - /administrators
  - Documentation for Administrators
page_name: certificate-management-extension-api-on-chrome-os
title: 'Developer Guide: Certificate Management Extension API on Chrome OS'
---

This document is for extension developers and describes how to use the extension
API
[enterprise.platformKeys](https://developer.chrome.com/extensions/enterprise_platformKeys)
for client certificate enrollment.

## Motivation

Client certificates allow secure authentication to digital resources, like
networks or web resources. A typical certificate based authentication protocol
is [Transport Layer
Security](http://en.wikipedia.org/wiki/Transport_Layer_Security) (TLS, formerly
known as SSL) and the protocols that are built on top like EAP-TLS for network
authentication and HTTPS for web resources.

This article describes how to manage and make use of client certificates on
Chrome OS using the
[enterprise.platformKeys](https://developer.chrome.com/extensions/enterprise_platformKeys)
extension API: in particular, how to provision a new client certificate and how
to use a client certificate for network or web authentication.

## Overview

Many certificate enrollment protocols exist, like
[SCEP](http://tools.ietf.org/html/draft-nourse-scep-23),
[EST](https://tools.ietf.org/html/rfc7030) or
[CMC](https://tools.ietf.org/html/rfc5272), that define the communication
between the client (in this case, the Chrome OS device) and the Certificate
Authority (CA), which can be accompanied by a Registration Authority. The
[enterprise.platformKeys](https://developer.chrome.com/extensions/enterprise_platformKeys)
API is designed in a way that extensions have the freedom to implement any
enrollment protocol based on what is supported by the target Certificate
Authority.

Independent of which specific protocol is used to communicate, the following
steps describe the typical flow of a certificate enrollment:

1. Trigger the enrollment process, e.g. the user tries to authenticate but no
client cert is installed, or the user manually starts the enrollment.
2. Obtain the enrollment configuration, e.g. URL of the CA, or the attributes
to use for the certification request.
3. Obtain credentials to authenticate the certification request, either by
asking the user or by using an API, e.g.
[chrome.identity](https://developer.chrome.com/apps/identity).
4. Generate public/private key pair locally on the device.
5. Create the certification request, which contains data like the public key
and some attributes, which in turn is signed by the private key. The private
key is kept secret and is not part of the request.
6. Send the certification request to the CA.
7. After the CA authorizes the request, it creates the client certificate and
sends it to the client.
8. The client receives the certificate and installs it.

After successful enrollment the certificate can be used to authenticate to
resources like a network or a web page.

## The enterprise.platformKeys API

This extension API of Chrome OS allows extensions to generate a key pair, sign a
certification request, and to manage the installed client certificates (import,
get and remove certificates). Using this API, an extension can drive the process
of installing a new client certificate to a Chrome OS device.

In order to use the API, an extension must be pre-installed by user policy. Only
extensions installed by policy can use the API.

### How to implement the enrollment process in an extension
1. The enrollment can be started by several events.
    * The extension can expose a link or an icon that the user can use to manually
    start the enrollment process, see for example
    [chrome.browserAction](https://developer.chrome.com/extensions/browserAction).
    * The first time the user tries to connect to a network that requires a client
    certificate for authentication, Chrome OS can automatically open the
    extension if the following required step has been taken by the
    administrator:
    The network must be configured by policy to use client certificates for
    authentication (e.g. a 802.1x WiFi with EAP-TLS) and the Client Enrollment
    URL must be set to a page of the extension, see [Manage
    Networks](https://support.google.com/chrome/a/answer/2634553).
    Every time the user attempts to connect to this network and there is no
    matching certificate in the user’s certificate store, Chrome OS will open
    the configured Client Enrollment URL in a new browser tab.
    * The extension can use an [event
    page](https://developer.chrome.com/extensions/event_pages) (succeeding
    [background page](https://developer.chrome.com/extensions/background_pages))
    to check whether a valid client certificate is already installed at certain
    times (e.g. after login and once per day). If not or if the certificate is
    expiring soon, the extension can trigger the enrollment flow.

2. The extension needs some configuration about the enrollment process, at a
minimum the URL of the CA and maybe attributes to embed in the certification
request.

    The configuration can

    * be part of the extension itself, which prevents reuse of the extension with
    other configurations,
    * be part of the Client Enrollment URL that is opened by Chrome OS (see
    previous step),
    * be pushed through [policy for
    extensions](https://developer.chrome.com/extensions/manifest/storage), which
    allows the administrator to configure the extension in the management
    console.

3. To obtain credentials to authenticate the certification request at the CA,
the extension can present any UI and ask the user to provide the credentials or
use any other APIs, for example, OAuth.

4. The extension has to obtain the user
[Token](https://developer.chrome.com/extensions/enterprise_platformKeys#type-Token)
(with the id `"user"`) using
[enterprise.platformKeys.getTokens](https://developer.chrome.com/extensions/enterprise_platformKeys#method-getTokens)
and generate a key pair using the
[subtleCrypto.generateKey](http://www.w3.org/TR/WebCryptoAPI/#subtlecrypto-interface)
method of the
[Token](https://developer.chrome.com/extensions/enterprise_platformKeys#type-Token).
The private key will be generated by the TPM and is guaranteed to never leave
the device or even the TPM.

    ```none
    function getUserToken(callback) {
    chrome.enterprise.platformKeys.getTokens(function(tokens) {
       for (var i = 0; i < tokens.length; i++) {
         if (tokens[i].id == "user") {
           callback(tokens[i]);
           return;
         }
       }
       callback(null);
     });
    }
    ```

    ```none
    var algorithm = {
     name: "RSASSA-PKCS1-v1_5",

      // RsaHashedKeyGenParams:
      modulusLength: 2048,

     // Equivalent to 65537
      publicExponent: new Uint8Array([0x01, 0x00, 0x01]), 
     hash: {
       name: "SHA-1"
     }
    };

    userToken.subtleCrypto.generateKey(algorithm, false /* not extractable */, ["sign"])
        .then(function(keyPair) { ... continue with generated keyPair ... },
           console.error.bind(console));
    ```

5. Extract the public key from the key handle using the
[subtleCrypto.exportKey](http://www.w3.org/TR/WebCryptoAPI/#subtlecrypto-interface)
method of the
[Token](https://developer.chrome.com/extensions/enterprise_platformKeys#type-Token):

    ```none
    userToken.subtleCrypto.exportKey("spki", keyPair.publicKey)
      .then(function(publicKey) { ... continue with publicKey ... },
           console.error.bind(console));
    ```

6. Create the content for the certification request in the extension. This
request must contain at least the public key. The CA may expect additional
attributes that must be added. If the request is PKCS#10 based, for example, the
open source library [forge](https://github.com/digitalbazaar/forge) may be used.

    ```none
    var request = CreateCertificationRequest();
    request.setPublicKey(publicKey);
    request.setSubject('CommonName', 'some name');
    ```

7. Sign the content of the certification request (using the
[subtleCrypto.sign](http://www.w3.org/TR/WebCryptoAPI/#subtlecrypto-interface)
method of the
[Token](https://developer.chrome.com/extensions/enterprise_platformKeys#type-Token))
and create the final request from the content and the signature. Any subsequent
attempt to use the same key for signing will fail for security reasons: This API
guarantees that only Chrome OS itself can use the private key and the
certificate for authentication.

    ```none
    function signData(data, callback) {
     userToken.subtleCrypto.sign({name : "RSASSA-PKCS1-v1_5"}, keyPair.privateKey, data)
       .then(callback, console.error.bind(console));
    }

    request.setSignFunction(signData);
    request.sign();
    ```

8. Send the certification request to the CA and receive the client certificate
(e.g. using XMLHttpRequest)

    ```none
    var xhr = new XMLHttpRequest();
    function onReadyStateChange() {
     if (xhr.readyState !== 4)
       return;
     if (xhr.status !== 200) {
        ... handle error ...
       return;
     }
     ... continue with xhr.response which contains the certificate ....
    }
    xhr.onreadystatechange = onReadyStateChange;
    xhr.open('POST', caUrl);
    xhr.setRequestHeader('Content-Type', ...);
    xhr.send(request);
    ```

9. Install the client certificate using
[enterprise.platformKeys.importCertificate](https://developer.chrome.com/extensions/enterprise_platformKeys#method-importCertificate)

    ```none
    chrome.enterprise.platformKeys.importCertificate(userToken.id, certificate);
    ```

10. Different methods to use the client certificate for authentication are
available

    For network authentication:
    * The user can manually select the client certificate in the network
    configuration dialog.
    * The selection can also be automated if the network is configured by policy.
    For network types that support client certificates, like EAP-TLS, the
    administrator can configure a Certificate Pattern that defines which client
    certificates are valid for authenticating to this network. Chrome OS will
    automatically select the most recent matching client certificate and use it
    for authentication on every connection attempt.

    For web pages requiring client certificate authentication:
    * When accessing a web page that requires the client to present a certificate,
    Chrome OS will show the user a list of available client certificates. After
    selecting one, Chrome OS will use it to authenticate.
    * The selection can also be automated for specific URLs using the policy
    [Automatically select client certificates for these
    sites](https://support.google.com/chrome/a/answer/2657289?#AutoSelectCertificateForUrls):
    For URLs that are listed in this policy, the most recent matching client
    certificate will automatically be used for authentication without prompting
    the user.

Note that the
[enterprise.platformKeys](https://developer.chrome.com/extensions/enterprise_platformKeys)
API guarantees, that client certificates imported using the API can only be used
by Chrome OS itself for authentication. The extension is not able to drive any
authentication with such a certificate and in particular the API guarantees that
the certificate can’t be extracted to authenticate any other user or device.

### Re-enrollment

To determine whether any valid client certificate is already installed and to
check the expiration of the installed certificates, an extension can use the
[platformKeys.getCertificates](https://developer.chrome.com/extensions/enterprise_platformKeys#method-getCertificates)
function and if necessary trigger the process to obtain a new client
certificate.

```none
chrome.enterprise.platformKeys.getCertificates(userToken.id, function(certificates) {
 for (var i = 0; i < certificates.length; i++) {
   var certificate = certificates[i];
   ... check whether certificate is valid and matches the required attributes ...
 }
});
```

An installed certificate can be removed from the user’s certificate store using
the function
[enterprise.platformKeys.removeCertificate](https://developer.chrome.com/extensions/enterprise_platformKeys#method-removeCertificate).
As client certificates can be selected automatically (see last step in the
enrollment process above), unnecessary certificates should be removed to prevent
conflicts.
