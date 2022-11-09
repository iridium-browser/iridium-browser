---
breadcrumbs:
- - /developers
  - For Developers
page_name: tree-sheriffs
title: Tree Sheriffs
---

#### For Chromium OS Sheriffing see [Sheriff FAQ: Chromium OS](/developers/tree-sheriffs/sheriff-details-chromium-os)

## Contacting Sheriffs

The currently oncall sheriffs can be viewed in the top-left corner of the [Chromium Main Console](https://ci.chromium.org/p/chromium/g/main/console). You can also get in touch with sheriffs using the [#sheriffing Slack channel](https://chromium.slack.com/messages/CGJ5WKRUH/).

### What is a sheriff?

A sheriff's responsibility is to keep the Chromium main and branches green (tests are passing on all platforms) and open (changes can land). If a failure occurs, sheriffs will communicate, track down the cause of the breakage and the people responsible as possible. Chrome sheriffs are staffed by Chrome committers. For information on managing Chrome Sheriff rotations and How-Tos, see [Chrome Sheriffing](http://goto.google.com/chrome-sheriffing).

**First time sheriff?** Learn more about our Sheriffing Philosophy, please read [How to be a Sheriff](https://goto.google.com/chrome-sheriffing-how-to).

### Available Sheriff Rotations

*   [Chromium Trunk
            Sheriffs](https://goto.google.com/chrome-trunk-sheriffing)
*   [Chrome Branch
            Sheriffs](https://goto.google.com/chrome-branch-sheriffing)
*   [Chrome on Android Build
            Sheriffs](https://goto.google.com/chrome-android-sheriffing)
*   [Chrome on iOS
            Sheriffs](http://goto.google.com/chrome-ios-sheriffing)
*   [GPU Pixel Wranglers](http://goto.google.com/gpu-pixel-wrangler)
*   [Performance Regression
            Sheriffs](http://goto.google.com/chrome-perf-regression-sheriffing)
*   [Perfbot Health
            Sheriffs](http://goto.google.com/perf-bot-health-sheriffs)
*   [Security
            Sheriffs](http://goto.google.com/chrome-security-sheriff-doc)

### **Chrome OS Sheriffs**

*   ## [Chrome OS Sheriffs](http://goto.google.com/cros-sheriffing)

### Joining a sheriff rotation

The Chrome team runs a variety of sheriff rotations, and the process for joining them varies:

*   To join, leave, or request other membership modifications to a sheriff rotation, file a bug [here](https://bugs.chromium.org/p/chromium/issues/entry?summary=Sheriff+rotation%3A+%24ADD_OR_REMOVE_USER&comment=What+modifications+to+a+sheriff+rotation+would+you+like+made%3F%0D%0A%0D%0AWhich+sheriff+rotation%3F&labels=Restrict-View-Google%2CPri-2%2CSheriff-Rotation-Chromium%2CLT-Trooper&cc=benhenry%40chromium.org%2Cefoo%40chromium.org&status=Untriaged&components=Infra%3ESheriffing%3ERotations).
*   Specify the Sheriff rotation you would like to join in thedescription of the bug filed.

Some rotations might be timezone specific (Branch rotation is only monitored by PT sheriffs) while others (i.e. trunk sheriff rotation) are spread across multiple timezones across the world. You should be joining a rotation with an existing shifts during your local timezone.

Once you join a sheriff rotation, get started by reading [How to be a Sheriff](https://goto.google.com/chrome-sheriffing-how-to).
