---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/testing
  - Testing and infrastructure
page_name: changedashboard
title: Making changes to test dashboard
---

**http://test-results.appspot.com**

Making changes:

1. In a Blink checkout, the code is at Tools/TestsResultServer.

2. Modify the files locally. If you're only modifying files in the
static-dashboards directory, then to test, you can just load the appropriate
HTML file off your local disk.

3. Run the flakiness_dashboard.html unittests by loading run-unittests.html.

4. Get the patch reviewed and committed as per the usual Blink procedure.

If you're fixing an emergency issue (e.g. the dashboard doesn't load at all),
it's acceptable to push the new server before you've gotten your patch reviewed
and committed.

In order to push the changes to the server, you need to get permissions to
modify the server. Contact one of the existing admins (e.g. ojan@chromium.org).
Anyone is welcome to permissions.

Pushing committed changes to the server:

1. Install the appengine SDK for python:
<http://code.google.com/appengine/downloads.html#Google_App_Engine_SDK_for_Python>

2. cd Tools/TestResultServer

3. appcfg.py update . --version rXXXXX

^^^ replace XXXXX with the SVN revision your Blink checkout is synced to. This
is just a string. The push is from your local repository, not the actual SVN
server. It's so we can keep track of what we've pushed to appengine.

4. Go to <https://appengine.google.com/deployment?&app_id=test-results-hrd>.

5. Verify that your version works by clicking on the "instances" link and
navigating around.

6. Select the version you just uploaded and click "Make Default".

7. Sanity check that your changes are all working as expected.
