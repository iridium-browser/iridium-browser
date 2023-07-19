---
breadcrumbs:
- - /developers
  - For Developers
page_name: useful-extensions
title: Useful extensions for developers
---

## Chromium

## [Chromite Butler](https://chrome.google.com/webstore/detail/chromite-butler/bhcnanendmgjjeghamaccjnochlnhcgj)

Gerrit:
\* Shows which file has been LGTMed by owners, which files are missing owner
reviews etc.
\* Suggests a list of reviewers for files that still don't have an owner as a
reviewer.
\* Allow adding OWNERS to the reviewer list.
LUCI:
\* Shows the blame-list, and the list of failures for a particular bot.
\* Links to the changed files from for a particular run in a buildbot.

**[Flake linker](https://chrome.google.com/webstore/detail/flake-linker/boamnmbgmfnobomddmenbaicodgglkhc)**

*   Links to Chromium flakiness dashboard from build result pages, so
            you can see all failures for a single test across the fleet.
*   Automatically hides green build steps so you can see the failure
            immediately.
*   Turns build log links into deep links directly to the failure line
            in the log.

**[Chromium Buildbot
Monitor](https://chrome.google.com/webstore/detail/chromium-buildbot-monitor/oafaagfgbdpldilgjjfjocjglfbolmac)**

Displays your CL's status of the Chromium buildbot in the toolbar. Click to see
more detailed status in a popup.
As the old waterfall is no longer used, update the waterfall link:
https://cros-goldeneye.corp.google.com/chromeos/legoland/builderSummary?builderGroups=cq&limit=2&buildBranch=master

**[Highlight Me in
Buildbot](https://chrome.google.com/webstore/detail/fkkgaddlippjidimcfbpllllclgnikdb)**

Highlight your commits on buildbot pages.

**[Version number helper for issue tracker](https://chrome.google.com/extensions/detail/hlibpmhacdkjajhplieepfmpagfaepem)**
Makes mouse-over-ing a version number in the chromium bug tracker show you what
release it is.

**[Source Quicklinks](https://chrome.google.com/extensions/detail/ncjnjlfeffaclcioiphpaofhkebnmknj)**
Adds a page action to quickly jump between [Code
Search](http://www.google.com/codesearch?vert=chromium), Trac, ViewVC, and
Gitweb.

**[Hide chromium code search banner](https://chrome.google.com/webstore/detail/hide-chromium-code-search/podmafjjpjkcjldlhcigjmelmdpignni)**
Saves wasted screen space by removing banner on code search.

[Chromium CodeSearch
Theme](https://chrome.google.com/webstore/detail/chromium-codesearch-theme/bncjaoffkbldggnfkakmnbgfpnikielm)

Displays the [Code Search](http://code.google.com/p/chromium/codesearch) pages
in customized themes and colored syntax highlighting. Contribute with new themes
via [github](https://github.com/kristijanburnik/codesearch-theme).

**[Colorize raw diffs on codereview.chromium.org](http://code.google.com/p/codereview-color-diffs/)**
Adds git diff-style colors to "raw patch sets" on the code review site.

**[Googler-specific extensions](http://goto.google.com/kjnqp)**
Note: must be on google network to load this link.

[**Reload build.chromium.org at a given
interval**](https://gist.github.com/danbeam/99e7c5d4e8182ff3762e)
Append ?reload=## to make build.chromium.org automatically reload itself every
so often.

**[Higher contrast buildbot
colors](https://gist.github.com/danbeam/ffb719d5c2922bdeffc4)**
Makes build.chromium.org easier to read for colorblind people (more contrast).

[**Crbug.com attachment
reminder**](https://gist.github.com/danbeam/55c45021e1ce268edfaf)
Prevents those embarrassing "screenshot attached" comments on bugs without
actually attaching something.

[**Crbug-Tree**](https://chrome.google.com/webstore/detail/crbug-tree/cjcdnhafnkeikpigldhhclhjnemlfehe)
Show a tree of dependent bugs in crbug.com.

**[Rietveld Usability
Toolkit](https://chrome.google.com/webstore/detail/rietveld-usability-toolki/nmljjlfbnbekmadhbpfpkcminoejelga)**
Various Rietveld tweaks (inline diffs, syntax highlighting, etc).

**[OmniChromium](https://chrome.google.com/webstore/detail/omnichromium/ainboabogofdlcfeciphjjmmicblabml?hl=en)**
Adds Chromium search+autocomplete tools to the Omnibox: codesearch, commits by
author, bugs, revisions.

**[Recent CrBugs](https://chrome.google.com/webstore/detail/recent-crbugs/bhddcpbcpjgehcdanidfppngfcjnpodi/related)**
Quickly access crbugs you have recently viewed.

[**TryBot
Re-Runner**](https://chrome.google.com/webstore/detail/trybot-re-runner/hidamppghiegbmolihgabohmibkmkgpn?hl=en-US)
Easily select failed trybots for re-running.

[**Open My
Editor**](https://chrome.google.com/webstore/detail/ome/ddmghiaepldohkneojcfejekplkakgjg)
OME gives you a context menu for opening files in your editor on Chromium Code
Search, Chromium Code Review, Chromium Build and Gerrit Code Review.

**[Stijl](https://chrome.google.com/webstore/detail/stijl/cpbiadoobgnpcacjecphfeoonpfccagk)**
Dashboard showing all your code reviews at Gerrit/Rietveld in a single page.
Also there is [a Googler-only version](http://go/stijl) with some internal
presets.

[**Gerrit
Monitor**](https://chrome.google.com/webstore/detail/gerrit-monitor/leakcdjcdifiihdgalplgkghidmfafoh)
Monitor multiple Gerrit instances for CLs requiring attention.

#### **Obsolete**

==~~**[UTC to 12h local time converter](https://gist.github.com/danbeam/545b33c9ff3140c3678b)**~~==
==~~Convert UTC times to local (12h) times on chromium-status.appspot.com (the
tree status).~~==

~~**[Autocomplete for bugs.webkit.org](https://chrome.google.com/extensions/detail/olaabhcgdogcbcoiolomlcodkngnemfb)**~~
Adds autocomplete to email boxes on bugs.webkit.org.~~ Now
[built-in](https://lists.webkit.org/pipermail/webkit-dev/2010-September/014361.html)
to bugs.webkit.org.

~~**[WebKit Buildbot monitor](https://chrome.google.com/extensions/detail/dfomjpbnljkkohdofbhnphphdkaojklg)**~~
~~Displays the status of the WebKit buildbot in the toolbar. Click to see more
detailed status in a popup.~~ Not useful after Blink fork.

~~**[Google Code enhancements for Chromium](http://code.google.com/p/chromium/downloads/detail?name=codesite.crx&can=2&q=)**~~
~~Inlined images and fix svn path in issues.~~ svn path is now handled by
crrev.com

~~[**Chromium CodeSearch**](https://chrome.google.com/webstore/detail/chromium-codesearch/igneackgkaaadndcfnacohbbmibnjkli)~~
~~Searches for selected text in Chromium [Code
Search](http://code.google.com/p/chromium/codesearch) ([github
code](https://github.com/miguelao/chromium_codesearch_extension)).~~

==~~**[BuildBot Error](https://chrome.google.com/extensions/detail/iehocdgbbocmkdidlbnnfbmbinnahbae)**~~==
==~~Helps find the error messages amidst all the stdio output on
build.chromium.org bots. Highly recommended for any committer.~~==

## Chromium OS

[**Wicked Good
Unarchiver**](https://chrome.google.com/webstore/detail/mljpablpddhocfbnokacjggdbmafjnon)
Open standard \*nix archive formats (e.g. tar.gz) using the Files app.

**[Tree
Status](https://chrome.google.com/webstore/detail/chrome-os-tree-status/ebdinlbmpcdebianielhijhbalinlahg)**
Displays the openness / closedness of the Chrome OS build tree.

~~[**Iteration Viewer**](https://chrome.google.com/webstore/detail/chromium-os-iteration-vie/gnpbpbbdcikepgimaaflpdobempiegcp)~~
~~Shows the current Chromium OS Iteration when looking at the issue tracker.~~
