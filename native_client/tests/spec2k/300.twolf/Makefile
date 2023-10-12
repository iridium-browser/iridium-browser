SRC=\
src/acceptt.c \
src/addimp.c \
src/alignfeed.c \
src/buildimp.c \
src/changrid.c \
src/clean.c \
src/closepins.c \
src/config1.c \
src/configure.c \
src/configuref.c \
src/controlf.c \
src/countf.c \
src/date.c \
src/debug.c \
src/dimbox.c \
src/finalwire.c \
src/findcost.c \
src/findcostf.c \
src/findest.c \
src/findfeeds.c \
src/findrchk.c \
src/findrcost.c \
src/findunlap.c \
src/globe.c \
src/globedbg.c \
src/globroute.c \
src/gparser.c \
src/grdcell.c \
src/hash.c \
src/implfeeds.c \
src/main.c \
src/mergplist.c \
src/move.c \
src/mt.c \
src/okmalloc.c \
src/outcm.c \
src/outpins.c \
src/output.c \
src/parser.c \
src/purcost.c \
src/qsorte.c \
src/qsortg.c \
src/qsortgdx.c \
src/qsortx.c \
src/rand.c \
src/readblck.c \
src/readcell.c \
src/readnets.c \
src/readpar.c \
src/readseg.c \
src/savewolf.c \
src/sortpin.c \
src/stats.c \
src/uc0.c \
src/ucg0.c \
src/ucgxp.c \
src/uclosepns.c \
src/ucxx1.c \
src/ucxx2.c \
src/ucxxo1.c \
src/ucxxo2.c \
src/ucxxp.c \
src/uglobe.c \
src/uglobrout.c \
src/ugpair.c \
src/uloop.c \
src/uloop2.c \
src/unetseg.c \
src/unlap.c \
src/upair.c \
src/urcost.c \
src/utemp.c \
src/utils.c \
src/xcompact.c \
src/xpickint.c



IT=twolf

CFLAGS_EXTRA = -DSPEC_CPU2000 -DHAVE_SIGNED_CHAR -Wno-return-type
EMCCFLAGS_EXTRA_TRAIN = \
--preload-file train.blk \
--preload-file train.cel \
--preload-file train.net \
--preload-file train.out \
--preload-file train.par \
--preload-file train.pin \
--preload-file train.pl1 \
--preload-file train.pl2 \
--preload-file train.sav \
--preload-file train.sv2 \
--preload-file train.twf
EMCCFLAGS_EXTRA_REF = \
--preload-file ref.blk \
--preload-file ref.cel \
--preload-file ref.net \
--preload-file ref.par

EXTRA_CLEAN= ref.* train.*

include ../Makefile.common



