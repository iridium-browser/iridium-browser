---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/how-tos
  - How-Tos
- - /developers/how-tos/gpu-wrangling
  - GPU Bots & Pixel Wrangling
page_name: check_gpu_bots-script
title: check_gpu_bots script
---

This is a script I hacked up during my wrangling shift. Hopefully this is only a
temporary measure, and will be deprecated by the awesome work being done on the
hackability CY and the sheriff-o-matic.

#### Purpose

While wrangling, I found it cumbersome and time consuming to keep track of
various waterfalls in a browser window with 10+ tabs. This script is meant to be
a quick-and-dirty way to avoid having to stare at the waterfalls quite so much
during a shift.

#### How to get it

[Here](/developers/how-tos/gpu-wrangling/check_gpu_bots-script/check_gpu_bots.py)
is an older version of the script available for direct download for convenience,
but there will be little continued support for it. The canonical version is now
checked into the Chromium repository at src/gpu/tools/check_gpu_bots.py.

#### Features

The script, when run directly with no args, simply polls the waterfall's [JSON
API](http://build.chromium.org/p/chromium/json/help), gathers some information
about (hopefully all of) the relevant GPU bots. It then prints out a summary of
the offline or failing bots, and exits.

It can also be set up to repeat itself periodically, sort of like an
auto-refresh.

The feature that made it most useful to me is that it can also optionally email
you when there's something that needs to be looked into. By default, "needs to
be looked into" is roughly defined as

*   there is at least one bot whose most recent build failed, and whose
            most recent build had passed on the previous execution of the
            script, OR
*   there is at least one bot which has been in the "offline" state for
            over an hour, and hadn't been the last time the script executed

As described above, it only sends an email *once* per bot, the first time
something goes wrong. I did that to prevent getting spammed. Currently this
can't be overridden with command-line arguments, but I intend to add the ability
to override this.

**NOTE** that currently only gmail addresses are supported as the source email
address, since the script hard-codes gmail's smtp server address. In addition,
if you use a weak password, gmail may require you to explicitly allow "[less
secure apps](https://support.google.com/accounts/answer/6010255)".

Running the script on repeat, and having it set up to email you in case of a
problem, takes away a good deal of staring at waterfalls. I recommend that you
take a look at the waterfall from time to time, just because this script is not
time-tested, and bugs aren't outside the realm of possibility.

### Examples

*   ./check_gpu_bots.py
    *   This runs once, and prints the results to stdout.
*   ./check_gpu_bots.py --help
    *   This prints basic usage information, including the list of
                possible command-line arguments.
*   ./check_gpu_bots.py --email-from &lt;source email&gt; --email-to
            &lt;dest email&gt; --send-email-on-error --repeat-delay 10
    *   '--repeat-delay 10' sets the script to poll the waterfall every
                10 minutes. '--send-email-on-error' tells the script to email
                you in case of error/exception (for example, an unreachable
                server), as well as the normal cases. The rest tells the script
                to send an email from &lt;source email&gt; to &lt;dest
                email&gt;. You do have to be in control of &lt;source email&gt;
                and will need to supply the password, either by saving it in a
                file on disk, or supplying it to the script's prompt. If you opt
                to save the password on disk, it is very strongly recommended
                that you use a dummy account for the source email address. Also
                note that the script can accept multiple destination email
                addresses.

#### Bugs

Please report bugs to [hartmanng@chromium.org](mailto:hartmanng@chromium.org).
Feedback, features requests, and suggestions are also welcome.
