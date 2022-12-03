---
breadcrumbs:
- - /teams
  - Teams
- - /teams/layout-team
  - Layout Team
- - /teams/layout-team/meeting-notes
  - Layout Team Meeting Notes
page_name: monday-april-25-2016
title: Monday, April 25, 2016
---

Updates since last meeting (on Monday, April 11, 2016):
CSS Flexbox (cbiesinger)
- Flexbox bug triage and bug fixes. Unrelated to scrolling for a change
(bug 576202, bug 341310, bug 375693).
- Ramping back up on scrolling, trying to help cbiesinger with flexbox
overflow:auto heartache. Lots of fun. (szager)
CSS Grid Layout (svillar, jfernandez, rego, javif)
\[[crbug.com/79180](http://crbug.com/79180)\]
- Reviewing CSS Grid specification. (cbiesinger)
CSS Multi-column (mstensho) \[[crbug.com/334335](http://crbug.com/334335)\]
- Continuing to focus on multi-col and fragmentation.
- Orphans and widows, need to clean up this code, to support
break-before & break-after.
CSS Houdini
- Waiting on codereivews for CSS Paint API. (ikilpatrick)
- Begun work on spec stuff, cleaning up paint, worklets. Starting to
work more on CSS Layout API. (ikilpatrick)
Add API for layout (leviw, pilgrim, dgrogan)
\[[crbug.com/495288](http://crbug.com/495288)\]
- Migrated most references of Document::layoutView() to Document::
layoutViewItem() in preparation for removing Document::layoutView()
and routing everything through the block layout API. (pilgrim)
- Migrating references to the layoutBlockView on document, and move to
item. (pilgrim)
- Start the LineLayoutAPI document with leviw. pilgrim, dgrogan to
clustering all the API methods into groups, figuring out we want
to rationalize these roots. (dgrogan)
LayoutNG \[[crbug.com/591099](http://crbug.com/591099)\]
- No updates since last week -
CSS Containment (leviw) \[[crbug.com/312978](http://crbug.com/312978)\]
- Sent out Intent to Ship. (eae)
Intersection Observer (szager, mpb)
\[[crbug.com/540528](http://crbug.com/540528)\]
- Wrapped up most Intersection Observer work, merged a patch into
M51. (szager)
Tables (dgrogan)
- Table layout refactor, putting out fires with patch going to stable.
- Working through list of highly starred table bugs.
- Much digging into tables for <https://crbug.com/427994> and
<https://crbug.com/178369> which are different but related issues.
mstensho has been consulting.
Text (eae, drott, kojii)
- Continued to work on font-variant-caps, blocked on rebaseline bot
being offline. (drott)
- font-variant property into a shorthand, CL up for review. (drott)
- Contributed to Koji's document on implementation/shipping plan for
hyphenation. (drott)
- Implemented font-variant property parsed as CSS shorthand (as opposed
to being a CSSPrimitive Value before), working well. Font-variant now
accepts font-variant-ligatures and font-variant-caps values, and
combines synthetic small caps with ligatures \\o/. Up for review.
(drott)
- Fixed bug where we sometimes omitted tab characters to due incorrect
rounding in LineWidth::snapUncommittedWidth. (eae)
- Fixed crash in Font::computeCanShapeWordByWord. (eae)
- Fixed handling of very large transforms in ContainerNode::boundingBox.
(eae)
Logistics:
- cbiesinger is now a core OWNER, congratulations!
- skobes out until end of the month.
- eae in Tokyo until end of week, Zurich next week.
- drott in Zurich next week.
