# Cast Integration Tests

This file contains notes about the integration tests under this directory,
including how they tie into other systems or tests if relevant.

## Device Authentication

The tests in `device_auth_test.cc` verify sender and receiver authentication
code against each other to ensure we are at least self-consistent.  These tests
encompass successful device authentication, authentication errors,
authenticating with a revocation list, and various revocation list failures.

In order to enforce sender and receiver code separation, these tests can also
record the protobuf data they generate for use in unit tests.  For example, a
`CastMessage` containing an `AuthChallenge` from sender code can be used as
fixed input to receiver code.  Currently, only receiver code uses this kind of
data because the sender code just uses existing test data imported from
Chromium.

New test data may need to be generated if a bug is found in either sender or
receiver code or if new test certificates need to be used.  To generate new
data, build and run `make_crl_tests` and run this specific integration test:
``` bash
$ out/Debug/openscreen_unittests --gtest_filter=DeviceAuthTest.MANUAL_SerializeTestData
```
Note that this test will not run without being exactly named in the filter.  The
paths to which they will write are fixed and are the same as from where the
tests expect to read.
