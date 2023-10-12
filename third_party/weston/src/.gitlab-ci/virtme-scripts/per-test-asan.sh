#!/bin/bash

# When running Debian's Xwayland and fontconfig, we hit memory leaks which
# aren't visible on other setups. We do have suppressions for these tests, but
# regrettably ASan can't see through omitted frame pointers in Expat, so for
# Xwayland specifically, we disable fast-unwind.
#
# Doing it globally makes the other tests far, far, too slow to run.
case "$1" in
  *xwayland*)
    export ASAN_OPTIONS="detect_leaks=0,fast_unwind_on_malloc=0"
    ;;
  *)
    export ASAN_OPTIONS="detect_leaks=0"
    ;;
esac

export ASAN_OPTIONS

exec "$@"
