# Non standard IPCs

This directory is for projects that have non-standard IPC protocols for
communicating between Chrome browser and ChromeOS.

## Adding new non-standard IPCs

First, consider using any of the existing IPC mechanisms that have better
support from existing tooling. Any new non-standard IPCs need a good reason to
exist.

If there is a strong reason to add a new IPC mechanism, please describe the
problem in Design document, and have security review approvals, and link them in
the code review request.
