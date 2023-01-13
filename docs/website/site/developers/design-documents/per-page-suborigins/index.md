---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: per-page-suborigins
title: Per-page Suborigins
---

[TOC]

# The following is deprecated and left for historical purposes. Please see the
editor's draft of the spec for the most up to date design:
<https://w3c.github.io/webappsec-suborigins/>

# Objective

Our objective is to provide a new mechanism for allowing sites to easily
separate their content into separate, flexible synthetic origins while serving
content from a single physical origin. Furthermore, the synthetic origins should
be predictable and convey the full physical origin so that compartmentalized
content can easily use current browser technologies, such as postMessage, to
interact with each other.

# Background

This document is based on (and much text lifted from) an original proposal
written by Michal Zalewski

Currently, web applications are almost always compartmentalized by using
separate host names to establish separate web origins. This is useful for
helping to prevent XSS and other cross-origin attacks, but has many unintended
consequences. For example, it causes latency due to additional DNS lookups,
removes the ability to use single-origin features (such as the history.pushState
API), and creates cryptic host name changes in the user experience. Perhaps most
importantly, it results in an extremely inflexible architecture that, once
rolled out, can’t be easily and transparently changed later on.

There are several mechanisms for reducing the attack surface for XSS without
creating separate host-name based origins, but each pose their own problems.
Per-page Suborigins is an attempt to fill some of those gaps. Two of the most
notable mechanisms are Sandboxed Frames and Content Security Policy (CSP). Both
are powerful but have shortcomings and there are many external developers
building legacy applications that find they cannot use those tools.

Sandboxed frames can be used to completely separate untrusted content, but they
pose a large problem for containing trusted but potentially buggy code because
it is very difficult, by design, for them to communicate with other frames. The
synthetic origins assigned in a sandboxed frame are random and unpredictable,
making the use of postMessage and CORS difficult.

Content Security Policy is also promising but is generally incompatible with
current website design. Many notable companies found it impractical to retrofit
most of their applications with it. On top of this, until all applications
hosted within a single origin are simultaneously put behind CSP, the mechanism
offers limited incremental benefits, which is especially problematic for
companies with large portfolios of disparate products all under the same domain.

# Overview

We want to create a new browser primitive to provide isolation within a single
origin between disparate components. Often, there are many web applications that
share a single real origin. One example of this is www.google.com, which many
functionally separate applications (in the hundreds) share for reasons related
to branding, DNS latency, and seamless interoperability, despite being very
different properties. The consequence of this is that one compromised web
application from an XSS at the www.google.com origin means that all properties
at www.google.com are compromised. Many other popular web destinations such as
Facebook, Twitter, Dropbox, etc. face similar issues.

This is an accident of the Same Origin Policy (SOP) in the sense that the SOP
assumes everything at a given real origin is part of the same application. A
Per-page Suborigin primitive would allow developers to apply the SOP at a finer
grained granularity and specify that different applications within the same real
origin should, in fact, be treated as different origins under the SOP. The
primitive should fill a space between Sandboxed Frames and CSP by allowing
consumers to separate trusted components into separated origins while still
allowing efficient cross-origin communication via postMessage and CORS, but also
without significant retrofitting limitations to legacy applications.

Per-page Suborigins would provide a way for applications at the same origin to
request to be placed in separate, synthetic suborigins. These suborigins would
be in a predictable namespace, so utilizing postMessage and CORS would be
possible just as normal. However, it would also provide the strong isolation
properties of regular SOP protection; despite being in the same real origin,
code from different suborigins would not be able to access the DOM of a
different suborigin.

Furthermore, Per-page Suborigins could actually help with a rollout of CSP. As
noted above, when many different properties are hosted on the same domain, if
only some have CSP applied, many of the benefits are lost because an XSS on one
of the non-CSP enabled pages poses an immediate threat to CSP-enabled content.
With Per-page Suborigins, by compartmentalizing different applications within
the same domain, an exploit of one application that is not CSP enabled would not
necessarily compromise an application in a different Per-page Suborigin, thus
increasing the value of gradually deploying CSP for high-value components.

