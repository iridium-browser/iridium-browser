---
breadcrumbs:
- - /for-testers
  - For Testers
page_name: bug-reporting-guidlines-for-the-mac-linux-builds
title: Bug Reporting Guidlines for the Mac & Linux builds
---

**IMPORTANT:**

In order for the bug reports we receive to be most effective, we need to be able
to go through them in a timely manner. That means keeping the number manageable.
The primary enemy of this effort is duplicate bugs which waste everyone's time
and clog up the system. Unlike some other public bug systems, we do not consider
the number of duplicates as an indicator of priority, demand, or seriousness. As
a result, there is little to be gained from filing a duplicate bug besides
slowing down our ability to produce a great piece of software.

Before you file any bug, search the bug tracker to see if your problem is a
known issues ([Mac
query](http://code.google.com/p/chromium/issues/list?can=2&q=os:mac&sort=-stars),
[Linux
query](http://code.google.com/p/chromium/issues/list?can=2&q=os:linux&sort=-stars)).
You can safely assume that huge holes in functionality (such as fullscreen not
working) are well known and there's no need to file a bug. If you find the bug
is already there, feel free to star it so you can find it later and indicate to
us another person is interested in its resolution. If and only if you can't find
the bug in the database should you consider filing a new one.

If you have any questions about whether or not you should file your bug, feel
free to visit #chromium on irc.freenode.net and ask the community. We'll be more
than willing to help you out.

**Steps to take before filing a bug:**

1. Try searching for the bug on the [Chromium bug tracker](http://crbug.com),
someone may already have reported it.

2. Try to formulate a reduction, i.e. the minimal set of steps to reproduce the
problem.

**Example of a good bug report:**

Chrome Version : 2.180.0.0 URLs : http://www.someurl.com Other browsers tested:
Safari 4: OK Firefox 3.x: OK IE 7: OK IE 8: OK

Chromium : FAIL What steps will reproduce the problem? 1. Launch Chrome 2. Load
http://www.someurl.com 3. Click on the picture of the elephant in the top right
corner.

4. All buttons in the Chrome UI turn pink. What is the expected result?

Color of Chrome UI shouldn't change. What happens instead?

My UI Turns Pink. Please provide any additional information below. Attach a
screenshot if possible.

System Configuration: Mac OS X 10.5.6 on a PowerMac with 4 gigs of RAM; I use
the Swedish-language UI.

Console Output:

5/22/09 2:13:02 PM \[0x0-0x12c92c8\].com.google.Chrome\[67080\] LEAK: 27
CachedResource

5/22/09 2:13:02 PM \[0x0-0x12c92c8\].com.google.Chrome\[67080\] LEAK: 287
WebCoreNode

5/22/09 2:13:02 PM \[0x0-0x12c92c8\].com.google.Chrome\[67080\] Leak 1 JS
wrappers.

5/22/09 2:13:03 PM \[0x0-0x12c92c8\].com.google.Chrome\[67080\]
\[67080:19971:1315528366186335:ERROR:
/Users/Shared/foo/chrome/src/chrome/src/chrome/plugin/plugin_thread.cc(166)\]
Not implemented reached in bool

Screenshot attached.

**Walkthrough - What's good about the above bug report:**

1. It contains a clear detailed description of the steps needed to reproduce the
bug from scratch, including URLs where appropriate. Be as specific as you can.
Saying to "click the link in the page" when there are fifty links is unhelpful.
When filing bugs about non-English sites, don't assume the developer understands
the language of the webpage.

2. Results in other browsers are noted.

3. It explains what you saw happen and what you where expecting to see.

4. Console output and system configuration are included, Console output in
particular can be extremely helpful diagnosing an issue! (on OS X you can see
the console output by opening /Applications/Utilities/Console.app).

5. A screenshot is attached showing the bug.

**Other Important stuff:**

On OSX, If you've opted into stats reporting Crash dumps will be saved in
`~/Library/Application Support/Chromium/Crash Reports/` please attach the files
in this directory when filing reports for crashing bugs they can really help us
in tracking down issues.

I can't write code, how can I help?:

One of the best ways people can contribute to an open source project is not by
writing code but by helping manage bugs in the bug database. There are many
activities that help immensely:

*   Finding and marking duplicate bugs that have already been filed.
*   Providing reduced test-cases (HTML, JavaScript, etc) for web site
            bugs on complex pages
*   Confirming or denying that a bug has been fixed. (Though if a bug
            has previously been reported and not yet marked fixed, saying "STILL
            NOT FIXED" doesn't add anything except some irritation for the poor
            people CCed on the bug. If it had been fixed, the fixer probably
            would have marked it as such, no? Don't add useless comments.)
*   Confirming or denying that a bug is valid or has enough information
            to be reproducible.

You don't need a PhD in computer science to do any of these, and they make a
huge difference to both engineering and QAs ability to effectively triage bugs.
