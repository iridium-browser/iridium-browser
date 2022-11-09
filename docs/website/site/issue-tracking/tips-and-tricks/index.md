---
breadcrumbs:
- - /issue-tracking
  - Issue Tracking
page_name: tips-and-tricks
title: Tips and Tricks
---

[TOC]

## Tip 1: It's important to learn your query operators ":" and "="

Components and labels can both be searched for using either ":" or "="
operators. The "=" operator looks for exact matches. The ":" operator for
components looks for the specified component or any subcomponent. The ":" for
labels (and most of fields) looks for words anywhere in the string.

Mastering these queries will allow you to find all issues under a parent
component.

Example 1.1: Querying all Blink issues
([component:Blink](https://bugs.chromium.org/p/chromium/issues/list?can=2&q=component%3ABlink)),
from Blink to Blink&gt;WebRTC&gt;Audio etc...

Example 1.1: Querying only issues explicitly w/ the Blink label
([component=Blink](https://bugs.chromium.org/p/chromium/issues/list?can=2&q=component%3DBlink))

## Tip 2: Hit the "?" key to see a list of keyboard shortcuts.

Spoiler alert:

Issue list: J and K for previous and next issue, X to check a checkbox.

Issue detail page: J and K for previous and next issue, U to go up to list, R to select the comment field on the issue detail page.

Anywhere: / to select the search box.

## Tip 3: To get a clean link to an issue, click on the issue number in the header.

That gives you a simple URL that you can share with others.

## Tip 4: Add monorail issue search as a Search Engine in the Chrome Omnibar.

Just pop up a menu on your chrome browsers location bar and choose "Edit Search Engines...". E.g., I have the shortcut "mm" search for issues in /p/monorail. Typing "mm 12345" takes me directly to that issue. You can also treat the issue entry page as a search engine, e.g., I have "mnew" mapped to "https://bugs.chromium.org/p/monorail/issues/entry?summary=%s"
