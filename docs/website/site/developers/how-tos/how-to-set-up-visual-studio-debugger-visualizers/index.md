---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/how-tos
  - How-Tos
page_name: how-to-set-up-visual-studio-debugger-visualizers
title: Setting up Visual Studio Debugger Visualizers
---

[TOC]

## Introduction

Visual Studio (2013 and 2015) allows you to plug in additional "visualizers" to
display data in the watch windows. This makes debugging some of our more complex
data types much easier. To add macros:

*   Copy the contents of Chrome\\src\\tools\\win\\DebugVisualizers\\ to
            %USERPROFILE%\\My Documents\\Visual Studio 2013\\Visualizers\\ (or
            for newer versions, %USERPROFILE%\\Documents\\Visual Studio
            2015\\Visualizers\\)
*   Start the debugger, and be amazed at the fancy new way it displays
            your favorite objects. When you edit the file, you shouldn't have to
            restart all of Visual Studio - it will get re-loaded when you start
            the debugger.

## Definitions

*   DisplayString: an expression (string literal or expression) to be
            shown in the Watch, QuickWatch or Command window; if the preview
            section is present and you also have a AutoExpand rule for it, the
            AutoExpand rule is ignored.
*   Expand: offer the possibility to construct hierarchies.

## References

*   <http://blogs.msdn.com/b/vcblog/archive/2013/06/28/using-visual-studio-2013-to-write-maintainable-native-visualizations-natvis.aspx>
*   <https://code.msdn.microsoft.com/windowsdesktop/Writing-type-visualizers-2eae77a2>
