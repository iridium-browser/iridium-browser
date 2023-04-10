---
breadcrumbs:
- - /administrators
  - Documentation for Administrators
- - /administrators/policy-list-3
  - Policy List
page_name: deprecated-policies
title: Deprecated Policies
---

### DefaultMediaStreamSetting (deprecated)

Default mediastream settingData type:Integer \[Windows:REG_DWORD\]Windows
registry location for Windows
clients:Software\\Policies\\Google\\Chrome\\DefaultMediaStreamSettingWindows
registry location for Google Chrome OS
clients:Software\\Policies\\Google\\ChromeOS\\DefaultMediaStreamSettingMac/Linux
preference name:DefaultMediaStreamSettingSupported on:

*   Google Chrome (Linux, Mac, Windows) since version 22
*   Google Chrome OS (Google Chrome OS) since version 22

Supported features:Dynamic Policy Refresh: Yes, Per Profile: YesDescription:

Allows you to set whether websites are allowed to get access to media capture
devices. Access to media capture devices can be allowed by default, or the user
can be asked every time a website wants to get access to media capture devices.

If this policy is left not set, 'PromptOnAccess' will be used and the user will
be able to change it.

*   2 = Do not allow any site to access the camera and microphone
*   3 = Ask every time a site wants to access the camera and/or
            microphone

