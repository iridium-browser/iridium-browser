---
breadcrumbs:
- - /updates
  - updates
page_name: same-site
title: SameSite Updates
---

*   **Confused?** [Start
            here](https://web.dev/samesite-cookies-explained/).
*   **Developers:** Check out our [testing and debugging
            tips](/updates/same-site/test-debug).
*   **Adding \`SameSite=None; Secure\` to your cookies?** Check the list
            of [incompatible
            clients](https://chromium.org/updates/same-site/incompatible-clients)
            here.
*   **Check the list of [Frequently Asked Questions
            (FAQ)](/updates/same-site/faq)** for common scenarios and use cases.

**Launch Timeline**

Last updated Mar 18, 2021.

Latest update:

**Mar 18, 2021:** The flags #same-site-by-default-cookies and
#cookies-without-same-site-must-be-secure have been removed from chrome://flags
as of Chrome 91, as the behavior is now enabled by default. In Chrome 94, the
command-line flag
--disable-features=SameSiteByDefaultCookies,CookiesWithoutSameSiteMustBeSecure
will be removed.

For the full Chrome release schedule, [see
here](https://chromiumdash.appspot.com/schedule). For the SameSite-by-default
and SameSite=None-requires-Secure launch timeline, see below:

*   **Early October, 2019**: Experimental
            [SameSite-by-default](https://www.chromestatus.com/feature/5088147346030592)
            and
            [SameSite=None-requires-Secure](https://www.chromestatus.com/feature/5633521622188032)
            behavior launched to 50% of users on Chrome Canary and Dev (Chrome
            Canary and Dev versions 78+). Windows and Mac users on domain-joined
            devices and Chrome OS users on enterprise-registered devices will be
            excluded from the experiment. Chrome 78 Beta users will not receive
            the experimental behavior.
*   **October 31, 2019**: Chrome 79 Beta released. Experiment extended
            to 50% of Chrome 79 Beta users, including domain-joined and
            enterprise-registered devices.
            [Policies](/administrators/policy-list-3/cookie-legacy-samesite-policies)
            to manage the experimental behavior (see below) will be available on
            Chrome 79.
*   **Dec 10, 2019**: Chrome 79 Stable released. Stable users on Chrome
            79 will NOT receive the new SameSite behavior.
*   **Dec 19, 2019**: Chrome 80 Beta released. Experimental behavior
            still enabled for 50% of Chrome 80 Beta users.
*   **February 4, 2020**: Chrome 80 Stable released. The enablement of
            the SameSite-by-default and SameSite=None-requires-Secure
            enforcement will not be included in this initial Chrome 80 stable
            rollout. Please see the next item for more detailed information on
            the when SameSite enforcement will be enabled for Chrome 80 stable.
*   **February, 2020**: Enforcement rollout for Chrome 80 Stable: The
            SameSite-by-default and SameSite=None-requires-Secure behaviors will
            begin rolling out to Chrome 80 Stable for an initial limited
            population starting the week of February 17, 2020, excluding the US
            President’s Day holiday on Monday. We will be closely monitoring and
            evaluating ecosystem impact from this initial limited phase through
            gradually increasing rollouts.
*   **March 2, 2020**: The enablement of the SameSite enforcements has
            been increased beyond the initial population. However, it is still
            targeting an overall limited global population of users on Chrome 80
            stable and newer. We continue to monitor metrics and ecosystem
            feedback via our [tracking
            bug](https://bugs.chromium.org/p/chromium/issues/detail?id=1052195),
            and other support channels.
*   **March 9, 2020**: The rollout population has been additionally
            increased, although it continues to target a fraction of the overall
            Chrome 80 stable population. We continue to monitor metrics and
            ecosystem feedback via our [tracking
            bug](https://bugs.chromium.org/p/chromium/issues/detail?id=1052195),
            and other support channels.
*   **April 3, 2020**: In light of the extraordinary global
            circumstances due to COVID-19, [we’ve decided to temporarily roll
            back the enforcement of SameSite cookie labeling on Chrome 80
            stable](https://blog.chromium.org/2020/04/temporarily-rolling-back-samesite.html).
            We recognize the efforts of sites and individual developers who
            prepared for this change as part of our ongoing effort to improve
            privacy and security across the web. We appreciate the feedback from
            across the web ecosystem which has helped inform this decision. We
            will provide advance notice on here and the Chromium blog when we
            plan to resume the gradual rollout, which we’re now aiming for over
            the summer. Non-stable Chrome channels (e.g. Dev, Canary, and Beta)
            will continue with 50% enablement in Chrome 80 and later. More
            details on [Chromium
            blog](https://blog.chromium.org/2020/04/temporarily-rolling-back-samesite.html).
*   **May 28, 2020**: We are planning to resume our SameSite cookie
            enforcement coinciding with the stable release of Chrome 84 on July
            14, with enforcement enabled for Chrome 80+. (In other words,
            starting July 14, Chrome users on the older Stable releases (80, 81,
            and 83 -- for whom we recommend installing the latest update!) as
            well as the newly released Chrome 84 will gradually begin to receive
            the SameSite-by-default behavior.) Read more on our [Chromium blog
            post](https://blog.chromium.org/2020/05/resuming-samesite-cookie-changes-in-july.html).
*   **July 14, 2020**: SameSite cookie enforcement has resumed, with a
            gradual rollout starting today (July 14) and ramping up over the
            next several weeks as we continue to monitor overall ecosystem
            readiness and engage with websites and services to ensure they are
            prepared for the SameSite labeling policy. The SameSite features are
            being enabled for Chrome Stable channel users on versions 80 and 81
            (who should update Chrome!), 83, as well as the newly released 84.
*   **July 28, 2020**: The rollout population has been increased to
            target a fraction of the overall Chrome 80+ stable population. We
            are monitoring metrics and ecosystem feedback on our [tracking
            bug](https://bugs.chromium.org/p/chromium/issues/detail?id=1052195).

    **Aug 11, 2020:** The target rollout population has been increased to 100%
    of users on Chrome Stable versions 80 and above, and the actual proportion
    of users with the new behavior enabled is now ramping up to 100% gradually.
    Users will receive the new behavior when they restart Chrome.

~~The new SameSite behavior will not be enforced on Android WebView until
later,~~ though app developers are advised to declare the appropriate SameSite
cookie settings for Android WebViews based on versions of Chrome that are
compatible with the None value, both for cookies accessed via HTTP(S) headers
and via Android WebView's [CookieManager
API](https://developer.android.com/reference/android/webkit/CookieManager).
\[**UPDATE Jan 8, 2021**: The modern SameSite behavior ([SameSite=Lax by
default, SameSite=None requires
Secure](https://web.dev/samesite-cookies-explained/), and [Schemeful
Same-Site](https://web.dev/schemeful-samesite/)) will be enabled by default for
Android WebView on apps targeting Android 12 and newer. Existing apps will not
be affected until they choose to update to target Android 12. Android 12 has not
yet been released. Existing apps can be tested with the new modern SameSite
behavior by toggling the flag webview-enable-modern-cookie-same-site in the
[developer
UI](https://chromium.googlesource.com/chromium/src/+/HEAD/android_webview/docs/developer-ui.md#Flag-UI).\]
This does not apply to Chrome browser on Android, which will begin to enforce
the new SameSite rules at the same time as the desktop versions of Chrome. The
new SameSite behavior will not affect Chrome on iOS.

**All updates:**

*   [Mar 18,
            2021](https://chromium.org/updates/same-site?pli=1#20210318)
*   [Jan 8, 2021](https://chromium.org/updates/same-site?pli=1#20210108)
*   [Aug 11,
            2020](https://chromium.org/updates/same-site?pli=1#20200811)
*   [July 28,
            2020](https://chromium.org/updates/same-site?pli=1#20200728)
*   [July 14,
            2020](https://chromium.org/updates/same-site?pli=1#20200714)
*   [May 28,
            2020](https://chromium.org/updates/same-site?pli=1#20200528)
*   [April 3,
            2020](https://chromium.org/updates/same-site?pli=1#20200403)
*   [Feb 10,
            2020](https://chromium.org/updates/same-site?pli=1#20200210)
*   [Nov 21,
            2019](https://chromium.org/updates/same-site?pli=1#20191121)
*   [Nov 1, 2019](https://chromium.org/updates/same-site?pli=1#20191101)
*   [Oct 2, 2019](https://chromium.org/updates/same-site?pli=1#20191002)
*   [Sept 30, 2019](#20190930)
*   [Sept 26,
            2019](https://chromium.org/updates/same-site?pli=1#20190926)

**Mar 18, 2021**

The flags #same-site-by-default-cookies and
#cookies-without-same-site-must-be-secure have been removed from chrome://flags
as of Chrome 91, as the behavior is now enabled by default. In Chrome 94, the
command-line flag
--disable-features=SameSiteByDefaultCookies,CookiesWithoutSameSiteMustBeSecure
will be removed.

**Jan 8, 2021**

The modern SameSite behavior ([SameSite=Lax by default, SameSite=None requires
Secure](https://web.dev/samesite-cookies-explained/), and [Schemeful
Same-Site](https://web.dev/schemeful-samesite/)) will be enabled by default for
Android WebView on apps targeting Android 12 (Android S) and newer. Existing
apps will not be affected until they choose to update to target Android 12.
Android 12 has not yet been released. Existing apps can be tested with the new
modern SameSite behavior by toggling the flag
webview-enable-modern-cookie-same-site in the [developer
UI](https://chromium.googlesource.com/chromium/src/+/HEAD/android_webview/docs/developer-ui.md#Flag-UI).

**Aug 11, 2020**

The target rollout population has been increased to 100% of users on Chrome
Stable versions 80 and above, and the actual proportion of users with the new
behavior enabled is now ramping up to 100% gradually. Users will receive the new
behavior when they restart Chrome.

**July 28, 2020**

The rollout population has been increased to target a fraction of the overall
Chrome 80+ stable population. We are monitoring metrics and ecosystem feedback
on our [tracking
bug](https://bugs.chromium.org/p/chromium/issues/detail?id=1052195).

**July 14, 2020**

SameSite cookie enforcement has resumed, with a gradual rollout starting today
(July 14) and ramping up over the next several weeks as we continue to monitor
overall ecosystem readiness and engage with websites and services to ensure they
are prepared for the SameSite labeling policy. The SameSite features are being
enabled for Chrome Stable channel users on versions 80 and 81 (who should update
Chrome!), 83, as well as the newly released 84.

**May 28, 2020**

We are planning to resume our SameSite cookie enforcement coinciding with the
stable release of Chrome 84 on July 14, with enforcement enabled for Chrome 80+.
(In other words, starting July 14, Chrome users on the older Stable releases
(80, 81, and 83 -- for whom we recommend installing the latest update!) as well
as the newly released Chrome 84 will gradually begin to receive the
SameSite-by-default behavior.) Read more on our [Chromium blog
post](https://blog.chromium.org/2020/05/resuming-samesite-cookie-changes-in-july.html).

**April 3, 2020**

[We’ve decided to temporarily roll back the enforcement of SameSite cookie
labeling on Chrome 80
Stable](https://blog.chromium.org/2020/04/temporarily-rolling-back-samesite.html)
and kSameSiteByDefaultCookies is once again set to
base::FEATURE_DISABLED_BY_DEFAULT in Chromium master.

**Feb 10, 2020**

The Chrome
[policies](/administrators/policy-list-3/cookie-legacy-samesite-policies)
LegacySameSiteCookieBehaviorEnabledForDomainList and
LegacySameSiteCookieBehaviorEnabled which revert to the legacy cookie behavior
for managed Chrome and ChromeOS instances will be available ~~for at least 12
months after the release of Chrome 80 stable~~ (**Edit - May 29, 2020**: until
at least July 14, 2021). We will be monitoring feedback about these policies and
will provide updates on their lifetime as appropriate.

**Nov 21, 2019**

Starting in Canary version **80.0.3975.0**, the Lax+POST temporary mitigation
can be disabled for testing purposes using the new flag
--enable-features=SameSiteDefaultChecksMethodRigorously to allow testing of
sites and services in the eventual end state of the feature where the mitigation
has been removed. (Note that to enable multiple features, you must append the
feature name to the comma-separated list of params for the --enable-features
flag. Do not use multiple separate --enable-features flags.)

In addition, there is [a bug](https://crbug.com/1027318) affecting Chrome 78-79
which causes spurious SameSite warning messages to be emitted to the console
when the user has cookies for other domains on the same site as a resource
fetched in a cross-site context. We apologize for the confusion. This will be
fixed in Chrome 80.

**Nov 1, 2019**

Clearing up some misconceptions and providing additional information about "Lax
+ POST" (which is mentioned briefly on the [chromestatus.com
page](https://www.chromestatus.com/feature/5088147346030592)):

*   "Lax + POST" does *not* result in the legacy behavior (i.e. the old
            behavior before the SameSite changes).
*   “Lax + POST” is an intervention for Lax-by-default cookies (cookies
            that don’t specify a \`SameSite\` attribute) which allows these
            cookies to be sent on top-level cross-site POST requests if they are
            at most 2 minutes old. “Normal” Lax cookies are not sent on
            cross-site POST requests (or any other cross-site requests with a
            non-idempotent HTTP method such as PUT). This intervention was put
            in place to mitigate breakage to some POST-based login flows.
*   If “Lax + POST” is affecting the cookies you are testing (i.e. if
            your cookie would have been excluded if not for the "+ POST"
            behavior due to its age), you will see a message in the DevTools
            console about the 2 minute threshold. This can be useful for
            debugging.
*   For integration testing (if your cookie needs to be sent on
            cross-site POST requests), we recommend test cases with cookie age
            both below and above the threshold. For this, there is a
            command-line flag --enable-features=ShortLaxAllowUnsafeThreshold,
            which will lower the 2 minute threshold to 10 seconds, so that your
            test doesn’t have to wait for 2 whole minutes. This flag is
            available in Chrome 79.0.3945.16 and newer. (Note that if you are
            also using other --enable-features flags such as
            --enable-features=SameSiteByDefaultCookies,CookiesWithoutSameSiteMustBeSecure,
            you must append the feature name to the comma-separated list rather
            than use multiple --enable-features flags.)
*   Note that the 2-minute window for "Lax+POST" is a temporary
            intervention and will be removed at some point in the future (some
            time after the Stable launch of Chrome 80), at which point cookies
            involved in these flows will require \`SameSite=None\` and
            \`Secure\` even if under 2 minutes old.

**Oct 2, 2019**

In response to feedback from users and enterprise customers, we are deferring
the experimental Beta launch of the "SameSite=Lax by Default" and "SameSite=None
requires Secure" features from Chrome 78 Beta to Chrome 79 Beta. Users of Chrome
78 Beta will not experience any change or disruption in cookie behavior.

When the experiment is launched to Chrome 79 Beta users, domain-joined or
enterprise-registered machines will be included in the experiment. Instead of
excluding them from the experiment entirely, policies will be made available in
Chrome 79 to manage the experimental behavior. This will provide extra time for
administrators to configure and test the policies in advance of the Stable
launch in Chrome 80.

One policy will allow administrators to specify a list of domains on which
cookies should be handled according to the legacy behavior
(LegacySameSiteCookieBehaviorEnabledForDomainList), and a second policy will
provide the option to set the global default to legacy SameSite behavior for all
cookies (LegacySameSiteCookieBehaviorEnabled). More details about these policies
will follow in future enterprise release notes before the Chrome 79 release.

These features will still become the default behavior on Stable starting in
Chrome 80.

**Sept 30, 2019**

**Note (Jan 30, 2020): Check out our more detailed [tips for testing and
debugging](/updates/same-site/test-debug).**

To test whether your sites may be affected by the SameSite changes:

    Go to chrome://flags and enable #same-site-by-default-cookies and
    #cookies-without-same-site-must-be-secure. Restart the browser for the
    changes to take effect.

    Test your sites, with a focus on anything involving federated login flows,
    multiple domains, or cross-site embedded content. Note that, because of the
    2 minute time threshold for the "Lax+POST" intervention, for any flows
    involving POST requests, you may want to test with and without a long (&gt;
    2 minute) delay.

    If your site stops working:

    *   Try turning off #cookies-without-same-site-must-be-secure. If
                this fixes the issue, you need to set \`Secure\` on any
                \`SameSite=None\` cookies your site may be relying upon. (This
                may require upgrading HTTP sites to HTTPS.)
    *   Try turning off both flags. If this fixes the issue, you need to
                identify the cookies being accessed in a cross-site context and
                apply the attributes \`SameSite=None\` and \`Secure\` to them.
                See ["SameSite cookies
                explained"](https://web.dev/samesite-cookies-explained/) for
                more information. If you are not the developer of the site,
                please reach out to the developer and/or vendor who authored the
                site.
    *   For flows involving POST requests, if a short delay (&lt; 2
                minutes) works but a long delay (&gt; 2 minutes) does not work,
                you will also need to add \`SameSite=None\` and \`Secure\` to
                the relevant cookies if the operation in question may take
                longer than 2 minutes. Note that the 2-minute window for
                "Lax+POST" is a temporary intervention and will be removed at
                some point in the future (some time after the Stable launch of
                Chrome 80), at which point cookies involved in these flows will
                require \`SameSite=None\` and \`Secure\` even if under 2
                minutes.

If you are an IT administrator managing a Chrome deployment for your
organization, policies will temporarily be made available to maintain Chrome's
existing behavior for your users. This is to give enterprises extra time to roll
out and test changes. You have two options:

1.  (Recommended) Apply the
            LegacySameSiteCookieBehaviorEnabledForDomainList policy to the
            specific domains on which cookies require legacy behavior.
2.  (Less recommended due to security and privacy implications) Apply
            the LegacySameSiteCookieBehaviorEnabled policy to revert all cookies
            to legacy behavior.

These policies will be made available starting in ~~Chrome 80~~. Chrome 79.
**(See Oct 2, 2019 update.)**

**Sept 26, 2019**

Starting in Chrome 80, cookies that do not specify a SameSite attribute will be
treated as if they were SameSite=Lax with the additional behavior that they will
still be included in [POST
requests](https://www.chromestatus.com/feature/5088147346030592) to ease the
transition for existing sites. Cookies that still need to be delivered in a
cross-site context can explicitly request SameSite=None, and must also be marked
Secure and delivered over HTTPS. We will provide policies if you need to
configure Chrome Browser to temporarily revert to legacy SameSite behavior.

**This section is obsolete: See Oct 2, 2019 update.**

~~While experiments for this change will be rolling out to Chrome 78 Beta users,
the Beta SameSite experiment rollout will exclude Windows and Mac devices that
are joined to a domain and Chrome OS devices that are enterprise-registered.
Beta users on Linux, iOS, Android, and Android Webview will also not be affected
by the experiments at this time. For Chrome Beta users unaffected by the
experiments, there should be no change in behavior to login services or embedded
content.~~

The new SameSite rules will become the default behavior on Stable in Chrome 80,
but the changes will be limited to pre-Stable versions of Chrome until then.

Policies to manage this behavior will be made available when it becomes the
default behavior for Chrome 80. One policy will allow administrators to specify
a list of domains on which cookies should be handled according to the legacy
behavior, and second policy will provide the option to set the global default to
legacy SameSite behavior for all cookies. More details about these policies will
follow in future enterprise release notes before the Chrome 80 release.

Chrome continues to engage with members of the web community and welcomes input
on these SameSite changes via our forum:
<https://groups.google.com/a/chromium.org/forum/#!forum/blink-dev>
