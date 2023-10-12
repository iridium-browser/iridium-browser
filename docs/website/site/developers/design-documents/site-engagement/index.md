---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: site-engagement
title: Site Engagement
---

The Site Engagement Service provides information about how engaged a user is
with a site. The primary signal is the amount of active time the user spends on
the site but various other signals may be incorporated (e.g whether a site is
added to the homescreen).

What is a site?

Site is an inexact term. For the purposes of site engagement, we’ll be treating
origins as sites.

What is engagement?

Engagement is an even more nebulous term. We want to track that the user is
giving some amount of active attention to the site. To approximate this, we
register a user as active when the user scrolls, clicks or types on the page.
Media playback also provides a small engagement increase.

Engagement Score

The engagement score has the following properties:

    The score is a double from 0-100. The highest number in the range represents
    a site the user engages with heavily, and the lowest number represents zero
    engagement.

    Scores are keyed by origin.

    Activity on a site increases its score, up to some maximum amount per day.

    After a period of inactivity the score will start to decay.

    The engagement score is not intended to be a dumping ground for ‘signals we
    want to include’. If other signals that are not directly user-attention
    based are required, they should be tracked separately.

The current activity based signals are:

    Direct navigations to a site (e.g omnibox, bookmark and not link clicks,
    popups)

    Active time on the site (where active time is represented by e.g. scrolling,
    clicking, keypresses)

    Media playback on a site

The current grant based signals are:

    Adding the site to the homescreen / desktop

The user engagement score are not synced, so decisions made on a given device
are made based on the users’ activity on that device alone.

Usage of the engagement score

Site Engagement clients should consider how their use case relates to
engagement. We expect that many clients should only be using engagement as one
facet of a more tailored heuristic. The engagement score has been designed with
these use cases in mind:

    Sorting or prioritizing sites in order of engagement (e.g tab discarding,
    most used list on NTP)

    Setting engagement cutoff points for features (e.g app banner, video
    autoplay, window.alert())

    Allocating resources based on the proportion of overall engagement a site
    has (e.g storage, background sync)

Viewing Your Own Scores

You can inspect the scores for a profile by navigating to
chrome://site-engagement within that profile.

Privacy Considerations

When in incognito mode, site engagement will be copied from the original profile
and then allowed to decay and grow independently. There will be no information
flow from the incognito profile back to the original profile. Incognito
information is deleted when the browser is shut down.

Engagement scores are cleared with browsing history. Origins are deleted when
the history service deletes URLs and subsequently reports zero URLs belonging to
that origin are left in history.

URLs are cleared when scores decay to zero.
