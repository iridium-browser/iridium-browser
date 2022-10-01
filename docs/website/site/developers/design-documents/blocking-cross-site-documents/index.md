---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: blocking-cross-site-documents
title: Blocking Cross-Site Documents for Site Isolation
---

### NOTE: This page represents earlier work that led to the current [Cross-Origin Read Blocking (CORB)](/Home/chromium-security/corb-for-developers) policy. Also see the [CORB explainer](https://chromium.googlesource.com/chromium/src/+/HEAD/services/network/cross_origin_read_blocking_explainer.md) and the relevant part of the [Fetch spec](https://fetch.spec.whatwg.org/#corb).

### Motivation

In general, Chrome's renderer processes should not be given access to data they
don't need. Our [Site Isolation
project](/developers/design-documents/site-isolation) is trying to make it
possible to limit a renderer process to pages from only a single site. As part
of this, Chrome needs to block such a process from receiving network responses
that the site isn't allowed to use.

A web site is allowed to request **documents** from its own site (or those that
have been made available using CORS), as well as **resources** from any site.

**Documents** include HTML, XML, and JSON files. **Resources** include a wide
variety of files that are "opaque" to the site (i.e., cannot be leaked back to
the site's owner), including images, JavaScript, CSS, fonts, and TextTrack
files, among others. **Cross-site documents** come from a different site than
the page that requests them and do not have CORS headers that would make them
accessible.

However, we can't always rely solely on the MIME type of the HTTP response to
distinguish documents from resources, since the MIME type on network responses
is frequently wrong. Content sniffing can help to confirm whether a document is
what it claims to be, but using typical content sniffers would lead to
overzealous blocking and create compatibility problems. Fortunately, our
experiments show that we can enforce a policy that blocks most cross-site
documents without breaking existing sites, using a combination of MIME type and
custom content sniffers.

### Proposed Policy

Our proposed **cross-site document blocking policy** prevents a renderer process
from receiving cross-site documents that (1) have a HTML, XML, or JSON MIME
type, and (2) are confirmed to be that type by a custom content sniffer (or are
marked NoSniff). It does not currently protect files that have other MIME types.
Documents that are blocked appear as empty to the process that requested them.

While implementing versions of this policy, we made several observations about
how the content sniffing logic must behave:

*   For HTML, we cannot assume "&lt;!--" is valid HTML prefix, since
            that also matches many JavaScript files in practice. Instead, we
            plan to find the end of the HTML comment and then sniff again.
*   For HTML and XML, we cannot block "&lt;?xml" unless we can confirm
            it isn't an SVG. Cross-site SVG files are permitted.
*   There is no existing JSON sniffer, but it is necessary because many
            existing JavaScript files are mislabeled as JSON. We have
            implemented a basic sniffer.

To preserve compatibility, we are aiming to avoid disrupting the behavior of
existing web sites when blocking responses, within reasonable limits. We
consider certain behavior changes acceptable, though. For example, we prevent
actual HTML, XML, or JSON files from being parsed as CSS. CSS parsers were
originally very lenient and could discover "rules" from almost any content,
which was used by security researchers for attacks \[1\]. CSS now requires a
valid MIME type, though, so it should be unaffected by the proposed blocking
behavior.

When measuring the potential impact of the policy, we discovered several
"non-disruptive" cases in which cross-site documents were blocked but that cause
no behavior change to the page. These include:

*   Responses with error status codes in any context except images.
            (Images continue to render regardless of status code, and this is
            sometimes used in practice for 404 images.)
*   Responses that could not have been parsed for the target. For
            example, files sniffed as HTML cannot be parsed as JavaScript, and
            non-SVG text files cannot be parsed as images.

We consider it harmless to block such network responses.

We only plan to enforce the policy on web renderer processes. Extension
processes in Chrome are allowed to request more documents (based on the host
permissions in the extension's manifest), and we could later enforce a similar
policy that accounts for their permissions. Plugin processes currently have an
exception as well since their requests currently go through the renderer
process, but this will be changed in https://crbug.com/778711.

Unfortunately, we do have to give up some security properties when extensions
with content scripts are installed. Such extensions can request permission in
their manifest to make XmlHttpRequests to other sites from within the renderer
process. We cannot block such documents from the renderer process, which would
allow an exploited renderer to also access such documents. We are investigating
how to update the content script security model to make this less of a concern
in practice.

### Compatibility Results

To measure compatibility, we ran an experiment that collected user metrics on
how many network responses would be blocked using our policy. Our experiment
followed the policy described above, with two caveats. First, it incorrectly
blocks cross-site SVG files. Second, it incorrectly blocks cross-site requests
made by extension content scripts. Using one week of data collected from Chrome
Beta users (using Chrome's opt-in UMA system), we found that 0.16% of network
responses were blocked overall, but more than half of these we could prove to be
non-disruptive (e.g., error status code). Only 0.075% of network responses were
potentially disruptive.

We intend to refine our experiment to avoid blocking content scripts and SVG
files. Based on the data we collected, we found that content scripts could
account for up to 0.009% of responses. Accounting for SVG files could let us
conclude that up to another 0.063% of responses were not disruptive. This
suggests that our updated policy could be disruptive for as few as 0.003% of
network responses.

Using these results, we have started to deploy the policy to users who have Site
Isolation enabled in Chrome 63. In combination with our other Site Isolation
work, this can help protect confidential documents from leaking to other sites.

### Current Policy

The current document blocking policy protects HTTP responses for URLs labeled
with one of these MIME types in the "Content-Type" HTTP response header:

*   text/html
*   text/xml
*   application/xml
*   application/rss+xml
*   application/json
*   text/json
*   text/x-json
*   text/plain

The HTTP response should also have a "X-Content-Type-Options: nosniff" HTTP
response header, which ensures that the protection will not depend on what the
contents of the file look like.

Note that we recommend not supporting [multipart range
requests](https://developer.mozilla.org/en-US/docs/Web/HTTP/Range_requests) for
sensitive documents, because this changes the MIME type to multipart/byteranges
and makes it harder for Chrome to protect.

In addition to the recommended cases above, Chrome will also do its best to
protect responses labeled with any of the MIME types above and without a
"nosniff" header, but this has limitations. Many JavaScript files on the web are
unfortunately labeled using some of these MIME types, and if Chrome blocked
access to them, existing websites would break. Thus, when the "nosniff" header
is not present, Chrome first looks at the start of the file to try to confirm
whether it is HTML, XML, or JSON, before deciding to protect it. If it cannot
confirm this, it allows the response to be received by the cross-site page's
process. This is a best-effort approach which adds some limited protection while
preserving compatibility with existing sites. We recommend that web developers
include the "nosniff" header to avoid relying on this approach.

### Presentation

#### Tech Talk: Cross-Site Document Blocking

### References

\[1\] Huang, Lin-Shung, et al. "Protecting browsers from cross-origin CSS
attacks." Proceedings of the 17th ACM conference on Computer and communications
security. ACM, 2010.