Example value:0x00000002 (Windows), 2 (Linux), 2 (Mac)[Back to top](#top)

### ScreenDimDelayAC (deprecated)

Screen dim delay when running on AC powerData type:Integer
\[Windows:REG_DWORD\]Windows registry location for Google Chrome OS
clients:Software\\Policies\\Google\\ChromeOS\\ScreenDimDelayACSupported on:

*   Google Chrome OS (Google Chrome OS) since version 26

Supported features:Dynamic Policy Refresh: Yes, Per Profile: NoDescription:

Specifies the length of time without user input after which the screen is dimmed
when running on AC power.

When this policy is set to a value greater than zero, it specifies the length of
time that the user must remain idle before Google Chrome OS dims the screen.

When this policy is set to zero, Google Chrome OS does not dim the screen when
the user becomes idle.

When this policy is unset, a default length of time is used.

The policy value should be specified in milliseconds. Values are clamped to be
less than or equal the screen off delay (if set) and the idle delay.

Example value:0x000668a0 (Windows)[Back to top](#top)

### ScreenOffDelayAC (deprecated)

Screen off delay when running on AC powerData type:Integer
\[Windows:REG_DWORD\]Windows registry location for Google Chrome OS
clients:Software\\Policies\\Google\\ChromeOS\\ScreenOffDelayACSupported on:

*   Google Chrome OS (Google Chrome OS) since version 26

Supported features:Dynamic Policy Refresh: Yes, Per Profile: NoDescription:

Specifies the length of time without user input after which the screen is turned
off when running on AC power.

When this policy is set to a value greater than zero, it specifies the length of
time that the user must remain idle before Google Chrome OS turns off the
screen.

When this policy is set to zero, Google Chrome OS does not turn off the screen
when the user becomes idle.

When this policy is unset, a default length of time is used.

The policy value should be specified in milliseconds. Values are clamped to be
less than or equal the idle delay.

Example value:0x00075300 (Windows)[Back to top](#top)

### ScreenLockDelayAC (deprecated)

Screen lock delay when running on AC powerData type:Integer
\[Windows:REG_DWORD\]Windows registry location for Google Chrome OS
clients:Software\\Policies\\Google\\ChromeOS\\ScreenLockDelayACSupported on:

*   Google Chrome OS (Google Chrome OS) since version 26

Supported features:Dynamic Policy Refresh: Yes, Per Profile: NoDescription:

Specifies the length of time without user input after which the screen is locked
when running on AC power.

When this policy is set to a value greater than zero, it specifies the length of
time that the user must remain idle before Google Chrome OS locks the screen.

When this policy is set to zero, Google Chrome OS does not lock the screen when
the user becomes idle.

When this policy is unset, a default length of time is used.

The recommended way to lock the screen on idle is to enable screen locking on
suspend and have Google Chrome OS suspend after the idle delay. This policy
should only be used when screen locking should occur a significant amount of
time sooner than suspend or when suspend on idle is not desired at all.

The policy value should be specified in milliseconds. Values are clamped to be
less than the idle delay.

Example value:0x000927c0 (Windows)[Back to top](#top)

### IdleWarningDelayAC (deprecated)

Idle warning delay when running on AC powerData type:Integer
\[Windows:REG_DWORD\]Windows registry location for Google Chrome OS
clients:Software\\Policies\\Google\\ChromeOS\\IdleWarningDelayACSupported on:

*   Google Chrome OS (Google Chrome OS) since version 27

Supported features:Dynamic Policy Refresh: Yes, Per Profile: NoDescription:

Specifies the length of time without user input after which a warning dialog is
shown when running on AC power.

When this policy is set, it specifies the length of time that the user must
remain idle before Google Chrome OS shows a warning dialog telling the user that
the idle action is about to be taken.

When this policy is unset, no warning dialog is shown.

The policy value should be specified in milliseconds. Values are clamped to be
less than or equal the idle delay.

The warning message is only shown if the idle action is to logout or shut down.

Example value:0x000850e8 (Windows)[Back to top](#top)

### IdleDelayAC (deprecated)

Idle delay when running on AC powerData type:Integer
\[Windows:REG_DWORD\]Windows registry location for Google Chrome OS
clients:Software\\Policies\\Google\\ChromeOS\\IdleDelayACSupported on:

*   Google Chrome OS (Google Chrome OS) since version 26

Supported features:Dynamic Policy Refresh: Yes, Per Profile: NoDescription:

Specifies the length of time without user input after which the idle action is
taken when running on AC power.

When this policy is set, it specifies the length of time that the user must
remain idle before Google Chrome OS takes the idle action, which can be
configured separately.

When this policy is unset, a default length of time is used.

The policy value should be specified in milliseconds.

Example value:0x001b7740 (Windows)[Back to top](#top)

### ScreenDimDelayBattery (deprecated)

Screen dim delay when running on battery powerData type:Integer
\[Windows:REG_DWORD\]Windows registry location for Google Chrome OS
clients:Software\\Policies\\Google\\ChromeOS\\ScreenDimDelayBatterySupported on:

*   Google Chrome OS (Google Chrome OS) since version 26

Supported features:Dynamic Policy Refresh: Yes, Per Profile: NoDescription:

Specifies the length of time without user input after which the screen is dimmed
when running on battery power.

When this policy is set to a value greater than zero, it specifies the length of
time that the user must remain idle before Google Chrome OS dims the screen.

When this policy is set to zero, Google Chrome OS does not dim the screen when
the user becomes idle.

When this policy is unset, a default length of time is used.

The policy value should be specified in milliseconds. Values are clamped to be
less than or equal the screen off delay (if set) and the idle delay.

Example value:0x000493e0 (Windows)[Back to top](#top)

### ScreenOffDelayBattery (deprecated)

Screen off delay when running on battery powerData type:Integer
\[Windows:REG_DWORD\]Windows registry location for Google Chrome OS
clients:Software\\Policies\\Google\\ChromeOS\\ScreenOffDelayBatterySupported on:

*   Google Chrome OS (Google Chrome OS) since version 26

Supported features:Dynamic Policy Refresh: Yes, Per Profile: NoDescription:

Specifies the length of time without user input after which the screen is turned
off when running on battery power.

When this policy is set to a value greater than zero, it specifies the length of
time that the user must remain idle before Google Chrome OS turns off the
screen.

When this policy is set to zero, Google Chrome OS does not turn off the screen
when the user becomes idle.

When this policy is unset, a default length of time is used.

The policy value should be specified in milliseconds. Values are clamped to be
less than or equal the idle delay.

Example value:0x00057e40 (Windows)[Back to top](#top)

### ScreenLockDelayBattery (deprecated)

Screen lock delay when running on battery powerData type:Integer
\[Windows:REG_DWORD\]Windows registry location for Google Chrome OS
clients:Software\\Policies\\Google\\ChromeOS\\ScreenLockDelayBatterySupported
on:

*   Google Chrome OS (Google Chrome OS) since version 26

Supported features:Dynamic Policy Refresh: Yes, Per Profile: NoDescription:

Specifies the length of time without user input after which the screen is locked
when running on battery power.

When this policy is set to a value greater than zero, it specifies the length of
time that the user must remain idle before Google Chrome OS locks the screen.

When this policy is set to zero, Google Chrome OS does not lock the screen when
the user becomes idle.

When this policy is unset, a default length of time is used.

The recommended way to lock the screen on idle is to enable screen locking on
suspend and have Google Chrome OS suspend after the idle delay. This policy
should only be used when screen locking should occur a significant amount of
time sooner than suspend or when suspend on idle is not desired at all.

The policy value should be specified in milliseconds. Values are clamped to be
less than the idle delay.

Example value:0x000927c0 (Windows)[Back to top](#top)

### IdleWarningDelayBattery (deprecated)

Idle warning delay when running on battery powerData type:Integer
\[Windows:REG_DWORD\]Windows registry location for Google Chrome OS
clients:Software\\Policies\\Google\\ChromeOS\\IdleWarningDelayBatterySupported
on:

*   Google Chrome OS (Google Chrome OS) since version 27

Supported features:Dynamic Policy Refresh: Yes, Per Profile: NoDescription:

Specifies the length of time without user input after which a warning dialog is
shown when running on battery power.

When this policy is set, it specifies the length of time that the user must
remain idle before Google Chrome OS shows a warning dialog telling the user that
the idle action is about to be taken.

When this policy is unset, no warning dialog is shown.

The policy value should be specified in milliseconds. Values are clamped to be
less than or equal the idle delay.

The warning message is only shown if the idle action is to logout or shut down.

Example value:0x000850e8 (Windows)[Back to top](#top)

### IdleDelayBattery (deprecated)

Idle delay when running on battery powerData type:Integer
\[Windows:REG_DWORD\]Windows registry location for Google Chrome OS
clients:Software\\Policies\\Google\\ChromeOS\\IdleDelayBatterySupported on:

*   Google Chrome OS (Google Chrome OS) since version 26

Supported features:Dynamic Policy Refresh: Yes, Per Profile: NoDescription:

Specifies the length of time without user input after which the idle action is
taken when running on battery power.

When this policy is set, it specifies the length of time that the user must
remain idle before Google Chrome OS takes the idle action, which can be
configured separately.

When this policy is unset, a default length of time is used.

The policy value should be specified in milliseconds.

Example value:0x000927c0 (Windows)[Back to top](#top)

### IdleAction (deprecated)

Action to take when the idle delay is reachedData type:Integer
\[Windows:REG_DWORD\]Windows registry location for Google Chrome OS
clients:Software\\Policies\\Google\\ChromeOS\\IdleActionSupported on:

*   Google Chrome OS (Google Chrome OS) since version 26

Supported features:Dynamic Policy Refresh: Yes, Per Profile: NoDescription:

Note that this policy is deprecated and will be removed in the future.

This policy provides a fallback value for the more-specific IdleActionAC and
IdleActionBattery policies. If this policy is set, its value gets used if the
respective more-specific policy is not set.

When this policy is unset, behavior of the more-specific policies remains
unaffected.

*   0 = Suspend
*   1 = Log the user out
*   2 = Shut down
*   3 = Do nothing

Example value:0x00000000 (Windows)[Back to top](#top)

### IdleActionAC (deprecated)

Action to take when the idle delay is reached while running on AC powerData
type:Integer \[Windows:REG_DWORD\]Windows registry location for Google Chrome OS
clients:Software\\Policies\\Google\\ChromeOS\\IdleActionACSupported on:

*   Google Chrome OS (Google Chrome OS) since version 30

Supported features:Dynamic Policy Refresh: Yes, Per Profile: NoDescription:

When this policy is set, it specifies the action that Google Chrome OS takes
when the user remains idle for the length of time given by the idle delay, which
can be configured separately.

When this policy is unset, the default action is taken, which is suspend.

If the action is suspend, Google Chrome OS can separately be configured to
either lock or not lock the screen before suspending.

*   0 = Suspend
*   1 = Log the user out
*   2 = Shut down
*   3 = Do nothing

Example value:0x00000000 (Windows)[Back to top](#top)

### IdleActionBattery (deprecated)

Action to take when the idle delay is reached while running on battery powerData
type:Integer \[Windows:REG_DWORD\]Windows registry location for Google Chrome OS
clients:Software\\Policies\\Google\\ChromeOS\\IdleActionBatterySupported on:

*   Google Chrome OS (Google Chrome OS) since version 30

Supported features:Dynamic Policy Refresh: Yes, Per Profile: NoDescription:

When this policy is set, it specifies the action that Google Chrome OS takes
when the user remains idle for the length of time given by the idle delay, which
can be configured separately.

When this policy is unset, the default action is taken, which is suspend.

If the action is suspend, Google Chrome OS can separately be configured to
either lock or not lock the screen before suspending.

*   0 = Suspend
*   1 = Log the user out
*   2 = Shut down
*   3 = Do nothing

Example value:0x00000000 (Windows)[Back to top](#top)

### ProxyServerMode (deprecated)

Choose how to specify proxy server settingsData type:Integer \[Android:choice,
Windows:REG_DWORD\]Windows registry location for Windows
clients:Software\\Policies\\Google\\Chrome\\ProxyServerModeWindows registry
location for Google Chrome OS
clients:Software\\Policies\\Google\\ChromeOS\\ProxyServerModeMac/Linux
preference name:ProxyServerModeAndroid restriction name:ProxyServerModeSupported
on:

*   Google Chrome (Linux, Mac, Windows) since version 8
*   Google Chrome OS (Google Chrome OS) since version 11
*   Google Chrome (Android) since version 30

Supported features:Dynamic Policy Refresh: Yes, Per Profile: YesDescription:

This policy is deprecated, use ProxyMode instead.

Allows you to specify the proxy server used by Google Chrome and prevents users
from changing proxy settings.

This policy only takes effect if the ProxySettings policy has not been
specified.

If you choose to never use a proxy server and always connect directly, all other
options are ignored.

If you choose to use system proxy settings or auto detect the proxy server, all
other options are ignored.

If you choose manual proxy settings, you can specify further options in 'Address
or URL of proxy server', 'URL to a proxy .pac file' and 'Comma-separated list of
proxy bypass rules'. Only the HTTP proxy server with the highest priority is
available for ARC-apps.

For detailed examples, visit:
<https://www.chromium.org/developers/design-documents/network-settings#TOC-Command-line-options-for-proxy-sett>.

If you enable this setting, Google Chrome ignores all proxy-related options
specified from the command line.

Leaving this policy not set will allow the users to choose the proxy settings on
their own.

*   0 = Never use a proxy
*   1 = Auto detect proxy settings
*   2 = Manually specify proxy settings
*   3 = Use system proxy settings

Note for Google Chrome OS devices supporting Android apps:

You cannot force Android apps to use a proxy. A subset of proxy settings is made
available to Android apps, which they may voluntarily choose to honor. See the
ProxyMode policy for more details.

Example value:0x00000002 (Windows), 2 (Linux), 2 (Android), 2 (Mac)[Back to
top](#top)

### RemoteAccessHostClientDomain (deprecated)

Configure the required domain name for remote access clientsData type:String
\[Windows:REG_SZ\]Windows registry location for Windows
clients:Software\\Policies\\Google\\Chrome\\RemoteAccessHostClientDomainWindows
registry location for Google Chrome OS
clients:Software\\Policies\\Google\\ChromeOS\\RemoteAccessHostClientDomainMac/Linux
preference name:RemoteAccessHostClientDomainSupported on:

*   Google Chrome (Linux, Mac, Windows) since version 22
*   Google Chrome OS (Google Chrome OS) since version 41

Supported features:Dynamic Policy Refresh: Yes, Per Profile: NoDescription:

This policy is deprecated. Please use RemoteAccessHostClientDomainList instead.

Example value:"my-awesome-domain.com"[Back to top](#top)

### RemoteAccessHostDomain (deprecated)

Configure the required domain name for remote access hostsData type:String
\[Windows:REG_SZ\]Windows registry location for Windows
clients:Software\\Policies\\Google\\Chrome\\RemoteAccessHostDomainWindows
registry location for Google Chrome OS
clients:Software\\Policies\\Google\\ChromeOS\\RemoteAccessHostDomainMac/Linux
preference name:RemoteAccessHostDomainSupported on:

*   Google Chrome (Linux, Mac, Windows) since version 22
*   Google Chrome OS (Google Chrome OS) since version 41

Supported features:Dynamic Policy Refresh: Yes, Per Profile: NoDescription:

This policy is deprecated. Please use RemoteAccessHostDomainList instead.

Example value:"my-awesome-domain.com"[Back to top](#top)

### SafeBrowsingExtendedReportingOptInAllowed (deprecated)

Allow users to opt in to Safe Browsing extended reportingData type:Boolean
\[Windows:REG_DWORD\]Windows registry location for Windows
clients:Software\\Policies\\Google\\Chrome\\SafeBrowsingExtendedReportingOptInAllowedWindows
registry location for Google Chrome OS
clients:Software\\Policies\\Google\\ChromeOS\\SafeBrowsingExtendedReportingOptInAllowedMac/Linux
preference name:SafeBrowsingExtendedReportingOptInAllowedSupported on:

*   Google Chrome (Linux, Mac, Windows) since version 44
*   Google Chrome OS (Google Chrome OS) since version 44

Supported features:Dynamic Policy Refresh: Yes, Per Profile: YesDescription:

This setting is deprecated, use SafeBrowsingExtendedReportingEnabled instead.
Enabling or disabling SafeBrowsingExtendedReportingEnabled is equivalent to
setting SafeBrowsingExtendedReportingOptInAllowed to False.

Setting this policy to false stops users from choosing to send some system
information and page content to Google servers. If this setting is true or not
configured, then users will be allowed to send some system information and page
content to Safe Browsing to help detect dangerous apps and sites.

See <https://developers.google.com/safe-browsing> for more info on Safe
Browsing.

Example value:0x00000001 (Windows), true (Linux), &lt;true /&gt; (Mac)[Back to
top](#top)

### AutoFillEnabled (deprecated)

Enable AutoFillData type:Boolean \[Windows:REG_DWORD\]Windows registry location
for Windows clients:Software\\Policies\\Google\\Chrome\\AutoFillEnabledWindows
registry location for Google Chrome OS
clients:Software\\Policies\\Google\\ChromeOS\\AutoFillEnabledMac/Linux
preference name:AutoFillEnabledAndroid restriction name:AutoFillEnabledSupported
on:

*   Google Chrome (Linux, Mac, Windows) since version 8
*   Google Chrome OS (Google Chrome OS) since version 11
*   Google Chrome (Android) since version 30

Supported features:Can Be Recommended: Yes, Dynamic Policy Refresh: Yes, Per
Profile: YesDescription:

This policy is deprecated in M70, please use AutofillAddressEnabled and
AutofillCreditCardEnabled instead.

Enables Google Chrome's AutoFill feature and allows users to auto complete web
forms using previously stored information such as address or credit card
information.

If you disable this setting, AutoFill will be inaccessible to users.

If you enable this setting or do not set a value, AutoFill will remain under the
control of the user. This will allow them to configure AutoFill profiles and to
switch AutoFill on or off at their own discretion.

Example value:0x00000000 (Windows), false (Linux), false (Android), &lt;false
/&gt; (Mac)[Back to top](#top)

### DeveloperToolsDisabled (deprecated)

Disable Developer ToolsData type:Boolean \[Windows:REG_DWORD\]Windows registry
location for Windows
clients:Software\\Policies\\Google\\Chrome\\DeveloperToolsDisabledWindows
registry location for Google Chrome OS
clients:Software\\Policies\\Google\\ChromeOS\\DeveloperToolsDisabledMac/Linux
preference name:DeveloperToolsDisabledSupported on:

*   Google Chrome (Linux, Mac, Windows) since version 9
*   Google Chrome OS (Google Chrome OS) since version 11

Supported features:Dynamic Policy Refresh: Yes, Per Profile: YesDescription:

This policy is deprecated in M68, please use DeveloperToolsAvailability instead.

Disables the Developer Tools and the JavaScript console.

If you enable this setting, the Developer Tools can not be accessed and web-site
elements can not be inspected anymore. Any keyboard shortcuts and any menu or
context menu entries to open the Developer Tools or the JavaScript Console will
be disabled.

Setting this option to disabled or leaving it not set allows the user to use the
Developer Tools and the JavaScript console.

If the policy DeveloperToolsAvailability is set, the value of the policy
DeveloperToolsDisabled is ignored.

Note for Google Chrome OS devices supporting Android apps:

This policy also controls access to Android Developer Options. If you set this
policy to true, users cannot access Developer Options. If you set this policy to
false or leave it unset, users can access Developer Options by tapping seven
times on the build number in the Android settings app.

Example value:0x00000000 (Windows), false (Linux), &lt;false /&gt; (Mac)[Back to
top](#top)

### DisabledPlugins (deprecated)

Specify a list of disabled pluginsData type:List of stringsWindows registry
location for Windows
clients:Software\\Policies\\Google\\Chrome\\DisabledPluginsWindows registry
location for Google Chrome OS
clients:Software\\Policies\\Google\\ChromeOS\\DisabledPluginsMac/Linux
preference name:DisabledPluginsSupported on:

*   Google Chrome (Linux, Mac, Windows) since version 8
*   Google Chrome OS (Google Chrome OS) since version 11

Supported features:Dynamic Policy Refresh: Yes, Per Profile: YesDescription:

This policy is deprecated. Please use the DefaultPluginsSetting to control the
avalability of the Flash plugin and AlwaysOpenPdfExternally to control whether
the integrated PDF viewer should be used for opening PDF files.

Specifies a list of plugins that are disabled in Google Chrome and prevents
users from changing this setting.

The wildcard characters '\*' and '?' can be used to match sequences of arbitrary
characters. '\*' matches an arbitrary number of characters while '?' specifies
an optional single character, i.e. matches zero or one characters. The escape
character is '\\', so to match actual '\*', '?', or '\\' characters, you can put
a '\\' in front of them.

If you enable this setting, the specified list of plugins is never used in
Google Chrome. The plugins are marked as disabled in 'about:plugins' and users
cannot enable them.

Note that this policy can be overridden by EnabledPlugins and
DisabledPluginsExceptions.

If this policy is left not set the user can use any plugin installed on the
system except for hard-coded incompatible, outdated or dangerous plugins.

Example value:Windows (Windows
clients):Software\\Policies\\Google\\Chrome\\DisabledPlugins\\1 = "Java"
Software\\Policies\\Google\\Chrome\\DisabledPlugins\\2 = "Shockwave Flash"
Software\\Policies\\Google\\Chrome\\DisabledPlugins\\3 = "Chrome PDF
Viewer"Windows (Google Chrome OS
clients):Software\\Policies\\Google\\ChromeOS\\DisabledPlugins\\1 = "Java"
Software\\Policies\\Google\\ChromeOS\\DisabledPlugins\\2 = "Shockwave Flash"
Software\\Policies\\Google\\ChromeOS\\DisabledPlugins\\3 = "Chrome PDF
Viewer"Android/Linux:\[ "Java", "Shockwave Flash", "Chrome PDF Viewer"
\]Mac:&lt;array&gt; &lt;string&gt;Java&lt;/string&gt; &lt;string&gt;Shockwave
Flash&lt;/string&gt; &lt;string&gt;Chrome PDF Viewer&lt;/string&gt;
&lt;/array&gt;[Back to top](#top)

### DisabledPluginsExceptions (deprecated)

Specify a list of plugins that the user can enable or disableData type:List of
stringsWindows registry location for Windows
clients:Software\\Policies\\Google\\Chrome\\DisabledPluginsExceptionsWindows
registry location for Google Chrome OS
clients:Software\\Policies\\Google\\ChromeOS\\DisabledPluginsExceptionsMac/Linux
preference name:DisabledPluginsExceptionsSupported on:

*   Google Chrome (Linux, Mac, Windows) since version 11
*   Google Chrome OS (Google Chrome OS) since version 11

Supported features:Dynamic Policy Refresh: Yes, Per Profile: YesDescription:

This policy is deprecated. Please use the DefaultPluginsSetting to control the
avalability of the Flash plugin and AlwaysOpenPdfExternally to control whether
the integrated PDF viewer should be used for opening PDF files.

Specifies a list of plugins that user can enable or disable in Google Chrome.

The wildcard characters '\*' and '?' can be used to match sequences of arbitrary
characters. '\*' matches an arbitrary number of characters while '?' specifies
an optional single character, i.e. matches zero or one characters. The escape
character is '\\', so to match actual '\*', '?', or '\\' characters, you can put
a '\\' in front of them.

If you enable this setting, the specified list of plugins can be used in Google
Chrome. Users can enable or disable them in 'about:plugins', even if the plugin
also matches a pattern in DisabledPlugins. Users can also enable and disable
plugins that don't match any patterns in DisabledPlugins,
DisabledPluginsExceptions and EnabledPlugins.

This policy is meant to allow for strict plugin blacklisting where the
'DisabledPlugins' list contains wildcarded entries like disable all plugins '\*'
or disable all Java plugins '\*Java\*' but the administrator wishes to enable
some particular version like 'IcedTea Java 2.3'. This particular versions can be
specified in this policy.

Note that both the plugin name and the plugin's group name have to be exempted.
Each plugin group is shown in a separate section in about:plugins; each section
may have one or more plugins. For example, the "Shockwave Flash" plugin belongs
to the "Adobe Flash Player" group, and both names have to have a match in the
exceptions list if that plugin is to be exempted from the blacklist.

If this policy is left not set any plugin that matches the patterns in the
'DisabledPlugins' will be locked disabled and the user won't be able to enable
them.

Example value:Windows (Windows
clients):Software\\Policies\\Google\\Chrome\\DisabledPluginsExceptions\\1 =
"Java" Software\\Policies\\Google\\Chrome\\DisabledPluginsExceptions\\2 =
"Shockwave Flash"
Software\\Policies\\Google\\Chrome\\DisabledPluginsExceptions\\3 = "Chrome PDF
Viewer"Windows (Google Chrome OS
clients):Software\\Policies\\Google\\ChromeOS\\DisabledPluginsExceptions\\1 =
"Java" Software\\Policies\\Google\\ChromeOS\\DisabledPluginsExceptions\\2 =
"Shockwave Flash"
Software\\Policies\\Google\\ChromeOS\\DisabledPluginsExceptions\\3 = "Chrome PDF
Viewer"Android/Linux:\[ "Java", "Shockwave Flash", "Chrome PDF Viewer"
\]Mac:&lt;array&gt; &lt;string&gt;Java&lt;/string&gt; &lt;string&gt;Shockwave
Flash&lt;/string&gt; &lt;string&gt;Chrome PDF Viewer&lt;/string&gt;
&lt;/array&gt;[Back to top](#top)

### DisabledSchemes (deprecated)

Disable URL protocol schemesData type:List of stringsWindows registry location
for Windows clients:Software\\Policies\\Google\\Chrome\\DisabledSchemesWindows
registry location for Google Chrome OS
clients:Software\\Policies\\Google\\ChromeOS\\DisabledSchemesMac/Linux
preference name:DisabledSchemesSupported on:

*   Google Chrome (Linux, Mac, Windows) since version 12
*   Google Chrome OS (Google Chrome OS) since version 12

Supported features:Dynamic Policy Refresh: Yes, Per Profile: YesDescription:

This policy is deprecated, please use URLBlacklist instead.

Disables the listed protocol schemes in Google Chrome.

URLs using a scheme from this list will not load and can not be navigated to.

If this policy is left not set or the list is empty all schemes will be
accessible in Google Chrome.

Example value:Windows (Windows
clients):Software\\Policies\\Google\\Chrome\\DisabledSchemes\\1 = "file"
Software\\Policies\\Google\\Chrome\\DisabledSchemes\\2 = "https"Windows (Google
Chrome OS clients):Software\\Policies\\Google\\ChromeOS\\DisabledSchemes\\1 =
"file" Software\\Policies\\Google\\ChromeOS\\DisabledSchemes\\2 =
"https"Android/Linux:\[ "file", "https" \]Mac:&lt;array&gt;
&lt;string&gt;file&lt;/string&gt; &lt;string&gt;https&lt;/string&gt;
&lt;/array&gt;[Back to top](#top)

### EnabledPlugins (deprecated)

Specify a list of enabled pluginsData type:List of stringsWindows registry
location for Windows
clients:Software\\Policies\\Google\\Chrome\\EnabledPluginsWindows registry
location for Google Chrome OS
clients:Software\\Policies\\Google\\ChromeOS\\EnabledPluginsMac/Linux preference
name:EnabledPluginsSupported on:

*   Google Chrome (Linux, Mac, Windows) since version 11
*   Google Chrome OS (Google Chrome OS) since version 11

Supported features:Dynamic Policy Refresh: Yes, Per Profile: YesDescription:

This policy is deprecated. Please use the DefaultPluginsSetting to control the
avalability of the Flash plugin and AlwaysOpenPdfExternally to control whether
the integrated PDF viewer should be used for opening PDF files.

Specifies a list of plugins that are enabled in Google Chrome and prevents users
from changing this setting.

The wildcard characters '\*' and '?' can be used to match sequences of arbitrary
characters. '\*' matches an arbitrary number of characters while '?' specifies
an optional single character, i.e. matches zero or one characters. The escape
character is '\\', so to match actual '\*', '?', or '\\' characters, you can put
a '\\' in front of them.

The specified list of plugins is always used in Google Chrome if they are
installed. The plugins are marked as enabled in 'about:plugins' and users cannot
disable them.

Note that this policy overrides both DisabledPlugins and
DisabledPluginsExceptions.

If this policy is left not set the user can disable any plugin installed on the
system.

Example value:Windows (Windows
clients):Software\\Policies\\Google\\Chrome\\EnabledPlugins\\1 = "Java"
Software\\Policies\\Google\\Chrome\\EnabledPlugins\\2 = "Shockwave Flash"
Software\\Policies\\Google\\Chrome\\EnabledPlugins\\3 = "Chrome PDF
Viewer"Windows (Google Chrome OS
clients):Software\\Policies\\Google\\ChromeOS\\EnabledPlugins\\1 = "Java"
Software\\Policies\\Google\\ChromeOS\\EnabledPlugins\\2 = "Shockwave Flash"
Software\\Policies\\Google\\ChromeOS\\EnabledPlugins\\3 = "Chrome PDF
Viewer"Android/Linux:\[ "Java", "Shockwave Flash", "Chrome PDF Viewer"
\]Mac:&lt;array&gt; &lt;string&gt;Java&lt;/string&gt; &lt;string&gt;Shockwave
Flash&lt;/string&gt; &lt;string&gt;Chrome PDF Viewer&lt;/string&gt;
&lt;/array&gt;[Back to top](#top)

### ForceBrowserSignin (deprecated)

Enable force sign in for Google ChromeData type:Boolean
\[Windows:REG_DWORD\]Windows registry location for Windows
clients:Software\\Policies\\Google\\Chrome\\ForceBrowserSigninMac/Linux
preference name:ForceBrowserSigninAndroid restriction
name:ForceBrowserSigninSupported on:

*   Google Chrome (Windows) since version 64
*   Google Chrome (Mac) since version 66
*   Google Chrome (Android) since version 65

Supported features:Dynamic Policy Refresh: No, Per Profile: NoDescription:

This policy is deprecated, consider using BrowserSignin instead.

If this policy is set to true, user has to sign in to Google Chrome with their
profile before using the browser. And the default value of
BrowserGuestModeEnabled will be set to false. Note that existing unsigned
profiles will be locked and inaccessible after enabling this policy. For more
information, see help center article.

If this policy is set to false or not configured, user can use the browser
without sign in to Google Chrome.

Example value:0x00000000 (Windows), false (Android), &lt;false /&gt; (Mac)[Back
to top](#top)

### ForceSafeSearch (deprecated)

Force SafeSearchData type:Boolean \[Windows:REG_DWORD\]Windows registry location
for Windows clients:Software\\Policies\\Google\\Chrome\\ForceSafeSearchWindows
registry location for Google Chrome OS
clients:Software\\Policies\\Google\\ChromeOS\\ForceSafeSearchMac/Linux
preference name:ForceSafeSearchAndroid restriction name:ForceSafeSearchSupported
on:

*   Google Chrome (Linux, Mac, Windows) since version 25
*   Google Chrome OS (Google Chrome OS) since version 25
*   Google Chrome (Android) since version 30

Supported features:Can Be Recommended: No, Dynamic Policy Refresh: Yes, Per
Profile: YesDescription:

This policy is deprecated, please use ForceGoogleSafeSearch and
ForceYouTubeRestrict instead. This policy is ignored if either the
ForceGoogleSafeSearch, the ForceYouTubeRestrict or the (deprecated)
ForceYouTubeSafetyMode policies are set.

Forces queries in Google Web Search to be done with SafeSearch set to active and
prevents users from changing this setting. This setting also forces Moderate
Restricted Mode on YouTube.

If you enable this setting, SafeSearch in Google Search and Moderate Restricted
Mode YouTube is always active.

If you disable this setting or do not set a value, SafeSearch in Google Search
and Restricted Mode in YouTube is not enforced.

Example value:0x00000000 (Windows), false (Linux), false (Android), &lt;false
/&gt; (Mac)[Back to top](#top)

### ForceYouTubeSafetyMode (deprecated)

Force YouTube Safety ModeData type:Boolean \[Windows:REG_DWORD\]Windows registry
location for Windows
clients:Software\\Policies\\Google\\Chrome\\ForceYouTubeSafetyModeWindows
registry location for Google Chrome OS
clients:Software\\Policies\\Google\\ChromeOS\\ForceYouTubeSafetyModeMac/Linux
preference name:ForceYouTubeSafetyModeAndroid restriction
name:ForceYouTubeSafetyModeSupported on:

*   Google Chrome (Linux, Mac, Windows) since version 41
*   Google Chrome OS (Google Chrome OS) since version 41
*   Google Chrome (Android) since version 41

Supported features:Can Be Recommended: No, Dynamic Policy Refresh: Yes, Per
Profile: YesDescription:

This policy is deprecated. Consider using ForceYouTubeRestrict, which overrides
this policy and allows more fine-grained tuning.

Forces YouTube Moderate Restricted Mode and prevents users from changing this
setting.

If this setting is enabled, Restricted Mode on YouTube is always enforced to be
at least Moderate.

If this setting is disabled or no value is set, Restricted Mode on YouTube is
not enforced by Google Chrome. External policies such as YouTube policies might
still enforce Restricted Mode, though.

Note for Google Chrome OS devices supporting Android apps:

This policy has no effect on the Android YouTube app. If Safety Mode on YouTube
should be enforced, installation of the Android YouTube app should be
disallowed.

Example value:0x00000000 (Windows), false (Linux), false (Android), &lt;false
/&gt; (Mac)[Back to top](#top)

### Http09OnNonDefaultPortsEnabled (deprecated)

Enable HTTP/0.9 support on non-default portsData type:Boolean
\[Windows:REG_DWORD\]Windows registry location for Windows
clients:Software\\Policies\\Google\\Chrome\\Http09OnNonDefaultPortsEnabledWindows
registry location for Google Chrome OS
clients:Software\\Policies\\Google\\ChromeOS\\Http09OnNonDefaultPortsEnabledMac/Linux
preference name:Http09OnNonDefaultPortsEnabledSupported on:

*   Google Chrome (Linux, Mac, Windows) since version 54 until version
            77
*   Google Chrome OS (Google Chrome OS) since version 54 until version
            77

Supported features:Dynamic Policy Refresh: No, Per Profile: NoDescription:

This policy is deprecated, and slated for removal in Chrome 78, with no
replacement.

This policy enables HTTP/0.9 on ports other than 80 for HTTP and 443 for HTTPS.

This policy is disabled by default, and if enabled, leaves users open to the
security issue <https://crbug.com/600352>.

This policy is intended to give enterprises a chance to migrate exising servers
off of HTTP/0.9, and will be removed in the future.

If this policy is not set, HTTP/0.9 will be disabled on non-default ports.

Example value:0x00000000 (Windows), false (Linux), &lt;false /&gt; (Mac)[Back to
top](#top)

### IncognitoEnabled (deprecated)

Enable Incognito modeData type:Boolean \[Windows:REG_DWORD\]Windows registry
location for Windows
clients:Software\\Policies\\Google\\Chrome\\IncognitoEnabledWindows registry
location for Google Chrome OS
clients:Software\\Policies\\Google\\ChromeOS\\IncognitoEnabledMac/Linux
preference name:IncognitoEnabledAndroid restriction
name:IncognitoEnabledSupported on:

*   Google Chrome (Linux, Mac, Windows) since version 11
*   Google Chrome OS (Google Chrome OS) since version 11
*   Google Chrome (Android) since version 30

Supported features:Dynamic Policy Refresh: Yes, Per Profile: YesDescription:

This policy is deprecated. Please, use IncognitoModeAvailability instead.
Enables Incognito mode in Google Chrome.

If this setting is enabled or not configured, users can open web pages in
incognito mode.

If this setting is disabled, users cannot open web pages in incognito mode.

If this policy is left not set, this will be enabled and the user will be able
to use incognito mode.

Example value:0x00000000 (Windows), false (Linux), false (Android), &lt;false
/&gt; (Mac)[Back to top](#top)

### JavascriptEnabled (deprecated)

Enable JavaScriptData type:Boolean \[Windows:REG_DWORD\]Windows registry
location for Windows
clients:Software\\Policies\\Google\\Chrome\\JavascriptEnabledWindows registry
location for Google Chrome OS
clients:Software\\Policies\\Google\\ChromeOS\\JavascriptEnabledMac/Linux
preference name:JavascriptEnabledAndroid restriction
name:JavascriptEnabledSupported on:

*   Google Chrome (Linux, Mac, Windows) since version 8
*   Google Chrome OS (Google Chrome OS) since version 11
*   Google Chrome (Android) since version 30

Supported features:Dynamic Policy Refresh: Yes, Per Profile: YesDescription:

This policy is deprecated, please use DefaultJavaScriptSetting instead.

Can be used to disabled JavaScript in Google Chrome.

If this setting is disabled, web pages cannot use JavaScript and the user cannot
change that setting.

If this setting is enabled or not set, web pages can use JavaScript but the user
can change that setting.

Example value:0x00000001 (Windows), true (Linux), true (Android), &lt;true /&gt;
(Mac)[Back to top](#top)

### MachineLevelUserCloudPolicyEnrollmentToken (deprecated)

The enrollment token of cloud policy on desktopData type:String
\[Windows:REG_SZ\]Windows registry location for Windows
clients:Software\\Policies\\Google\\Chrome\\MachineLevelUserCloudPolicyEnrollmentTokenMac/Linux
preference name:MachineLevelUserCloudPolicyEnrollmentTokenSupported on:

*   Google Chrome (Linux, Mac, Windows) since version 66

Supported features:Dynamic Policy Refresh: No, Per Profile: NoDescription:

This policy is deprecated in M72. Please use CloudManagementEnrollmentToken
instead.

Example value:"37185d02-e055-11e7-80c1-9a214cf093ae"[Back to top](#top)

### SigninAllowed (deprecated)

Allow sign in to Google ChromeData type:Boolean \[Windows:REG_DWORD\]Windows
registry location for Windows
clients:Software\\Policies\\Google\\Chrome\\SigninAllowedMac/Linux preference
name:SigninAllowedAndroid restriction name:SigninAllowedSupported on:

*   Google Chrome (Linux, Mac, Windows) since version 27
*   Google Chrome (Android) since version 38

Supported features:Dynamic Policy Refresh: Yes, Per Profile: YesDescription:

This policy is deprecated, consider using BrowserSignin instead.

Allows the user to sign in to Google Chrome.

If you set this policy, you can configure whether a user is allowed to sign in
to Google Chrome. Setting this policy to 'False' will prevent apps and
extensions that use the chrome.identity API from functioning, so you may want to
use SyncDisabled instead.

Example value:0x00000001 (Windows), true (Linux), true (Android), &lt;true /&gt;
(Mac)[Back to top](#top)

### UnsafelyTreatInsecureOriginAsSecure (deprecated)

Origins or hostname patterns for which restrictions on insecure origins should
not applyData type:List of stringsWindows registry location for Windows
clients:Software\\Policies\\Google\\Chrome\\UnsafelyTreatInsecureOriginAsSecureMac/Linux
preference name:UnsafelyTreatInsecureOriginAsSecureSupported on:

*   Google Chrome (Linux, Mac, Windows) since version 65

Supported features:Dynamic Policy Refresh: No, Per Profile: NoDescription:

Deprecated in M69. Use OverrideSecurityRestrictionsOnInsecureOrigin instead.

The policy specifies a list of origins (URLs) or hostname patterns (such as
"\*.example.com") for which security restrictions on insecure origins will not
apply.

The intent is to allow organizations to allow origins for legacy applications
that cannot deploy TLS, or to set up a staging server for internal web
development so that their developers can test out features requiring secure
contexts without having to deploy TLS on the staging server. This policy will
also prevent the origin from being labeled "Not Secure" in the omnibox.

Setting a list of URLs in this policy has the same effect as setting the
command-line flag '--unsafely-treat-insecure-origin-as-secure' to a
comma-separated list of the same URLs. If the policy is set, it will override
the command-line flag.

This policy is deprecated in M69 in favor of
OverrideSecurityRestrictionsOnInsecureOrigin. If both policies are present,
OverrideSecurityRestrictionsOnInsecureOrigin will override this policy.

For more information on secure contexts, see
<https://www.w3.org/TR/secure-contexts/>

Example value:Windows (Windows
clients):Software\\Policies\\Google\\Chrome\\UnsafelyTreatInsecureOriginAsSecure\\1
= "http://testserver.example.com/"
Software\\Policies\\Google\\Chrome\\UnsafelyTreatInsecureOriginAsSecure\\2 =
"\*.example.org"Android/Linux:\[ "http://testserver.example.com/",
"\*.example.org" \]Mac:&lt;array&gt;
&lt;string&gt;http://testserver.example.com/&lt;/string&gt;
&lt;string&gt;\*.example.org&lt;/string&gt; &lt;/array&gt;[Back to top](#top)
