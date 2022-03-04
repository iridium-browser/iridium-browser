---
breadcrumbs:
- - /administrators
  - Documentation for Administrators
page_name: ios-mdm-policy-format
title: iOS MDM Policy Format
---

Policy Support on Chrome on iOS is being removed in Chrome 48 as part of
Chrome's move to WKWebView where supporting many of the policies was not
possible.

Policies can be configured for Chrome on iOS via MDM starting with version 35.

The iOS device must be enrolled for Mobile Device Management (MDM) with an MDM
vendor to enable this feature. This page documents the format of the managed app
configuration settings that Chrome expects, and is meant to help MDM vendors
integrate Chrome management into their solutions.

This uses the managed app configuration API introduced in iOS 7, and only work
with device running iOS 7 or later. For an introduction to the API see session
301 from WWDC 2013, "Extending Your Apps for Enterprise and Education Use", at
<https://developer.apple.com/videos/wwdc/2013/?id=301>.

#### Keys loaded by Chrome

The managed app configuration pushed to Chrome should be an NSDictionary,
encoded as a &lt;dict&gt; element in the Plist format. Chrome then looks for 2
keys in this dictionary:

*   **ChromePolicy** is the main key to configure policies for Chrome.
            This key must map to another NSDictionary, which then maps a Chrome
            policy to its value.

*   **EncodedChromePolicy** is a fallback key which is loaded only if
            ChromePolicy is not present. This key supports the same policies but
            its value should be a single NSString (encoded as a &lt;string&gt;
            on the wire), which contains a serialized Plist encoded in Base64.

ChromePolicy is the recommended key to configure Chrome. EncodedChromePolicy may
be used by legacy products that are limited to String values. As of version 35
Chrome doesn't load any other keys; new keys may be introduced in future
versions.

#### Example ChromePolicy

This example contains the full Plist that can be set down as the managed app
configuration for Chrome:

```
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC '-//Apple//DTD PLIST 1.0//EN' 'http://www.apple.com/DTDs/PropertyList-1.0.dtd'>
<plist version="1.0">
 <dict>
 <key>ChromePolicy</key>
 <dict>
 <key>AutoFillEnabled</key>
 <false/>
 <key>CookiesAllowedForUrls</key>
 <array>
 <string>http://www.example.com</string>
 <string>[*.]example.edu</string>
 </array>
 <key>CookiesBlockedForUrls</key>
 <array>
 <string>http://www.example.com</string>
 <string>[*.]example.edu</string>
 </array>
 <key>CookiesSessionOnlyForUrls</key>
 <array>
 <string>http://www.example.com</string>
 <string>[*.]example.edu</string>
 </array>
 <key>DefaultCookiesSetting</key>
 <integer>1</integer>
 <key>DefaultPopupsSetting</key>
 <integer>1</integer>
 <key>DefaultSearchProviderEnabled</key>
 <true/>
 <key>DefaultSearchProviderKeyword</key>
 <string>mis</string>
 <key>DefaultSearchProviderName</key>
 <string>My Intranet Search</string>
 <key>DefaultSearchProviderSearchURL</key>
 <string>http://search.my.company/search?q={searchTerms}</string>
 <key>ManagedBookmarks</key>
 <array>
 <dict>
 <key>name</key>
 <string>Google</string>
 <key>url</key>
 <string>google.com</string>
 </dict>
 <dict>
 <key>name</key>
 <string>Youtube</string>
 <key>url</key>
 <string>youtube.com</string>
 </dict>
 </array>
 <key>PasswordManagerEnabled</key>
 <true/>
 <key>PopupsAllowedForUrls</key>
 <array>
 <string>http://www.example.com</string>
 <string>[*.]example.edu</string>
 </array>
 <key>PopupsBlockedForUrls</key>
 <array>
 <string>http://www.example.com</string>
 <string>[*.]example.edu</string>
 </array>
 <key>ProxyBypassList</key>
 <string>http://www.example1.com,http://www.example2.com,http://internalsite/</string>
 <key>ProxyMode</key>
 <string>direct</string>
 <key>ProxyPacUrl</key>
 <string>http://internal.site/example.pac</string>
 <key>ProxyServer</key>
 <string>123.123.123.123:8080</string>
 <key>SearchSuggestEnabled</key>
 <true/>
 <key>TranslateEnabled</key>
 <true/>
 <key>URLBlocklist</key>
 <array>
 <string>example.com</string>
 <string>https://ssl.server.com</string>
 <string>hosting.com/bad_path</string>
 <string>http://server:8080/path</string>
 <string>.exact.hostname.com</string>
 </array>
 <key>URLAllowlist</key>
 <array>
 <string>example.com</string>
 <string>https://ssl.server.com</string>
 <string>hosting.com/bad_path</string>
 <string>http://server:8080/path</string>
 <string>.exact.hostname.com</string>
 </array>
 </dict>
 </dict>
</plist>
```

