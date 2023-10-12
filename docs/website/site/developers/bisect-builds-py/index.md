---
breadcrumbs:
- - /developers
  - For Developers
page_name: bisect-builds-py
title: bisect-builds.py
---

Have you ever hit a regression bug like this: *"In chromium 85.0.4183.121,
things were broken. Back in chromium 86.0.4240.193, it was fine."*? A good way
to attack bugs like this – where it's unclear what change could have caused the
regression, but where you have a reliable repro – is to bisect.

tools/bisect-builds.py automates downloading builds of Chrome across a
regression range, conducting a binary search for the problematic change.

If you don't have a chromium checkout, you can fetch just this script with the
commands below:

Linux

```none
curl -s --basic -n "https://chromium.googlesource.com/chromium/src/+/HEAD/tools/bisect-builds.py?format=TEXT" | base64 -d > bisect-builds.py
```

macOS

```none
curl -s --basic -n "https://chromium.googlesource.com/chromium/src/+/HEAD/tools/bisect-builds.py?format=TEXT" | base64 -D > bisect-builds.py
```

Windows (no curl, base64) python3

```none
python3 -c "import base64; import urllib.request; print(str(base64.b64decode(urllib.request.urlopen(\"https://chromium.googlesource.com/chromium/src/+/HEAD/tools/bisect-builds.py?format=TEXT\").read()),'utf-8'))" > bisect-builds.py
```

Run it like this:

<pre><code>python tools/bisect-builds.py -a <i>platform</i> -g <i>good-revision</i> -b <i>bad-revision</i> -- <i>flags-for-chrome</i>
</code></pre>

For example,

```none
python tools/bisect-builds.py -a mac -g 782793 -b 800218 --use-local-cache --verify-range -- --no-first-run --user-data-dir=/tmp http://example.com
```

Alternately, if bisecting between stable releases you can specify either end of
the bisect range using milestone numbers, like this:

```none
python tools/bisect-builds.py -a mac -g M85 -b M86 --use-local-cache --verify-range
```

Finally, you can specify either end of the bisect range using version numbers.

```none
python tools/bisect-builds.py -a mac -g 85.0.4183.121 -b 86.0.4240.193 --use-local-cache --verify-range
```

The three range specifications above are equivalent. Note that in all cases the
bisect is being done from trunk builds so merges to a release branch will not be
bisected.

Valid archive types (the -a parameter) are `mac, mac64, win, win64, linux (not
supported for builds after March 2016), linux64, linux-arm, and chromeos`.

You can also use the `-p` option to specify a profile. If no `-p` or
`--user-data-dir` option is specified, a new profile will be created in a
temporary directory each time you are asked to try a build. If you specify a
profile folder, point to the directory that's a parent of Default/.

The script will download a build in the revision range and execute it. You must
then manually check if the bug still repros. Quit Chromium, and the script will
ask you if the bug reproduced or not. It will use your answer to drive a binary
search, and after just a few steps it will tell you "this regression happened
somewhere between revisions 793241 and 793248". From that list, it's usually
easy to spot the offending CL. If you're adding the range as a comment to a bug,
please always paste the output from bisect-builds.py, as this includes links to
the chromium changes in the regression range.

View code changes in revision range with this useful URL (replacing SUCCESS_REV
and FAILURE_REV with the range start and end):

<https://test-results.appspot.com/revision_range?start=SUCCESS_REV&end=FAILURE_REV>

**Notes:** For internal usage, we also enable bisect builds by commits. Please
refer to [internal
doc](https://sites.google.com/a/google.com/chrome-te/home/tools/bisect_builds)
for more information.

**Getting an initial revision range**

If you have two Chrome binaries, one which doesn't work, one which does, you can
find their revision numbers as follows.

First, visit the [chrome://version](javascript:void(0);) page and copy the
version number (for example 85.0.4183.121). Then, use this version number as the
parameter to -g or -b. Alternately you can use stable-release milestone numbers
(M85, M86) or look in git commit messages for "Cr-Commit-Position:
refs/heads/master@{{ '{%' }}825204}".

### Verifying the range

If your revision range is incorrect, or if something about your environment
interferes with your reproduction of the bug, you will not get useful results
from bisect-builds.py. If you would prefer to know this as soon as possible,
rather than after downloading and checking O(log n) builds, pass the
**--verify-range** option to bisect-builds.py. This will check the first and
last builds in the range before starting the bisect.


### Field trials

The bisect script uses non-Chrome branded builds and therefore uses the [field
trial testing config](
https://chromium.googlesource.com/chromium/src/+/main/testing/variations/README.md).
This means that behavior may differ from what you see in Chrome.  Consider
using the `disable-field-trial-config` command line switch if this matters to
you.

To find out which variation causes a regression, you can use [bisect-variations.py](
https://chromium.googlesource.com/chromium/src/+/refs/heads/main/tools/variations/bisect_variations.py)

### If Pepper Flash is required to repro

You will have to locate a Flash binary from an official build. If you suspect a
Chromium change causing the regression and the Flash version doesn't matter
locate any binary on your machine. For instance:

```none
./bisect-builds.py -f /opt/google/chrome/PepperFlash/libpepflashplayer.so -b 232915 -g 230425 -a linux64
```

```none
python bisect-builds.py -f "C:\Program Files (x86)\Google\Chrome\Application\31.0.1650.39\PepperFlash\pepflashplayer.dll" -b 232915 -g 230425 -a win
```

```none
./bisect-builds.py -f "/Applications/Google Chrome.app/Contents/Versions/33.0.1707.0/Google Chrome Framework.framework/Internet Plug-Ins/PepperFlash/PepperFlashPlayer.plugin" -b 232915 -g 230425 -a mac
```

**API Keys and Chrome OS builds**

Without API keys, Chrome OS won't allow you to log in as a specific user. To run
a chromeos bisect on your Linux desktop, add the following variables to your
environment (e.g., via .bashrc):

GOOGLE_API_KEY=&lt;key&gt;

GOOGLE_DEFAULT_CLIENT_ID=&lt;id&gt;

GOOGLE_DEFAULT_CLIENT_SECRET=&lt;secret&gt;

See <https://www.chromium.org/developers/how-tos/api-keys> for more info about
API keys.

**Bisecting Per-Revision Builds (Googlers Only)**

The -o and -r options available in the internal version of the script allow
tighter bisects. Google employees should visit
[go/bisect-builds](https://goto.google.com/bisect-builds) for configuration
instructions.
