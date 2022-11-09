---
breadcrumbs:
- - /for-testers
  - For Testers
- - /for-testers/frontend-testing
  - Frontend testing
- - /for-testers/frontend-testing/spell-check
  - Spell Check (test plan)
page_name: spell-check--basic-testing
title: 'Spell Check: Basic Testing'
---

## Testcases

<table>
<tr>
Test case Steps Expected Result </tr>
<tr>
<td>Red underline for spelling errors</td>

1.  <td>Launch Chromium.</td>
2.  <td>Navigate to <a
            href="http://mail.google.com">http://mail.google.com</a>.</td>
3.  <td>Sign in to your account.</td>
4.  <td>Click <b>Compose Mail</b>.</td>
5.  <td>Enter <b>gogl</b> plus a space in the message field.</td>

<td><b>gogle</b> will be underlined in red.</td>
</tr>
<tr>
<td>Login form: username will not be red underlined</td>

1.  <td>Launch Chromium.</td>
2.  <td>Visit <a
            href="http://mail.google.com">http://mail.google.com</a> (if signed
            in, sign out to see the login page)</td>
3.  <td>Enter any text in the <b>Username</b> field.</td>

<td>Username will not be underlined in red.</td>
</tr>
<tr>

<td>Mis-spelled text that is copied and pasted will be underlined in red</td>

1.  <td>Launch Chromium.</td>
2.  <td>Navigate to <a
            href="http://mail.google.com">http://mail.google.com</a>.</td>
3.  <td>Sign in to your account.</td>
4.  <td>Click <b>Compose Mail</b></td>
5.  <td>Copy and paste any mis-spelled text into the message field.</td>

<td>Mis-spelled text will be underlined in red.</td>
</tr>
</table>
