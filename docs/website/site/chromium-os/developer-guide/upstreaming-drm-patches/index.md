---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/developer-guide
  - Chromium OS Developer Guide
page_name: upstreaming-drm-patches
title: Upstreaming drm patches
---

## Pre-requisites

*   [Subscribe to dri-devel mailing
            list](https://lists.freedesktop.org/mailman/listinfo/dri-devel)
*   Setup git send-email to work with your email (@chromium, @android,
            @google) ([Check out the Examples
            section](https://www.kernel.org/pub/software/scm/git/docs/git-send-email.html))
    *   This requires the git-email package (sudo apt-get install
                git-email)

## Sending the patch

*   Add the upstream repo as a remote in your git tree
    *   For kernel patches, consult the MAINTAINERS file or
                scripts/get_maintainer.pl to find the remote git tree location
    *   For libdrm patches, the tree is located at
                git://anongit.freedesktop.org/mesa/drm

> > `; git remote add -f upstream <remote_addr>`
> > `; git fetch upstream`

*   Create a new branch which tracks upstream
    *   For kernel, the branch will depend on how the maintainer has
                setup their tree. Ask on IRC if you're unsure.
    *   For libdrm, use the master branch

> > `; git branch -t for_upstream upstream/<upstream_branch>`

*   Apply the patch to your for_upstream branch (resolving any
            conflicts) and test the patch still works
*   Generate a patch file for upstream, it will be saved as
            0001-&lt;my-commit-subject&gt;.patch
    *   For libdrm, add --subject-prefix 'PATCH libdrm' to the command
    *   If you have not added a Signed-off-by: line to your commit
                message, add --signoff to the command

> > `; git format-patch <optional parameters> HEAD^..`

*   Open the patch file and strip out all of the Google-specific kruft
            (Bug, Test, Change-Id, etc). Once again, ensure you have a
            Signed-off-by: line in the commit message
    *   If this is a v2/v3/vN patch, add vX to the subject after PATCH
                (ie: \[PATCH vX\] my commit subject)

*   Send the patch to the list
    *   For kernel patches:
        *   Run the patch through scripts/checkpatch.pl to ensure it
                    conforms with kernel style guide
        *   Run the patch through scripts/get_maintainer.pl to generate
                    a list of people/lists who should be cc'd
    *   Make sure you cc anyone who provided you with a downstream
                review, as well as people who can help with upstream review
                (seanpaul@, marcheu@, etc)
        *   Use the --cc argument to git send-email
    *   To reply to an email with a patch (ex: after code review), add
                the --in-reply-to=&lt;Message-Id&gt; argument. You can get
                &lt;Message-Id&gt; from mail headers
    *   If you want ONE MORE chance to ensure that everything is right
                for the patch, add the --annotate flag to send-email, it'll let
                you review the message before sending
    *   If this is a series of patches, add a cover letter to explain
                the series with the --cover-letter

> > `; git send-email --to=dri-devel@lists.freedesktop.org`
