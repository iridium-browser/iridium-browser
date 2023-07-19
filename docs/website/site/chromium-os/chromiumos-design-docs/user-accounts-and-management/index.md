---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/chromiumos-design-docs
  - Design Documents
page_name: user-accounts-and-management
title: User Accounts and Management
---

[TOC]

## Abstract

*   End users must already have, or be willing to create, an account
            that is used to store preferences and settings. Along with Google
            Accounts, we want to support other authentication systems,
            particularly OpenID.
*   Storing preferences and settings with the account in the cloud
            allows end users to maintain a consistent user experience while
            switching among Chromium OS devices.
*   Chromium OS devices are designed to be easily shared among users,
            while still providing a high level of security for the owner of the
            device and other authorized users.
*   Owners of Chromium OS devices can choose to manage a list of users
            who are allowed to log in to a device, providing a higher level of
            security for the device.
*   Chromium OS also provides a Guest mode, allowing anyone to browse
            with the device without preserving any data, preferences, or other
            information after the session ends. The owner of the device can
            choose to disable this mode.

## Objective

For V1, at least, we are asserting the following about logging in to Chromium OS
devices:

1.  We want the device to be as shareable as a Google Doc.
2.  The first time a given user logs into the device, we require
            connectivity for the initial authentication.
3.  We believe users want to control who can log in to their machine.
4.  We consider traditional user account management to be onerous.

Our goal is to enable use cases like the following:
> I have a Chromium OS device that I use and have configured to work on my home
> WiFi network, which has some Network Attached Storage (a NAS). A friend asks
> if she can borrow my device to do some work. I log out and hand her the
> device. She logs in. The device should remain associated with my WiFi network,
> though her preferences (address bar state, installed apps, privacy settings,
> etc) should sync down and be applied for her account. As a byproduct, my
> friend has access to my NAS. That's okay.

without also enabling attacks like this:
> I have a Chromium OS device that I use and have configured to work on my home
> WiFi network, which has a NAS. Someone steals my device. The thief has a
> Google Account. He logs in, and not only does he have a local account on my
> machine, which removes a significant obstacle to compromise, but he also has
> access to my NAS and everything else on my home network.

At a high level, what we're saying is that, for certain settings, the right
thing to do is have a per-device **system setting** override a **user
preference**—but that some per-device settings could be sensitive. This insight
dictates several requirements:

*   For users who log in to the system, we must come up with a sane way
            to overlay user preferences on system settings.
*   There must be some mechanism to allow users to manage who can log in
            to the device, including allowing anyone (who has a Google Account)
            to log in to the device.
*   We must not drive users toward having a single, shared account;
            common multi-user use cases should fit well within our design.

It's worth noting that we also have some thoughts about supporting users without
a Google Account; see the [Login
document](/chromium-os/chromiumos-design-docs/login), and the discussion of
"Guest" mode later in this document.

## Design highlights

We've shown above that we must, at least, deal with the wrinkle where we need to
access some available, but secured, wireless network to log a new user in. After
that, ideally, we would like to apply all of a user's cloud-cached preferences
locally so that their computing environment can follow them seamlessly from
device to device. In addition to the issue of wireless networks, consider also
the case of keyboard layouts:
> Antoine is French and has set up his Chromium OS device with a US keyboard to
> have some key mappings that will handle accents nicely for him. When he goes
> to France and logs into his sister's Chromium OS device with a French
> keyboard, it doesn't make sense for his mapping from home to apply. In fact,
> it should fail, as the machine has a different keyboard. When I log in to
> Antoine's machine, though, my default US keyboard mapping can be applied, and
> should be.

The specifics of trying to apply a given user preference and having it fail vary
depending on the preference in question. We define **system settings** as any
setting we need to be able to apply before a user has logged in.
Currently, we break down **system settings** and **user preferences** as
follows:

<table>
<tr>
System Settings User Preferences </tr>
<tr>
<td>Locale</td>
<td> Wifi networks</td>
<td> Described in this doc:</td>

<td>Owner</td>
<td> Whitelist</td>
<td> Guest mode</td>

<td>Proxies</td>

