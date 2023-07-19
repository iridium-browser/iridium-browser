---
breadcrumbs:
- - /updates
  - updates
page_name: ua-ch
title: User-Agent Client-Hints
---

# **Overview**

This page provides a central reference point about the User-Agent Client Hints
(UA-CH) rollout for Chrome 85+ stable. If you’re not familiar with User-Agent
Client Hints, you can read up on them
[here](https://web.dev/user-agent-client-hints/).

# **Launch Timeline**

Last updated November 22, 2021

*   **March 3, 2021: UA-CH is rolling out to 100% of stable population
            in tandem with version 89**
*   **October 14, 2020:** We've paused increasing the population subset
            of the roll out while we examine ecosystem impact and investigate
            possible issues due to the Structured Headers format.
*   **October 5, 2020:** Gradual roll out continues to a larger subset
            of the population.
*   **September 9, 2020:** UA-CH will begin roll out to an initial small
            percentage of the Chrome 85 stable population.
*   **May, 2020:** UA-CH was rolled out to most of Chrome Canary, Dev
            and Beta populations.
*   **June 10, 2021**: Critical-CH and ACCEPT_CH features were disabled
            due to a potential crash while investigating and implementing fixes.
*   **November 22, 2021**: Intent to Ship: Sec-CH-UA-Full-Version-List
            user-agent client hint.

# **Possible Site Compatibility Issue**

UA-CH is an additive feature, which adds two new request headers that are sent
by default: \`sec-ch-ua\` and \`sec-ch-ua-mobile\`. Those request headers are
based off of [Structured Field
Values](https://httpwg.org/http-extensions/draft-ietf-httpbis-header-structure.html),
an emerging standard related to HTTP header values. They contain characters
that, though permitted in the HTTP specification, weren’t previously common in
request headers, such as double-quotes (“), equal signs (=), forward-slashes
(/), and question marks (?). Some Web-Application-Firewall (WAF) software, as
well as backend security measures, may
[mis-categorize](https://bugs.chromium.org/p/chromium/issues/detail?id=1091285)
those new characters as “suspicious”, and as such, block those requests.

This typically presents itself as a 4XX HTTP response code (e.g. 400, 404 or
422), either on any request or on POST requests or API endpoints (which servers
tend to take special precautions with).

# **How to file bugs**

Here are some basic troubleshooting steps:

*   Check `chrome://version` and make sure the version number is at
            least 85. The UA-CH feature is only enabled for Chrome 85+.
*   If the issue is primarily the browser or tab experiencing a crash or
            hang, it is not likely to be a UA-CH related issue.
*   Test again with the \``--disable-features=UserAgentClientHint`\`
            command line flag. You can use the [test
            site](https://user-agent-client-hints.glitch.me/) to verify that the
            feature is indeed off.
*   If the problem persists, the source of the issue is likely to be
            elsewhere.
*   In case the problem went away, that’s a strong indicator that it is
            a UA-CH related issue

If after trying these things, you have determined that it may be a UA-CH issue,
please [file a CR bug](https://crbug.com/new), including \[UA-CH\] in its title.
Add a comment about it to the following [tracking
bug](https://bugs.chromium.org/p/chromium/issues/detail?id=1091285).

# **(Deprecated) How to identify if a user is in the stable experiment**

**Note:** As of Chrome 89, this feature is rolled out to full population.

Go to `chrome://version?show-variations-cmd`. Search for “UserAgentClientHint”
in the long command string towards the bottom. If they are in the enabled group,
the `--force-fieldtrials` value will say something like one of the following:

*   /\*UserAgentClientHintRollout/Enabled/
*   /\*UserAgentClientHintRollout/Enabled_v2/
*   /\*UserAgentClientHintRollout/Enabled_v3/
*   /\*UserAgentClientHintRollout/Enabled_v4/
*   /\*UserAgentClientHintRollout/Enabled_v5/

If they do not have the features enabled, the string will be absent.

Alternatively, going to this [test
site](https://user-agent-client-hints.glitch.me/) can confirm the feature is
enabled - when the feature is enabled \`sec-ch-ua\` and \`sec-ch-ua-mobile\`
requests will be listed as being sent by the client.
