---
breadcrumbs:
- - /updates
  - updates
page_name: first-party-sets
title: First-Party Sets
---

## First-Party Sets (aka FPS)

## Motivation

Borrowing from the [First-Party Sets explainer](https://github.com/WICG/first-party-sets): In defining this scope (of ‘first-party’ in privacy models), we must balance two goals: the scope should be small enough to meet the user's privacy expectations, yet large enough to provide the user's desired functionality on the site they are interacting with.

----
**The below instructions are outdated; please see [here](https://developer.chrome.com/blog/first-party-sets-testing-instructions/) for updated testing instructions.**

<details>
<summary>Old instructions</summary
## End-to-End Testing

These instructions describe how a web developer can perform end-to-end testing of their sites in Chromium, while forcing Chromium to treat those sites as members of a First-Party Set, without needing to publicly establish a First-Party Set in Chromium's distribution list.

Note: these instructions will only work with a Chromium instance M89 or above.

1. Navigate to chrome://flags/#use-first-party-set.
1. Enable the First-Party Set flag. For the flag value, enter a comma-separated list of domains. (Note that all domains must be HTTPS.) E.g.: `https://fps-owner.example,https://fps-member1.example,https://fps-member2.example`. This flag can also be enabled by appending e.g. `--use-first-party-set="https://fps-owner.example,https://fps-member1.example,https://fps-member2.example"` to Chromium's command-line.
1. \[Optional\]: Navigate to chrome://flags/#sameparty-cookies-considered-first-party, and enable the flag. This flag changes the behavior of the "block third-party cookies" setting, such that `SameParty` cookies are not blocked. (Available in Chromium M93 and later.) This flag can also be enabled by appending `--sameparty-cookies-considered-first-party` to Chromium's command-line.
1. Restart Chromium by clicking the "Relaunch" button in the bottom-right corner, or by navigating to chrome://restart.
1. Perform end-to-end testing of the domains that were used in step 2. These sites will now have access to their `SameParty` cookies in same-party contexts.
1. When ready to revert to "standard" behavior, navigate to chrome://flags, disable the flags that you enabled in previous steps, and restart Chromium.

## Origin Trial

First-Party Sets will begin an origin trial in M89.

The goals of the origin trial are to:

* Test FPS functionality within a limited prototype, including the `SameParty` attribute
* Build awareness/interest in the FPS feature.
* Receive set membership requests and determine if FPS policy needs to be adjusted in order to meet privacy norms.
* Determine if FPS functionality meets needs of common-owned, separate-domain entities (during or post-OT)
* Test FPS UI options for user visibility and usefulness

## Origin Trial Policy

In order to apply a structured approach to examining user understanding of FPS relationships, the following policy constraints will apply to the First-Party Sets origin trial:

* First-Party Sets during OT will be limited to five registrable domains (plus any TLD variants of those five). This allows for viable testing of FPS functionality, and a small set size to initially gauge user understanding of the feature.
* An individual domain may only be included in a single Set.
* First-Party Sets during OT will only be available to common-owned, common-controlled domains. Common ownership and control has been proposed to aid in user understanding of FPS relationships.
* First-Party Set requests during OT will specify the location of the privacy policy or privacy policies of their proposed set members, and will list any differences in those policies.

## Origin Trial Functionality

The FPS origin trial will be “cosmetic” in that it will not change data sharing capabilities for FPS member domains; therefore, UX treatments will not be required for initial OT. In addition, when the user has third-party cookie blocking enabled, Chrome's normal functionality will persist and cookies will not be shared in cross-domain contexts - even if the domains are part of the First-Party Set OT.

In parallel with the origin trial, we will be conducting user research to better understand user expectations with respect to First-Party Sets. It is expected that this will allow testing of browser user interface options to make First-Party Sets discoverable and transparent for users.

## Joining the Origin Trial

If you are interested in participating in the [origin trial](https://github.com/GoogleChrome/OriginTrials/blob/gh-pages/developer-guide.md) for First-Party Sets and `SameParty`, please follow the below instructions:

1. Identify the members and owner of your organization's First-Party Set.
1. Identify which of your site(s)'s cookies could benefit from having the `SameParty` attribute set. Modify your site(s) to begin setting the `SameParty` attribute on the appropriate cookies. (Note that this attribute will be ignored by user agents that have not implemented the `SameParty` attribute; plan accordingly, using the SameSite attribute to specify a fallback policy.)
1. Modify your site(s) to collect appropriate metrics, for you to determine whether the origin trial is a success. E.g., record the contexts in which the `SameParty` cookies get set and sent, and compare the metrics to what you had expected.
1. Modify each of your sites to serve &lt;site&gt;/.well-known/first-party-set. These files will be used at registration-time to verify opt-in on each site.
    * &lt;owner site&gt;/.well-known/first-party-set should be a JSON file whose content is an object listing the owner and the members. E.g., `{owner: "https://fps-owner.example", members: ["https://fps-member1.example", "https://fps-member2.example"]}`.
    * &lt;member site&gt;/.well-known/first-party-set should be a JSON file whose content is an object listing the owner. E.g., `{owner: "https://fps-owner.example"}`.
    * This is similar to the .well-known machinery described in the First-Party Sets explainer, but does not include assertions or versioning.
1. Follow the standard origin trial signup process for the experiment here: https://developers.chrome.com/origintrials/#/trials/active.
    * You only need to register a single domain - the "owner domain" of the set.
    * We will use origin trial registrations for feedback and followup as needed (e.g. if performance issues or other unintended consequences arise during the experiment, we may end the experiment and notify participants). We will also use registrations to communicate what percentage of Chrome users will have the origin trial active; this will be important for interpreting your metrics.
    * After signup, you will see origin trial tokens on the registration page, but they do not need to be deployed to your site. The tokens will have no effect on the enabling of the FPS functionality.
    * In case you are already familiar with origin trials, we will not be using the standard origin-trial meta tag and Origin-Trial HTTP header.
1. Submit your proposed set by creating a bug using [this bug template](https://bugs.chromium.org/p/chromium/issues/entry?template=Defect+report+from+user&summary=%5BFormation%5D%2F%5BDissolution%5D+of+First-Party+Set+membership+in+Origin+Trial&comment=The+purpose+of+this+template+is+to+request+the+formation%2C+or+dissolution%2C+of+a+First-Party+Set.+For+more+details+about+First-Party+Sets%2C+please+see+the+explainer%3A+https%3A%2F%2Fgithub.com%2Fprivacycg%2Ffirst-party-sets%0A%0APlease+fill+in+the+following+items+below+and+your+request+will+be+publicly+visible+and+reviewed+by+the+Chrome+team.+Any+follow-up+questions+will+be+added+to+this+bug.+Once+all+details+are+confirmed+as+adhering+to+the+requirements+for+this+Origin+Trial%2C+your+First-Party+Set+will+be+included+in+a+component+list+of+Chrome.%0A%0A1%29+Webmaster+contact%28s%29+of+domains+in+set%2C+if+different+from+the+user+filing+this+bug%3A%0A+-+%5Bemail+address%5D%0A%0A2a%29+Please+list+the+registrable+domains+%28see+https%3A%2F%2Fpublicsuffix.org%2F+for+definition+of+registrable+domain%29+that+you+would+like+to+be+in+the+First-Party+Set%2C+with+all+desired+TLD+variants.+The+first+listed+domain+should+be+the+%E2%80%9Cowner%E2%80%9D+domain+of+the+set.+Note%3A+For+the+Origin+Trial%2C+we+will+be+using+a+limit+of+5+registrable+domains+%28not+including+ccTLD+variants%29.%0A%5Bowner.example%5D%0A%5Bowner.test%5D%0A%5Bbrand.example%5D%0A%5Bbrand.test%5D%0A%5Bcobrand.example%5D%0A%5Bcobrand.test%5D+...%0A%0A2b%29+%5BOptional%5D+Our+initial+origin+trial+is+limited+to+5+registrable+domains.+If+you+are+interested+in+your+set+containing+additional+domains+beyond+the+5+you+have+listed+above+in+%282a%29%2C+please+feel+free+to+list+them+here%3A%0A%0A%0A3%29+Please+confirm+that+the+domains+listed+above+have+the+same+common+owner+and+controller.+Optional%3A+you+may+add+additional+detail+here+to+help+in+the+verification+of+this.%0A-+%5BYes%2FNo%5D%0A-+%5BOptional+detail%5D+%0A%0A4%29+Please+list+the+URL+of+the+Privacy+Policies+for+each+domain.+If+there+are+known+differences+between+the+privacy+policies%2C+please+summarize+those+differences+as+well.%0A+-+%5BURL%5D+%0A+-+%5BURL%5D++%0A-+%5BSummary+of+differences+in+privacy+policies%5D%0A%0A5%29+Have+you+hosted+%60.well-known%2Ffirst-party-set%60+files+on+all+domains+as+described+in+the+instructions%3F&components=Internals%3ENetwork%3EFirst-Party-Sets&status=Assigned&owner=chrome-first-party-sets%40chromium.org&labels=allpublic,Type-Bug,Pri-2).
    * Bugs (and any resulting comments/questions) will be publicly visible once submitted.
    * Note that for the origin trial, we are not using the Sec-First-Party-Set machinery. Creating the above bug and serving .well-known/first-party-set files are the only steps necessary for declaring your First-Party Set.
1. After the above bug is marked "Fixed", monitor key metrics to ensure no unexpected breakage occurs on your site(s) during the duration of the trial.

</details>

---

If you have any questions, please reach out to chrome-first-party-sets@chromium.org.

## Update History
* November 23, 2020: created First-Party Sets page on chromium.org
* January 5, 2021: expanded details about First-Party Sets and added origin trial information
* February 2, 2021: added information on end-to-end testing. Expanded signup-instructions for origin trial
* March 17, 2021: added instructions for serving .well-known/first-party-set file.
* December 1, 2022: added link to updated testing instructions.

## Resources
* [First-Party Sets explainer](https://github.com/privacycg/first-party-sets)
* [First-Party Sets Intent-to-Prototype](https://groups.google.com/a/chromium.org/g/blink-dev/c/0EMGi-xbI-8/m/d_UxAJeiBwAJ)
* [First-Party Sets on ChromeStatus](https://chromestatus.com/feature/5640066519007232)
* [First-Party Sets Ready-for-Trial](https://groups.google.com/u/1/a/chromium.org/g/blink-dev/c/-_kPNC3tF2s)
* [`SameParty` explainer](https://github.com/cfredric/sameparty)
* [`SameParty` Intent-to-Prototype](https://groups.google.com/u/1/a/chromium.org/g/blink-dev/c/-unZxHbw8Pc)
* [`SameParty` on ChromeStatus](https://chromestatus.com/feature/5280634094223360)
