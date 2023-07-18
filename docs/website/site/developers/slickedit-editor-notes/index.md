---
breadcrumbs:
- - /developers
  - For Developers
page_name: slickedit-editor-notes
title: SlickEdit Editor Notes
---

[SlickEdit](http://www.slickedit.com/) is a paid multi-platform IDE / text
editor. (Googlers have a site license) with many features. Notably:

*   Chromium code is indexed *rapidly* (faster than Eclipse) and setup
            to do so is simple.
    *   'Goto Definition', 'Goto Reference' work well across multiple
                languages
*   Generally competitive features to Eclipse, but much faster. Visual
            Studio users will find many features they miss as well.
*   Minimum project configuration required, easily works with files not
            'in project'.
*   Many strong GUI IDE features:
    *   Tiled tab groups
    *   Find replace with RegEx etc
    *   Multiple cursor & block edit modes
    *   Highly customizable and scriptable.
    *   Source control integration
*   With more setup, debugging and in-IDE builds can work, but the setup
            complexity may not be worth it.

## Settings

1.  Context tagging is much faster with multiple background threads.
    In settings search for and set:
    1.  Number of tagging threads to start: 8
    2.  Create dedicated tagging thread for reading: Off
2.  Languages: Applications: C/C++: Formatting: Google Style

## Project & Workspace

1.  Create a Workspace
    1.  Project: New: Workspace: Blank
    2.  Give it a name that will be different than the project, so that
                you can later create other projects that include the same
                project and not have the project & workspace tag file names
                collide.
    3.  src/.. directory is a reasonable place to create this.
    4.  Files will be created:
        1.  workspace.vpw (tiny config listing the projects included)
        2.  workspace.vpwhistu (window layout, etc)
        3.  workspace.vtg (tag file, can become large though settings
                    below will keep it small)
2.  Create a Project
    1.  Project: New: Project: (Other)
    2.  Name it uniquely from the workspace.
    3.  Files will be created
        1.  project.vpj
        2.  project.vtg (if you change the project setting, as below,
                    and will be large)
    4.  Change project setting to add tag files to a project specific
                tag file.
        1.  (Done on project settings, Files tab, bottom of dialog)
        2.  This will allow you to quickly switch between multiple
                    workspaces having different saved window layouts without
                    incurring a cost to re-tag the project.
    5.  Add Tree...
        1.  For each sub-path of interest (all of 'src' would be
                    overkill, but you can include most of it. Come back and add
                    directories as you have the need):
            1.  Select src/directory of interest
            2.  exclude: .svn/;.git
            3.  Select 'Add as wildcard'
    6.  <https://github.com/scheib/chromium-config> has an example, you
                may copy paste the project folders to your own to bootstrap.
3.  Optionally create additional workspaces to make it easy to switch
            between tasks with many files open and tiled window layout saved.
    1.  StackOverflow answer on [How to save and restore window layout
                in
                SlickEdit?](http://stackoverflow.com/questions/15304321/how-to-save-and-restore-window-layout-in-slickedit/32850676#32850676)
