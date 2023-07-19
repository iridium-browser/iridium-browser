---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/developer-guide
  - Chromium OS Developer Guide
page_name: gerrit-guide
title: Gerrit credentials setup (for Chromium OS and Chrome OS)
---

[TOC]

## Introduction

We have two gerrit instances: Chromium OS and the (internal) Chrome OS gerrit
instance.

For the Chromium OS instance: This is where most of the development happens. You
do not have to be an "official" Chromium contributor or Googler or anything else
to interact with the Chromium OS gerrit instance and/or upload changes to it. We
restrict access by only allowing certain people to *approve* changes before
they're allowed to go into the main tree.

For the internal Chrome OS instance: Access is restricted to Googlers and
partners with a '&lt;partner&gt;.corp-partner.google.com' email address.

The gerrit instance for Chromium OS and Chrome OS uses Google Accounts to
provide authentication. This means any account you can use to log into
google.com can also be used to authenticate with Gerrit.

### (EVERYONE) To get access to the Chromium Gerrit instance

Follow the steps in [Chromium's Gerrit Guide](/developers/gerrit-guide).

### (Googlers & Partners) To get access to the internal Chrome Gerrit instance

1.  (Googlers only) You must also do the steps above for your @chromium
            account first
2.  Go to <http://google.com/> and verify you are logged into your
            @google.com account
3.  In the same window and session, load
            <https://chrome-internal.googlesource.com/new-password>
    1.  **Make sure you are logged into your @google.com account.**
    2.  **You can verify this by ensuring that the Username field looks
                like git-&lt;user&gt;.google.com**
4.  Follow the directions on the new-password page to append to your
            .gitcookies file. You should click the radio button labeled "only
            chrome-internal.googlesource.com" if it exists.
5.  **Verification:**
    *   **Googlers -** Run `git ls-remote
                https://chrome-internal.googlesource.com/chromeos/manifest-internal.git`
    *   **Partners -** Find a repo in your local manifest and run `git
                ls-remote
                https://chrome-internal.googlesource.com/<REPO_PATH>.git -
                Example: git ls-remote
                https://chrome-internal.googlesource.com/chromeos/overlays/chromeos-partner-overlay.git`
    *   Note that This should **not** prompt for any credentials, and
                should just print out a list of git references.

### (Googler) Link @chromium.org & @google.com accounts

Follow the steps in [Chromium's Gerrit Guide](/developers/gerrit-guide). You
must link your accounts to have proper access in Gerrit.

## **Using Gerrit**

Check out the [Contributing
Guide](https://chromium.googlesource.com/chromiumos/docs/+/HEAD/contributing.md)
for using Gerrit and getting through the Chromium OS review process.

## More Gerrit Tips

Check out the random tips in [Chromium's Gerrit
Guide](/developers/gerrit-guide).
