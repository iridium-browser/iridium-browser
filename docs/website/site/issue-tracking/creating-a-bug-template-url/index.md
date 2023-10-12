---
breadcrumbs:
- - /issue-tracking
  - Issue Tracking
page_name: creating-a-bug-template-url
title: Creating a Bug Template Url
---

The following fields can be pre-populated with a value supplied in the URL.
Using links with these query parameters present allows you to guide end-users to
enter an issue that is tailored to a specific need. Usually defining a template
and specifying just a template value is better than specifying other values in a
URL. The user must still review, edit, and submit the form.

<table>
<tr>
<td> Form field</td>
<td> URL query parameter</td>
<td> Values</td>
</tr>
<tr>
<td> Template</td>
<td> template</td>
<td> Name of a defined template in this project</td>
</tr>
<tr>
<td> Summary</td>
<td> summary</td>
<td> Initial summary string</td>
</tr>
<tr>
<td> Description</td>
<td> comment</td>
<td> Prompt text to show in issue description area</td>
</tr>
<tr>
<td> Status</td>
<td> status</td>
<td> Initial status value</td>
</tr>
<tr>
<td> Owner</td>
<td> owner</td>
<td> Username of initial issue owner</td>
</tr>
<tr>
<td> CC</td>
<td> cc</td>
<td> List of users to CC</td>
</tr>
<tr>
<td> Components</td>
<td> components</td>
<td> Comma-separated list of initial component values</td>
</tr>
<tr>
<td> Labels</td>
<td> labels</td>
<td> Comma-separated list of initial label values</td>
</tr>
</table>

**Constructing your URL:**

*   Start w/ the base url -
            https://bugs.chromium.org/p/**&lt;project_name&gt;**/issues/entry
    *   Example: https://bugs.chromium.org/p/chromium/issues/entry
*   Add your query parameters
    *   Example (using the "Defect from user" template) -
                https://bugs.chromium.org/p/chromium/issues/entry***?template=Defect%20report%20from%20user***
    *   [Wiki](https://en.wikipedia.org/wiki/Query_string#Web_forms) -
                How url parameters work.
*   Publish as a quick link (<https://goo.gl/> works well).

**Tips and Tricks:**

*   For descriptions you'll need to url encode the text. Here's a
            [site](http://meyerweb.com/eric/tools/dencoder/) to help you.
