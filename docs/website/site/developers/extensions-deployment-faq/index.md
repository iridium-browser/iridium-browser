---
breadcrumbs:
- - /developers
  - For Developers
page_name: extensions-deployment-faq
title: Chrome Extension Developer FAQ for upcoming changes in May 2015 related to
  hosting extensions
---

As a follow-up to our blog post on [continuing to protect Chrome users from
malicious
extensions](http://blog.chromium.org/2015/05/continuing-to-protect-chrome-users-from.html),
we’re enforcing the following changes starting in May 2015 for all Windows
channels and starting in July 2015 for all Mac channels:

*   Users can only install extensions hosted in the Chrome Web store,
            except for installs via [enterprise
            policy](https://support.google.com/chrome/a/answer/188453) or
            [developer
            mode](http://developer.chrome.com/extensions/getstarted.html#unpacked).
*   Extensions that were previously installed, but not hosted on the
            Chrome Web Store will be
            [hard-disabled](https://support.google.com/chrome/answer/2811969)
            (i.e. the user cannot enable these extensions again), except for
            installs via [enterprise
            policy](https://support.google.com/chrome/a/answer/188453) or
            [developer
            mode](http://developer.chrome.com/extensions/getstarted.html#unpacked).

[TOC]

## **What’s the rationale for this measure?**

See [Continuing to protect Chrome users from malicious
extensions](http://blog.chromium.org/2015/05/continuing-to-protect-chrome-users-from.html).

## For extensions that are currently hosted outside the Chrome Web Store, what should be done and by when?

If your extensions are currently hosted outside the Chrome Web Store you should
migrate them to the Chrome Web Store as soon as possible. The above changes will
very soon become effective for all channels on Windows and will become effective
for all Mac channels in July 2015. Once you migrate your extensions to the
Chrome Web Store, there will be no impact to your users, who will still be able
to use your extension as if nothing changed. And if you have a dedicated
installation flow from your own website, you can make use of the existing
[inline
installs](https://developers.google.com/chrome/web-store/docs/inline_installation)
feature.

## What will happen if I migrate the extension to the Chrome Web Store sometime in the future? Will I lose all my users?

Users will have their off-store extensions hard-disabled once the enforcement
rolls out. However, if the extension is migrated to the Chrome Web Store after
the rollout, users will be able to manually enable the migrated extension from
extensions settings page (chrome://extensions) or from the Chrome Web Store
listing.

## What if I want to restrict access to certain users or prevent my extension from being listed on the Chrome Web Store?

You can restrict access to your extension by limiting its visibility to Trusted
Tester or by unlisting the extension from the Chrome Web Store.

## Which operating systems and Chrome channels are affected by this change?

As of May 2015, these changes are effective for all Windows channels starting
with Chrome 33 and for all Mac channels starting with Chrome 44 (around end of
July 2014).

## Will this affect my ability to develop my extensions on Windows?

No. You can still load unpacked extensions in [developer
mode](http://developer.chrome.com/extensions/getstarted.html#unpacked) on
Windows and Mac.

## Why couldn't this problem be solved by having a setting/option to load extensions that are not hosted in the Chrome Web Store?

Unlike modern mobile operating systems, Windows and Mac do not sandbox
applications. Hence we wouldn’t be able to differentiate between a user opting
in to this setting and a malicious downloaded program overriding the user’s
desired setting.

## What are the supported deployment options for extensions after this change?

Apart from users installing extensions from the Chrome Web Store, the following
deployment options will be supported:

*   For OSX and Linux, extensions can be installed via a [preferences
            JSON
            file](http://developer.chrome.com/extensions/external_extensions.html#preferences).
*   For Windows, extensions can be installed via the [Windows
            registry](http://developer.chrome.com/extensions/external_extensions.html#registry).
*   In the Windows registry and in an OS X preferences JSON file, ensure
            that the update_url registry key points to the following URL:
            <https://clients2.google.com/service/update2/crx>. Local .crx
            installs via the path registry key are deprecated. Note that this
            deployment option works only for Chrome Web Store hosted extensions,
            and update_url cannot point to any other host other than
            <https://clients2.google.com/service/update2/crx>.
*   For Enterprises, we’ll continue to support [group
            policy](https://support.google.com/chrome/a/answer/188453?hl=en) to
            install extensions, irrespective of where the extensions are hosted.
            Note that any extension which is not hosted on the Web Store and
            installed via GPO on a machine which has not joined a domain will be
            **hard-disabled**.

## Are there any other considerations to be aware of for extensions that depend on a native application binary?

Previously when off-store extensions were supported, it was possible to have the
third party application binaries and the sideloaded extension be updated in
lockstep. However, extensions hosted on the Chrome Web Store are updated via the
Chrome update mechanism which developers do not control. Extension developers
should be careful about updating extensions that have a dependency on the native
application binary (for example, extensions using [native
messaging](https://developer.chrome.com/extensions/messaging.html#native-messaging)
or legacy extensions using
[NPAPI](http://developer.chrome.com/extensions/npapi.html)).

## What will users see when their off-store extension is disabled as a result of this rollout?

They will get a notification that says: “Unsupported extensions disabled” with a
link to the following [support
article](https://support.google.com/chrome/answer/2811969).

## Why do I see a bubble about “Disable developer mode extensions” when loading an unpacked extension in Windows stable/beta channels?

We do not want the developer mode to be used as an attack vector for spreading
malicious extensions. Hence we’re informing users about developer mode
extensions on all Windows and Mac channels and giving them an option to disable
these extensions.
