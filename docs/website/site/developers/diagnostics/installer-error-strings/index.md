---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/diagnostics
  - Diagnostics
page_name: installer-error-strings
title: Installer Error Strings
---

Chrome's Windows installer provides localized error strings to Google Update for
presentation to the user. The strings are written to the registry by
[WriteInstallerResult](http://codesearch.google.com/codesearch#OAMlx_jo-ck/src/chrome/installer/util/installer_state.h&exact_package=chromium&q=WriteInstallerResult&l=183&ct=rc&cd=1).
As of this writing, the set of error string message identifiers (search for them
in
[google_chrome_strings.grd](http://src.chromium.org/viewvc/chrome/trunk/src/chrome/app/google_chrome_strings.grd?view=markup)
for their corresponding text) is:

<table>
<tr>
<td> <b>Message ID</b></td>
<td> <b>Product(s)</b></td>
<td> <b>Operation</b></td>
</tr>
<tr>
<td> IDS_INSTALL_DIR_IN_USE</td>
<td> All</td>
<td> First install</td>
</tr>
<tr>
<td> IDS_INSTALL_FAILED</td>
<td> All</td>
<td> All </td>
</tr>
<tr>
<td> IDS_INSTALL_HIGHER_VERSION</td>
<td> Chrome</td>
<td> Update</td>
</tr>
<tr>
<td> IDS_INSTALL_HIGHER_VERSION_CB_CF</td>
<td> Chrome + Chrome Frame</td>
<td> Update</td>
</tr>
<tr>
<td> IDS_INSTALL_HIGHER_VERSION_CF</td>
<td> Chrome Frame</td>
<td> Update</td>
</tr>
<tr>
<td> IDS_INSTALL_INCONSISTENT_UPDATE_POLICY</td>
<td> All</td>
<td> Update</td>
</tr>
<tr>
<td> IDS_INSTALL_INSUFFICIENT_RIGHTS</td>
<td> All</td>
<td> All system-level</td>
</tr>
<tr>
<td> IDS_INSTALL_INVALID_ARCHIVE</td>
<td> All</td>
<td> All</td>
</tr>
<tr>
<td> IDS_INSTALL_MULTI_INSTALLATION_EXISTS</td>
<td> All</td>
<td> Update or Repair</td>
</tr>
<tr>
<td> IDS_INSTALL_NON_MULTI_INSTALLATION_EXISTS</td>
<td> Chrome Frame</td>
<td> Update or Repair</td>
</tr>
<tr>
<td> IDS_INSTALL_NO_PRODUCTS_TO_UPDATE</td>
<td> Chrome</td>
<td> First install</td>
</tr>
<tr>
<td> IDS_INSTALL_OS_ERROR</td>
<td> All</td>
<td> All</td>
</tr>
<tr>
<td> IDS_INSTALL_OS_NOT_SUPPORTED</td>
<td> All</td>
<td> All</td>
</tr>
<tr>
<td> IDS_INSTALL_READY_MODE_REQUIRES_CHROME</td>
<td> Chrome Frame</td>
<td> All</td>
</tr>
<tr>
<td> IDS_INSTALL_SYSTEM_LEVEL_EXISTS</td>
<td> All</td>
<td> Update or Repair</td>
</tr>
<tr>
<td> IDS_INSTALL_TEMP_DIR_FAILED</td>
<td> All</td>
<td> All</td>
</tr>
<tr>
<td> IDS_INSTALL_UNCOMPRESSION_FAILED</td>
<td> All</td>
<td> All</td>
</tr>
<tr>
<td> IDS_SAME_VERSION_REPAIR_FAILED</td>
<td> Chrome</td>
<td> Repair</td>
</tr>
<tr>
<td> IDS_SAME_VERSION_REPAIR_FAILED_CF</td>
<td> Chrome Frame</td>
<td> Repair</td>
</tr>
<tr>
<td> IDS_SETUP_PATCH_FAILED</td>
<td> All</td>
<td> All</td>
</tr>
</table>