We see effectively three different use cases for Per-page Suborigins:

    Separating distinct applications that happen to be served from the same
    domain, but do not need to extensively interact with other content. Examples
    include marketing campaigns, simple search UIs, and so on. This use requires
    very little engineering effort and faces very few constraints; the
    applications may use XMLHttpRequest and postMessage to communicate with
    their host domain as required.

    Allowing for modularity within a larger web application by splitting the
    functional components into different suborigins. For example, Gmail might
    put the contacts widget, settings tab, and HTML message views in separate
    Per-page Suborigins. Such deployments may require relatively modest
    refactorings to switch to postMessage and CORS where direct DOM access and
    same-origin XMLHttpRequest are currently used, but we believe doing so is
    considerably easier than retrofitting CSP onto arbitrary code bases and can
    be done very incrementally.

    Similar to (2), applications with many users can split information relating
    to different users into their own suborigin. For example, Twitter might put
    each user profile into a unique suborigin so that an XSS within one profile
    cannot be used to immediately infect other users or read their personal
    messages stored within the account.

# Detailed Design

## Definition of Suborigins

Suborigins will be a synthetic origin, similar to the already implemented
synthetic suborigins for HTML5 sandboxed frames. However, suborigins will have
the important property of being predictable, well-defined, and hierarchical,
allowing postMessage and CORS communication between sandboxed content.

A suborigin may be specified by a HTTP header such as:

Suborigin: &lt;name&gt;

where &lt;name&gt; is defined by the server or developer and should be
predictable.

We will treat suborigins as a separate field from origins that must be checked
whenever the SOP is being enforced. Thus, the above suborigin will result in the
following SOP origin for the frame of:

origin: &lt;protocol&gt;://&lt;host&gt;:&lt;port&gt;

suborigin:      &lt;name&gt;

where &lt;protocol&gt;, &lt;host&gt;, and &lt;port&gt; remain as they always
have, and &lt;name&gt; is a name defined by the server of alpha-numeric
characters only. In the case that no suborigin is specified, the value of
suborigin should be treated as the empty string.

A potentially preferable option would be to implement this as a CSP directive.
For example:

Content-Security-Policy: suborigin &lt;name&gt;;

which would result in the same full origin as above.

An important point to note here is that, because of security considerations,
there is no way to “surrender” or “escape” a suborigin. That is, once assigned a
suborigin, a page cannot return within that instance to the real origin; the
ability to do so would completely defeat the sandboxing purpose of the
mechanism.

We have considered alternative approaches to encoding suborigins but concluded
that including it in the header is the best approach. For example, one
alternative would be to encode the suborigin as part of the host or protocol.
Two ways you might do this are:

origin: &lt;protocol&gt;+&lt;name&gt;://&lt;host&gt;:&lt;port&gt;

or      origin: &lt;protocol&gt;://&lt;name&gt;@&lt;host&gt;:&lt;port&gt;

These have the advantage that they (a) encode the suborigin as part of the
current origin, thus no new suborigin field is required, and (b) current
enforcement mechanisms would require relatively few changes since it would
effectively be encoding the suborigin as part of either the protocol or host,
and the current SOP already enforces these components.

However, suborigins are not intended to be user understandable or interactive,
and encoding suborigins in the scheme/host/port would risk users interacting
with them, such as in the URL bar or having them accidentally displayed.
Additionally, suborigins, as we will discuss, cannot be used in a variety of
contexts that origins are valid, such as resource identifiers in &lt;iframe&gt;
or &lt;img&gt; tags because we require that the server be the authority on what
entities are place in suborigins. Thus, we create suborigins as a unique field
alongside the origin field.

Permissions that are normally associated with origins should not be applied to
Suborigins. For example, geolocation permissions and fullscreen permissions
should not be propagated to execution contexts in a new Suborigin. Moreover,
there should be no way for Suborigins to obtain such permissions. We apply these
restrictions because there should be no way for users to see or interact with
Suborigins, and any special permissions might require user confirmation that
would require knowledge about the implementation level details of suborigins.

Cross-origin and cross-suborigin communication via postMessage is one of the
most important features of per-page suborigins. Per-page suborigins with
postMessage is a mostly straightforward extension of the current implementation.
The main concern is on the receiving end of a message listener since we need
both backwards compatibility and do not want to let legacy implementations
accidentally accept messages from incorrect origins. Thus, the receiving event
object will have event.origin set to null so that legacy applications will not
confuse a message from the foo suborigin at bar.com for a message from the top
level suborigin at bar.com. Instead, we propose a new property on the event
object, event.finerorigin, that will contain two properties, origin and
suborigin, containing the host and suborigin values respectively.

