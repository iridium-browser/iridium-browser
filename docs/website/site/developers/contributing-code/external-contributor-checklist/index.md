---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/contributing-code
  - Contributing Code
page_name: external-contributor-checklist
title: External Contributor Checklist
---

Before you LGTM a change from a non-chromium.org address, make sure we have the
right to accept the contribution:

*   Definition: The "author" is the email address that owns the code
            review request on <https://chromiumcodereview.appspot.com/>
*   Make sure the author is already listed in src/AUTHORS. In some
            cases, the author's company might have a wildcard rule in AUTHORS
            (eg, \*@google.com).
*   If the author or his/her company is not listed, the CL should
            include a new AUTHORS entry.
    *   If you don't work for Google, find someone who does to review
                the new entry.
    *   Googlers: To verify that someone has signed the CLA, search for
                Google CLA signers at <https://signcla.corp.google.com/>.
        *   If there's no entry, ask the person to sign the CLA:
                    <https://cla.developers.google.com/> (individual or
                    corporate, select 'any other Google project')
        *   If there's a corporate CLA for the author's company, it must
                    list the person explicitly (or the list of authorized
                    contributors must say something like "All employees"). If
                    the author is not on their company's roster, do not accept
                    the change.

If you have any questions or doubts, cc accounts@chromium.org for help. Also,
please see this for more information:
<https://chromium.googlesource.com/chromium/src/+/HEAD/docs/contributing.md>