#### Example EncodedChromePolicy

This example contains an EncodedChromePolicy value that contains the same Plist
as above, after being encoded in Base64:

```
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC '-//Apple//DTD PLIST 1.0//EN' 'http://www.apple.com/DTDs/PropertyList-1.0.dtd'>
<plist version="1.0">
 <dict>
 <key>EncodedChromePolicy</key>
 <string>PD94bWwgdmVyc2lvbj0iMS4wIiA/PjwhRE9DVFlQRSBwbGlzdCAgUFVCTElDICctLy9BcHBsZS8vRFREIFBMSVNUIDEuMC8vRU4nICAnaHR0cDovL3d3dy5hcHBsZS5jb20vRFREcy9Qcm9wZXJ0eUxpc3QtMS4wLmR0ZCc+PHBsaXN0IHZlcnNpb249IjEuMCI+PGRpY3Q+PGtleT5BdXRvRmlsbEVuYWJsZWQ8L2tleT48ZmFsc2UvPjxrZXk+Q29va2llc0FsbG93ZWRGb3JVcmxzPC9rZXk+PGFycmF5PjxzdHJpbmc+aHR0cDovL3d3dy5leGFtcGxlLmNvbTwvc3RyaW5nPjxzdHJpbmc+WyouXWV4YW1wbGUuZWR1PC9zdHJpbmc+PC9hcnJheT48a2V5PkNvb2tpZXNCbG9ja2VkRm9yVXJsczwva2V5PjxhcnJheT48c3RyaW5nPmh0dHA6Ly93d3cuZXhhbXBsZS5jb208L3N0cmluZz48c3RyaW5nPlsqLl1leGFtcGxlLmVkdTwvc3RyaW5nPjwvYXJyYXk+PGtleT5Db29raWVzU2Vzc2lvbk9ubHlGb3JVcmxzPC9rZXk+PGFycmF5PjxzdHJpbmc+aHR0cDovL3d3dy5leGFtcGxlLmNvbTwvc3RyaW5nPjxzdHJpbmc+WyouXWV4YW1wbGUuZWR1PC9zdHJpbmc+PC9hcnJheT48a2V5PkRlZmF1bHRDb29raWVzU2V0dGluZzwva2V5PjxpbnRlZ2VyPjE8L2ludGVnZXI+PGtleT5EZWZhdWx0UG9wdXBzU2V0dGluZzwva2V5PjxpbnRlZ2VyPjE8L2ludGVnZXI+PGtleT5EZWZhdWx0U2VhcmNoUHJvdmlkZXJFbmFibGVkPC9rZXk+PHRydWUvPjxrZXk+RGVmYXVsdFNlYXJjaFByb3ZpZGVyS2V5d29yZDwva2V5PjxzdHJpbmc+bWlzPC9zdHJpbmc+PGtleT5EZWZhdWx0U2VhcmNoUHJvdmlkZXJOYW1lPC9rZXk+PHN0cmluZz5NeSBJbnRyYW5ldCBTZWFyY2g8L3N0cmluZz48a2V5PkRlZmF1bHRTZWFyY2hQcm92aWRlclNlYXJjaFVSTDwva2V5PjxzdHJpbmc+aHR0cDovL3NlYXJjaC5teS5jb21wYW55L3NlYXJjaD9xPXtzZWFyY2hUZXJtc308L3N0cmluZz48a2V5Pk1hbmFnZWRCb29rbWFya3M8L2tleT48YXJyYXk+PGRpY3Q+PGtleT5uYW1lPC9rZXk+PHN0cmluZz5Hb29nbGU8L3N0cmluZz48a2V5PnVybDwva2V5PjxzdHJpbmc+Z29vZ2xlLmNvbTwvc3RyaW5nPjwvZGljdD48ZGljdD48a2V5Pm5hbWU8L2tleT48c3RyaW5nPllvdXR1YmU8L3N0cmluZz48a2V5PnVybDwva2V5PjxzdHJpbmc+eW91dHViZS5jb208L3N0cmluZz48L2RpY3Q+PC9hcnJheT48a2V5PlBhc3N3b3JkTWFuYWdlckVuYWJsZWQ8L2tleT48dHJ1ZS8+PGtleT5Qb3B1cHNBbGxvd2VkRm9yVXJsczwva2V5PjxhcnJheT48c3RyaW5nPmh0dHA6Ly93d3cuZXhhbXBsZS5jb208L3N0cmluZz48c3RyaW5nPlsqLl1leGFtcGxlLmVkdTwvc3RyaW5nPjwvYXJyYXk+PGtleT5Qb3B1cHNCbG9ja2VkRm9yVXJsczwva2V5PjxhcnJheT48c3RyaW5nPmh0dHA6Ly93d3cuZXhhbXBsZS5jb208L3N0cmluZz48c3RyaW5nPlsqLl1leGFtcGxlLmVkdTwvc3RyaW5nPjwvYXJyYXk+PGtleT5Qcm94eUJ5cGFzc0xpc3Q8L2tleT48c3RyaW5nPmh0dHA6Ly93d3cuZXhhbXBsZTEuY29tLGh0dHA6Ly93d3cuZXhhbXBsZTIuY29tLGh0dHA6Ly9pbnRlcm5hbHNpdGUvPC9zdHJpbmc+PGtleT5Qcm94eU1vZGU8L2tleT48c3RyaW5nPmRpcmVjdDwvc3RyaW5nPjxrZXk+UHJveHlQYWNVcmw8L2tleT48c3RyaW5nPmh0dHA6Ly9pbnRlcm5hbC5zaXRlL2V4YW1wbGUucGFjPC9zdHJpbmc+PGtleT5Qcm94eVNlcnZlcjwva2V5PjxzdHJpbmc+MTIzLjEyMy4xMjMuMTIzOjgwODA8L3N0cmluZz48a2V5PlNlYXJjaFN1Z2dlc3RFbmFibGVkPC9rZXk+PHRydWUvPjxrZXk+VHJhbnNsYXRlRW5hYmxlZDwva2V5Pjx0cnVlLz48a2V5PlVSTEJsYWNrbGlzdDwva2V5PjxhcnJheT48c3RyaW5nPmV4YW1wbGUuY29tPC9zdHJpbmc+PHN0cmluZz5odHRwczovL3NzbC5zZXJ2ZXIuY29tPC9zdHJpbmc+PHN0cmluZz5ob3N0aW5nLmNvbS9iYWRfcGF0aDwvc3RyaW5nPjxzdHJpbmc+aHR0cDovL3NlcnZlcjo4MDgwL3BhdGg8L3N0cmluZz48c3RyaW5nPi5leGFjdC5ob3N0bmFtZS5jb208L3N0cmluZz48L2FycmF5PjxrZXk+VVJMV2hpdGVsaXN0PC9rZXk+PGFycmF5PjxzdHJpbmc+ZXhhbXBsZS5jb208L3N0cmluZz48c3RyaW5nPmh0dHBzOi8vc3NsLnNlcnZlci5jb208L3N0cmluZz48c3RyaW5nPmhvc3RpbmcuY29tL2JhZF9wYXRoPC9zdHJpbmc+PHN0cmluZz5odHRwOi8vc2VydmVyOjgwODAvcGF0aDwvc3RyaW5nPjxzdHJpbmc+LmV4YWN0Lmhvc3RuYW1lLmNvbTwvc3RyaW5nPjwvYXJyYXk+PC9kaWN0PjwvcGxpc3Q+</string>
 </dict>
</plist>
```

#### Verifying a Plist

A .plist file can be verified with the plutil command on Mac OS X:

```
plutil -lint file.plist
```

The EncodedChromePolicy value should decode correctly as Base64:

```
base64 &lt; encoded.txt &gt; decoded.plist
plutil -lint decoded.plist
```

#### Policies supported by Chrome on iOS

The page at <https://chromeenterprise.google/policies/> indicates
which platforms are supported for each policy.
