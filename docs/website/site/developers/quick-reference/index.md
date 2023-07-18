---
breadcrumbs:
- - /developers
  - For Developers
page_name: quick-reference
title: Quick reference
---

**THIS PAGE IS DEPRECATED.**

This page provides a quick reference for common development related commands.

### Checking the status of your repository

> **Main status command:** svn status

> > This command shows you the status of the repository in the current directory
> > and all subdirectories. It does not check directories outside the current
> > one. C:\\chrome\\src&gt;svn status
> > ? chrome\\mypatch.diff
> > M chrome\\browser\\navigation_controller.cc
> > C chrome\\browser\\navigation_controller.h

> > Note that this does not show changelists. Use gcl opened or gcl changes to
> > see those.

> > **Key of common status codes:** "?" this file is not added to the
> > repository, "A" this file is being added, "M" this file is modified, "C"
> > there is a conflict (see "if you get conflicts" below on fixing this).

> **Helper commands:** gcl provides a few commands to only show parts of the
> subversion status:

> *   gcl opened shows modified files and which changelists they belong
              to, if any.
> *   gcl nothave shows files not under version control.

> **Show modifications:** svn diff

> *   With no extra arguments, this creates a diff of all modified files
              in the current and child directories relative to your last
              checkout.
> *   svn diff *&lt;filename&gt;* shows a diff for the given file.
> *   See also "Managing changelists" below.

### Syncing your checkout

> **To update your checkout:** gclient sync

> *   For a faster update, use the -j / --jobs flag to update multiple
              repositories in parallel, for example: gclient sync --jobs 12
> *   Be sure to run this command within the repository (the root of
              your checkout or any other svn-managed directory). The "Debug" and
              "Release" directories aren't under version control and won't work.
> *   Under Windows, it's best to exit Visual Studio before syncing. It
              will not pick up changes to property sheets (.vsprops) while it's
              running, which can cause strange compiler errors.
> *   *Don't* use svn update. It will only update the current
              repository. A Chromium checkout consists of multiple repositories,
              which gcl tracks simultaneously.

> **If you get conflicts:** Conflicts denoted by a "C" in the status list, and
> indicate that both you and somebody else has changed the same code. After you
> update the file and remove the conflict markers, run svn resolved
> *&lt;filename&gt;* remove the conflict flag. You won't be able to check in
> with the conflict flag set.

### Adding, editing, and reverting

> Unlike some version control systems such as Perforce, Subversion doesn't
> require you to do anything special to begin editing a file. Just edit it.

> **To add a new file to Subversion:** svn add *&lt;filename&gt;*

> **To add a new file to the build: Find the .gyp file for the target you are
> modifying and add the new file into the sources list.**

> **To revert a single file:** svn revert *&lt;filename&gt;*

> **To recursively revert all files a directory:** svn revert -R
> *&lt;directory&gt;* (you can use "." for the current directory).

> **To revert every file in your checkout with extreme prejudice:** gclient
> revert

### Managing changelists

> **To create or modify a changelist:** gcl change *&lt;changename&gt;*

> *   The name is used only on your local computer to identify your
              change, so give it any name that will help you remember.
> *   If you don't specify a name, a random one will be created for you.
> *   After executing this command, your editor will open and you will
              be able to modify the changelist description and move files in and
              out of the changelist by copying and pasting their names.

> **To list all changelists:** gcl changes

> > This lists all files in all changes, even outside of the current directory
> > and its subdirectories.

> > C:\\chrome\\src\\chrome\\browser&gt;gcl changes
> > --- Changelist mychange
> > M chrome\\browser\\navigation_controller.cc

> > This does not show files not in any changelist. Use gcl opened to see those
> > as well.

> **To see a diff of a changelist:** gcl diff *&lt;changename&gt;*

> **To delete a changelist:** gcl delete *&lt;changename&gt;*

> > This does not actually revert the files in the changelist, it leaves the
> > files in their current state, just outside of any changelist.

### Review and commit

> Please see the [contributing code](/developers/contributing-code) page for
> full details.

> **To upload a changelist for review:** gcl upload &lt;changename&gt;

> > This may prompt your for a username and password. Use a Google login (such
> > as your Gmail address and password). Chromium team members should use their
> > full "@chromium.org" address.

> **To see your open reviews:** Go to <http://codereview.chromium.org/> and log
> in. You will see you uploaded, unsubmitted patches as well as patches that
> others have requested you to review.

> **To request review:** Go to the patch on your review page and select "Edit
> issue" on the left (you may need to click on "log in" to see these commands).
> Enter the email address of the reviewer, any additional message (this is
> optional, your changelist description will also be included in the email), and
> press "Update issue".

> **To submit a changelist:** If you don't have commit access, you should ask
> your reviewer to submit the patch for you. If you do have commit access: gcl
> commit &lt;changename&gt;

### Further documentation

You can look at the documentation directly with:
gcl help
and
python /path_to_depot_tools/depot_tools/release/upload.py --help
(gcl will pass through all your arguments to upload.py)