In the case of pages with a non-empty string suborigin fields, all
XMLHttpRequest requests to any URL should be treated as cross-origin, thus
triggering CORS logic. This contains a similar concern to the postMessage issue
outlined above. In order to prevent legacy applications from accidentally
accepting CORS requests from suborigins, CORS headers originating from a
suborigin execution context must specify a Suborigin: &lt;name&gt; header and a
Finer-Origin: &lt;origin&gt; header, while explicitly not setting an Origin
header. Similarly, responses should be checked for matching new
Access-Control-Allow-Finer-Origin and Access-Control-Allow-Finer-Suborigin
headers, while Access-Control-Allow-Origin headers should be ignored.

Execution contexts with a Suborigin should be tagged with a flag is_suborigin
that is inherited by all child contexts that normally inherit the origin of the
creator. The SOP should apply as it currently does, using the suborigin in
addition to the traditional origin for making policy decisions. Thus,
containment should nearly be “for free,” that is it should work using the
current SOP mechanisms with an additional check for the Suborigin of an
execution context.

However, in order to compartmentalize, there may be things that may require
limited access. As mentioned before, various permissions, such as geolocation,
must be disallowed in Suborigins. Additional restrictions also apply.

Notably, a suborigin frame may not access document.cookie. While cookies will
still be sent with HTTP requests based on the real origin, to simplify
compartmentalization and security, suborigin contexts will not be able to access
the cookie object, similar to sandboxed contexts. This will prevent information
leak and limit the damage a compromised frame can do. If information from a
cookie is needed, it will need to communicate with a non-suborigin frame via
postMessage or another cross-origin mode of communication.

Additionally, plugins pose a problem because they use questionable methods to
determine the origin of the embedding page, and may bestow some privileges
associated with this origin to the plugin-executed code. In light of this,
&lt;embed&gt;, &lt;object&gt;, or &lt;applet&gt; content within documents where
is_suborign is set will not be allowed. As opposed to sandboxed frames, however,
is_suborign will not unconditionally propagate to all subframes - and therefore,
many forms of rich ads or Flash videos placed on sandboxed pages should continue
to work as-is or with only minimal tweaks.

## Checking Origins and Suborigins

As mentioned before, the SOP will now be applied to the new composite orign,
that includes both an origin and suborigin field. SOP checks should remain
functionally the same, however they should now include an additional check of
the suborigin field for lexical case-insensitive equality.

## Concerns

There are many potential unintended consequences of establishing finer- or
coarser-grained origins, and there is a long history of security issues and
surprising corner cases caused by many of the previously proposed mechanisms
(including sandboxed frames and CSP).

Nevertheless, we think that this proposal avoids most of the pitfalls evident in
previous efforts - in large part because it does not try to enforce any new,
complex security rules for the sandboxed content, and does not routinely expose
the new boundaries to the user.

One concern, however, is with phishing attacks. As mentioned earlier, suborigins
are purposefully create as a new field, rather than being fit into the current
scheme/host/port scheme. This is because we don’t want users to see these
implementation details. However, this has the consequence that phishing attacks
are possible by suborigin content where content pretends to be from another
suborigin. However, this is already the case with CSP.

# Security Considerations

When implementing the feature, care should be taken to provide several
relatively straightforward guarantees:

    The origin needs to be correctly communicated and matched by the postMessage
    API and in the Origin header in CORS requests (this should require no code
    changes, but should be confirmed by the implementor). All XMLHttpRequest
    initiated from an isolated origin would be treated as CORS.

    The document.cookie API must be inaccessible from contexts marked as
    is_suborigin, similar to how sandboxed contexts are currently treated.

    Password manager and the “content permissions” (geolocation, full-screen
    access, microphone & webcam APIs) generally should be disallowed in
    Suborigin execution contexts. This is generally because of UI considerations
    and the desire to not display anything about Suborigins to users.

# Future Work and Extensions

Allow some limited access to document.cookie. Although we suggest using
postMessage for transmitting and cookie related information to a suborigin, it
might be useful to allow some limited access to cookies. One future approach
might be to create suborigin-specific sections of cookies that only the named
suborigin can access.
