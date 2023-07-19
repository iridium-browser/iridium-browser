---
breadcrumbs:
- - /developers
  - For Developers
page_name: triggered-reset-api
title: Chromium Triggered Reset API (Windows-only)
---

## **Chromium (and Google Chrome) on Windows provide a way for third parties to trigger a reset flow the next time a Chromium profile is opened by the user.**

## **The reset flow will prompt the user to reset their settings to the built-in default state of the browser when it was first installed on the system.**

## **This mechanism is intended to be used by third parties implementing clean up tools who wish to provide a way to prompt a user to clean their settings. If the user accepts the prompt, then their homepage, new tab page and search engine will be set back to factory defaults, all of their extensions will be disabled (though not removed) and all pinned tabs will be unpinned.**

## **The prompt the user receives will look like this:**

## **<img alt="image" src="https://lh3.googleusercontent.com/rtQtZI_dBoD6D6tP8PkDQBw5SCDpUCQoEbzD01nT_ekbpgfalNlnfugK2GcogB6NA3yfFFqGvQh9OI3e37uFcRpzfqNF6Fiae0WxNCoypPbySshE2STyT4wFjAQupMOyGIHlaU8p" height=415 width=619>**

## How to use it

## **Chromium on Windows exposes the triggered profile reset API through the registry, a place that is easy and convenient for a third party Windows tool to write to.**

## **To use the triggered reset flow, the third party tool will :**

    ## **Create (or open) the registry key
    HCKU\\Software\\Chromium\\TriggeredReset or
    HCKU\\Software\\Google\\Chrome\\TriggeredReset, depending which distribution
    you are targeting.**

    ## **Set a REG_SZ value called "ToolName" to the name of the tool. This
    string will be displayed in a notification UI. The "ToolName" should be just
    the name of the tool, e.g. "AwesomeAV".**

    ## **Set a REG_QWORD value called "Timestamp" with the timestamp of the
    reset. This value should be obtained from a call to
    ::GetSystemTimeAsFileTime(). The value will be persisted in a reset profile
    and will be used to avoid multiple resets. The value will not be cleared
    from the registry by Chromium.**

## **The above steps should be performed for the user profile hive (e.g. under HKCU) of all users on the system the tool wishes to reset.**

## **On [Profile](https://code.google.com/p/chromium/codesearch#chromium/src/chrome/browser/profiles/profile.h&l=70) creation, Chromium will inspect the timestamp and if different from the persisted value, it will notify the user and trigger a profile reset flow. This will be done for each profile that is created.**

## **To avoid resetting future new Profiles, when a Profile is first created any timestamp currently present in the registry will be copied in without a reset.**

## FAQs

### What about users with multiple Chromium/Chrome profiles?

## **The reset UI flow is per-profile, so a user with multiple Chromium/Chrome profiles will see multiple reset prompts.**

### What if a user creates a new profile after I set the appropriate registry values?

## Subsequently created profiles will not experience a reset flow. Only profiles that existed before the registry values are set will be prompted for reset.
