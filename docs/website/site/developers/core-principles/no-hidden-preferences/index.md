---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/core-principles
  - Core Principles
page_name: no-hidden-preferences
title: No Hidden Preferences
---

One of the [Core Principles](/developers/core-principles) is Simplicity:

> We tend to avoid complex configuration steps in every aspect of the product,
> from installation through options. We take care of keeping the software up to
> date so you never need to be concerned about running the latest, safest
> version of Chrome.

> We try not to interrupt users with questions they are not prepared to answer,
> or get in their way with cumbersome modal dialog boxes that they didn't ask
> for.

The target user base for Chromium is incredibly diverse, and while we try to
make Chromium flexible and powerful enough to serve everyone well, the reality
is that we can't always do that. Some users might want different behaviors, and
the easy way out is to give them that option. That is a temptation that must be
resisted.

When we make a hidden preference, it is admitting failure. If our users complain
about a feature, do we add a preference for it to be turned off? If we do so
we're ignoring the feedback of our users that perhaps the feature could be
smarter in satisfying their needs. In addition, as software developers we
usually don't think about it, but setting a preference is a pretty technically
complicated task, so adding one wouldn't actually help the vast majority of the
users, who will end up suffering with the feature in silence. Take the feedback
to improve the feature, not to add a switch to turn it off.

Hidden settings also make it nearly impossible to do full testing. Each
boolean-valued preference means that the code can behave one of two ways. Do you
write all your tests to take those two branches into account? Each additional
preference means twice the number of code paths, meaning exponential growth.
This means preferences are bad for maintainability.

about:flags and command-line flags are only for temporary testing. Long-lived
command-line flags are prohibited except for rare cases where we need them for
long-term testing and development.

Recommended external links:

*   [ignore the code: Preferences Considered
            Harmful](http://ignorethecode.net/blog/2008/05/18/preferences-considered-harmful/)
*   [Joel Spolsky:
            Choices](http://www.joelonsoftware.com/uibook/chapters/fog0000000059.html)
