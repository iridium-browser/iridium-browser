---
breadcrumbs:
- - /teams
  - Teams
- - /teams/layout-team
  - Layout Team
page_name: bug-triage
title: Bug Triage Instructions
---

The layout team has a bug triage rotation where for a week at a time a member of
the team is responsible for triaging bugs associated with parts of the code that
the team is responsible for. This document outlines the policy and suggestions
surrounding bug triage.

Our SLA is to triage all incoming bugs within one week and to fix all P1
regressions within one release.

## Suggested workflow:

1.  Every other day check the list of [Untriaged
            bugs](https://bugs.chromium.org/p/chromium/issues/list?can=37830509&sort=-id&colspec=ID+Pri+M+Status+Owner+Summary+Modified)
            with the aim to reduce the number to zero.
    For each bug, check whether the issue can be reproduced.
    *   If not either mark as WontFix or add the "Needs-Feedback" label
                and ask for follow up information.
    *   If it can be reproduced, determine if it's a valid bug. Check
                behavior in other browsers and the relevant specifications.
    *   If it is a valid bug determine whether it is a regression or
                not. Set type to Bug-Regression if it is and determine [when it
                broke](/developers/bisect-builds-py). Set relevant sub category
                and assign to the person that introduced the regression.
    *   If valid and not a regression determine severity. For P0 and P1
                assign to owner for specific area. For P2 set status to
                Available.
2.  At least once a week check the list of regressions
            ([P0/P1](https://bugs.chromium.org/p/chromium/issues/list?can=37830592&q=Pri%3D0%2C1&sort=-id&colspec=ID+Pri+M+Status+Owner+Summary+Modified),
            [P2/P3](https://bugs.chromium.org/p/chromium/issues/list?can=37830592&q=-Pri%3D0%2C1&sort=-id&colspec=ID+Pri+M+Status+Owner+Summary+Modified)).
    For each P0 or P1 regression, check if it can still be reproduced.
    *   If not mark as Fixed and add comment to CL that fixed it.
    *   If the issue persists ping the owner or assign one as needed.
    Continue with P2 and P3 regressions as time allows. Don't spend more than
    ~1h/week on regression triage.

## Relevant Links

*   [Untriaged
            bugs](https://bugs.chromium.org/p/chromium/issues/list?can=37830509&sort=-id&colspec=ID+Pri+M+Status+Owner+Summary+Modified)
*   [P0 and P1
            regressions](https://bugs.chromium.org/p/chromium/issues/list?can=37830592&q=Pri%3D0%2C1&sort=-id&colspec=ID+Pri+M+Status+Owner+Summary+Modified)
*   [P2 and P3
            regressions](https://bugs.chromium.org/p/chromium/issues/list?can=37830592&q=-Pri%3D0%2C1&sort=-id&colspec=ID+Pri+M+Status+Owner+Summary+Modified)
*   [Bisecting builds](/developers/bisect-builds-py)
