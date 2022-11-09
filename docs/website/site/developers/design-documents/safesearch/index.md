---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: safesearch
title: SafeSearch
---

## Design

We wanted to introduce the option to enforce SafeSearch being active on all
Google Search queries done in Chrome.

In order to do that we introduced both a preference(currently
prefs::kForceSafeSearch) and a policy in Chrome which when set active will make
sure that the parameters safe=active and ssui=on are present.

The implementation of this feature is done in
chrome/browser/net/chrome_network_delegate.cc, where we see if it is a Google
search URL (if the feature is active of course). Since an extension could come
and modify the URL after us we let the extensions (if any) do their work first.
If the extension modifies the URL the network stack will detect that and it will
generate a OnBeforeRedirect which is followed by OnBeforeURLRequest.

In OnBeforeURLRequest we have two scenarios:

*   either the extensions(if any) return immediately and we enforce the
            SafeSearch parameters there.
*   or the extensions(if any) use the callback to announce completion.
            For this case we use a wrapper that enforces the SafeSearch
            parameters and then calls the regular callback.

While enforcing the SafeSearch parameters we split the query part of the URL
(everything between ? and #) into parameters. Then we remove any safe= and ssui=
parameters and add the correct values at the end. As stated
[here](/developers/chromium-string-usage) the query part is in ASCII..

## Other search engines

At the current time this change only affects Google Web Search queries which
come form any source (omnibox, google.tld websites). This is the result of a
lack of standardization when it comes to setting SafeSearch. Maybe in the future
we will pursue implementing this option for multiple search engines or thorugh
OpenSearch.

## Tests

This feature is tested using unit tests in
chrome/browser/net/chrome_network_delegate_unittest.cc. To run the test simply
compile the unit_tests executable and run:

out/Debug/unit_tests "--gtest_filter=ChromeNetworkDelegateSafesearchTest.\*"

The policy part of it is tested using browser tests in
chrome/browser/policy/policy_browsertest.cc. To run it:

out/Debug/browser_tests --gtest_filter=PolicyTest.ForceSafeSearch

Note: This feature is still in review and has not landed yet in Chrome.
Initially it will also not be visible to the users.
