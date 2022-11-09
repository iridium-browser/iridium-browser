---
breadcrumbs:
- - /developers
  - For Developers
page_name: irc
title: IRC
---

The channel **#chromium** on **https://webchat.freenode.net/** is used for
technical discussion as well as communicating about the current state of the
tree.

For user questions, visit the unofficial support channel **#chromium-support**
at **https://webchat.freenode.net/**.

Googlers, see [internal IRC
notes](https://wiki.corp.google.com/twiki/bin/view/Main/ChromeIRC).

For info on IRC servers and ports, see <https://freenode.net/kb/answer/chat>

**Logging**

An unofficial IRC log seems to be hosted at
https://echelog.com/logs/browse/chromium/ (EDIT: this URL appears to be dead so
the link has been removed). It's likely that anything you say here will be
copied in triplicate forever, so only say things your mother would approve of or
you wouldn't mind being on the news.

**Contributors**

If you are contributing or wish to do so, it is a good idea to connect to this
channel. In particular, if you are committing, please connect to the channel so
that the sheriff or others may notify you of build breaks and other
time-sensitive issues with the build infrastructure. Consider adding your IRC
nick to the table here:
<https://chromium.googlesource.com/chromium/src/+/HEAD/docs/user_handle_mapping.md>,
espeically if it varies wildly from your @chromium.org account name. (This helps
us know who to ping when things go wrong.)

To find out whether your mapping is working (or to find out who to yell at if
you're a sheriff), you can ask trungl-bot who a user is:

nick#chromium&gt; trungl-bot: whois user

and trungl-bot should respond shortly with an answer like:

nick#chromium&gt; nick: It looks like user could be: user@chromium.org /
:da_nick / user@google.com.

You can also do this sneakily in a private message with:

/msg trungl-bot whois user

**Bots**

trungl-bot is a helpful automated servant who can answer many questions for you
in the channel. Find out what it can do by asking for help, er, "halp":

/msg trungl-bot halp

For example, find the current sheriffs using:

/msg trungl-bot sheriffs

**Getting op permissions for #chromium via chanserv**

If people in the channel repeatedly get off topic and are making it hard for
sheriffs or chromium contributors to talk without distractions, it may be
suitable to give them a ban with timeout.

You must first be identified (chanserv requires this) and have the correct
permissions. Ask someone who already has privileges with chanserv (anyone who
gets an automatic +v when joining) to grant you the appropriate flags. They will
need to run:

/msg chanserv flags #chromium &lt;your_nick&gt; +ARVfiorstv

on your behalf. You should know this is successful if you are given voice
automatically on joining #chromium. If there are problems just ask on IRC (and
if no one responds, start with maruel@, thakis@, or awong@).

**Off-topic folks in the channel**

If you already have permissions with chanserv, to give an off-topic nick an hour
to file a bug, gather repro steps/screenshots/test cases, and otherwise see some
sunlight, use:

/msg chanserv akick #chromium add &lt;their_nick&gt; !T 1h \[optional rationale
that's logged with chanserv\]

If you'd rather do this manually, you can op yourself as follows:

/msg chanserv op #chromium &lt;your_nick&gt;

and kick the user with something like:

/kick #chromium &lt;nick_to_kick&gt; \[optional rationale for kicking user\]

and if you'd like to un-op yourself after:

/mode #chromium -o &lt;your_nick&gt;
