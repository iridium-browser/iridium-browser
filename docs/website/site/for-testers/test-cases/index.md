---
breadcrumbs:
- - /for-testers
  - For Testers
page_name: test-cases
title: Test Cases
---

<div class="two-column-container">
<div class="column">

### Guidelines

1.  Please make sure that you are on the dev channel. We have recently
            refreshed the dev channel, so you will have to sign up for the dev
            channel once again. Instructions for joining the dev channel are
            here: <http://www.chromium.org/getting-involved/dev-channel/>.
2.  Make sure you are running the latest version. You can find
            information about Google Chrome versions here:
            <http://www.chromium.org/getting-involved/dev-channel/release-notes>.
3.  IMPORTANT: Before you begin testing, please [set up a new
            profile](/developers/creating-and-using-profiles). We strongly
            suggest that you use a separate profile folder for testing purposes
            so that the testing does not tamper with your existing profile and
            also to ensure all the volunteers are starting with a similar setup.
    1.  You can clear the profile between tests, as described in the
                [profile page](/developers/creating-and-using-profiles).
    2.  When you are done testing, you can edit the shortcut again to go
                back to your original profile.
4.  Please go through as many test cases as possible. Detailed
            description about the test cases is provided in the column on the
            left. Each test case should be answered with a 'Pass' or 'Fail'
5.  Please try and associate a failing test case with a bug. It could be
            an existing bug or a new issue. Please refer to bug reporting
            guidelines here:
            <http://www.chromium.org/for-testers/bug-reporting-guidelines>. Use
            the 'Other' response field in the test cases to include the bug
            numbers.
6.  Please do not use this form to send us feature requests or
            information about unrelated crashes or bugs. Please file them
            directly on <http://crbug.com> where they will be looked at along
            with other bugs.

### Test cases

### Test case: Default Browser (admin user)

*   Pre-conditions/setup: Make sure you are logged in with
            administrative privileges and Google Chrome is not your default
            browser.
*   Go to Chrome Menu -&gt; Options -&gt; Basic and click on 'Make
            Google Chrome my default browser'.
*   Verification: Go to Start -&gt; Run and type and valid URL like
            'http://www.google.com' in the run dialog. When you hit enter, it
            should be opened in a new Google Chrome window.

#### Test case: Default Browser (non-admin user)

*   Pre-conditions/setup: Make sure you are logged in without
            administrative privileges (limited user) and Google Chrome is not
            your default browser.
*   Go to Chrome Menu -&gt; Options -&gt; Basic and click on 'Make
            Google Chrome my default browser'.
*   Verification: Go to Start -&gt; Run and type and valid URL like
            'http://www.google.com' in the run dialog. When you hit enter, it
            should be opened in a new Google Chrome window.

#### Test case: Clear Browsing Data (time range 1-day)

*   Pre-conditons/setup: Start with a fresh Chrome profile. Browse to a
            few websites. Log on to your email account and choose to save
            passwords. Download a few files. This will help populate your cache,
            cookies, download history, etc.
*   Go to Chrome Menu -&gt; Options -&gt; Clear browsing data. Select
            all check boxes on the dialog, leave the time period as 'Last day'
            and click on 'Clear browsing data'.
*   Verification: All your recent browsing history should be cleared.
            Your history (Ctrl + H) and downloads (Ctrl + J) pages should not
            show up any entries for the current day. Your saved passwords under
            Chrome Menu -&gt; Options -&gt; Minor Tweaks -&gt; Show saved
            passwords should be empty. There should be no cookies present under
            Chrome Menu -&gt; Options -&gt; Under the Hood -&gt; Show cookies.

#### Test case: Clear Browsing Data (specific data types - cookies)

*   Pre-conditions/setup: Start with a fresh Chrome profile. Visit some
            websites such as http://www.nytimes.com, http://www.cnn.com, etc.
            Log on to an email account (e.g. your GMail account). Go to Chrome
            Menu -&gt; Options -&gt; Under the Hood -&gt; Show cookies and
            verify that cookies have been stored.
