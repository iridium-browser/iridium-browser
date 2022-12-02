---
breadcrumbs:
- - /user-experience
  - User Experience
page_name: feed-subscriptions
title: Subscribing to feeds
---

This document describes how Chrome will enable users to subscribe to RSS/Atom
feeds in the feed reader of their choice. It does not cover any feed reading
capability in Chrome itself.

## **Table of contents**

*   Discovery
*   Preview
*   Subscription
*   Adding feed readers
*   Podcasts
*   Tasks to implement

## Discovery

We will autodetect RSS and Atom feeds using the standard [autodiscovery
tags](http://diveintomark.org/archives/2003/12/19/atom-autodiscovery):
`<link rel="alternate" type="application/atom+xml"
href="http://www.example.com/xml/atom.xml">`
When a feed is available for a page, we will display an rss icon in the address
bar:

[<img alt="image"
src="/user-experience/feed-subscriptions/default.png">](/user-experience/feed-subscriptions/default.png)

Clicking on the feed icon will redirect the user to feed, nicely formatted for
the browser. If the page specifies multiple feed formats, the icon will link to
the first feed specified. The default formatting for feeds may allow the user to
access other options (see **Preview** below).
We should also consider whether we want to detect feed links in the content of
the page (for pages without auto-discovery). However, given the popularity of
auto-discovery mechanisms, this probably won't be necessary.

## Preview

When the user follows a link to a feed file, Chrome formats it for display in
the browser:

[<img alt="image"
src="/user-experience/feed-subscriptions/rss_preview.png">](/user-experience/feed-subscriptions/rss_preview.png)

**Caveat:** parsing feeds can be tricky, so in the interest of shipping this
quickly, the initial implementation will likely not include a preview of the
content.

If the feed is malformed, we would display an error instead of the feed contents
(rather than including a lot of unnecessary feed parsing code). Users could
still see the title and suscribe to the feed.
If the feed specifies custom formatting (such as Feedburner's BrowserFriendly),
we could use that formatting instead. In that case, we would still display the
feed icon in the address bar, which would redirect to the Chrome-formatted
version of the feed.
If the user access the feed preview page from the rss icon in the address bar,
and the referring page had multiple autolinked feeds, these could also be chosen
from the preview page.

## Subscribing

The preview page display includes a large button to subscribe to the feed in the
feed reader of your choice. Initially, this setting is "choose your feed
reader". After this, it defaults to the most recently used feed reader.
By default, we will include a limited number of popular feed readers (list TBD).
Ideally, we should also detect popular feed-readers installed locally. If
settings have been imported from Firefox, we should use that list instead
(including remembering the default setting). Finally, users can also add the
feed reader of their choice (see **Adding feed readers** below).
Users can set a preference to bypass the preview page. From then on feeds will
be redirected to the subscription page for the feed reader of their choice.

## Adding feed readers

Users can add web-based and local feed readers from the feed preview page:
\[mock that allows to add a web-based or local feed reader\]
We will also need a way to change your feed reader from the settings dialog, in
case you have chosen to bypass the feed preview page.
A newly added feed reader becomes the default selected option the next time a
feed is previewed.
Web-based feed readers can also be added through Javascript using the
[registerContentHandler](https://developer.mozilla.org/en/DOM/window.navigator.registerContentHandler):
navigator.registerContentHandler("`application/atom+xml`",
"http://www.theeasyreaderurl.com?feed=%s",
"Easy Reader");

When this function is called, the following dialog is displayed:

\[mock of permissions to install a feed reader\]

## Podcasts

The feed reader choice for podcasts (i.e. media feeds that contain audio or
video files) should be stored as a separate setting. The default options should
also be different (e.g. iTunes, Window Media Player...).
\[mock of how we handle podcasts in add/manage feeds\]
Ideally, the formatting for Media RSS feeds should display embedded readers for
the attached media files.

## **Tasks to implement**

**Minimum**

*   Feed autodetect
*   Feed icon in address bar
*   Basic feed preview page (title and subscribe button)
*   Adding/managing feed readers
*   registerContentHandler
*   Import of Firefox feed settings
*   Separate handler support for podcasts

**Nice to have**

*   Preview of feed contents
*   Handling of custom formatting
*   Embedded players for podcasts
*   Detection of locally installed feed readers
*   Handling of multiple autolinked feeds
