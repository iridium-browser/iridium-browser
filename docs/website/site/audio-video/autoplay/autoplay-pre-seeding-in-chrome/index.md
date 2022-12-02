---
breadcrumbs:
- - /audio-video
  - Audio/Video
- - /audio-video/autoplay
  - Autoplay
page_name: autoplay-pre-seeding-in-chrome
title: Autoplay Pre-Seeding in Chrome
---

### Chrome's Autoplay policy uses a metric called Media Engagement Index (MEI) to determine whether or not to permit media to autoplay on a given site. MEI learns, locally, from a user's individual browsing behaviors, on a site-by-site basis, whether the user regularly consumes media on each site. This helps Chrome to meet user expectations; if a user frequently consumes media for longer than 7 seconds in a previously activated tab, their media engagement score will increase, and eventually autoplay will be permitted for that site without a user gesture. Today, Chrome typically learns a user’s preferences within six to twenty visits.

Generally users do expect autoplay behavior for some sites, and until Chrome
learns individual preferences, based on MEI history, simply blocking all
autoplay isn’t the best match for user expectations. So, for new users, or users
who clear their browsing history, who won’t have historical media engagement
data, Chrome has a pre-seeded list of sites that are permitted to autoplay
media, based on aggregated anonymized data on what percentage of visitors to
that site regularly permit media playback with sound. This ensures that the
default behavior for sites with a high frequency of permitted media playbacks is
most likely to match what a typical visitor to that site expects - as a user
continues to use Chrome, their individual preferences will supplant the
pre-seeded MEI scores.

The pre-seeded site list is generated based on the global percentage of site
visitors who train Chrome to allow autoplay for that site; a site will be
included on the list if a sizable majority of site visitors permit autoplay on
it. The list is algorithmically generated, rather than manually curated, and
with no minimum traffic requirement. With the implementation of the autoplay
policy for Web Audio in M71, Web Audio playback is also included in calculating
the MEI score for a given site.
