---
breadcrumbs:
- - /developers
  - For Developers
page_name: about-signin-internals
title: about-signin-internals
---

**## Overview**

**The “chrome://signin-internals” webpage is a special URL in chromium that displays a summary of all signin and authentication related information in Google Chrome. It is useful in the triage and pinpointing of errors that could potentially be authentication related.**

**### Summary**

**#### Information Displayed**

    **Chrome Version - The version, branch and channel of chrome**

    **Signin Status - Whether the user is signed in or not**

    **User Id - The id for the currently signed in user or none if not signed
    in.**

    **Sid/Lsid - The hash of the SID/LSID for this session.**

**### Last Signin Details**

**#### Information Displayed**

    **Type - Is this a credential or a cookie jar/OAuth based login**

    **Time - The time stamp of the most recent login**

    **Last OnClientLogin Status/Time - The status and timestamp of the last
    credential based login attempt. ‘Successful’ if everything went right, an
    error message if something went wrong.**

    **Last OnOAuthLogin Status/Time - The status and timestamp of the last OAuth
    based login attempt. ‘Successful’ if everything went right, an error message
    if something went wrong.**

    **Last OnGetUserInfo Status/Time - The status and timestamp of the last
    attempt to fetch information about the currently authenticated user on a
    successful login. The user info is fetched immediately after a Client or
    OAuth login, so the timestamp should be close to one of the preceding
    values.**

**#### Room for improvement**

    **Add ChromeOS specific information (Retail login, etc).**

**### Token Details**

**For each service, displays the hash of the current token (or 0 if we don’t have a token), the time stamp of the last token fetch attempt, and the status of the last token fetch attempt. Currently the following services are tracked.**

**#### Services managed by TokenService**

*   **chromiumsync**
*   **gaia**
*   **lso**
*   **mobilesync**
*   **oauth2LoginAccessToken**
*   **oauth2LoginRefreshToken**

**#### Services using OAuth2AccessTokenConsumer**

*   **AppNotifyChannelSetup**
*   **ChromeToMobile**
*   **OperationsBase**
*   **ProfileDownloader**
*   **UserCloudPolicyManager**

**#### Room for improvement**

*   **Once OAuth2AccessTokenConsumers are migrated to use
            OAuth2TokenService, centralize signin-internals notifications from
            those services similar to how TokenService behaves now.**
