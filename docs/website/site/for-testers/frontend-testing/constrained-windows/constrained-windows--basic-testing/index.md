---
breadcrumbs:
- - /for-testers
  - For Testers
- - /for-testers/frontend-testing
  - Frontend testing
- - /for-testers/frontend-testing/constrained-windows
  - Constrained Windows (test plan)
page_name: constrained-windows--basic-testing
title: 'Constrained Windows: Basic Testing'
---

## Testcases

<table>
<tr>
Test case Steps Expected Result </tr>
<tr>
<td>Simple test: constrained window</td>

1.  <td>Log in to <a
            href="http://mail.google.com">http://mail.google.com</a>.</td>
2.  <td>Google Talk in Gmail will load on left side. Click on any
            contact to open a chat window </td>
3.  <td>Pop-out the chat window.</td>

<td>Chat window will open as a constrained window on the left side of the page ; title bar : default icon and title "name - chat - Google Chrome"</td>
</tr>
<tr>
<td>Pop-up displayed in constrained window</td>

1.  <td>Visit any website which opens a pop-up</td>

<td>If your settings are checked for : Options&gt; Under the hood&gt; Web Content : Notify me when a pop-up is blocked then blocked pop-up will be notified at bottom right side of the page with title bar : "Blocked pop-up"</td>
<td>On clicking blocked pop-up, title bar will change to "default icon and title - Google Chrome". If no title then "default icon and Google Chrome"</td>
</tr>
<tr>
<td>Play video in constrained window</td>

1.  <td>Visit any website which plays video in constrained window</td>

<td>A constrained window will launch for the video link ; title bar : "default icon and title - google chrome"</td>
</tr>
<tr>
<td>Constrained window - launch with window.open() and close with window.close()</td>

1.  <td>Visit <a
            href="http://html.tests.googlepages.com/example1.html">http://html.tests.googlepages.com/example1.html</a>
            </td>
2.  <td>Click <b>Open window</b>.</td>
3.  <td>Click <b>Close window</b>. </td>

<td>After Step 2: A constrained window with height=200px and width=300px will be launched.</td>
<td> 	After Step 3: The constrained window will be closed.</td>
</tr>
</table>
