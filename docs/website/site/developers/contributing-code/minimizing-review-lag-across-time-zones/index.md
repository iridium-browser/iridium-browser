---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/contributing-code
  - Contributing Code
page_name: minimizing-review-lag-across-time-zones
title: Minimizing Review Lag Across Time Zones
---

If you're reviewing a change for someone in a different time zone, one review
cycle can take up to 24 hours.

It's useful to remember that many contributors working outside of PST are
constantly blocked on reviews. We can't rewrite the laws of physics but here are
some tips on making the process go faster:

*   Please consider "LGTM with nits.", If a review from a committer has
            only minor issues. This small phrase can unblock an excited non-PST
            engineer a full 24 hours sooner!
*   Don't require a second round for trivial changes.
*   For owner reviews: provide an owner LGTM if the overall approach and
            structure of the CL looks good, but ask the CL author to find a
            reviewer in their timezone to work with to get all the details
            right.
*   If you have changes you want to see or if you don't like the CL
            **==suggest a solution and provide as much information as
            possible.==** Terse responses mean a full 48 hours can go by before
            the non-PST person can ask "How would you suggest I accomplish
            this?" and then another 24 while they wait for the answer. Please
            don't simply ask questions on the review; **==suggest an acceptable
            solution==**.
*   Along the same lines, if you don't like the way something is named
            **==please suggest an alternate name==** that you'd find acceptable.
*   See [this
            notice](https://groups.google.com/a/chromium.org/d/msg/chromium-dev/ZeNYH4JBJVY/5bhKpwGuCAAJ)
            about acceptable latency for code reviews.
