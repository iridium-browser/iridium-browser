---
breadcrumbs:
- - /administrators
  - Documentation for Administrators
page_name: log-messages
title: Administrative Log Messages
---

When Chrome encounters events that may be important to system administrators, it
adds entries to a system log file that administrators can access and monitor.
For example, these may include informational messages about enterprise policies
or warnings about security events.

These events are logged to the Windows Event Log on Windows, or to the messages
log file on POSIX systems. In Chrome's code, the SYSLOG macro is used to
generate the logs, and the code around the uses of the [SYSLOG
macro](https://cs.chromium.org/chromium/src/base/syslog_logging.h) can often
give some context to the meaning of the message.

More information about Chrome's debug logs can be found
[here](https://support.google.com/chrome/a/answer/6271282?hl=en).

## Site Isolation SYSLOGs

One category of administrative log messages has been added in Chrome 68 to track
violations of [Site Isolation](/Home/chromium-security/site-isolation) security
restrictions. These messages may indicate that a renderer process has been
compromised and is trying to access cross-site data that would otherwise be
off-limits, such as cookies, passwords, or localStorage. The messages occur when
Chrome has decided to kill a misbehaving renderer process in one of these
situations. (Keep in mind that there may be some false positives in the log if a
bug in Chrome causes a renderer process to send the wrong IPCs.)

The current log messages include:

*   Cookies:
    *   Killing renderer: illegal cookie write. Reason: N
    *   Killing renderer: illegal cookie read. Reason: N
*   Passwords:
    *   Killing renderer: illegal password access from about: or data:
                URL. Reason: N
    *   Killing renderer: illegal password access. Reason: N
*   localStorage:
    *   Killing renderer: illegal localStorage request.

Most of these messages include a reason code, which corresponds to an enum value
in
[content/browser/bad_message.h](https://cs.chromium.org/chromium/src/content/browser/bad_message.h)
(or, in the case of passwords, in
[components/password_manager/content/browser/bad_message.h](https://cs.chromium.org/chromium/src/components/password_manager/content/browser/bad_message.h)).
These enum values can point administrators to the relevant part of Chrome's code
to understand more about what happened during the event.
