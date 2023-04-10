---
breadcrumbs:
- - /developers
  - For Developers
page_name: postmortems
title: Postmortems
---

### Why Write a Postmortem

To understand how failures happen, in order to prevent future occurrences by
education and process changes.

### When To Write a Postmortem

A postmortem is expected for any tree closures lasting longer than 4 hours,
within 72 hours of the outage.

### Who Should Write the Postmortem

The postmortem should be written by someone involved with detecting and
correcting the issue, preferably someone who can take responsibility for the
followup.

### What to Include

Please use the postmortem template found
[here](https://docs.google.com/document/d/1oBYQmpBthPfxMvW0XgHn7Bu918n6eFLlQM7nVhEdF_0/edit?usp=sharing)
(file -&gt; make a copy).

Your postmortem should include the following sections:

1.  Title
2.  Summary of the event
3.  Full timeline
4.  Root cause(s)
5.  What worked and what didn't (a.k.a., lessons learned)
6.  Action items (followup bugs assigned to specific people)

### Where to Put It

Whenever possible, postmortems should be accessible to the entire Chromium
community. If you are a Google employee, and your postmortem contains internal
details, see the internal infrastructure team's postmortem site instead.

1.  With your chromium.org, write it in a Google Doc, set sharing
            permissions to “Anyone who has the link can comment”
2.  Add it to the list below.
3.  Send the link to
            [chromium-dev@chromium.org](https://groups.google.com/a/chromium.org/forum/#!forum/chromium-dev)
            or
            [infra-dev@chromium.org](https://groups.google.com/a/chromium.org/forum/#!forum/infra-dev),
            as relevant.

See also:

*   <http://codeascraft.com/2012/05/22/blameless-postmortems/>
*   [Example of a great
            postmortem](https://docs.google.com/document/d/1AyeS2du6wp_Pw8Grg8WovbE_A_HV4EUMqdiqeq1KUZ8/edit#heading=h.40nli0xmdtb5).

<table>
  <tr>
    <th>Title</th>
    <th>Date</th>
  </tr>
  <tr>
    <td><a href="https://docs.google.com/document/d/1pHtodNt3Bk0YliLTPmq42LaZaKGcD_v-ZqDtrJFxXzc/edit?usp=sharing">SPDY/QUIC Connection Pooling Bug Postmortem</a></td>
    <td></td>
  </tr>
  <tr>
    <td><a href="https://docs.google.com/document/d/1MnSceOmgw5VPgegNWaMV5iakrvRROeVYw6P0FXvvC28/edit">chromium.perf tests on android userdebug builds failing for 7 days</a></td>
    <td>2015-01-30</td>
  </tr>
  <tr>
    <td><a href="https://docs.google.com/a/chromium.org/document/d/1eCR2v9uR9jR-LQi2GiZ9soQmW4p8lfAFjj-uewBGbh8/edit#heading=h.cg9oxhygkrer">No data from some android bots on chromium.perf for 2 weeks</a></td>
    <td>2014-12-22</td>
  </tr>
  <tr>
    <td><a href="https://docs.google.com/a/chromium.org/document/d/1O2CUqFGMJ6uHP_hBdh9i9AHIOY96nTYaKRa6c4oAHJs/edit#heading=h.cg9oxhygkrer">Grit Compile Errors Require Clobber</a></td>
    <td>2014-07-25</td>
  </tr>
  <tr>
    <td><a href="https://docs.google.com/document/d/1IQMoBeZe8cKjaCf4VIlj2AIt5ZBEsVfrh4o7DP1MPo0/edit#heading=h.xt2hz3y9j243">Swarming Postmortem: 2014-12-04</a></td>
    <td>2014-12-04</td>
  </tr>
  <tr>
    <td><a href="https://docs.google.com/a/google.com/document/d/1qQS8dZ5_wt3iOZ44vYmfrAn3EsAIkwFh9sAMdDPraQE/edit">.dbconfig files loss on master3</a></td>
    <td>2014-09-19</td>
  </tr>
  <tr>
    <td><a href="https://docs.google.com/a/chromium.org/document/d/1wkxPKzWt3Xb3tjczhHl66Y5uCgO0EeE5mlDd9xIHbiE/edit">Postmortem: 15 hour tree closure by a file named "about"</a></td>
    <td>2014-05-09</td>
  </tr>
  <tr>
    <td><a href="https://docs.google.com/document/d/1E9B03cFkyLXjdLjVCu-bTK762E2RJiBhkvbv_4rjDr0">Swarming Postmortem: Undeleteable directorie</a></td>
    <td>2015-11-22</td>
  </tr>
</table>
