# Life of a Test Change

## Lifetime of new test plans

For anything in the spec/API/ToC that is not currently covered by
the test plan. Note that (the completed portions of) the initial version of
this document, is based on parts of the planned
[Table of Contents](https://github.com/gpuweb/gpuweb/wiki/Table-of-Contents)
as of ~2020-08.

1. Test plan is written, reviewed, and landed (if the implementer is different from the planner,
  they should review), with TODOs as needed.
1. Tests are implemented, reviewed, and landed, with TODOs as needed.

## Lifetime of test plan changes to match spec changes

For changes that come through the specification process.

1. Spec changes go through the [WebGPU project tracker](https://github.com/orgs/gpuweb/projects/1).
1. Once they reach the "Needs Test Plan" column, they can be added into the CTS.
  The item doesn't have to be fully tested, but must be reflected in the test plan in enough
  detail (with any necessary TODOs) to ensure the tests are implemented fully, later on.
  Then, the item can move to the "Specification Done" column.
    - Some features may have tests written before their specification is complete.
      If they are still in the "Needs Specification" column just make sure there
      is a note on the issue that tests are being written, and make sure any spec
      changes get reflected in the tests.
1. Plan and implement as above.

## Lifetime of additions to existing test plans

For any new cases or testing found by any test plan author, WebGPU spec author,
WebGPU implementer, WebGPU user, etc. For example, inspiration could come from
reading an existing test suite (like dEQP or WebGL).

1. Add notes, plans, or implementation to the CTS, with TODOs as needed to ensure the addition
  gets implemented fully.

A change may (or may not) have an associated issue on the spec or CTS repository on GitHub.
If it is otherwise resolved, then it can be closed once reflected in the CTS, so that test work
is tracked in just one place.
