---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: blink-scrollbarthemes
title: Blink ScrollbarThemes
---

ScrollbarTheme class hierarchy

ScrollbarTheme

|

|--- ScrollbarThemeMacCommon

| |--- ScrollbarThemeMacNonOverlayAPI

| \\--- ScrollbarThemeMacOverlayAPI

|

|--- ScrollbarThemeNonMacCommon

| |--- ScrollbarThemeAuraOrGtk

| \\--- ScrollbarThemeWin

|

\\--- ScrollbarThemeOverlay

ScrollbarThemeOverlay is used by Android and optionally by Aura or GTK+.

Future work:

- Perhaps try to consolidate the various overlay scrollbar classes so Mac and
non-Mac can share.

- Once Windows switches to Aura, we can remove ScrollbarThemeWin and
ScrollbarThemeNonMacCommon.
