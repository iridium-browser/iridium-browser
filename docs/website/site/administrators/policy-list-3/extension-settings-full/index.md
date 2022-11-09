---
breadcrumbs:
- - /administrators
  - Documentation for Administrators
- - /administrators/policy-list-3
  - Policy List
page_name: extension-settings-full
title: Extension Settings Full Description
---

**For help setting this policy, see the help center for examples for
[Windows](https://support.google.com/chrome/a/answer/7532015?hl=en&ref_topic=7517516),
[macOS](https://support.google.com/chrome/a/answer/7517624?hl=en&ref_topic=7517516),
and
[Linux](https://support.google.com/chrome/a/answer/7517525?hl=en&ref_topic=7517516).**

This policy controls multiple settings, including settings controlled by any
existing extension-related policies. This policy will override any legacy
policies if both are set.

This policy maps an extension ID to its configuration. With an extension ID,
configuration will be applied to the specified extension only. A default
configuration can be set for the special ID "\*", which will apply to all
extensions that don't have a custom configuration set in this policy.

The configuration for each extension is another dictionary that can contain the
fields documented below.

*   "installation_mode": Maps to a string indicating the installation
            mode for the extension. The valid strings are:
    *   "allowed": allows the extension to be installed by the user.
                This is the default behavior.
    *   "blocked": blocks installation of the extension.
    *   "removed" 3: blocks installation of the extension and removes it
                from the device if already installed.
    *   "force_installed" 1: the extension is automatically installed
                and can't be removed by the user.
    *   "normal_installed" 1: the extension is automatically installed
                but can be disabled by the user.
*   "update_url": Maps to a string indicating where Chrome can download
            a force_installed or normal_installed extension.
    *   The update URL set in this policy is only used for the initial
                installation; subsequent updates of the extension will use the
                update URL indicated in the extension's manifest.
    *   The update URL should point to an Update Manifest XML document
                as mentioned above.
    *   If installing from the Chrome Web Store, use the following URL
        *   http://clients2.google.com/service/update2/crx
*   "blocked_permissions": maps to a list of strings indicating the
            blocked API permissions for the extension.
    *   The permissions names are same as the permission strings
                declared in manifest of extension as described at
                <https://developer.chrome.com/extensions/declare_permissions>.
                This setting also can be configured for "\*" extension. If the
                extension requires a permission which is on the blocklist, the
                user will be blocked from installing the extension. If the user
                already has an extension installed which matches the blocklist
                it will not be allowed to load. If it contains a blocked
                permission as optional requirement, it will be handled in the
                normal way, but requesting conflicting permissions will be
                declined automatically at runtime.
*   "minimum_version_required": maps to a version string.
    *   The format of the version string is the same as the one used in
                extension manifest, as described at
                <https://developer.chrome.com/apps/manifest/version>. An
                extension with a version older than the specified minimum
                version will be disabled. This applies to force-installed
                extensions as well.
*   "install_sources" 2: Each item in this list is an extension-style
            [match
            pattern](https://developer.chrome.com/extensions/match_patterns).
    *   Users will be able to easily install items from any URL that
                matches an item in this list. Both the location of the \*.crx
                file and the page where the download is started from (i.e. the
                referrer) must be allowed by these patterns.
*   "allowed_types" 2: This setting lists the allowed types of
            extension/apps that can be installed in Google Chrome.
    *   The value is a list of strings, each of which should be one of
                the following:
    *   "extension", "theme", "user_script", "hosted_app",
                "legacy_packaged_app", "platform_app"
    *   See extensions documentation for more information on these
                types.
*   "blocked_install_message": This maps to a string specifying the
            error message to display to users if they're blocked from installing
            an extension.
    *   This setting allows you to append text to the generic error
                message displayed on the Chrome Web Store. This could be be used
                to direct users to your help desk, explain why a particular
                extension is blocked, or something else.
    *   This error message will be truncated if longer than 1000
                characters.
*   "runtime_blocked_hosts": Maps to a list of strings representing
            hosts whose webpages the extension will be blocked from modifying.
    *   This includes injecting javascript, altering and viewing
                webRequests / webNavigation, viewing and altering cookies,
                exceptions to the same-origin policy, etc.
    *   The format is similar to [match
                patterns](https://developer.chrome.com/extensions/match_patterns)
                except no paths may be defined.
        *   e.g. "\*://\*.example.com"
        *   This also supports effective TLD wildcarding e.g.
                    "\*://example.\*"
    *   The list can contain not more than 100 entries. Any further
                entry is discarded.
*   "runtime_allowed_hosts": Maps to a list of strings representing
            hosts that an extension can interact with regardless of whether they
            are listed in "runtime_blocked_hosts"
    *   This is the same format as "runtime_blocked_hosts".
    *   The list can contain not more than 100 entries. Any further
                entry is discarded.

____________________________________________________________________________

1: This option is **not** valid for the "\*" id as Chrome wouldn't know which
extension to automatically install. The "update_url" setting MUST be set for
this extension or the policy will be invalid.

2: This settings can be used **only** for the default configuration "\*".

3: This setting is available since Chrome 75.
