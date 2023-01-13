---
breadcrumbs:
- - /issue-tracking
  - Issue Tracking
page_name: migrating-issue-components
title: Migrating Issue Components
---

Basically it's a two step process 1.) [Bulk
edit](/issue-tracking/how-to-bulk-edit) 2.) [Edit the
component](/issue-tracking/editing-components), marking it as "Deprecated".

Let's go through an example, migrating issues from
"Blink&gt;Webrtc&gt;GetUserMedia" to "Blink&gt;GetUserMedia".

## Step 1 - Bulk Edit

1. Search "All issues" for the component that you want to deprecate (e.g.
Blink&gt;&gt;Webrtc&gt;GetUserMedia). Once the results list appears, Select
"All," go to the "Actions..." drop down and select "Bulk edit...".

[<img alt="image"
src="/issue-tracking/migrating-issue-components/Bulk%20Edit.png">](/issue-tracking/migrating-issue-components/Bulk%20Edit.png)

​

2. In the Components field (w/ Append += set), add the replacement component
(e.g. Blink&gt;GetUserMedia), followed by the component that you want to remove/
deprecate starting with a minus ("-"). In this example
("-Blink&gt;Webrtc&gt;GetUserMedia). Click the "Update ## Issues" button.

**Pro tip:** Uncheck the Send email box.

[<img alt="image"
src="/issue-tracking/migrating-issue-components/Bulk%20Edit%20-%20Confirm.png">](/issue-tracking/migrating-issue-components/Bulk%20Edit%20-%20Confirm.png)

## Step 2 - Deprecate the component (to keep it out of Auto-complete)

1. Once you load up the issue tracker, click on the "Development process" link
on the top most menu.

[<img alt="image"
src="/issue-tracking/migrating-issue-components/Development%20Process.png">](/issue-tracking/migrating-issue-components/Development%20Process.png)

2. Select "Components" from the sub-menu that appears.

[<img alt="image"
src="/issue-tracking/migrating-issue-components/Components.png">](/issue-tracking/migrating-issue-components/Components.png)

3. Find your component from the current set of "Active components" (Pro tip:
There are a lot, so ctrl+F is helpful).

[<img alt="image"
src="/issue-tracking/migrating-issue-components/Component%20List.png">](/issue-tracking/migrating-issue-components/Component%20List.png)

4. Check "Deprecated" and click on "Submit changes."

**Pro Tip:** "Deprecated" versus "Delete component". Both will remove the
component from auto-complete, however Delete will remove all references to the
component from any existing issues that might have it (e.g. if one were to
forget to query for "All issues" to migrate over fixed issues). Delete basically
has no "undo," in case a mistake is made. Given that errors happen, it's best
practice to use "Deprecated" over delete to afford a measure of recovery.

[<img alt="image"
src="/issue-tracking/migrating-issue-components/Depercate%20Component.png">](/issue-tracking/migrating-issue-components/Depercate%20Component.png)
