# Platform API

The Platform API is designed to allow easy porting of the Open Screen Library
between platforms. This folder contains classes and logic used by Open Screen to
access system resources and other common OS-level facilities, such as the clock
and task runner. Note: utilities are generally not homed here, instead
preferring the top level util/ directory. New classes added here must NOT
depend on any other folders in the openscreen repository, excepting third party
libraries like Abseil or GTest. We include "default implementations" of many
platform features.

## Directory structure

 - api/ contains the Platform API which is used by the Open Screen Library.
   Some of the public API may also include adapter code that acts as a small
   shim above the C functions to be implemented by the platform. The entire
   Open Screen repository can depend on files from api/, though classes
   defined in api/ can only depend on third_party or platform/base files.

 - base/ contains declarations/definitions of API constructs. Classes homed here
   shall only depend on platform/api or this folder.

 - impl/ contains the implementation of the standalone use case, namely a
   self-contained binary using the default implementation of the platform API.
   Note: people familiar with the old layout may notice that all files from the
   posix/, linux/, and mac/ directories have been moved here with an OS-specific
   suffix (e.g. _mac, _posix).

 - test/ contains API implementations used for testing purposes, like the
   FakeClock or FakeTaskRunner.
