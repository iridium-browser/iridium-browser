---
breadcrumbs:
- - /administrators
  - Documentation for Administrators
- - /administrators/policy-list-3
  - Policy List
page_name: atomic_groups
title: Atomic Policy Groups
---

## *This page is obsolete. Please see [Chrome Enterprise policy list](https://chromeenterprise.google/policies/) instead.*

Both Chromium and Google Chrome have some groups of policies that depend on each
other to provide control over a feature. These sets are represented by the
following policy groups. Given that policies can have multiple sources, only
values coming from the highest priority source will be applied. Values coming
from a lower priority source in the same group will be ignored. The order of
priority is defined in <https://support.google.com/chrome/a/?p=policy_order>.

<table>
<tr>
<td>Policy Name</td>
<td>Description</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#ActiveDirectoryManagement">ActiveDirectoryManagement</a></td>
<td>Microsoft® Active Directory® management settings</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#DeviceMachinePasswordChangeRate">DeviceMachinePasswordChangeRate</a></td>
<td>Machine password change rate</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#DeviceUserPolicyLoopbackProcessingMode">DeviceUserPolicyLoopbackProcessingMode</a></td>
<td>User policy loopback processing mode</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#DeviceKerberosEncryptionTypes">DeviceKerberosEncryptionTypes</a></td>
<td>Allowed Kerberos encryption types</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#DeviceGpoCacheLifetime">DeviceGpoCacheLifetime</a></td>
<td>GPO cache lifetime</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#DeviceAuthDataCacheLifetime">DeviceAuthDataCacheLifetime</a></td>
<td>Authentication data cache lifetime</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#Attestation">Attestation</a></td>
<td>Attestation</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#AttestationEnabledForDevice">AttestationEnabledForDevice</a></td>
<td>Enable remote attestation for the device</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#AttestationEnabledForUser">AttestationEnabledForUser</a></td>
<td>Enable remote attestation for the user</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#AttestationExtensionWhitelist">AttestationExtensionWhitelist</a></td>
<td>Extensions allowed to to use the remote attestation API</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#AttestationForContentProtectionEnabled">AttestationForContentProtectionEnabled</a></td>
<td>Enable the use of remote attestation for content protection for the device</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#BrowserSwitcher">BrowserSwitcher</a></td>
<td>Legacy Browser Support</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#AlternativeBrowserPath">AlternativeBrowserPath</a></td>
<td>Alternative browser to launch for configured websites.</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#AlternativeBrowserParameters">AlternativeBrowserParameters</a></td>
<td>Command-line parameters for the alternative browser.</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#BrowserSwitcherChromePath">BrowserSwitcherChromePath</a></td>
<td>Path to Chrome for switching from the alternative browser.</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#BrowserSwitcherChromeParameters">BrowserSwitcherChromeParameters</a></td>
<td>Command-line parameters for switching from the alternative browser.</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#BrowserSwitcherDelay">BrowserSwitcherDelay</a></td>
<td>Delay before launching alternative browser (milliseconds)</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#BrowserSwitcherEnabled">BrowserSwitcherEnabled</a></td>
<td>Enable the Legacy Browser Support feature.</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#BrowserSwitcherExternalSitelistUrl">BrowserSwitcherExternalSitelistUrl</a></td>
<td>URL of an XML file that contains URLs to load in an alternative browser.</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#BrowserSwitcherExternalGreylistUrl">BrowserSwitcherExternalGreylistUrl</a></td>
<td>URL of an XML file that contains URLs that should never trigger a browser switch.</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#BrowserSwitcherKeepLastChromeTab">BrowserSwitcherKeepLastChromeTab</a></td>
<td>Keep last tab open in Chrome.</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#BrowserSwitcherUrlList">BrowserSwitcherUrlList</a></td>
<td>Websites to open in alternative browser</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#BrowserSwitcherUrlGreylist">BrowserSwitcherUrlGreylist</a></td>
<td>Websites that should never trigger a browser switch.</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#BrowserSwitcherUseIeSitelist">BrowserSwitcherUseIeSitelist</a></td>
<td>Use Internet Explorer's SiteList policy for Legacy Browser Support.</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#ChromeReportingExtension">ChromeReportingExtension</a></td>
<td>Chrome Reporting Extension</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#ReportVersionData">ReportVersionData</a></td>
<td>Report OS and Chromium Version Information</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#ReportPolicyData">ReportPolicyData</a></td>
<td>Report Chromium Policy Information</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#ReportMachineIDData">ReportMachineIDData</a></td>
<td>Report Machine Identification information</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#ReportUserIDData">ReportUserIDData</a></td>
<td>Report User Identification information</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#ReportExtensionsAndPluginsData">ReportExtensionsAndPluginsData</a></td>
<td>Report Extensions and Plugins information</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#ReportSafeBrowsingData">ReportSafeBrowsingData</a></td>
<td>Report Safe Browsing information</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#CloudReportingEnabled">CloudReportingEnabled</a></td>
<td>Enables Chromium cloud reporting</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#ContentPack">ContentPack</a></td>
<td>Content pack</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#ContentPackDefaultFilteringBehavior">ContentPackDefaultFilteringBehavior</a></td>
<td>Default behavior for sites not in any content pack</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#ContentPackManualBehaviorHosts">ContentPackManualBehaviorHosts</a></td>
<td>Managed user manual exception hosts</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#ContentPackManualBehaviorURLs">ContentPackManualBehaviorURLs</a></td>
<td>Managed user manual exception URLs</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#CookiesSettings">CookiesSettings</a></td>
<td>Cookies settings</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#DefaultCookiesSetting">DefaultCookiesSetting</a></td>
<td>Default cookies setting</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#CookiesAllowedForUrls">CookiesAllowedForUrls</a></td>
<td>Allow cookies on these sites</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#CookiesBlockedForUrls">CookiesBlockedForUrls</a></td>
<td>Block cookies on these sites</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#CookiesSessionOnlyForUrls">CookiesSessionOnlyForUrls</a></td>
<td>Limit cookies from matching URLs to the current session</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#DateAndTime">DateAndTime</a></td>
<td>Date and time</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#SystemTimezone">SystemTimezone</a></td>
<td>Timezone</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#SystemTimezoneAutomaticDetection">SystemTimezoneAutomaticDetection</a></td>
<td>Configure the automatic timezone detection method</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#DefaultSearchProvider">DefaultSearchProvider</a></td>
<td>Default search provider</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#DefaultSearchProviderEnabled">DefaultSearchProviderEnabled</a></td>
<td>Enable the default search provider</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#DefaultSearchProviderName">DefaultSearchProviderName</a></td>
<td>Default search provider name</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#DefaultSearchProviderKeyword">DefaultSearchProviderKeyword</a></td>
<td>Default search provider keyword</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#DefaultSearchProviderSearchURL">DefaultSearchProviderSearchURL</a></td>
<td>Default search provider search URL</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#DefaultSearchProviderSuggestURL">DefaultSearchProviderSuggestURL</a></td>
<td>Default search provider suggest URL</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#DefaultSearchProviderInstantURL">DefaultSearchProviderInstantURL</a></td>
<td>Default search provider instant URL</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#DefaultSearchProviderIconURL">DefaultSearchProviderIconURL</a></td>
<td>Default search provider icon</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#DefaultSearchProviderEncodings">DefaultSearchProviderEncodings</a></td>
<td>Default search provider encodings</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#DefaultSearchProviderAlternateURLs">DefaultSearchProviderAlternateURLs</a></td>
<td>List of alternate URLs for the default search provider</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#DefaultSearchProviderSearchTermsReplacementKey">DefaultSearchProviderSearchTermsReplacementKey</a></td>
<td>Parameter controlling search term placement for the default search provider</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#DefaultSearchProviderImageURL">DefaultSearchProviderImageURL</a></td>
<td>Parameter providing search-by-image feature for the default search provider</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#DefaultSearchProviderNewTabURL">DefaultSearchProviderNewTabURL</a></td>
<td>Default search provider new tab page URL</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#DefaultSearchProviderSearchURLPostParams">DefaultSearchProviderSearchURLPostParams</a></td>
<td>Parameters for search URL which uses POST</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#DefaultSearchProviderSuggestURLPostParams">DefaultSearchProviderSuggestURLPostParams</a></td>
<td>Parameters for suggest URL which uses POST</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#DefaultSearchProviderInstantURLPostParams">DefaultSearchProviderInstantURLPostParams</a></td>
<td>Parameters for instant URL which uses POST</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#DefaultSearchProviderImageURLPostParams">DefaultSearchProviderImageURLPostParams</a></td>
<td>Parameters for image URL which uses POST</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#Display">Display</a></td>
<td>Display</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#DeviceDisplayResolution">DeviceDisplayResolution</a></td>
<td>Set display resolution and scale factor</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#DisplayRotationDefault">DisplayRotationDefault</a></td>
<td>Set default display rotation, reapplied on every reboot</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#Drive">Drive</a></td>
<td>Drive</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#DriveDisabled">DriveDisabled</a></td>
<td>Disable Drive in the Chromium OS Files app</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#DriveDisabledOverCellular">DriveDisabledOverCellular</a></td>
<td>Disable Google Drive over cellular connections in the Chromium OS Files app</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#Extensions">Extensions</a></td>
<td>Extensions</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#ExtensionInstallBlacklist">ExtensionInstallBlacklist</a></td>
<td>Configure extension installation blacklist</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#ExtensionInstallWhitelist">ExtensionInstallWhitelist</a></td>
<td>Configure extension installation whitelist</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#ExtensionInstallForcelist">ExtensionInstallForcelist</a></td>
<td>Configure the list of force-installed apps and extensions</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#ExtensionInstallSources">ExtensionInstallSources</a></td>
<td>Configure extension, app, and user script install sources</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#ExtensionAllowedTypes">ExtensionAllowedTypes</a></td>
<td>Configure allowed app/extension types</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#ExtensionAllowInsecureUpdates">ExtensionAllowInsecureUpdates</a></td>
<td>Allow insecure algorithms in integrity checks on extension updates and installs</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#ExtensionSettings">ExtensionSettings</a></td>
<td>Extension management settings</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#GoogleCast">GoogleCast</a></td>
<td>Google Cast</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#CastReceiverEnabled">CastReceiverEnabled</a></td>
<td>Enable casting content to the device</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#CastReceiverName">CastReceiverName</a></td>
<td>Name of the Google Cast destination</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#Homepage">Homepage</a></td>
<td>Homepage</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#HomepageLocation">HomepageLocation</a></td>
<td>Configure the home page URL</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#HomepageIsNewTabPage">HomepageIsNewTabPage</a></td>
<td>Use New Tab Page as homepage</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#NewTabPageLocation">NewTabPageLocation</a></td>
<td>Configure the New Tab page URL</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#ShowHomeButton">ShowHomeButton</a></td>
<td>Show Home button on toolbar</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#ImageSettings">ImageSettings</a></td>
<td>Image settings</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#DefaultImagesSetting">DefaultImagesSetting</a></td>
<td>Default images setting</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#ImagesAllowedForUrls">ImagesAllowedForUrls</a></td>
<td>Allow images on these sites</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#ImagesBlockedForUrls">ImagesBlockedForUrls</a></td>
<td>Block images on these sites</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#JavascriptSettings">JavascriptSettings</a></td>
<td>Javascript settings</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#DefaultJavaScriptSetting">DefaultJavaScriptSetting</a></td>
<td>Default JavaScript setting</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#JavaScriptAllowedForUrls">JavaScriptAllowedForUrls</a></td>
<td>Allow JavaScript on these sites</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#JavaScriptBlockedForUrls">JavaScriptBlockedForUrls</a></td>
<td>Block JavaScript on these sites</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#KeygenSettings">KeygenSettings</a></td>
<td>Keygen settings</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#DefaultKeygenSetting">DefaultKeygenSetting</a></td>
<td>Default key generation setting</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#KeygenAllowedForUrls">KeygenAllowedForUrls</a></td>
<td>Allow key generation on these sites</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#KeygenBlockedForUrls">KeygenBlockedForUrls</a></td>
<td>Block key generation on these sites</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#Kiosk">Kiosk</a></td>
<td>Kiosk settings</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#DeviceLocalAccounts">DeviceLocalAccounts</a></td>
<td>Device-local accounts</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#DeviceLocalAccountAutoLoginId">DeviceLocalAccountAutoLoginId</a></td>
<td>Device-local account for auto-login</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#DeviceLocalAccountAutoLoginDelay">DeviceLocalAccountAutoLoginDelay</a></td>
<td>Device-local account auto-login timer</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#DeviceLocalAccountAutoLoginBailoutEnabled">DeviceLocalAccountAutoLoginBailoutEnabled</a></td>
<td>Enable bailout keyboard shortcut for auto-login</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#DeviceLocalAccountPromptForNetworkWhenOffline">DeviceLocalAccountPromptForNetworkWhenOffline</a></td>
<td>Enable network configuration prompt when offline</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#LoginScreenOrigins">LoginScreenOrigins</a></td>
<td>Login and screen origins</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#DeviceLoginScreenIsolateOrigins">DeviceLoginScreenIsolateOrigins</a></td>
<td>Enable Site Isolation for specified origins</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#DeviceLoginScreenSitePerProcess">DeviceLoginScreenSitePerProcess</a></td>
<td>Enable Site Isolation for every site</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#NativeMessaging">NativeMessaging</a></td>
<td>Native messaging</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#NativeMessagingBlacklist">NativeMessagingBlacklist</a></td>
<td>Configure native messaging blacklist</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#NativeMessagingWhitelist">NativeMessagingWhitelist</a></td>
<td>Configure native messaging whitelist</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#NativeMessagingUserLevelHosts">NativeMessagingUserLevelHosts</a></td>
<td>Allow user-level Native Messaging hosts (installed without admin permissions)</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#NetworkFileShares">NetworkFileShares</a></td>
<td>Network File Shares settings</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#NetworkFileSharesAllowed">NetworkFileSharesAllowed</a></td>
<td>Controls Network File Shares for ChromeOS availability</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#NetBiosShareDiscoveryEnabled">NetBiosShareDiscoveryEnabled</a></td>
<td>Controls Network File Share discovery via NetBIOS</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#NTLMShareAuthenticationEnabled">NTLMShareAuthenticationEnabled</a></td>
<td>Controls enabling NTLM as an authentication protocol for SMB mounts</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#NetworkFileSharesPreconfiguredShares">NetworkFileSharesPreconfiguredShares</a></td>
<td>List of preconfigured network file shares.</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#NotificationsSettings">NotificationsSettings</a></td>
<td>Notification settings</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#DefaultNotificationsSetting">DefaultNotificationsSetting</a></td>
<td>Default notification setting</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#NotificationsAllowedForUrls">NotificationsAllowedForUrls</a></td>
<td>Allow notifications on these sites</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#NotificationsBlockedForUrls">NotificationsBlockedForUrls</a></td>
<td>Block notifications on these sites</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#PasswordManager">PasswordManager</a></td>
<td>Password manager</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#PasswordManagerEnabled">PasswordManagerEnabled</a></td>
<td>Enable saving passwords to the password manager</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#PasswordManagerAllowShowPasswords">PasswordManagerAllowShowPasswords</a></td>
<td>Allow users to show passwords in Password Manager (deprecated)</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#PasswordProtection">PasswordProtection</a></td>
<td>Password protection</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#PasswordProtectionWarningTrigger">PasswordProtectionWarningTrigger</a></td>
<td>Password protection warning trigger</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#PasswordProtectionLoginURLs">PasswordProtectionLoginURLs</a></td>
<td>Configure the list of enterprise login URLs where password protection service should capture fingerprint of password.</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#PasswordProtectionChangePasswordURL">PasswordProtectionChangePasswordURL</a></td>
<td>Configure the change password URL.</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#PinUnlock">PinUnlock</a></td>
<td>Pin unlock</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#PinUnlockMinimumLength">PinUnlockMinimumLength</a></td>
<td>Set the minimum length of the lock screen PIN</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#PinUnlockMaximumLength">PinUnlockMaximumLength</a></td>
<td>Set the maximum length of the lock screen PIN</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#PinUnlockWeakPinsAllowed">PinUnlockWeakPinsAllowed</a></td>
<td>Enable users to set weak PINs for the lock screen PIN</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#PluginVm">PluginVm</a></td>
<td>PluginVm</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#PluginVmAllowed">PluginVmAllowed</a></td>
<td>Allow devices to use a PluginVm on Chromium OS</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#PluginVmLicenseKey">PluginVmLicenseKey</a></td>
<td>PluginVm license key</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#PluginVmImage">PluginVmImage</a></td>
<td>PluginVm image</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#PluginsSettings">PluginsSettings</a></td>
<td>Plugins settings</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#DefaultPluginsSetting">DefaultPluginsSetting</a></td>
<td>Default Flash setting</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#PluginsAllowedForUrls">PluginsAllowedForUrls</a></td>
<td>Allow the Flash plugin on these sites</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#PluginsBlockedForUrls">PluginsBlockedForUrls</a></td>
<td>Block the Flash plugin on these sites</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#PopupsSettings">PopupsSettings</a></td>
<td>Popups settings</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#DefaultPopupsSetting">DefaultPopupsSetting</a></td>
<td>Default popups setting</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#PopupsAllowedForUrls">PopupsAllowedForUrls</a></td>
<td>Allow popups on these sites</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#PopupsBlockedForUrls">PopupsBlockedForUrls</a></td>
<td>Block popups on these sites</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#Proxy">Proxy</a></td>
<td>Proxy</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#ProxyMode">ProxyMode</a></td>
<td>Choose how to specify proxy server settings</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#ProxyServerMode">ProxyServerMode</a></td>
<td>Choose how to specify proxy server settings</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#ProxyServer">ProxyServer</a></td>
<td>Address or URL of proxy server</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#ProxyPacUrl">ProxyPacUrl</a></td>
<td>URL to a proxy .pac file</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#ProxyBypassList">ProxyBypassList</a></td>
<td>Proxy bypass rules</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#ProxySettings">ProxySettings</a></td>
<td>Proxy settings</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#QuickUnlock">QuickUnlock</a></td>
<td>Quick unlock</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#QuickUnlockModeWhitelist">QuickUnlockModeWhitelist</a></td>
<td>Configure allowed quick unlock modes</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#QuickUnlockTimeout">QuickUnlockTimeout</a></td>
<td>Set how often user has to enter password to use quick unlock</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#RemoteAccess">RemoteAccess</a></td>
<td>Remote access</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#RemoteAccessClientFirewallTraversal">RemoteAccessClientFirewallTraversal</a></td>
<td>Enable firewall traversal from remote access client</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#RemoteAccessHostClientDomain">RemoteAccessHostClientDomain</a></td>
<td>Configure the required domain name for remote access clients</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#RemoteAccessHostClientDomainList">RemoteAccessHostClientDomainList</a></td>
<td>Configure the required domain names for remote access clients</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#RemoteAccessHostFirewallTraversal">RemoteAccessHostFirewallTraversal</a></td>
<td>Enable firewall traversal from remote access host</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#RemoteAccessHostDomain">RemoteAccessHostDomain</a></td>
<td>Configure the required domain name for remote access hosts</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#RemoteAccessHostDomainList">RemoteAccessHostDomainList</a></td>
<td>Configure the required domain names for remote access hosts</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#RemoteAccessHostRequireTwoFactor">RemoteAccessHostRequireTwoFactor</a></td>
<td>Enable two-factor authentication for remote access hosts</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#RemoteAccessHostTalkGadgetPrefix">RemoteAccessHostTalkGadgetPrefix</a></td>
<td>Configure the TalkGadget prefix for remote access hosts</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#RemoteAccessHostRequireCurtain">RemoteAccessHostRequireCurtain</a></td>
<td>Enable curtaining of remote access hosts</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#RemoteAccessHostAllowClientPairing">RemoteAccessHostAllowClientPairing</a></td>
<td>Enable or disable PIN-less authentication for remote access hosts</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#RemoteAccessHostAllowGnubbyAuth">RemoteAccessHostAllowGnubbyAuth</a></td>
<td>Allow gnubby authentication for remote access hosts</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#RemoteAccessHostAllowRelayedConnection">RemoteAccessHostAllowRelayedConnection</a></td>
<td>Enable the use of relay servers by the remote access host</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#RemoteAccessHostUdpPortRange">RemoteAccessHostUdpPortRange</a></td>
<td>Restrict the UDP port range used by the remote access host</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#RemoteAccessHostMatchUsername">RemoteAccessHostMatchUsername</a></td>
<td>Require that the name of the local user and the remote access host owner match</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#RemoteAccessHostTokenUrl">RemoteAccessHostTokenUrl</a></td>
<td>URL where remote access clients should obtain their authentication token</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#RemoteAccessHostTokenValidationUrl">RemoteAccessHostTokenValidationUrl</a></td>
<td>URL for validating remote access client authentication token</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#RemoteAccessHostTokenValidationCertificateIssuer">RemoteAccessHostTokenValidationCertificateIssuer</a></td>
<td>Client certificate for connecting to RemoteAccessHostTokenValidationUrl</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#RemoteAccessHostDebugOverridePolicies">RemoteAccessHostDebugOverridePolicies</a></td>
<td>Policy overrides for Debug builds of the remote access host</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#RemoteAccessHostAllowUiAccessForRemoteAssistance">RemoteAccessHostAllowUiAccessForRemoteAssistance</a></td>
<td>Allow remote users to interact with elevated windows in remote assistance sessions</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#RemoteAccessHostAllowFileTransfer">RemoteAccessHostAllowFileTransfer</a></td>
<td>Allow remote access users to transfer files to/from the host</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#RestoreOnStartup">RestoreOnStartup</a></td>
<td>Action on startup</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#RestoreOnStartup">RestoreOnStartup</a></td>
<td>Action on startup</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#RestoreOnStartupURLs">RestoreOnStartupURLs</a></td>
<td>URLs to open on startup</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#SAML">SAML</a></td>
<td>SAML</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#DeviceTransferSAMLCookies">DeviceTransferSAMLCookies</a></td>
<td>Transfer SAML IdP cookies during login</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#SafeBrowsing">SafeBrowsing</a></td>
<td>Safe Browsing settings</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#SafeBrowsingEnabled">SafeBrowsingEnabled</a></td>
<td>Enable Safe Browsing</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#SafeBrowsingExtendedReportingEnabled">SafeBrowsingExtendedReportingEnabled</a></td>
<td>Enable Safe Browsing Extended Reporting</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#SafeBrowsingExtendedReportingOptInAllowed">SafeBrowsingExtendedReportingOptInAllowed</a></td>
<td>Allow users to opt in to Safe Browsing extended reporting</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#SafeBrowsingWhitelistDomains">SafeBrowsingWhitelistDomains</a></td>
<td>Configure the list of domains on which Safe Browsing will not trigger warnings.</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#SupervisedUsers">SupervisedUsers</a></td>
<td>Supervised users</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#SupervisedUsersEnabled">SupervisedUsersEnabled</a></td>
<td>Enable supervised users</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#SupervisedUserCreationEnabled">SupervisedUserCreationEnabled</a></td>
<td>Enable creation of supervised users</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#SupervisedUserContentProviderEnabled">SupervisedUserContentProviderEnabled</a></td>
<td>Enable the supervised user content provider</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#UserAndDeviceReporting">UserAndDeviceReporting</a></td>
<td>User and device reporting</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#ReportDeviceVersionInfo">ReportDeviceVersionInfo</a></td>
<td>Report OS and firmware version</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#ReportDeviceBootMode">ReportDeviceBootMode</a></td>
<td>Report device boot mode</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#ReportDeviceUsers">ReportDeviceUsers</a></td>
<td>Report device users</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#ReportDeviceActivityTimes">ReportDeviceActivityTimes</a></td>
<td>Report device activity times</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#ReportDeviceLocation">ReportDeviceLocation</a></td>
<td>Report device location</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#ReportDeviceNetworkInterfaces">ReportDeviceNetworkInterfaces</a></td>
<td>Report device network interfaces</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#ReportDeviceHardwareStatus">ReportDeviceHardwareStatus</a></td>
<td>Report hardware status</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#ReportDeviceSessionStatus">ReportDeviceSessionStatus</a></td>
<td>Report information about active kiosk sessions</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#ReportDeviceBoardStatus">ReportDeviceBoardStatus</a></td>
<td>Report board status</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#ReportDevicePowerStatus">ReportDevicePowerStatus</a></td>
<td>Report power status</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#ReportDeviceStorageStatus">ReportDeviceStorageStatus</a></td>
<td>Report storage status</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#ReportUploadFrequency">ReportUploadFrequency</a></td>
<td>Frequency of device status report uploads</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#ReportArcStatusEnabled">ReportArcStatusEnabled</a></td>
<td>Report information about status of Android</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#HeartbeatEnabled">HeartbeatEnabled</a></td>
<td>Send network packets to the management server to monitor online status</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#HeartbeatFrequency">HeartbeatFrequency</a></td>
<td>Frequency of monitoring network packets</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#LogUploadEnabled">LogUploadEnabled</a></td>
<td>Send system logs to the management server</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#DeviceMetricsReportingEnabled">DeviceMetricsReportingEnabled</a></td>
<td>Enable metrics reporting</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#WebUsbSettings">WebUsbSettings</a></td>
<td>Web USB settings</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#DefaultWebUsbGuardSetting">DefaultWebUsbGuardSetting</a></td>
<td>Control use of the WebUSB API</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#DeviceWebUsbAllowDevicesForUrls">DeviceWebUsbAllowDevicesForUrls</a></td>
<td>Automatically grant permission to these sites to connect to USB devices with the given vendor and product IDs.</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#WebUsbAllowDevicesForUrls">WebUsbAllowDevicesForUrls</a></td>
<td>Automatically grant permission to these sites to connect to USB devices with the given vendor and product IDs.</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#WebUsbAskForUrls">WebUsbAskForUrls</a></td>
<td>Allow WebUSB on these sites</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#WebUsbBlockedForUrls">WebUsbBlockedForUrls</a></td>
<td>Block WebUSB on these sites</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#WiFi">WiFi</a></td>
<td>WiFi</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#DeviceWiFiFastTransitionEnabled">DeviceWiFiFastTransitionEnabled</a></td>
<td>Enable 802.11r Fast Transition</td>
</tr>
<tr>
<td><a href="/administrators/policy-list-3#DeviceWiFiAllowed">DeviceWiFiAllowed</a></td>
<td>Enable WiFi</td>
</tr>
</table>
