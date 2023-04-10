---
breadcrumbs:
- - /updates
  - updates
- - /updates/same-site
  - SameSite Updates
page_name: incompatible-clients
title: 'SameSite=None: Known Incompatible Clients'
---

Last updated: Nov 18, 2019

Some user agents are known to be incompatible with the \`SameSite=None\`
attribute.

*   Versions of Chrome from Chrome 51 to Chrome 66 (inclusive on both
            ends). These Chrome versions will reject a cookie with
            \`SameSite=None\`. This also affects older versions of
            [Chromium-derived
            browsers](https://en.wikipedia.org/wiki/Chromium_(web_browser)#Browsers_based_on_Chromium),
            as well as Android WebView. This behavior was correct according to
            the version of the cookie specification at that time, but with the
            addition of the new "None" value to the specification, this behavior
            has been updated in Chrome 67 and newer. (Prior to Chrome 51, the
            SameSite attribute was ignored entirely and all cookies were treated
            as if they were \`SameSite=None\`.)
*   Versions of UC Browser on Android prior to version 12.13.2. Older
            versions will reject a cookie with \`SameSite=None\`. This behavior
            was correct according to the version of the cookie specification at
            that time, but with the addition of the new "None" value to the
            specification, this behavior has been updated in newer versions of
            UC Browser.
*   Versions of Safari and embedded browsers on MacOS 10.14 and all
            browsers on iOS 12. These versions will erroneously treat cookies
            marked with \`SameSite=None\` as if they were marked
            \`SameSite=Strict\`. This
            [bug](https://bugs.webkit.org/show_bug.cgi?id=198181) has been fixed
            on newer versions of iOS and MacOS.

Here is a potential approach to working around incompatible clients (in
pseudocode). If you implement this sample, we highly encourage you to do your
own testing to ensure that your implementation is working as intended. Note: The
sample regular expression patterns below may not be perfect, as User-Agent
strings can vary widely; we encourage you to use a tested User-Agent parsing
library if possible.

// Copyright 2019 Google LLC. // SPDX-License-Identifier: Apache-2.0 // Don’t
send \`SameSite=None\` to known incompatible clients. bool
shouldSendSameSiteNone(string useragent): return
!isSameSiteNoneIncompatible(useragent) // Classes of browsers known to be
incompatible. bool isSameSiteNoneIncompatible(string useragent): return
hasWebKitSameSiteBug(useragent) || dropsUnrecognizedSameSiteCookies(useragent)
bool hasWebKitSameSiteBug(string useragent): return isIosVersion(major:12,
useragent) || (isMacosxVersion(major:10, minor:14, useragent) &&
(isSafari(useragent) || isMacEmbeddedBrowser(useragent))) bool
dropsUnrecognizedSameSiteCookies(string useragent): if isUcBrowser(useragent):
return !isUcBrowserVersionAtLeast(major:12, minor:13, build:2, useragent) return
isChromiumBased(useragent) && isChromiumVersionAtLeast(major:51, useragent) &&
!isChromiumVersionAtLeast(major:67, useragent) // Regex parsing of User-Agent
string. (See note above!) bool isIosVersion(int major, string useragent): string
regex = "\\(iP.+; CPU .\*OS (\\d+)\[_\\d\]\*.\*\\) AppleWebKit\\/" // Extract
digits from first capturing group. return useragent.regexMatch(regex)\[0\] ==
intToString(major) bool isMacosxVersion(int major, int minor, string useragent):
string regex = "\\(Macintosh;.\*Mac OS X (\\d+)_(\\d+)\[_\\d\]\*.\*\\)
AppleWebKit\\/" // Extract digits from first and second capturing groups. return
(useragent.regexMatch(regex)\[0\] == intToString(major)) &&
(useragent.regexMatch(regex)\[1\] == intToString(minor)) bool isSafari(string
useragent): string safari_regex = "Version\\/.\* Safari\\/" return
useragent.regexContains(safari_regex) && !isChromiumBased(useragent) bool
isMacEmbeddedBrowser(string useragent): string regex = "^Mozilla\\/\[\\.\\d\]+
\\(Macintosh;.\*Mac OS X \[_\\d\]+\\) " + "AppleWebKit\\/\[\\.\\d\]+ \\(KHTML,
like Gecko\\)$" return useragent.regexContains(regex) bool
isChromiumBased(string useragent): string regex = "Chrom(e|ium)" return
useragent.regexContains(regex) bool isChromiumVersionAtLeast(int major, string
useragent): string regex = "Chrom\[^ \\/\]+\\/(\\d+)\[\\.\\d\]\* " // Extract
digits from first capturing group. int version =
stringToInt(useragent.regexMatch(regex)\[0\]) return version &gt;= major bool
isUcBrowser(string useragent): string regex = "UCBrowser\\/" return
useragent.regexContains(regex) bool isUcBrowserVersionAtLeast(int major, int
minor, int build, string useragent): string regex =
"UCBrowser\\/(\\d+)\\.(\\d+)\\.(\\d+)\[\\.\\d\]\* " // Extract digits from three
capturing groups. int major_version =
stringToInt(useragent.regexMatch(regex)\[0\]) int minor_version =
stringToInt(useragent.regexMatch(regex)\[1\]) int build_version =
stringToInt(useragent.regexMatch(regex)\[2\]) if major_version != major: return
major_version &gt; major if minor_version != minor: return minor_version &gt;
minor return build_version &gt;= build
