---
breadcrumbs:
- - /administrators
  - Documentation for Administrators
page_name: common-problems-and-solutions
title: Common Problems and Solutions
---

**Installation**

**What method should we use to install and auto-update?**

> There are two ways to handle installation and version management:

> 1. Install the MSI and leave auto-updates on.

> Implications to consider:

> *   The installations will auto-update themselves, so the MSI you use
              to deploy will eventually not work for repairs and over-installs.
              You'll need to get the latest MSI.
> *   Updates are performed silently, and occur often.
> *   Your users will always be up-to-date with the latest stable
              version.

> 2. Install the MSI and turn auto-updates off.

> Implications to consider:

> *   You must keep the MSI you use to deploy archived for repairs or
              over-installs, as we do not publish old MSIs.
> *   Not auto-updating will mean users are running old, and potentially
              vulnerable, versions of the browser.
> *   You can push a new MSI install over the old MSI install when
              you're ready.

I'm trying to install behind a firewall, and the install is timing out and
failing.

> The problem is that by default, Google Update is attempting to check for an
> update on install and fails at the firewall.

> When passed a special parameter, the installation will not do this check, and
> should install regardless of outside connectivity.

> The special parameter is "NOGOOGLEUPDATEPING=1", and is used like this:

> msiexec /i GoogleChromeStandaloneEnterprise.msi NOGOOGLEUPDATEPING=1 /l\*v
> log.txt

When I attempt to push a new MSI, the install fails with an error that says: **"Google Chrome or Google Chrome Frame cannot be updated on account of inconsistent Google Update Group Policy settings. Use the Group Policy Editor to set the update policy override for the Google Chrome Binaries application and try again."

> Between Chrome 12 and 13, the group policy settings to control auto-updates
> changed. Rather than have auto-updates start unexpectedly, this error was
> added so that you know to set the new auto-update group policies.

> Find out more information here:

> <http://www.google.com/support/a/bin/answer.py?answer=1385049&topic=1064263>

**Can I store my users' Chrome profiles on a Roaming Profile? Or sync it to a
network drive?**

