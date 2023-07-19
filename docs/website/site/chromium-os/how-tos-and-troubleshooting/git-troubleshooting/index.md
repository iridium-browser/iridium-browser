---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/how-tos-and-troubleshooting
  - How Tos and Troubleshooting
page_name: git-troubleshooting
title: Git Troubleshooting
---

[TOC]

### My pull is failing

If you get an error like this:

ssh: connect to host gitrw.chromium.org port 9223: Connection refused

ssh is failing to connect for some reason. (In the above example, I had the port
wrong.) Verify your ssh configuration is correct -- that you have the keys set
up, that you have right hostname and port, etc. depending on the error message.

If you get an error like this when pulling:

ERROR:gitosis.serve.main:Repository read access denied

The "gitosis" in the error message indicates you've successfully sshed to the
server, but that something has gone wrong after that. Verify the path to the git
repository you're trying to pull is correct with something like

git config remote.origin.url

### My push was rejected

If the output of git-cl looks like this:

```none
$ git cl push
Description: 'Clean up to move NewStringCopy from util to chromeos/string.\n\nReview URL: <http://codereview.chromium.org/460001>'
 .gitignore                    |    8 +++++++-
1 file changed, 7 insertions(+), 1 deletion(-)
About to commit; enter to confirm.
To ssh://gitrw.chromium.org/chromiumos
 ! [rejected]        HEAD -> master (non-fast forward)
error: failed to push some refs to 'ssh://gitrw.chromium.org/chromiumos'
```

Your branch is out of date. You need to run:

```none
$ gclient sync
$ git cl push
```

### I can't gclient sync

If gclient sync is outputting something like this:

```none
$ gclient sync
        Already in a conflict, i.e. (no branch).
        Fix the conflict and run gclient again.
        Or to abort run:
                git-rebase --abort
        See man git-rebase for details.
```

You are inside a rebase conflict. You can either fix the conflict:
<http://www.neeraj.name/blog/articles/811-how-to-resolve-git-merge-conflict-caused-while-doing-git-rebase>
Or undo the rebase:

```none
$ git rebase --abort
```

### Missing dependency

If **setup_board** is failing with output something like this:

```none
>>> Failed to emerge chromeos-base/kernel-headers-0.0.1 for /build/XXX/, Log file:
>>>  '/build/XXX/tmp/portage/chromeos-base/kernel-headers-0.0.1/temp/build.log'
 * Using kernel files: trunk/src/third_party/kernel-XXX
cp: cannot stat `trunk/src/third_party/kernel-XXX/*': No such file or directory
```

You might be missing extra components, and need to add them to your **.gclient**
file so that it contains a custom_deps clause something like:

```none
    "custom_deps" : {
      # To use the trunk of a component instead of what's in DEPS:
      #"component": "https://svnserver/component/trunk/",
      # To exclude a component from your working copy:
      #"data/really_large_component": None,
      "chromeos/src/third_party/kernel-XXX": "ssh://chromiumos-git/kernel-XXX.git"
    },
```

where the LHS is the location in your source tree relative to the config file,
and the RHS is a valid git URL. After fixing up the config file, a **gclient
sync** should fetch the missing bits.
