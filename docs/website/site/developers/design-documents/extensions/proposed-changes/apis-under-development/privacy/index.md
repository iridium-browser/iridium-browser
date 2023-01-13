---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
- - /developers/design-documents/extensions
  - Extensions
- - /developers/design-documents/extensions/proposed-changes
  - Proposed & Proposing New Changes
- - /developers/design-documents/extensions/proposed-changes/apis-under-development
  - API Proposals (New APIs Start Here)
page_name: privacy
title: Privacy-relevant Preferences API
---

****Overview****

**Chromium includes a variety of features that provide valuable services to users either by making requests on their behalf (DNS Prefetching, for example), or by sending non-personal information to third-parties (Omnibox suggestions, for example). While we firmly believe that these services offer a huge net benefit, we also want to fully support users who choose not to take part. Users can opt-out (or opt-in, depending) to each, either via chrome://flags or chrome://settings; extensions should have programmatic access to these preferences to offer choices to users in forms that Chromium chooses not to.**
**Use cases**
**Every feature that’s listed in the[ privacy whitepaper](http://static.googleusercontent.com/external_content/untrusted_dlcp/www.google.com/en/us/intl/en/landing/chrome/google-chrome-privacy-whitepaper.pdf) is relevant to extensions that want to offer privacy protections for users above and beyond what is reasonable for Chromium to offer as a default to everyone. Extensions like Tor have expressed interest in programmatically disabling DNS prefetching (and prerendering, et al.), and it would be valuable for everyone concerned if respected advocates such as the EFF could build “EFF-approved“ default settings that users could choose to install via an extension.**
**Relatedly, but closer to home, reworking the whitepaper itself into an application from which users could directly toggle preferences without digging through preferences pages would be valuable.**
**Could this API be part of the web platform?**
**These preferences don’t relate to anything like the web platform’s storage options, but instead how Chromium interprets, requests, and processes content on the web. These options are clearly Chromium-specific, and not generally applicable to specific websites (nor would we want to offer any website the ability to, for example, toggle UMA settings).**
**Do you expect this API to be fairly stable?**
**These preferences map fairly directly to the services and features the browser offers, which means that the API would only shift in relation to changes in browser-functionality. As it is by necessity browser-specific, that seems reasonable in terms of stability.**
**What UI does this API expose?**
**None. The preferences are already exposed via chrome://settings and chrome://flags, this proposal simply adds a programmatic interface.**
**How could this API be abused?**
**Malicious extensions could send Omnibox keystrokes and other such data to a third-party server by tricking the user into changing her default search engine, and toggling the relevant preferences for Suggest. Likewise, they could force loading of arbitrary sites by doing the same, and enabling Instant.**
**How would you implement your desired features if this API didn't exist?**
**Individual users would be pointed to the various bits of UI where these settings exist (preferences and flags), and asked to toggle settings manually. This is, for example, what the Chrome privacy whitepaper does right now.**
**Are you willing and able to develop and maintain this API?**
**Yes.**
**Draft API spec**
**TODO.**
**This API could be provided in two ways (possibly together).**
**Option 1**
**Chromium already provides APIs related to subsets of the “Clear Browsing Data” form’s functionality. Adding methods to each of those seems like a reasonable way of addressing the request. We call pretty much everything cookies, so adding chrome.cookies.clear might look like the following:**
**{**
**"namespace": "cookies",**
**…**
**"functions": \[**
**…**
**{**
**"name": "clear",**
**"description": "Clears cookies and other site data modified within a time period.",**
**"type": "function",**
**"parameters": \[**
**{**
**"name": "timePeriod",**
**"type": "string",**
**"enum": \["last_hour", "last_day", "last_week", "last_month", "everything"\],**
**"optional": "false",**
**"description": "The time period inside which to delete cookies and site data."**
**},**
**{**
**"name": "callback",**
**"type": "function",**
**"description": "Called when deletion has completed.",**
**"optional": "true",**
**"parameters": \[**
**{**
**"name": "result",**
**"type": "boolean",**
**"description": "Was the data deletion successful?"**
**}**
**\]**
**}**
**\]**
**},**
**…**
**\],**
**…**
**}**
**chrome.cookies and chrome.history seem like excellent candidates for these sorts of methods. The history namespace could probably be overloaded to include methods to clear the browser cache, downloaded files, stored passwords, and Autofill data (since it’s all arguably historical in nature).**
**It would be valuable, however, to distinguish the namespace from the permission required to execute the methods. Extensions focused on removing data (see the use-cases above) shouldn’t be required to request read access to cookies on a variety of hosts in order to clear them. Adding an explicit removeBrowsingData permission that would grant access to the relevant methods across namespaces would be valuable (cookies should grant the same access, but only to the cookies-specific chrome.cookies.clear).**
**Option 2**
**A drawback to the option above is that clearing all browsing data requires many (expensive) executions of BrowsingDataRemover::Remove. Providing a single method that encapsulated all the options in a single call would be much more efficient. That might look like the following:**
**{**
**"namespace": "experimental.browsingData",**
**"functions": \[**
**{**
**"name": "clear",**
**"description": "Clears data generated by browsing within a particular timeframe.",**
**"type": "function",**
**"parameters": \[**
**{**
**"name": "timePeriod",**
**"type": "string",**
**"enum": \["last_hour", "last_day", "last_week", "last_month", "everything"\],**
**"optional": "false",**
**"description": "The timeframe inside of which to delete browsing data.”**
**},**
**{**
**"name": "dataToRemove",**
**"type": "object",**
**"optional": "false",**
**"properties": {**
**"cache": {**
**"type": "boolean",**
**"optional": true,**
**"description": "Should the browser cache be cleared?"**
**},**
**"cookies": {**
**"type": "boolean",**
**"optional": true,**
**"description": "Should the browser's cookies be cleared?"**
**},**
**"downloads": {**
**"type": "boolean",**
**"optional": true,**
**"description": "Should the browser's download list be cleared?"**
**},**
**"form_data": {**
**"type": "boolean",**
**"optional": true,**
**"description": "Should stored form data be cleared?"**
**},**
**"history": {**
**"type": "boolean",**
**"optional": true,**
**"description": "Should the browser's history be cleared?"**
**},**
**"lso_data": {**
**"type": "boolean",**
**"optional": true,**
**"description": "Should LSO data be cleared?"**
**},**
**"passwords": {**
**"type": "boolean",**
**"optional": true,**
**"description": "Should the stored passwords be cleared?"**
**}**
**}**
**},**
**{**
**"name": "callback",**
**"type": "function",**
**"description": "Called when deletion has completed.",**
**"optional": "true",**
**"parameters": \[**
**{**
**"name": "result",**
**"type": "boolean",**
**"description": "Was the data deletion successful?"**
**}**
**\]**
**}**
**\]**
**}**
**\]**
**}**
**Open questions**

1.  **BrowsingDataRemover offers the option of clearing browsing
            history, download history, clearing the cache, deleting cookies +
            site + plug-in data, clearing saved passwords, and clearing Autofill
            data. Homes should be found for programmatic methods that clear each
            type of data.**
2.  **Should we offer a separate namespace for clearing browsing data?
            If so, are there other delete-only methods that would make sense to
            include in such a namespace?**
