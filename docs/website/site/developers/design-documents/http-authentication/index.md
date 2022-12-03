---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: http-authentication
title: HTTP authentication
---

As specified in [RFC 2617](http://www.ietf.org/rfc/rfc2617.txt), HTTP supports
authentication using the WWW-Authenticate request headers and the Authorization
response headers (and the Proxy-Authenticate and Proxy-Authorization headers for
proxy authentication).

**Supported authentication schemes**

> Chrome supports four authentication schemes: Basic, Digest, NTLM, and
> Negotiate. Basic, Digest, and NTLM are supported on all platforms by default.
> Negotiate is supported on all platforms except Chrome OS by default.

> The Basic and Digest schemes are specified in [RFC
> 2617](http://www.ietf.org/rfc/rfc2617.txt). NTLM is a Microsoft proprietary
> protocol. The Negotiate (or SPNEGO) scheme is specified in [RFC
> 4559](http://www.ietf.org/rfc/rfc4559.txt) and can be used to negotiate
> multiple authentication schemes, but typically defaults to either Kerberos or
> NTLM.

> The list of supported authentication schemes may be overridden using the
> [AuthSchemes](/administrators/policy-list-3#AuthSchemes) policy. See [this
> page](/administrators) for details on using administrative policies.

**Choosing an authentication scheme**

> When a server or proxy accepts multiple authentication schemes, our network
> stack selects via HttpAuth::ChooseBestChallenge() the authentication scheme
> with the highest score:

    *   Basic: 1
    *   Digest: 2
    *   NTLM: 3
    *   Negotiate: 4

> The Basic scheme has the lowest score because it sends the username/password
> unencrypted to the server or proxy.

> So we choose the most secure scheme, and we ignore the server or proxy's
> preference, indicated by the order in which the schemes are listed in the
> WWW-Authenticate or Proxy-Authenticate response headers. This could be a
> source of compatibility problems because [MSDN documents that "WinInet chooses
> the first method it
> recognizes."](http://msdn.microsoft.com/en-us/library/aa384220%28VS.85%29.aspx)
> Note: In IE7 or later, WinInet chooses the first *non-Basic* method it
> recognizes.

**Integrated Authentication**

> With Integrated Authentication, Chrome can authenticate the user to an
> Intranet server or proxy without prompting the user for a username or
> password. It does this by using cached credentials which are established when
> the user initially logs in to the machine that the Chrome browser is running
> on. Integrated Authentication is supported for Negotiate and NTLM challenges
> only.

> Due to potential attacks, Integrated Authentication is only enabled when
> Chrome receives an authentication challenge from a proxy, or when it receives
> a challenge from a server which is in the permitted list.

> This list is passed in to Chrome using a comma-separated list of URLs to
> Chrome via the
> [AuthServerWhitelist](/administrators/policy-list-3#AuthServerWhitelist)
> policy setting. For example, if the AuthServerWhitelist policy setting was:

> > \*example.com,\*foobar.com,\*baz

> ...then Chrome would consider that any URL ending in either 'example.com',
> 'foobar.com', or 'baz' is in the permitted list. Without the '\*' prefix, the
> URL has to match exactly.

> In **==Windows only==**, if the AuthServerWhitelist setting is not specified,
> the permitted list consists of those servers allowed by the Windows Zones
> Security Manager (queried for URLACTION_CREDENTIALS_USE). By default, this
> includes servers in the Local Machine or Local Intranet security zones. For
> example, when the host in the URL includes a "." character, by default it is
> outside the Local Intranet security zone). This behavior matches Internet
> Explorer and other Windows components.

> If a challenge comes from a server outside of the permitted list, the user
> will need to enter the username and password.

> Starting in Chrome 81, Integrated Authentication is [disabled by default for
> off-the-record (Incognito/Guest)
> profiles](https://bugs.chromium.org/p/chromium/issues/detail?id=458508#c62),
> and the user will need to enter the username and password.

**Kerberos SPN generation**

> When a server or proxy presents Chrome with a Negotiate challenge, Chrome
> tries to generate a Kerberos SPN (Service Principal Name) based on the host
> and port of the original URI. Unfortunately, the server does not indicate what
> the SPN should be as part of the authentication challenge, so Chrome (and
> other browsers) have to guess what it should be based on standard conventions.

> The default SPN is: HTTP/&lt;host name&gt;, where &lt;host name&gt; is the
> canonical DNS name of the server. This mirrors the SPN generation logic of IE
> and Firefox.

> The SPN generation can be customized via policy settings:

> *   [DisableAuthNegotiateCnameLookup](/administrators/policy-list-3#DisableAuthNegotiateCnameLookup)
              determines whether the original hostname in the URL is used rather
              than the canonical name. If left unset or set to false, Chrome
              uses the canonical name.
> *   [EnableAuthNegotiatePort](/administrators/policy-list-3#EnableAuthNegotiatePort)
              determines whether the port is appended to the SPN if it is a
              non-standard (not 80 or 443) port. If set to true, the port is
              appended. Otherwise (or if left unset) the port is not used.

> For example, assume that an intranet has a DNS configuration like

> auth-a.example.com IN CNAME auth-server.example.com

> auth-server.example.com IN A 10.0.5.3

> <table>
> <tr>
> <td> URL</td>
> <td> Default SPN </td>
> <td> With DisableAuthNegotiateCnameLookup</td>
> <td> With EnableAuthNegotiatePort </td>
> </tr>
> <tr>
> <td> http://auth-a</td>
> <td> HTTP/auth-server.example.com</td>
> <td> HTTP/auth-a</td>
> <td> HTTP/auth-server.example.com</td>
> </tr>
> <tr>
> <td> https://auth-a</td>
> <td> HTTP/auth-server.example.com</td>
> <td> HTTP/auth-a </td>
> <td> HTTP/auth-server.example.com</td>
> </tr>
> <tr>
> <td> http://auth-a:80</td>
> <td> HTTP/auth-server.example.com</td>
> <td> HTTP/auth-a</td>
> <td> HTTP/auth-server.example.com</td>
> </tr>
> <tr>
> <td> https://auth-a:443</td>
> <td> HTTP/auth-server.example.com</td>
> <td> HTTP/auth-a</td>
> <td> HTTP/auth-server.example.com</td>
> </tr>
> <tr>
> <td> http://auth-a:4678</td>
> <td> HTTP/auth-server.example.com</td>
> <td> HTTP/auth-a</td>
> <td> HTTP/auth-server.example.com:4678</td>
> </tr>
> <tr>
> <td> http://auth-a.example.com</td>
> <td> HTTP/auth-server.example.com</td>
> <td> HTTP/auth-a.example.com</td>
> <td> HTTP/auth-server.example.com</td>
> </tr>
> <tr>
> <td> http://auth-server</td>
> <td> HTTP/auth-server.example.com</td>
> <td> HTTP/auth-server</td>
> <td> HTTP/auth-server.example.com</td>
> </tr>
> <tr>
> <td> http://auth-server.example.com</td>
> <td> HTTP/auth-server.example.com</td>
> <td> HTTP/auth-server.example.com</td>
> <td> HTTP/auth-server.example.com</td>
> </tr>
> </table>

**Kerberos Credentials Delegation (Forwardable Tickets)**

> Some services require delegation of the users identity (for example, an IIS
> server accessing a MSSQL database). By default, Chrome does not allow this.
> You can use the
> [AuthNegotiateDelegateWhitelist](/administrators/policy-list-3#AuthNegotiateDelegateWhitelist)
> policy to enable it for the servers.

> Delegation does not work for proxy authentication.

### **Negotiate external libraries**

> On Windows, Negotiate is implemented using the SSPI libraries and depends on
> code in secur32.dll.

> On Android, Negotiate is implemented using an external Authentication app
> provided by third parties. Details are given in [Writing a SPNEGO
> Authenticator for Chrome on
> Android](/developers/design-documents/http-authentication/writing-a-spnego-authenticator-for-chrome-on-android).
> The AuthAndroidNegotiateAccountType policy is used to tell Chrome the Android
> account type provided by the app, hence letting it find the app.

> On other platforms, Negotiate is implemented using the system GSSAPI
> libraries. The first time a Negotiate challenge is seen, Chrome tries to
> dlopen one of several possible shared libraries. If it is unable to find an
> appropriate library, Chrome remembers for the session and all Negotiate
> challenges are ignored for lower priority challenges.

> The [GSSAPILibraryName](/administrators/policy-list-3#GSSAPILibraryName)
> policy can be used to specify the path to a GSSAPI library that Chrome should
> use.

> Otherwise, Chrome tries to dlopen/dlsym each of the following fixed names in
> the order specified:

    *   OSX: libgssapi_krb5.dylib
    *   Linux: libgssapi_krb5.so.2, libgssapi.so.4, libgssapi.so.2,
                libgssapi.so.1

> Chrome OS follows the Linux behavior, but does not have a system gssapi
> library, so all Negotiate challenges are ignored.

**Remaining work**

*   Support GSSAPI on Windows \[for MIT Kerberos for Windows or
            Heimdal\]
*   Offer [a policy to disable Basic authentication
            scheme](https://bugs.chromium.org/p/chromium/issues/detail?id=1025002)
            over unencrypted channels.

**Questions?**

> Please feel free to send mail to net-dev@chromium.org
