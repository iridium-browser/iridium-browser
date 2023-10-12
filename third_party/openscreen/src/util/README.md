# Utility Code

The util/ folder is meant to house core utility classes and logic that can be
used by everything else in the Open Screen repository.

Code here is permitted to include `platform/api` and `platform/base` -- same as
the rest of the repository.  However, `platform/api` and `platform/base` are not
allowed to use code in `util/` to avoid circular dependencies.

Includes things like string utils, JSON parsing and serialization, our
std_util.h header, numeric helpers, additional container classes, URL handling,
and the alarm.

`crypto` contains helper classes for working with cryptographic functions and
X.509 certificates.
