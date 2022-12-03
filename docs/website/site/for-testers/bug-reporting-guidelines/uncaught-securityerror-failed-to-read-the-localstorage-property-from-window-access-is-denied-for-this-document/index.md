---
breadcrumbs:
- - /for-testers
  - For Testers
- - /for-testers/bug-reporting-guidelines
  - Bug Life Cycle and Reporting Guidelines
page_name: uncaught-securityerror-failed-to-read-the-localstorage-property-from-window-access-is-denied-for-this-document
title: 'Uncaught SecurityError: Failed to read the ''localStorage'' property from
  ''Window'': Access is denied for this document.'
---

This exception is thrown when the "Block third-party cookies and site data"
checkbox is set in Content Settings.

To find the setting, open Chrome settings, type "third" in the search box, click
the Content Settings button, and view the fourth item under Cookies.

[<img alt="image"
src="/for-testers/bug-reporting-guidelines/uncaught-securityerror-failed-to-read-the-localstorage-property-from-window-access-is-denied-for-this-document/blockthirdpartycookies.png"
width=600>](/for-testers/bug-reporting-guidelines/uncaught-securityerror-failed-to-read-the-localstorage-property-from-window-access-is-denied-for-this-document/blockthirdpartycookies.png)

If this setting is checked, third-party scripts cookies are disallowed and
access to localStorage may result in thrown SecurityError exceptions.