*   Go to Chrome Menu -&gt; Options -&gt; Clear browsing data. Select
            only the checkbox labeled cookies and click on 'Clear browsing
            data'.
*   Verification: There should be no cookies present under Chrome Menu
            -&gt; Options -&gt; Under the Hood -&gt; Show cookies.

#### Test case: Find in Page (matches inside html pages, edit boxes, unicode text)

*   Pre-conditions/setup: None.
*   Navigate to a simple page such as <http://www.chromium.org/>. Hit
            'Ctrl + F' Search for the word 'chromium'.
*   Next, go to a website that has a text edit boxes (e.g. compose a
            message in GMail). Type the following text in that box: 'the quick
            brown fox quickly jumps over the lazy dog'. Hit Ctrl + F and enter
            the search text 'QUICK'.
*   Finally go to http://news.sina.com. Bring up the find in page dialog
            and search for: 北美首頁.
*   Verification: In each of the cases mentioned above, the search
            should highlight every single occurrence of the search term in
            Yellow. The search box should show a count of the occurrences as
            well as the the occurrence it has currently focused on which is
            highlighted in Orange. You should be able to toggle between each of
            the highlighted search terms by clicking on the arrow icons in the
            find dialog or by using the keyboard (F3 and Shift+F3).

#### Test case: Tab dragging (screen edges)

*   Pre-conditions/setup: None.
*   When you drag a tab to certain positions on the monitor, a docking
            icon will appear. Release the mouse over the docking icon to have
            the tab snap to the docking position instead of being dropped at the
            same size as the original window.
*   Verification:
    *   Monitor top: make the dropped tab maximized.
    *   Monitor left/right: make the dropped tab full-height and
                half-width, aligned with the monitor edge.
    *   Monitor bottom: make the dropped tab full-width and half-height,
                aligned with the bottom of the monitor.
    *   Browser-window left/right: fit the browser window and the
                dropped tab side-by-side across the screen.
    *   Browser-window bottom: fit the browser window and the dropped
                tab top-to-bottom across the screen.

#### Test case: Profiles (open a tab in new profile, create a new profile)

*   Pre-conditions/setup: Start with a clean profile. Browse to a few
            websites. Log on to your email account and choose to save passwords.
            Download a few files. This will help populate your cache, cookies,
            download history, etc in your current profile.
*   Go to Chrome Menu -&gt; Options -&gt; New window in profile -&gt;
            &lt;New Profile&gt;. Create a new profile and name it (e.g. 'work').
            Leave the 'Create desktop shortcut...' box checked.
*   Verification: There should be a new shortcut on your desktop called
            'Google Chrome for work' (or whatever profile name you choose). If
            you double click on that shortcut, it should launch a new instance
            of Google Chrome. This new instance should have an empty new tab
            page. The history should be empty and there should be no cookies
            present.

#### Test case: Auto scroll

*   Pre-conditions/setup: None.
*   Verification: On pages that have vertical and/or horizontal scroll
            bars, if you press the mouse scroll wheel (middle-click) auto-scroll
            should be enabled and the webpage should scroll as you move your
            mouse.

#### Test case: Downloads (download various types of file)

*   Pre-condition/setup: Start with a clean profile.
*   Download different types of files (various types of file formats and
            sizes). Note: please download files only from sources you trust.
*   Verification: The files should get successfully downloaded to the
            download location, which by default should be My
            Documents\\Downloads. The download history page (Ctrl + J) should
            list all the downloads.

#### Test case: Downloads (change download location)

*   Pre-condition/setup: Start with a clean profile.
*   Go to Chrome Menu -&gt; Options -&gt; Minor Tweaks and change the
            download location to some other folder on your computer.
*   Verification: All subsequent downloads should get downloaded to the
            new download location.

</div>
<div class="column">

</div>
</div>
