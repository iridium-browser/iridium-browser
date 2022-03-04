---
breadcrumbs:
- - /for-testers
  - For Testers
- - /for-testers/bug-reporting-guidelines
  - Bug Life Cycle and Reporting Guidelines
page_name: chromium-bug-labels
title: Chromium Bug Labels
---

<table>
<tr>
Label 	 Allowed values 	 Description </tr>
<tr>
<td>Type-*value*</td>

*   <td>`Bug`</td>
*   <td>`Bug-Regression`</td>
*   <td>`Bug-Security`</td>
*   <td>`Compat`</td>
*   <td>`Feature`</td>
*   <td>`Launch`</td>

<td>The issue type. An issue can only have one type.</td>
</tr>
<tr>

<td>Pri-*value*</td>

*   <td>`0 to 3`</td>

<td>The priority. An issue can only have one priority value. `0` is most urgent; `3` is least urgent. </td>
</tr>
<tr>
<td>OS-*value*</td>

*   <td>`All`</td>
*   <td>`Chrome`</td>
*   <td>`Linux`</td>
*   <td>`Mac`</td>
*   <td>`Windows`</td>

<td>The operating system(s) on which the bug occurs. </td>
</tr>
<tr>
<td>M-*value*</td>

*   <td>`27, 28, 29`</td>

<td>A release milestone before which we want to resolve the issue. An issue can only be assigned to one milestone† . `Mstone-X` is 'no milestone' (doesn't apply or not blocking any milestone).</td>
<td>† Except for security bugs. For those, the additional milestones denote branches that the bug should be merged to once fixed. See the <a href="/developers/severity-guidelines">severity guidelines</a>.</td>
</tr>
<tr>
<td>Cr-*value*</td>

*   <td>Blink</td>
*   <td>Internals</td>
*   <td>Platform</td>
*   <td>UI-Shell</td>
*   <td>UI-Browser == Browser</td>
*   <td>Cr-OS-Hardware</td>
*   <td>Cr-OS-Kernel</td>
*   <td>Cr-OS-Systems</td>

<td>The product category to which an issue belongs. A bug can belong to multiple categories. </td>
<td> <table></td>
<td> <tr></td>
<td> <td> Cr-Blink</td></td>
<td> <td> HTML, CSS, Javascript, and HTML5 features</td></td>
<td> </tr></td>
<td> <tr></td>
<td> <td> Cr-Internals</td></td>
<td> <td> Ugly guts, networking, IPC, storage backend, installer, etc</td></td>
<td> </tr></td>
<td> <tr></td>
<td> <td> Cr-Platform</td></td>
<td> <td> Developer Platform and Tools (Ext, AppsV2, NaCl, DevTools)</td></td>
<td> </tr></td>
<td> <tr></td>
<td> <td> Cr-UI-Shell</td></td>
<td> <td> Chrome OS Shell & Window Manager</td></td>
<td> </tr></td>
<td> <tr></td>
<td> <td> Cr-UI-Browser</td></td>
<td> <td> Browser related features (e.g. bookmarks, omnibox, etc...)</td></td>
<td> </tr></td>
<td> <tr></td>
<td> <td> Cr-OS-Hardware</td></td>
<td> <td> Chrome OS hardware related issues</td></td>
<td> </tr></td>
<td><tr></td>
<td><td> Cr-OS-Kernel</td></td>
<td><td> Chrome OS kernel level issues</td></td>
<td></tr></td>
<td><tr></td>
<td><td> Cr-OS-Systems</td></td>
<td><td> Chrome OS system level issues </td></td>
<td></tr></td>
<td> </table></td>
</tr>
<tr>
<td>Restrict-View-EditIssue</td>
<td>-</td>
<td>Used for security bugs to make a bug visible only to project members and the reporter. </td>
</tr>
</table>

Some other labels include:

*   Needs-Reduction: this bug requires a reduced test case. See
            <http://dev.chromium.org/for-testers/backend-testing/website-compatibility/reduced-test-cases>.
*   Cr-UI-Internationalization: Internationalization issues (font
            selection and fallback, right-to-left display issues, IME, etc.)
*   Cr-UI-Localization: Translation issues
*   Type-Bug-Regression: Features that worked in prior releases and are
            now broken. These should be Pri-1.

Component and Labels are created in the system via
<https://www.chromium.org/issue-tracking/requesting-a-component-or-label >