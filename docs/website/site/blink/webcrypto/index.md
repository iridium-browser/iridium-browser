---
breadcrumbs:
- - /blink
  - Blink (Rendering Engine)
page_name: webcrypto
title: WebCrypto
---

[TOC]

## Accessing it

*   The WebCrypto API was enabled by default starting in **Chrome 37**
            (August 26, 2014)
*   Access to the WebCrypto API is restricted to [secure
            origins](http://www.chromium.org/Home/chromium-security/security-faq#TOC-Which-origins-are-secure-)
            (which is to say https:// pages).
    *   Note: [In the spec](https://github.com/w3c/webcrypto/issues/28),
                crypto.subtle is supposed to be undefined in insecure contexts,
                whereas in Chrome it is defined however any operation on it
                fails with NotSupportedError. (This will be updated in the
                future).

## Standards compliance

Chromium's implementation follows the [Web Cryptography API Editor's
Draft](https://w3c.github.io/webcrypto/Overview.html).

Be sure to refer to the copy of the spec on github **NOT the one hosted on
w3c.org**.

The version on w3c.org is horribly out of date (as of October 3 2016).

## Reporting bugs

*   Issues with the implementation should be filed on [Chromium's bug
            tracker](https://code.google.com/p/chromium/issues/list), and given
            the component
            [Blink&gt;WebCrypto](https://bugs.chromium.org/p/chromium/issues/list?q=component:Blink%3EWebCrypto).
*   Issues with the [Web Cryptography
            specification](https://w3c.github.io/webcrypto/Overview.html) should
            be filed on the [github](https://github.com/w3c/webcrypto/issues).

## Supported algorithms (as of Chrome 53)

The WebCrypto specification does not mandate any particular algorithms.

At this time Chromium implements all of the algorithms described by the main
specification:

> <table>
> <tr>
> <td><b> Algorithm</b></td>
> <td><b> Supported</b></td>
> <td><b> Notes</b></td>
> </tr>
> <tr>
> <td> RSASSA-PKCS1-v1_5</td>
> <td> YES</td>
> </tr>
> <tr>
> <td> RSA-PSS </td>
> <td> YES</td>
> </tr>
> <tr>
> <td> RSA-OAEP</td>
> <td> YES</td>
> </tr>
> <tr>
> <td> ECDSA</td>
> <td> YES</td>
> </tr>
> <tr>
> <td> ECDH</td>
> <td> YES</td>
> </tr>
> <tr>
> <td> AES-CTR</td>
> <td> YES</td>
> </tr>
> <tr>
> <td> AES-CBC</td>
> <td> YES</td>
> </tr>
> <tr>
> <td> AES-GCM</td>
> <td> YES</td>
> </tr>
> <tr>
> <td> AES-KW</td>
> <td> YES</td>
> </tr>
> <tr>
> <td> HMAC</td>
> <td> YES</td>
> </tr>
> <tr>
> <td> SHA-1</td>
> <td> YES</td>
> </tr>
> <tr>
> <td> SHA-256</td>
> <td> YES</td>
> </tr>
> <tr>
> <td> SHA-384</td>
> <td> YES</td>
> </tr>
> <tr>
> <td> SHA-512</td>
> <td> YES</td>
> </tr>
> <tr>
> <td> HKDF</td>
> <td> YES</td>
> </tr>
> <tr>
> <td> PBKDF2</td>
> <td> YES</td>
> </tr>
> </table>

**Abandoned algorithms**

Earlier drafts of the specification contained additional algorithms, which have
since been pulled from both the spec and from Chromium's implementation:

> <table>
> <tr>
> <td><b> Algorithm</b></td>
> <td><b> Supported</b></td>
> <td><b> Notes</b></td>
> </tr>
> <tr>
> <td> AES-CMAC</td>
> <td> NO</td>

> *   <td>No longer part of the spec</td>
> *   <td>Was never implemented by Chrome</td>

> </tr>
> <tr>
> <td> AES-CFB</td>
> <td> NO</td>

> *   <td>No longer part of the spec</td>
> *   <td>Was never implemented by Chrome</td>

> </tr>
> <tr>
> <td> Diffie-Hellman</td>
> <td> NO</td>

> *   <td>No longer part of the spec</td>
> *   <td>Was never implemented by Chrome</td>

> </tr>
> <tr>
> <td> Concat KDF</td>
> <td> NO</td>

> *   <td>No longer part of the spec</td>
> *   <td>Was never implemented by Chrome</td>

> </tr>
> <tr>
> <td> HKDF-CTR</td>
> <td> NO</td>

> *   <td>No longer part of the spec</td>
> *   <td>Was never implemented by Chrome</td>
> *   <td>The spec has redefined this as <a
              href="https://github.com/w3c/webcrypto/issues/27">redefined this
              as HKDF</a></td>

> </tr>
> <tr>
> <td> RSAES-PKCS1-v1_5</td>
> <td> NO</td>

> *   <td>No longer part of the spec.</td>
> *   <td>Chrome supported this in early days before Web Crypto was
              enabled by default, but has since dropped support.</td>

> </tr>
> </table>

> ### RSA support

    *   The modulus length must be a multiple of 8 bits
    *   The modulus length must be &gt;= 256 and &lt;= 16384 bits
    *   When *generating* RSA keys the public exponent must be 3 or
                65537. This limitation does not apply when importing keys.

> ### AES support

    *   The supported key sizes are:
        *   128 bits
        *   256 bits
        *   192 bit AES keys are not supported

> ### EC support

    *   The supported
                [namedCurves](https://dvcs.w3.org/hg/webcrypto-api/raw-file/tip/spec/Overview.html#dfn-NamedCurve)
                are:
        *   P-256
        *   P-384
        *   P-521

## Supported key formats

Chromium's WebCrypto implementation supports all of the key formats - "raw",
"spki", "pkcs8", "jwk", with the following caveats:

*   There are differences in DER key format handling between Web Crypto
            implementations. Where possible for compatibility prefer using "raw"
            keys or "jwk" which have better interoperability.
*   When importing/exporting "spki" and "pkcs8" formats, the only OIDs
            supported by Chromiumare those recognized by OpenSSL/BoringSSL.

    *   Importing ECDH keys [does not accept
                id-ecDH](https://bugs.chromium.org/p/chromium/issues/detail?id=532728).
                The OID must instead be id-ecPublicKey (This can cause problems
                importing keys generated by Firefox; use "raw" EC keys as a
                workaround; Chromium in this case is not spec compliant)
    *   Importing RSA-PSS keys does not accept id-RSASSA-PSS. The OID
                must instead be rsaEncryption
    *   Importing RSA-OAEP keys does not accept id-RSAES-OAEP. The OID
                must instead be rsaEncryption.
    *   Exporting ECDH keys uses OID id-ecPublicKey, whereas the
                WebCrypto spec says to use id-ecDH.
    *   Exporting RSA-PSS keys uses OID rsaEncryption, whereas the
                WebCrypto spec says to use RSA-PSS.
    *   Exporting RSA-OAEP keys uses OID rsaEncryption, whereas the
                WebCrypto spec says to use id-RSAES-OAEP.

## Examples of how to use WebCrypto

Some examples of using WebCrypto can be found in the [Blink
LayoutTests](https://chromium.googlesource.com/chromium/blink/+/HEAD/LayoutTests/crypto/).

(These can't be run directly in the browser because they access functionality
from the test harness, however it gives an idea of how to call the various
operations)

## Usage data for WebCrypto

Google Chrome measures how commonly WebCrypto algorithms and methods are across
web pages.
To explore the data use the [Chromium feature stack rank
dashboard](https://www.chromestatus.com/metrics/feature/popularity). This counts
the number of pageloads that made use of the given feature (internal users can
navigate an equivalent histogram using "WebCore.FeatureObserver").
For details on how the metrics are measured read the comment block in
[CryptoHistograms.h](https://code.google.com/p/chromium/codesearch#chromium/src/third_party/WebKit/Source/modules/crypto/CryptoHistograms.h).
Here is the correspondence between the feature names found on the [Chromium
feature stack rank
dashboard](https://www.chromestatus.com/metrics/feature/popularity) and
WebCrypto's operations/algorithms:

<table>
<tr>
<td> <b>Feature</b></td>
<td> <b>WebCrypto method</b></td>
</tr>
<tr>
<td> `CryptoGetRandomValues` </td>
<td> crypto.getRandomValues()</td>
</tr>
<tr>
<td> `SubtleCryptoEncrypt`</td>
<td> crypto.subtle.encrypt()</td>
</tr>
<tr>
<td> `SubtleCryptoDecrypt`</td>
<td> crypto.subtle.decrypt() </td>
</tr>
<tr>
<td> `SubtleCryptoSign`</td>
<td> crypto.subtle.sign() </td>
</tr>
<tr>
<td> `SubtleCryptoVerify`</td>
<td> crypto.subtle.verify() </td>
</tr>
<tr>
<td> `SubtleCryptoDigest`</td>
<td> crypto.subtle.digest()</td>
</tr>
<tr>
<td> `SubtleCryptoGenerateKey`</td>
<td> crypto.subtle.generateKey() </td>
</tr>
<tr>
<td> `SubtleCryptoImportKey`</td>
<td> crypto.subtle.importKey() </td>
</tr>
<tr>
<td> `SubtleCryptoExportKey`</td>
<td> crypto.subtle.exportKey() </td>
</tr>
<tr>
<td> `SubtleCryptoDeriveBits`</td>
<td> crypto.subtle.deriveBits()</td>
</tr>
<tr>
<td> `SubtleCryptoDeriveKey`</td>
<td> crypto.subtle.deriveKey() </td>
</tr>
<tr>
<td> `SubtleCryptoWrapKey`</td>
<td> crypto.subtle.wrapKey() </td>
</tr>
<tr>
<td> `SubtleCryptoUnwrapKey`</td>
<td> crypto.subtle.unwrapKey() </td>
</tr>
</table>

<table>
<tr>
<td> <b>Feature</b></td>
<td><b>WebCrypto algorithm</b></td>
</tr>
<tr>
<td> CryptoAlgorithmAesCbc `</td>
<td> AES-CBC</td>
</tr>
<tr>
<td> `CryptoAlgorithmHmac`</td>
<td> HMAC</td>
</tr>
<tr>
<td> `CryptoAlgorithmRsaSsaPkcs1v1_5`</td>
<td> RSASSA-PKCS1-v1_5</td>
</tr>
<tr>
<td> `CryptoAlgorithmSha1`</td>
<td> SHA-1</td>
</tr>
<tr>
<td> `CryptoAlgorithmSha256`</td>
<td> SHA-256</td>
</tr>
<tr>
<td> `CryptoAlgorithmSha384`</td>
<td> SHA-384 </td>
</tr>
<tr>
<td> `CryptoAlgorithmSha512`</td>
<td> SHA-512</td>
</tr>
<tr>
<td> `CryptoAlgorithmAesGcm`</td>
<td> AES-GCM</td>
</tr>
<tr>
<td> `CryptoAlgorithmRsaOaep`</td>
<td> RSA-OAEP</td>
</tr>
<tr>
<td> `CryptoAlgorithmAesCtr`</td>
<td> AES-CTR</td>
</tr>
<tr>
<td> `CryptoAlgorithmAesKw`</td>
<td> AES-KW</td>
</tr>
<tr>
<td> `CryptoAlgorithmRsaPss`</td>
<td> RSA-PSS</td>
</tr>
<tr>
<td> `CryptoAlgorithmEcdsa`</td>
<td> ECDSA</td>
</tr>
<tr>
<td> `CryptoAlgorithmEcdh`</td>
<td> ECDH</td>
</tr>
<tr>
<td> `CryptoAlgorithmHkdf`</td>
<td> HKDF</td>
</tr>
<tr>
<td> `CryptoAlgorithmPbkdf2`</td>
<td> PBKDF2</td>
</tr>
</table>

## Chromium developer's guide

This section is intended for Chromium developers writing patches to address
WebCrypto bugs/features.

### Code location reference

*   [src/components/webcrypto](https://chromium.googlesource.com/chromium/src/+/HEAD/components/webcrypto)
    Contains the actual crypto algorithm implementations (HMAC, ECDH, RSA-PSS,
    etc.).
*   [src/components/test/data/webcrypto](https://chromium.googlesource.com/chromium/src/+/HEAD/components/test/data/webcrypto)
    Test data used by
    [src/components/webcrypto](https://chromium.googlesource.com/chromium/src/+/HEAD/components/webcrypto).
    Note that more extensive tests live in LayoutTests, and these will
    eventually be transitioned there too.
*   [src/third_party/WebKit/LayoutTests/crypto](https://chromium.googlesource.com/chromium/blink/+/HEAD/LayoutTests/crypto)
    The end-to-end Javascript tests that exercise WebCrypto's crypto.subtle.\*
    methods.

*   [src/third_party/WebKit/public/platform/WebCrypto.h](https://chromium.googlesource.com/chromium/blink/+/HEAD/public/platform/WebCrypto.h)
    Public interface that defines the Blink &lt;--&gt; Chromium layer
*   [src/third_party/WebKit/Source/modules/crypto](https://chromium.googlesource.com/chromium/blink/+/HEAD/Source/modules/crypto)
    The Blink layer (responsible for interacting with the Javascript), and then
    calling into Chromium side using the WebCrypto interface.
*   [src/third_party/WebKit/Source/bindings/modules/v8/ScriptValueSerializerForModules.cpp](https://chromium.googlesource.com/chromium/blink/+/HEAD/Source/bindings/modules/v8/ScriptValueSerializerForModules.cpp)
    Implements the structured clone for CryptoKey

### Running the Chromium unit-tests

> `cd src`

> `ninja -C out/Debug components_unittests`

> `./out/Debug/components_unittests --gtest_filter="WebCrypto*"`

### Running the Blink LayoutTests

> ### `cd src/third_party/WebKit`

> ### `ninja -C out/Debug blink_tests`

> ### `./Tools/Scripts/run-webkit-tests --debug crypto`

### Getting "XOpenDisplay failed" when running unittests

Unfortunately `components_unittests` requires a display to run, even though
WebCrypto itself has no such dependency. If you are running from a terminal and
get this error the easiest fix is to use Xvfb to create a mock display server
(on port 4 in my example)

> `#Run this in the background....`
> Xvfb :4 -screen 0 1024x768x24
> # And once it is up, re-run the unit-tests with its display port
> DISPLAY:4 ./out/Debug/components_unittests --gtest_filter="WebCrypto\*"
