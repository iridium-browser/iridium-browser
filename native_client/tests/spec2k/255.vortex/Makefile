SRC = \
src/bitvec.c \
src/bmt.c \
src/bmt0.c \
src/bmt01.c \
src/bmt10.c \
src/bmtlib.c \
src/bmtobj.c \
src/core01.c \
src/dba.c \
src/dbm0.c \
src/dbm1.c \
src/dbm2.c \
src/domain.c \
src/draw07.c \
src/draw7.c \
src/drawlib.c \
src/drawobj.c \
src/emplib.c \
src/empobj.c \
src/env0.c \
src/env01.c \
src/env1.c \
src/fm.c \
src/gdbm.c \
src/grp0.c \
src/grp1.c \
src/grp2.c \
src/hm.c \
src/iam.c \
src/ifm.c \
src/im.c \
src/km.c \
src/list01.c \
src/mem00.c \
src/mem01.c \
src/mem10.c \
src/oa0.c \
src/oa1.c \
src/oadmp.c \
src/obj01.c \
src/odbm.c \
src/ogrp.c \
src/om.c \
src/point.c \
src/primal.c \
src/pstub.c \
src/query.c \
src/rect.c \
src/rects.c \
src/sa.c \
src/shell.c \
src/sm.c \
src/testobj.c \
src/tm.c \
src/trans00.c \
src/trans01.c \
src/trans10.c \
src/trans20.c \
src/tree0.c \
src/tree00.c \
src/tree01.c \
src/ut.c \
src/vchunk.c \
src/vdbm.c \
src/voa.c \
src/vom.c


IT=vortex
CFLAGS_EXTRA=-fno-strict-aliasing
EMCCFLAGS_EXTRA_TRAIN = --preload-file bendian.raw \
--preload-file bendian.rnv \
--preload-file bendian.wnv \
--preload-file lendian.raw \
--preload-file lendian.rnv \
--preload-file lendian.wnv \
--preload-file persons.250

EMCCFLAGS_EXTRA_REF = --preload-file bendian1.raw \
--preload-file bendian2.raw \
--preload-file bendian3.raw \
--preload-file bendian.rnv \
--preload-file bendian.wnv \
--preload-file lendian1.raw \
--preload-file lendian2.raw \
--preload-file lendian3.raw \
--preload-file lendian.rnv \
--preload-file lendian.wnv \
--preload-file persons.1k

EXTRA_CLEAN=persons.* lendian* bendian*
include ../Makefile.common
