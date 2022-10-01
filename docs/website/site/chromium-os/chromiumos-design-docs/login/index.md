---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/chromiumos-design-docs
  - Design Documents
page_name: login
title: Login
---

[TOC]

## Abstract

*   The Chromium OS-based device login mechanism will provide a single
            sign on (SSO) capability that users can use to streamline access to
            cloud-based services
*   This mechanism will be designed for security, privacy and
            ease-of-use
*   We want to ensure that people can fully use Chromium OS without
            needing a Google login
*   While the initial work on the login mechanism has been focused on
            providing instant access to Google services for users with Google
            accounts, we are investigating support for OpenID to allow people to
            fully use the system without needing a Google login.

## Objective

Chromium OS devices are cloud devices. They sync users' data and preferences
with cloud-based services. To do so, they need some way to bundle all the user's
data together and to know which machines to sync it down to. To enable this
functionality, users currently need to log in to Chromium OS devices with a
Google account. Other authentication service providers may also support this
functionality in the future.

Chromium OS devices will be able to:

1.  Authenticate the user against Google if possible, so that it always
            uses the user's latest password
2.  Enable the user to log in when offline (assuming the user has logged
            in online at least once)
3.  Enable an SSO experience for Google properties
4.  Allow the user to opt-in to auto-login that still does SSO, but does
            not cache the user's password

We also plan to support alternative authentication systems:

1.  Give users an SSO experience at OpenID relying parties
2.  Give users an SSO experience at sites for which they've already
            typed in credentials on a Chromium OS device

We are also currently investigating the technical issues involved with allowing
users to log in to a Chromium OS device using a non-Google OpenID provider. We
are investigating how to enable 3rd parties to provide interoperable sync
services.

## Design highlights

### Phase 1 (complete)

#### Current implementation

The initial design achieves the minimum requirements by using Chromium's HTTPS
stack to talk to existing Google Accounts [HTTP(s)
APIs](http://code.google.com/apis/accounts/docs/AuthForInstalledApps.html) to
authenticate the user and get the appropriate cookies to log the user in to all
Google services the instant the browser UI shows up. Since the login screen is
actually presented by the browser process itself, it is a simple matter to
ensure that all cookies acquired as a result of authentication wind up in the
user's cookie jar. Once the user has successfully authenticated online once, we
use a hash of her password and the Trusted Platform Module (TPM) to wrap a magic
string. Offline login is implemented by asking for the user's password again and
attempting to successfully unwrap this magic value, an operation that cannot be
performed without access to both the specific TPM chip in the user's device and
the user's password. In the offline case, obviously, no cookies can be acquired.
Many users, though, will have unexpired cookies from a previous authentication
already cached in their Chromium browser session, and these can be used once
they get back online. The UI for logging in is provided by a the Chromium
browser itself in a special mode. We allow the user to select a keyboard layout
on this login screen, and locale will either be set by the device's owner, or be
selectable on the fly.

For our needs, the normal Google Accounts ClientLogin API is not enough, as it
is designed to provide cookies that allow a client application to authenticate
to a single Google service. We want the cookies your browser gets when you run
through a normal web-based login, so that we can get them into Chromium after
the user's session has begun. Therefore, we currently go through a three-step
process:

1.  https://www.google.com/accounts/ClientLogin, to get a Google cookie
2.  https://www.google.com/accounts/IssueAuthToken, to get a one-time
            use token (good for a couple of minutes) that will authenticate the
            user to any service
3.  https://www.google.com/accounts/TokenAuth, to exchange the token for
            the full set of browser cookies we need to do SSO

In the future, we're looking to deploy another API that would minimize the
number of round trips.

#### Making sure we're talking to Google

In order to remain somewhat resistant to network-level attacks, the only root
certificate that the login process is willing to trust to identify web servers
is the one that issues Google's SSL certs. Thus, if an attacker is going to
trick a registrar into giving him an SSL cert for www.google.com, at least he
has to trick Google's registrar.

#### Auto-login

We don't want to implement auto-login by caching the user's password and running
it through the normal login process on his behalf; doing so would expose the
user's password to unnecessary risk. As auto-login is opt-in, the user action
that sets it up will take place after login, during an active user session --
which means we have the Chromium browser up and running. Thus, turning on
auto-login will be a web-based flow that results in the user getting an OAuth
übertoken that we cache. These tokens are revokable, which allows the user to
limit his risk if one gets compromised.

We would store this token in the same encrypted-while-shutdown storage as we're
using for system settings. The login process would check for it, and then, upon
successfully exchanging it for Google cookies, log the user in. If the login
would need to proceed offline, then the presence of the token is deemed to be
enough to allow login to succeed. The user opted in to auto-login; there is only
so much we can do to protect him and his data.

### Phase 2

#### More efficient, more flexible

Support for web-based authentication flows, like OpenID or those used internally
by many enterprises, is the next area we hope to explore. This will allow our
users to authenticate against providers other than Google, though there are
challenges around supporting arbitrary web-bases flows and still being able to
manage per-user data encryption without asking users to set up a separate, local
credential specifically for this purpose -- a user experience that we find less
compelling than what we provide today.

#### SSO beyond Google

Efforts in this space have not proceeded much at this time. It's probable that,
if users have chosen to "Log in via Google" at OpenID relying parties, the
presence of Google authentication cookies in their browser will Just Work. This
remains to be verified. As for logging in to third-party sites for which the
user has entered their password already, such behavior will likely be tied to a
password sync system.