<td>Bookmarks</td>
<td> New Tab page</td>
<td> Preferences (browser settings)</td>
<td> Apps</td>

<td>Extensions</td>
<td> Themes</td>
<td> Pinned tabs</td>
<td> Notifications</td>
<td> Printers list</td>
<td>Thumbnails</td>
<td> Autofill data</td>

</tr>
</table>

For at least the "current WiFi" setting, we need a user to have done the
configuration first. In other cases, we will have sensible defaults that we will
use to initially configure the owner's account.
These features dictate several requirements:

1.  Settings deemed system settings need to be accessible to all
            authenticated users, but not to just anyone who pulls the hard
            drive.
2.  User preferences should be stored so that only the user in question
            can access them.
3.  The client side of the sync engine needs to be able to distinguish
            system settings from user preferences, so that it can store them
            separately.

### Syncing mechanism

Out of scope for this document. All that matters from our point of view is that
the client side can distinguish between system settings and user preferences.

### Caching and applying config info

The Linux system services that rely on these settings and preferences access
them in a variety of ways, though mostly through configuration files. We must
design a system that can use the preferences and settings we sync down from the
cloud and apply them at the appropriate time. This is to-be-designed, but must
have the following characteristics:

1.  User preferences will be synced down by a Chromium-based browser and
            cached in the user's data partition, which is encrypted by default.
            The system should be capable of applying these as the user's session
            is being brought up.
2.  System settings must be usable during the boot process; otherwise,
            we won't be able to connect to a network for online login. We would
            like, however, to at least protect these settings while the device
            is off.
3.  The system settings should be writable only by the Owner; we can't
            cryptographically enforce this, but we can at least use some system
            daemon to enforce it.
4.  We need to enable configuration to be federated; multiple sources of
            config data (settings and prefs) need to be sensibly overlaid on one
            another. The precise semantics depend on the setting in question.

### Keeping track of the Owner and the whitelist

The point of keeping track of who the Owner is and who's on the whitelist is to
keep the attacker from getting an account on the device. If they've already
managed to root the box, or to exercise a kernel vuln, they can work around
Ownership and whitelisting anyway (replace the session manager or the chrome
binary, or something else). So, designing a mechanism that is robust against
root seems ... futile (we're working hard to take root away from all userland
processes regardless, in our [system hardening
efforts](/chromium-os/chromiumos-design-docs/system-hardening)).

If the owner has opted out of whitelisting, we allow every user with a valid
Google Account to log in.

### Guest mode

We are considering logically extending the notion of "Incognito" mode found in
Chromium-based browsers to Chromium OS-based devices: a stateless session that
requires no login and that caches data only until the session is over. *No*
state would sync down from the cloud, and *no* state from this session would
persist past termination. The option to start such a session will be on by
default, though the Owner can disable it when she is logged in. We create a
tmpfs on which to store the Chromium browser's session data, and open up an
instance of the browser in incognito mode for the user. Once that instance is
exited, the session is over and we return to the login screen.
By default, a Chromium OS device will be **opted out of whitelisting** and
**opted-in to Guest mode**. We will provide UI that allows a logged-in Owner to
explicitly add users, and provide the Owner an option at that time to opt-in to
whitelisting and Guest mode.

### Managing write access to system settings

Only the Owner should be able to change system settings. We don't want random
users able to add to the whitelist or enable guest mode inadvertently. As a
root/kernel exploit means the game is over anyway, it's not worth trying to
design anything that is robust against that kind of compromise; the attacker
could just work around our security measures. So, we will have a settings
daemon, settingsd, that manages write access to the system settings.
When the first user logs in, we generate an RSA keypair and store the private
half in his encrypted home directory, exporting the public half via settingsd.
All requests to update the signed settings store (which includes the whitelist)
must contain a valid signature generated with the owner's private key. When
settingsd receives a request for the value of a system setting, it returns the
signature over that value along with the requested data. The requesting process
then verifies this signature, ensuring that only values which are correctly
signed by the Owner are respected. Of course, a root-level exploit could replace
the exported Owner public key. As stated above, though, the game is already over
in this case.
