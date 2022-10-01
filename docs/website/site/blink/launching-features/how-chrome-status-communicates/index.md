---
breadcrumbs:
- - /blink
  - Blink (Rendering Engine)
- - /blink/launching-features
  - Launching Features
page_name: how-chrome-status-communicates
title: How Chrome Status Communicates with Web Developers
---

Chrome Status is the first level of communication with the web development
community about new web platform and related features. Its audience is not, as
many believe, exclusively Chromium engineers.

# Summary

Provide one or two complete sentences explaining the feature to web developers
followed by one or two sentences explaining either how it helps web developers
or how it improves the web platform. Do this even if it seems obvious to you.

*   Use complete sentences.
*   Exclude returns in the text. They will not be shown in some views.

Tips:

*   Omit needless words. This is from "[The Elements of
            Style](https://www.amazon.com/Elements-Style-Fourth-William-Strunk/dp/020530902X/ref=pd_bxgy_14_img_3?_encoding=UTF8&pd_rd_i=020530902X&pd_rd_r=de87e78e-8526-11e8-914f-9f4b0c666b45&pd_rd_w=mZsdS&pd_rd_wg=bq1yC&pf_rd_i=desktop-dp-sims&pf_rd_m=ATVPDKIKX0DER&pf_rd_p=3914568618330124508&pf_rd_r=KQZKKVTXAZHWAEFD7SR9&pf_rd_s=desktop-dp-sims&pf_rd_t=40701&psc=1&refRID=KQZKKVTXAZHWAEFD7SR9)".
            Most people have words in their writing that can be removed without
            affecting comprehension or meaning.
*   Do not take shortcuts at the cost of clarity. If you cut so many
            words the meaning is obscured, then you have gone too far in
            omitting needless words. This is also from "The Elements of Style."
*   If you absolutely, positively cannot describe your feature in the
            space required, continue the description in the comments box.

# Chromium Status

**Behind a flag**—Applies to one of two conditions.

*   The flag is listed in chrome://flags AND it is disabled by default.
*   The flag is listed in chrome://flags with a value of 'default' and
            is disabled by default somewhere in the compiled code.

Additionally:

*   Origin trials have their own labels.
*   This does NOT apply to Finch or test flags.

**Enabled by default**—Applies to one of three conditions.

*   There is no flag in chrome://flags because the feature is available
            to all users.
*   The flag is listed in chrome://flags AND it is enabled by default.
*   The flag is listed in chrome://flags with a value of 'default' and
            is enabled by default somewhere in the compiled code.
