---
breadcrumbs:
- - /Home
  - Chromium
page_name: third-party-developers
title: Third Party Developers
---

[TOC]

## Important Notice

The
[ForceNetworkInProcess](https://cloud.google.com/docs/chrome-enterprise/policies/?policy=ForceNetworkInProcess)
will be removed soon. The official announcement will start from [Chrome
Enterprise release notes](https://support.google.com/chrome/a/answer/7679408)
for Chrome 80. The original plan was to remove it at Chrome 82, but will be
postponed to do it at Chrome 84.

## Policy

At Google, we believe that if we focus on the user, all else will follow. In our
[Software Principles](https://www.google.com/about/software-principles.html), we
provided general recommendations for software that delivers a great user
experience. This policy expands upon those general recommendations by defining
third party software, and clarifying how it will be treated by Chrome. Software
that meets the definition is considered as potentially harmful to the Chrome
user experience, and we will take steps to prevent such software from injecting
itself into Chrome’s process space. It steps beyond this simple definition to
lay guidelines for our interactions with third party software developers, and
our commitment to helping them as well as the ecosystem in general.

This policy was initially announced in November of 2017 via this [blog
post](https://blog.chromium.org/2017/11/reducing-chrome-crashes-caused-by-third.html).

### Definition

*Any module that is not signed by Microsoft or Google is considered to be third
party software.*

Software that is produced by third parties but that is signed by Microsoft via
their WHQL program is not considered third party software under this definition.
This is intended, and required for hardware support in some cases.

### Baseline Policy

*Software should stay out of Chrome’s address space. Period.*

The intent of this policy is take a stance against third party code injecting
into Chrome’s processes, for any reason.

### Exceptions

*Software that is deemed critical to the ecosystem or to certain end users may
be excluded from being blocked.*

The third party blocking mechanism will have provision to allow (at least
temporarily) acceptable third party software. This is primarily intended to
allow accessibility software to continue to work with Chrome. This may be
extended to other classes of software at Google’s discretion to include other
modules that would otherwise be considered third party. Exceptions should be
considered as temporary, and are intended to allow the developer sufficient time
to develop alternative solutions that do not violate this policy.

### Commitment to Building Alternatives

*Where no viable alternative to code injection exists for a legitimate use case
that provides concrete value to users, Chrome is committed to developing and/or
implementing alternative mechanisms that does not require code injection.*

This may include working with specific publishers, developers, industry steering
groups or standards bodies. We are generally convinced that most legitimate use
cases can be addressed via existing platform and extension APIs, but are willing
to consider supporting new APIs where there is a clear and justified use case.

Where long term support of a legacy injection mechanism is required to enable a
valid exceptional use case we will make efforts to move the logic to an external
utility process so that the injection does not need to occur in the Chrome
browser process. For more information refer to the FAQ for questions around
printing, shell extensions and AMSI implementations.

### Outreach and Grace Period

*Developers should be notified that their software will start to be blocked, and
provided a reasonable period of time during which they may develop alternative
solutions.*

The overall third party strategy includes a relatively lengthy notification-only
period, during which time we will make all efforts to directly engage with
developers of third party software with non-trivial user bases. This will also
include direct communication with the community via blog posts and other means.

### Escalation

*Developers should have a mechanism for escalating their particular use case.*

It is very likely that there is a long tail of use cases that we have not
considered. If a developer feels they have a use case that is not easily
addressed using alternative APIs or known workarounds they can feel free to
reach out to the team by [filing a
bug](https://bugs.chromium.org/p/chromium/issues/entry?description=Please+describe+your+third+party+use+case+here.&labels=Hotlist-ThirdPartySoftware,OS-Windows&oss=linux&owner=chrisha@chromium.org&components=Internals%3EPlatformIntegration).
You can also reach out to the team on the
[windows-third-party@chromium.org](mailto:windows-third-party@chromium.org)
mailing list.

## FAQ

**Q. How can I determine how this feature impacts my software?**

A. We have provided testing documentation that [walks you through the
process](https://docs.google.com/document/u/1/d/e/2PACX-1vT-nKiuYFLx6faY7sx6NFfYA6V9DgwzIpbOLSnIh44caYxvKjMXZNhU2EOqg795eoBL02Ri1L09VgMY/pub)
of evaluating the impact of blocking on your software.

**Q. Can my software be added to the allowlist?**

A. The short answer is no. Unless you are a publisher of accessibility software,
in which case we will happily allow your software until suitable alternative
APIs are available. Please [file a
bug](https://bugs.chromium.org/p/chromium/issues/entry?description=Please+describe+your+third+party+use+case+here.&labels=Hotlist-ThirdPartySoftware,OS-Windows&oss=linux&owner=chrisha@chromium.org&components=Internals%3EPlatformIntegration)!

**Q. What about shell extensions?**

A. For the time being we are warning about shell extensions that inject, and
blocking them when possible. We realize that this is less than ideal given that
most shell extensions injection innocently. We are actively [moving all file
dialog code to an external utility
process](https://bugs.chromium.org/p/chromium/issues/detail?id=884075&q=out%20of%20process%20file%20dialogs&colspec=ID%20Pri%20M%20Stars%20ReleaseBlock%20Component%20Status%20Owner%20Summary%20OS%20Modified)
where injection will be allowed. As of November 2018 this is being experimented
with.

**Q. What about printing?**

A. Chrome will allow software to inject as part of the printing stack, and will
not warn about modules that are injected during printing. We are actively
[working to move printing to an external utility
process](https://bugs.chromium.org/p/chromium/issues/detail?id=809738) so that
this injection will no longer incur in the browser process.

**Q. What about IME?**

A. Currenty Chrome will allow registered IME modules to inject so as to break
alternative text entry systems. Chrome is moving towards
[TSF](https://docs.microsoft.com/en-us/windows/desktop/tsf/text-services-framework)
rather than
[IMM32](https://docs.microsoft.com/en-us/windows/desktop/api/_intl/). Eventually
IMM32 will be deprecated, and these modules will no longer be allowed to inject.

**Q. What about Win32 event hooks?**

A. On Windows 8 and above we will be blocking event hooks (and other legacy
injection mechanisms) from injecting into the browser process using the
[PROCESS_CREATION_MITIGATION_POLICY_EXTENSION_POINT_DISABLE_ALWAYS_ON](https://www.google.com/search?q=PROCESS_CREATION_MITIGATION_POLICY_EXTENSION_POINT_DISABLE_ALWAYS_ON&oq=PROCESS_CREATION_MITIGATION_POLICY_EXTENSION_POINT_DISABLE_ALWAYS_ON&aqs=chrome..69i57j69i59.542j0j4&sourceid=chrome&ie=UTF-8)
mitigation policy. Note that there is [ongoing work](/developers/mus-ash) to
move window management and input to an external UI process. We will be
evaluating whether or not to bring this technology to the windows platform, and
if we do, whether or not to allow legacy injection in this process as part of
that work.

**Q. What about AMSI providers?**

A.
[AMSI](https://docs.microsoft.com/en-us/windows/desktop/amsi/antimalware-scan-interface-portal)
providers are currently allowed to inject into Chrome, and no warning or
blocking will apply. They are invoked when downloaded files are scanned using
[IAttachmentExecute](https://docs.microsoft.com/en-us/windows/desktop/api/shobjidl_core/nn-shobjidl_core-iattachmentexecute)
(and also in some other situations, for example, by some printer drivers), which
is used to enable [Mark of the
Web](https://technet.microsoft.com/en-us/ms537628(v=VS.71)) support on the
Windows platform. This scanning is being [moved to a utility
process](https://bugs.chromium.org/p/chromium/issues/detail?id=883477&q=owner%3Apmonette%40chromium.org%20&colspec=ID%20Pri%20M%20Stars%20ReleaseBlock%20Component%20Status%20Owner%20Summary%20OS%20Modified)
so that the injection will no longer incur in the browser process.

**Q. What about *\[feature or use case X\]*?**

A. Our general advice is to stop injecting into Chrome, and to seek alternative
mechanisms that use some combination of modern APIs, external processes, Chrome
extensions and [native
messaging](https://developer.chrome.com/apps/nativeMessaging).

**Q. What about enterprise use cases?**

A. We maintain an [enterprise
policy](/administrators/policy-list-3#ThirdPartyBlockingEnabled) for
specifically *allowing* third party injection, and a [companion
policy](/administrators/policy-list-3#ForceNetworkInProcess) for controlling
whether or not Chromium's network stack is in the main browser process. These
policies will be supported for at minimum the calendar year 2019, and 6 months
(4 Chromium versions) notice will be provided prior to removing them. Notice
will be provided on this page, as well as via the [enterprise release
notes](https://support.google.com/chrome/a/answer/7679408?hl=en) for the
affected release.
