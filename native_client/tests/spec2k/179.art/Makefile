SRC=src/scanner.c


IT=art
CFLAGS_EXTRA = -DSPEC_CPU2000
EMCCFLAGS_EXTRA_TRAIN = --preload-file c756hel.in --preload-file a10.img --preload-file hc.img
EMCCFLAGS_EXTRA_REF = --preload-file c756hel.in --preload-file a10.img --preload-file hc.img

EXTRA_CLEAN= ref.* train.*

include ../Makefile.common
