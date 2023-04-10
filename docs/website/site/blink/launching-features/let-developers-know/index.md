---
breadcrumbs:
- - /blink
  - Blink (Rendering Engine)
- - /blink/launching-features
  - Launching Features
page_name: let-developers-know
title: Let Web Developers Know about New Features
---

So, You're Building a Feature. Now what?

Web developers need to know about it so they can use it.

Chrome Developer Relations has a process for that, but we need a little help
from you. The big picture is that we use a release blog to post information
about every new developer-facing or developer-affecting feature in every Chrome
beta version. Developer Relations writes articles about some of these features.
(We're not big enough to write about all of them.) This blog attracts the
attention of the tech press. They write news stories about those features and
usually link to our articles.

If you miss getting your feature mentioned in the beta release post, you miss
the best opportunity to tell the world about what you built.

# The Process

1.  Write a good Chrome Status description.
    This is your first opportunity to communicate to the world what you're
    doing. Chrome Status autogenerates an Intent email for you, but the summary
    is public. If you write this well, it will appear as-is in the beta release
    post later. You can find a few [guidelines
    here](/blink/launching-features/how-chrome-status-communicates).
2.  Feature freeze (four weeks before beta): Let Developer Relations
            know that you plan to ship your feature in the upcoming beta. (See
            [Contacts](#Contacts), below.)
    **Inform us no matter how small the chance of shipping.** Waiting until you
    know for sure does us no favor. Preparing for a released feature at the last
    minute is difficult. Holding back text for a feature that wasn't released is
    trivial.
    If you're not sure that something is shipping you probably shouldn't update
    your Chrome Status entry. But you must tell us. Shipping in this case means
    available by default or available in an origin trial. It does not mean other
    types of flags.
3.  You may be asked at some point to review a longer article for
            technical accuracy. Developer Relations is a small group. We don't
            have the resources to write about everything.

That's it. **Your communication responsibilities are complete.** If you want to
confirm that something is on our radar follow
[go/what's-shipping](https://docs.google.com/spreadsheets/d/155euqrhdqVhtbAID7ydaUPjBstLIYZ4PJkpFmqJ6j-o/edit#gid=1093066458)
(externally accessible with permission) to see what DevRel already knows about.
(Note: we keep a separate planning doc because so we can track things that may
not be ready for Chrome Status.)

# What Happens Next?

Obviously, that's not the end of the story. I threw a bunch of information at
you at the top, which I'll now explain.

1.  Between feature freeze and beta, Chrome Developer Relations creates
            [articles](https://web.dev/blog/) and
            [videos](https://www.youtube.com/channel/UCnUYZLuoy1rq1aVMwx4aTzw)
            about the new features. We also draft a [beta release
            post](https://blog.chromium.org/search/label/beta) for the [Chromium
            blog](https://blog.chromium.org/).
2.  As soon as Chrome beta is released, the beta release post is
            published on the Chromium blog. A member of our team tweets a blog
            post link to [@ChromiumDev](https://twitter.com/ChromiumDev).

# What about Non-Google Engineers

I haven't forgotten that engineers from outside Google work on Chromium. We want
to give proper credit where it is due, but are still working on the most
appropriate way to do this. We still want to let developers know about features
you've built. Please, let us know what you've added to Chromium and we'll make
sure it gets in the beta release post.

**Frequently Asked Questions**

**My feature, which adds a missing method or property, is only a bug. Do I need
to tell you?**

Yes. We need to know everything.

**My feature is an update to the spec. Do I need to tell you?**

Yes. We need to know everything.

**I’m hoping to get 3 LGTMs before beta, but I haven’t written an intent yet. Do
I need to tell you?**

Yes. We need to know everything.

(Are detecting a theme yet.)

**I don’t think I’ll be done until after the branch point, but I’m hoping to
merge to the current beta. Do I need to tell you?**

Yes. We need to know everything.

**My feature fixes a regression in a recent version of Chrome. Do I need to tell
you?**

We don’t need it for the beta release, but we may need to update MDN. Please let
us know anyway. In other words, yes. We need to know everything.

# Contacts

Joe Medley - Help with getting the word out on your new feature.
([jmedley@google.com](mailto:jmedley@google.com))
Paul Kinlan - Head of Developer Relations for Chrome and the web platform.
([paulkinlan@google.com](mailto:paulkinlan@google.com))
Kayce Basques - Lead technical writer for Chrome and the web platform.
([kayce@google.com](mailto:kayce@google.com))
