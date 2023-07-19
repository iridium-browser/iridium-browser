---
breadcrumbs:
- - /developers
  - For Developers
page_name: code-browsing-in-chromium
title: Code Browsing in Chromium
---

**You're new, and Chromium is a big code base. You’ll spend a lot of time searching around Chromium to discover for yourself how the code works, so it makes sense to spend a bit of time investing in a good code browsing solution. Fortunately, there are lots of options.**
**Code search: The source code is indexed and available on the web, complete with hyperlinks to jump to other symbols. If you have one tab open for each search, then you can easily back up.** <https://code.google.com/p/chromium/codesearch>
**SlickEdit: This editor works on Linux, Mac, and Windows, and has code searching built in. One drawback is that you may have to spend 20 minutes or so rebuilding your SlickEdit project whenever you sync.**
**Xcode: The code browsing in Xcode 4.x has a hard time keeping up with the size of the codebase - after a sync, just opening Xcode 4.x with browsing turned on can take more than an hour. We recommend turning symbol indexing off if you use Xcode. That being said, it is a nice browsing environment if you are willing to pay the startup cost, careful to never shut down Xcode, and don't sync often.**
**Visual Studio and Intellisense: Also nice, also takes a long time.**
**Emacs (or VI) and CScope: Emacs plugins can run the cscope indexing tool, and it runs very fast (a minute or two after a fresh sync). the xcscope.el extension will run it for you automatically when you search. Check the cscope page for instructions to integrate with VI. <http://cscope.sourceforge.net/>. This works on Windows, Mac, and Linux. [Example Linux setup](/developers/how-tos/cscope-emacs-example-linux-setup).**
**Emacs (or VI) and tags: This still works.**
**Emacs and rgrep - a bit slower since there is no prebuilt index, but if you
can limit your search to a fraction of the source code tree, it is not too slow.
Even if you have to search all cpp and h files, it is not too bad, but other
methods are faster. There is an emacs extension called grep-a-lot.el which will
put the results of each grep into a separate buffer to make it easy to back up
when grepping around.**

**==Emacs and git-emacs==: The [git-emacs](https://github.com/tsgates/git-emacs/tree/master) package provides 'git-grep' and 'git-find' which are very fast and useful for quickly find keywords or files. It's not actively maintained so may need minor tweaks with recent versions of Emacs.**
**Plain grep from the command line: This is still available for those who may perfer it and have learned the syntax.**
**Fancier grep: you can also use “git gs &lt;search term here&gt;” to limit the search to files in the git repository, or “git ls-files | xargs grep -H &lt;search term here&gt;”. These are a lot faster than just letting grep move its way through the whole directory structure. "gclient grep" conceptually runs "git gs" on all subrepositories.**
**Source Insight - PC only - SI has great browsing support, and the helpful references window that you can configure to see where a function gets called from (among other things). It is important to learn how to customize the .tom file (c.tom) to get the best out of Source Insight.**
**Sublime Text - Sublime has a built in search feature that works pretty well although slower than Code Search (but sometimes more reliable). Version 3 also has a built in "Goto Definition" feature that opens a list of possible answers which is useful but not very precise (it seems text based, not real x-refs). Indexing is somewhat resource intensive, made continuously in the background but mind that it's limited to currently opened filed and folders included in the current project (so you must "Add folder to Project" for it to work properly).**
