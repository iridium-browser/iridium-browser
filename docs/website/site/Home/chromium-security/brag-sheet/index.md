---
breadcrumbs:
- - /Home
  - Chromium
- - /Home/chromium-security
  - Chromium Security
page_name: brag-sheet
title: Security Brag Sheet
---

### Our Team and Resources

*   Our team includes some of the best security professionals in the
            business.
*   We work closely with top researchers like Michal Zalewski (lcamtuf)
            and Tavis Ormandy (taviso).
*   We contract with experts like iSec Partners and Chris Rohlf for
            targeted assessments.
*   We dedicate thousands of CPU cores to fuzz projects such as
            [WebKit](http://blog.chromium.org/2012/04/fuzzing-for-security.html),
            [Adobe
            Flash](http://googleonlinesecurity.blogspot.com/2011/08/fuzzing-at-scale.html)
            or [Chrome's PDF viewer](http://j00ru.vexillium.org/?p=1175).

**White Papers**

*   Chrome leads in [white papers from 2 different security
            firms](https://www.blog.google/products/chrome-enterprise/2-new-white-papers-examine-enterprise-web-browser-security/).
*   Chrome leads in [white paper from respected security firm
            Accuvant](http://www.accuvant.com/sites/default/files/AccuvantBrowserSecCompar_FINAL.pdf).
*   Chrome leads in response time and reward program effectiveness in
            [this independent study from
            Berkeley](https://www.usenix.org/system/files/conference/usenixsecurity13/sec13-paper_finifter.pdf).
*   Chrome leads in [recommendations from respected German government
            organization, the
            BSI](https://www.bsi-fuer-buerger.de/SharedDocs/Downloads/DE/BSIFB/Publikationen/BSI-E-CS_001.pdf).

### Containing Attacks

*   We have an [integrated sandbox](/Home/chromium-security/guts) that
            reduces the impact of most common vulnerabilities, and is much
            stronger than approaches used by other browsers.
*   We have [Site Isolation](/Home/chromium-security/site-isolation) to
            protect website data from compromised renderer processes and side
            channel attacks like Spectre.
*   We have [critical](/developers/severity-guidelines) security
            vulnerabilities relatively infrequently compared to other browsers.
*   We have [leading sandbox protection for the Adobe Flash
            plug-in](http://blog.chromium.org/2012/08/the-road-to-safer-more-stable-and.html).
*   We have [unique
            techniques](http://blog.chromium.org/2010/06/improving-plug-in-security.html)
            for significantly mitigating the security risks posed by plug-ins.
*   We have a robust built-in [sandboxed PDF
            viewer](http://chrome.blogspot.com/2010/11/pdf-goodness-in-chrome.html)
            which has leading security.
*   We implement [Strict Transport
            Security](https://en.wikipedia.org/wiki/HTTP_Strict_Transport_Security)
            and [preloaded public key
            pinning](http://www.imperialviolet.org/2011/05/04/pinning.html),
            which protected our users against the [fraudulent Diginotar
            certificate](https://blog.mozilla.com/security/2011/08/29/fraudulent-google-com-certificate/)
            for \*.google.com.
*   We implement [root CA verification by the underlying operating
            system](/Home/chromium-security/root-ca-policy).
*   We have leading HTTPS security through features such as [mixed
            script
            blocking](http://blog.chromium.org/2012/08/ending-mixed-scripting-vulnerabilities.html).

### Vulnerability Response

*   We are committed to releasing a fix for any
            [critical](/developers/severity-guidelines) security vulnerabilities
            in [under 60
            days](http://googleonlinesecurity.blogspot.com/2010/07/rebooting-responsible-disclosure-focus.html).
*   On average, we release fixes for [high and
            critical](/developers/severity-guidelines) severity vulnerabilities
            in about 30 days.
*   We have a demonstrated ability to get fixes to users [in
            under](http://googlechromereleases.blogspot.com/2011/03/stable-and-beta-channel-updates.html)
            [24 hours](http://twitter.com/VUPEN/status/46391969903161345).
*   We ensure updates are deployed in a [timely
            manner](http://www.techzoom.net/publications/silent-updates/), and
            invest in [new
            technologies](/developers/design-documents/software-updates-courgette)
            to do so.
*   We have a [Vulnerability Rewards
            Program](http://www.chromium.org/Home/chromium-security/vulnerability-rewards-program)
            to encourage third-party researchers to report vulnerabilities they
            discover.
*   We work with the security community and have a [Security Hall of
            Fame](http://www.chromium.org/Home/chromium-security/hall-of-fame)
            to acknowledge third-parties that materially contribute to improving
            our security.
*   We have the [successful Pwnium
            competition](http://chrome.blogspot.com/2012/03/pwnium-great-exploits-fast-patches.html),
            with large prizes, to keep us up to date with the latest, most
            advanced attacks.

### Advanced Anti- Phishing and Malware defenses

*   We [warn
            you](http://www.google.com/support/chrome/bin/answer.py?answer=99020&hl=en)
            when you're about to visit a website we've previously identify as a
            malware or phishing site.
*   We keep the user better informed against phishing and similar
            attacks by [presenting the most relevant
            information](http://chrome.blogspot.com/2010/10/understanding-omnibox-for-better.html).
*   We implement new, [browser-based security
            enhancements](http://blog.chromium.org/2010/01/security-in-depth-new-security-features.html)
            to protect you against malicious sites.

### High profile researchers and publications say nice things about us

*   A [Fortune
            article's](http://tech.fortune.cnn.com/2011/03/21/google-fixes-flashs-security-issues-ahead-of-adobe/?utm_source=feedburner&utm_medium=feed&utm_campaign=Feed%3A+fortunebrainstormtech+%28Fortune+Brainstorm+Tech%29)
            headline subtext: "Google's record on Chrome browser security is
            impressive, and that is important."
*   An [interview with Dino Dai Zovi and Charlie
            Miller](http://www.h-online.com/security/features/Hackers-versus-Apple-1202598.html):
            "I recommend that users surf the web with Google Chrome, disable
            unnecessary plug-ins, and use site-based plug-in security settings
            for the plug-ins that they do need."
*   An article noting [Chrome's unique 3-years-in-a-row
            survival](http://www.computerworld.com/s/article/9214022/Google_s_Chrome_untouched_at_Pwn2Own_hack_match)
            at the Pwn2Own competition: "the browser will have survived three
            consecutive Pwn2Owns, a record."
*   An article [noting our agility and fast security
            updates](http://www.h-online.com/security/news/item/Google-closes-Flash-hole-faster-than-Adobe-1209932.html):
            "Google has once again reacted faster than Adobe itself"
*   A more mainstream publication [interviews HD
            Moore](http://content.usatoday.com/communities/technologylive/post/2011/03/20-grand-not-enough-to-entice-hackers-to-crack-google-chrome/1),
            who calls Chrome the toughest browser: "Chrome was likely the most
            difficult target due to the extensive sandboxing."
*   An [article in the very mainstream Washington
            Post](http://www.washingtonpost.com/business/apples-taking-30-percent-of-app-store-subscriptions-is-an-unkind-cut/2011/02/14/ABbMfvH_story.html)
            notes that whilst other browsers are starting to chase Chrome's
            speed, Chrome is still the choice of the security conscious: "Both
            IE 9 and Firefox 4 look like major, welcome advances. But each falls
            short of Chrome in one key aspect: security."
*   A [TIME
            article's](http://techland.time.com/2011/03/14/pwn2own-roundup-apple-fails-google-stays-strong/)
            headline includes: "Google Stays Strong"
*   An [interesting interview with John Wilandar and Chaouki
            Bekrar](http://www.securityvibes.com/community/en/blog/2011/03/25/firefox-4-and-the-state-of-browser-security--the-expert-view)
            (VUPEN CEO). The interview is nominally about Firefox 4 but includes
            quotes such as "I'd say Chrome's sandboxing model still beats all
            the other browsers from an end user perspective.", "At VUPEN, we
            measure the security of web browsers not by counting the number of
            their vulnerabilities, but by counting the number of days, weeks, or
            months that the vendor is taking to fix vulnerabilities affecting
            their browsers... Today, Google is fixing Chrome vulnerabilities
            much faster than any other vendor – usually one or two security
            updates each month. Microsoft, Mozilla, and Apple are are usually
            releasing security updates for their browsers every 3 months, which
            is too long.", "Relying on third-party auditor through reward and
            bounty programs is the most effective way to improve the security of
            browsers".
