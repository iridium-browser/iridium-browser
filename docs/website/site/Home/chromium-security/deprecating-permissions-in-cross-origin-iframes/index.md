---
breadcrumbs:
- - /Home
  - Chromium
- - /Home/chromium-security
  - Chromium Security
page_name: deprecating-permissions-in-cross-origin-iframes
title: Deprecating Permissions in Cross-Origin Iframes
---

**Contents**

[TOC]

## Proposal

It’s proposed that by default the following permissions cannot be requested or
used by content contained in cross-origin iframes:

*  Geolocation (getCurrentPosition and watchPosition)
*  Midi (requestMIDIAccess)
*  Encrypted media extensions (requestMediaKeySystemAccess)
*  Microphone, Camera (getUserMedia)

In order for a cross-origin frame to use these features, the embedding page must
specify a Permission Policy enables the feature for the frame. For example, to
enable geolocation in an iframe, the embedder could specify the iframe tag as:

```html
<iframe src="<https://example.com>" allow="geolocation"></iframe>
```

You can find the original blink [intent to deprecate thread
here](https://groups.google.com/a/chromium.org/forum/#!topic/blink-dev/mG6vL09JMOQ).

**This is a living document** — as we learn more, we'll probably need to change
this page.

## Motivation

Untrusted third-party content, such as ads, are frequently embedded in iframes
on websites. Currently, permissions like geolocation, midi, etc. can be directly
requested and used by this content.

UI for displaying permission prompts that are triggered by iframes can be very
confusing. Often permission prompts appear to be coming from the top-level
origin. As a result, users can be misled into granting permission to third-party
content that they did not intend to. At the very least, third-party content has
the ability to annoy users by displaying prompts even if they are undesired by
the embedding page.

Furthermore, even if a user has previously granted persistent permission to an
origin, they are unlikely to be aware when that origin is loaded in an iframe on
a website on the drive-by-web. This may result in unexpected and unwanted access
a user’s camera, location, etc.

The goal of this proposal is to protect users by disabling permissions by
default in iframes. Embedding websites would have the ability to re-enable
features for trusted content. This means that in order for a site to request
permission, the embedding website must express trust in the origin, in addition
to the user’s trust expressed through a permission grant.

It should also be noted that several new features being implemented (e.g.
Payment request, WebVR) are adopting the model of disabling sensitive features
in cross-origin iframes from the beginning. This change will bring older
features into line with the direction the web is heading.

## To continue to use permissions from iframes on your website...

This deprecation is expected to ship in Chrome M64 (around January 2018). At
that time, if a cross-origin iframe attempts to use permission without the
feature being explicitly allowed, a console warning will be logged and the
feature will fail in a similar way as it would if a user had denied a permission
prompt.

If you are a developer of a website which uses cross-origin iframes and you want
those iframes to continue to be able to request/use one of the above features,
the page that embeds the iframe will need to be changed. The simplest way to do
that is to modify the &lt;iframe&gt; tag to include an allow attribute which
specifies the name of the permission. For example, to enable geolocation and
mic/camera for an iframe, the following would be specified:

```html
<iframe src="<https://example.com>" allow="geolocation; microphone;
camera"></iframe>
```

Note that the above will grant geolocation, microphone and camera access to the
origin specified in the "src" attribute, i.e. in this case it would be
https://example.com. In some cases, other origins will be loaded in the iframe
that you may also wish to grant access to. In those cases you can explicitly
specify the origins to grant access to:

```html
<iframe src="<https://example.com>" allow="geolocation https://example.com
https://foo.com;"></iframe>
```

The above example would grant geolocation to https://example.com as well as
https://foo.com when they are loaded in the iframe. To grant access to all
origins that might be loaded in the iframe, the \* syntax can be used. This
should be used carefully as it means that any page that gets loaded in the
iframe can request geolocation, which is often not the intent. The code would
look as follows:

```html
<iframe src="<https://example.com>" allow="geolocation *;"></iframe>
```

Valid values for allow include:

*   geolocation
*   microphone
*   camera
*   midi
*   encrypted-media

Note that if the iframe which is using the permission has the same origin as the
top level page, then no changes have to be made.

## More Information

To find more information about Permissions Policy, take a look at the following
resources:

*   [The permissions policy
            specification](https://w3c.github.io/webappsec-permissions-policy/)
*   [The permissions policy
            explainer](https://github.com/w3c/webappsec-permissions-policy/blob/main/permissions-policy-explainer.md)
*   [chromestatus.com entry for this
            change](https://www.chromestatus.com/feature/5023919287304192)
