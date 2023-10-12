---
breadcrumbs:
- - /administrators
  - Documentation for Administrators
- - /administrators/policy-list-3
  - Policy List
page_name: cookie-legacy-samesite-policies
title: Cookie Legacy SameSite Policies
---

**NOTE: These policies are available as of Chrome 79.**

**(Jul 7, 2023) LegacySameSiteCookieBehaviorEnabledForDomainList will be
available until at least Jul 30, 2024. We will be monitoring feedback and will
provide updates on its lifetime as appropriate. LegacySameSiteCookieBehaviorEnabled was
previously available until it was [removed in Chrome 93](https://chromium.googlesource.com/chromium/src/+/a5d81113983931597cc6cfb96558decfa615d464)
released on Aug 31, 2021.**

~~**(Feb 24, 2023) LegacySameSiteCookieBehaviorEnabledForDomainList will be
available until at least Jan 9, 2024. We will be monitoring feedback and will
provide updates on its lifetime as appropriate. LegacySameSiteCookieBehaviorEnabled was
previously available until it was [removed in Chrome 93](https://chromium.googlesource.com/chromium/src/+/a5d81113983931597cc6cfb96558decfa615d464)
released on Aug 31, 2021.**~~

~~**(Apr 27, 2022) LegacySameSiteCookieBehaviorEnabledForDomainList will be
available until at least Jun 27, 2023. We will be monitoring feedback and will
provide updates on its lifetime as appropriate. LegacySameSiteCookieBehaviorEnabled was
previously available until it was [removed in Chrome 93](https://chromium.googlesource.com/chromium/src/+/a5d81113983931597cc6cfb96558decfa615d464)
released on Aug 31, 2021.**~~

~~**(May 6, 2021) LegacySameSiteCookieBehaviorEnabledForDomainList will be
available until at least Dec 31, 2022. LegacySameSiteCookieBehaviorEnabled was
previously available until it was [removed in Chrome 93](https://chromium.googlesource.com/chromium/src/+/a5d81113983931597cc6cfb96558decfa615d464)
released on Aug 31, 2021. We will be monitoring feedback about these policies
and will provide updates on their lifetime as appropriate.**~~

~~**(Nov 6, 2020) LegacySameSiteCookieBehaviorEnabled will be available until at
least Aug 31, 2021. LegacySameSiteCookieBehaviorEnabledForDomainList will be
available until at least Mar 8, 2022. We will be monitoring feedback about these
policies and will provide updates on their lifetime as appropriate.**~~

~~**(May 29, 2020) These policies will be available until at least July 14,
2021. We will be monitoring feedback about these policies and will provide
updates on their lifetime as appropriate.**~~

~~**(Feb 10, 2020) These policies will be available for at least 12 months after
the release of Chrome 80 stable. We will be monitoring feedback about these
policies and will provide updates on their lifetime as appropriate.**~~

The policies
[LegacySameSiteCookieBehaviorEnabled](https://cloud.google.com/docs/chrome-enterprise/policies/?policy=LegacySameSiteCookieBehaviorEnabled)
and
[LegacySameSiteCookieBehaviorEnabledForDomainList](https://cloud.google.com/docs/chrome-enterprise/policies/?policy=LegacySameSiteCookieBehaviorEnabledForDomainList)
allow you to revert the SameSite behavior of cookies (possibly on specific
domains) to legacy behavior.

All cookies that match a domain pattern listed in
LegacySameSiteCookieBehaviorEnabledForDomainList (see below) will be reverted to
legacy behavior. For cookies that do not match a domain pattern listed in
LegacySameSiteCookieBehaviorEnabledForDomainList, or for all cookies if
LegacySameSiteCookieBehaviorEnabledForDomainList is not set, the global default
setting will be used. If LegacySameSiteCookieBehaviorEnabled is set, legacy
behavior will be enabled for all cookies as a global default. If
LegacySameSiteCookieBehaviorEnabled is not set, the user's personal
configuration will determine the global default setting.

**The SameSite attribute**

The SameSite attribute of a cookie specifies whether the cookie should be
restricted to a first-party or same-site context. Several values of SameSite are
allowed:

*   A cookie with "SameSite=Strict" will only be sent with a same-site
            request.
*   A cookie with "SameSite=Lax" will be sent with a same-site request,
            or a cross-site top-level navigation with a "safe" HTTP method.
*   A cookie with "SameSite=None" will be sent with both same-site and
            cross-site requests.

See
<https://tools.ietf.org/html/draft-ietf-httpbis-rfc6265bis-03#section-4.1.2.7>
for the definition of the SameSite attribute. See
<https://web.dev/samesite-cookies-explained/> for a more detailed explanation of
the SameSite attribute with examples.

Schemeful Same-Site

Schemeful Same-Site is a modification of the definition of a “site” to include
both the scheme and the registrable domain. This means that, with Schemeful
Same-Site, <http://site.example> and <https://site.example> are now considered
cross-site whereas previously they would be considered same-site.

This feature is still being prototyped and has a tentative M88 launch. See
[Chrome Platform Status
page](https://www.chromestatus.com/feature/5096179480133632).

See the [Schemeful Same-Site
explainer](https://github.com/sbingler/schemeful-same-site) for more details and
examples.

See
<https://mikewest.github.io/cookie-incrementalism/draft-west-cookie-incrementalism.html#rfc.section.3.3>
for the spec.

**Legacy SameSite behavior**

As of Chrome 80 (see [launch timeline](/updates/same-site)), a cookie that does
not explicitly specify a SameSite attribute will be treated as if it were
"SameSite=Lax". In addition, any cookie that specifies "SameSite=None" must also
have the Secure attribute. (See
<https://tools.ietf.org/html/draft-ietf-httpbis-rfc6265bis-03#section-4.1.2.5>
for the definition of the Secure attribute.)

Reverting to the legacy SameSite behavior causes cookies to be handled like they
were prior to May 2019 (when the new SameSite behavior described above first
became available). Under legacy behavior, cookies that don't explicitly specify
a SameSite attribute are treated as if they were "SameSite=None", i.e., they
will be sent with both same-site and cross-site requests. In addition, reverting
to the legacy behavior removes the requirement that "SameSite=None" cookies must
also specify the Secure attribute. As of Chrome 86 reverting to legacy behavior
will also disable Schemeful Same-Site.

**Configuring LegacySameSiteCookieBehaviorEnabledForDomainList**

In this policy setting, you can list specific domains for which legacy SameSite
behavior will be used. For cookies you want to revert to legacy SameSite
behavior, list the domain/host on which the cookies are set, NOT the
domains/hosts from which cross-site requests are made.

The domain of a cookie specifies those hosts to which the cookie will be sent.
If the Domain attribute of the cookie is specified, then the cookie will be sent
to hosts for which the specified Domain attribute is a suffix of the hostname,
and reversion to legacy SameSite behavior will be triggered only if the value of
the specified Domain attribute matches any of the patterns listed in this policy
setting. If the Domain attribute of the cookie is not specified, then the cookie
will only be sent to the origin server which set the cookie, and reversion to
legacy SameSite behavior will be triggered only if the hostname of the origin
server matches any of the patterns listed in this policy setting. See
<https://tools.ietf.org/html/draft-ietf-httpbis-rfc6265bis-03#section-4.1.2.3>
for the definition of the Domain attribute.

*Example 1*: If the Domain attribute of the cookie is set to
"Domain=example.com", the cookie will be sent when making HTTP requests to
example.com, www.example.com, or www.corp.example.com. To revert to legacy
behavior for such a cookie, use the pattern '\[\*.\]example.com' or
'example.com'. The value of the Domain attribute (example.com) will match either
of these patterns. Even though the cookie would be sent to www.example.com, the
pattern 'www.example.com' will NOT match such a cookie, because the Domain
attribute value (example.com) does not match the pattern 'www.example.com'.

*Example 2*: If the Domain attribute of a cookie set by www.example.com is not
specified, the cookie will be sent only when making HTTP requests to
www.example.com. The cookie will not be sent when making HTTP requests to
example.com or sub.www.example.com. To revert to legacy behavior for such a
cookie, use the pattern 'www.example.com' or '\[\*.\]example.com'. The origin
server's hostname (www.example.com) will match either of these patterns.

Note that patterns you list here are treated as domains, not URLs, so you should
not specify a scheme or port. Specifying a scheme or port may result in
undefined behavior.
