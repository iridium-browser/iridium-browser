



<hr>

**Author checklist for test code/plans:**

- [ ] All outstanding work is tracked with "TODO" in a test/file description or `.unimplemented()` on a test.
- [ ] New helpers, if any, are documented using `/** doc comments */` and can be found via `helper_index.txt`.
- [ ] (Optional, sometimes not possible.) Tests pass (or partially pass without unexpected issues) in an implementation. (Add any extra details above.)

**[Reviewer sign-off](https://github.com/gpuweb/cts/blob/main/docs/reviews.md) for test code/plans:** (Note: feel free to pull in other reviewers at any time for any reason.)

- [ ] The test path is reasonable, the [description](https://github.com/gpuweb/cts/blob/main/docs/intro/plans.md) "describes the test, succinctly, but in enough detail that a reader can read only the test plans in a file or directory and evaluate the completeness of the test coverage."
- [ ] Tests appear to cover this area completely, except for outstanding TODOs. Validation tests use control cases.
    (This is critical for coverage. Assume anything without a TODO will be forgotten about forever.)
- [ ] Existing (or new) test helpers are used where they would reduce complexity.
- [ ] TypeScript code is readable and understandable (is unobtrusive, has reasonable type-safety/verbosity/dynamicity).