> Chrome user profiles are not backwards-compatible. If you try to use
> mismatched profiles and Chrome versions, **you may experience crashes or data
> loss**. This mismatch can often occur if a Chrome profile is synced to a
> roaming profile or network drive across multiple machines that have different
> versions of Chrome. We strongly encourage admins & users to consider using
> [Google Chrome
> Sync](http://www.google.com/support/chrome/bin/answer.py?hl=en&answer=165139&from=165140&rd=1),
> which persists user settings across machines, instead of using roaming
> profiles for the time being.

> That said, there are policies for controlling the location of the user profile
> and the cache:

> <http://www.chromium.org/administrators/policy-list-3#UserDataDir>

> <http://www.chromium.org/administrators/policy-list-3#DiskCacheDir>

**I'm trying to install the MSI over an existing MSI install, but I keep getting
the error "more recent version exists".**

> If you left auto-updates on, it's possible that the version of the MSI you're
> trying to install is already outdated. If you already have the dev channel or
> beta channel installed, the MSI cannot overwrite that since they'll be on
> newer versions too.

> Verify that you are attempting to install a later version of the MSI by
> downloading the latest version
> [here](http://www.google.com/chrome/eula.html?msi=true).

> To see what the latest stable version of Chrome is for Windows, click
> [here](http://omahaproxy.appspot.com/win).

**I'm trying to repair an installation, but the repair is failing.**

> If you left auto-updates on, it is likely that your installs auto-updated to a
> later version. This is causing the repair to fail because you're already on a
> later version than the MSI.

> You can download the latest stable MSI
> [here](http://www.google.com/chrome/eula.html?msi=true), and use this MSI for
> the repair instead.

**When applying the MSI over a previous install, I get the error "There is a
problem with this Windows Installer package. A program required for this install
to complete could not be run. Contact your support personal or package vendor"**

> If you are running with the /f flag to run it as a "minor" update, you will
> need to use the /i flag to run it as a "major" update instead.

**Do you have old versions of the MSI available?**

> No. The latest stable version is always available at the link above.

> If you are looking to repair an existing MSI install, and you left
> auto-updates on, you can repair with the latest stable MSI.

**How often do you update the MSI?**

> We update the MSI on every stable release. We'll release a new "major" version
> to the stable channel about every six weeks, although there are often several
> updates to the stable channel between major updates that typically contain
> crash and security fixes.

> If, for some reason, the version of the MSI at the link above is old, please
> let us know by [filing a
> bug](http://code.google.com/p/chromium/issues/entry?template=Enterprise%20Issue)
> so we can fix it!

**What happens if the user already has 'consumer Chrome' installed and I try to
push out 'Enterprise Chrome'? Will they end up with two Chromes?**

> There is only ever one Chrome on the machine. When the MSI notices that
> consumer Chrome is already there, it will remove it and update the user's
> shortcuts.

> The next time the user attempts to launch Chrome, it will launch the
> 'Enterprise Chrome' instead.

> This should look seamless to the user, but sometimes behaves inconsistently.
> You may want to consider uninstalling 'consumer Chrome' before pushing out the
> MSI.

**I want to remove 'consumer Chrome' from target machines entirely before
pushing out 'Enterprise Chrome'. What command can I use to uninstall?**

> You can append several registry keys together with an additional parameter,
> and execute them.

> HKEY_CURRENT_USER\\Software\\Google\\Update\\ClientState\\{8A69D345-D564-463c-AFF1-A69D9E530F96}\\UninstallString
> +
> HKEY_CURRENT_USER\\Software\\Google\\Update\\ClientState\\{8A69D345-D564-463c-AFF1-A69D9E530F96}\\UninstallArguments
> + ' --force-uninstall'

> The command will end up looking something like

> '\[Path to user's data directory\]\\setup.exe --uninstall --force-uninstall'

**If I uninstall 'consumer Chrome', will that wipe out the user's data?**

> No. Users' data is kept separate from the Chrome installation.

> However, if the user was using a more recent version of Chrome, their profile
> data may not work in an older version of Chrome and they'll see a warning when
> they try to run it. You may want to verify that you're pushing out the latest
> stable channel version of Chrome.

**Auto-Update**

**I see a new version was just released, by my installs don't seem to be
auto-updating.**

> We typically throttle updates the first few days after an update to watch
> stability rates and make sure we're pushing out a good update. Not all users
> will get the update immediately, so you may see some machines auto-update and
> others not in this timeframe.

> If you require the most up-to-date version immediately, you may want to
> download the latest MSI and push it out manually.

**Policy**

**What policy settings do I need to use to integrate with my Windows NTLM
login?**

> Using the AuthServerWhitelist policy, set the value of the policy to the list
> of the **Domain Controllers' FQDN and the proxy server**. Also, using the
> AuthSchemes policy, specify **all four** of basic, digest, ntlm, and
> negotiate.

**I'm trying to force-install an extension that is internal to our network (not
on the Chrome Web Store), but it's not being force-installed.**

> First, verify that the policy you are applying is being properly picked up by
> the browser. Navigate to "about:policy" and look for a policy named
> "ExtensionInstallForcelist" -- your setting should be there. If it is not, it
> is possible that your policy is under the wrong registry key.

> Second, verify that the extension you built installs manually -- install the
> extension CRX and verify it installs correctly (you may see some security
> warnings when installing manually that you will not see when
> force-installing).

> Third, verify that other extensions from the Chrome Web Store install
> properly. There is an example in the documentation.

> Lastly, if all of that does not work, it is likely that the URL you specified
> as part of the policy setting does not point to a valid auto-update XML file.
> The URL specified in the policy ==must point to the auto-update XML file, not
> the CRX==.

**I'm trying to set the default search provider, but it's not working.**

> The policy uses a different format for the search provider URL from the format
> you see when you open the options dialog. Using the 'options dialog format'
> will actually cause the policy to not be respected.

> Instead of using "%s" in the search URL to denote the search keywords, you
> must use "{searchTerms}" instead. Using Google's search URL as an example:

> Wrong: http://www.google.com/?q=%s

> Right: http://www.google.com/?q={searchTerms}

**Chrome does not read policy from the Windows registry.**

> Please use GPO instead. Chrome only loads policies from the registry on
> machines that are part of an Active Directory domain.

**Chrome does not honor policy provided via GPO.**

> Even when using GPO, very few policies still require the machine to be part of
> an Active Directory domain. See the [Policy
> List](/administrators/policy-list-3) for more details.

**Didn't Find the Answer?**

If you didn't find the question and answer you were looking for, please [file a
bug](http://code.google.com/p/chromium/issues/entry?template=Enterprise%20Issue).
