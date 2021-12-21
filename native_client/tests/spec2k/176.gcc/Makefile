
SRC= \
src/bc-emit.c      src/calls.c       src/function.c      src/loop.c        src/rtl.c\
src/bc-optab.c     src/combine.c     src/getpwd.c        src/m88k.c        src/rtlanal.c\
src/c-aux-info.c   src/convert.c     src/global.c        src/obstack.c     src/sched.c\
src/c-common.c     src/cse.c         src/insn-attrtab.c  src/optabs.c      src/sdbout.c\
src/c-convert.c    src/dbxout.c      src/insn-emit.c     src/print-rtl.c   src/stmt.c\
src/c-decl.c       src/dwarfout.c    src/insn-extract.c  src/print-tree.c  src/stor-layout.c\
src/c-iterate.c    src/emit-rtl.c    src/insn-opinit.c   src/real.c        src/stupid.c\
src/c-lang.c       src/explow.c      src/insn-output.c   src/recog.c       src/toplev.c\
src/c-lex.c        src/expmed.c      src/insn-peep.c     src/reg-stack.c   src/tree.c\
src/c-parse.c      src/expr.c        src/insn-recog.c    src/regclass.c    src/unroll.c\
src/c-pragma.c     src/final.c       src/integrate.c     src/reload.c      src/varasm.c\
src/c-typeck.c     src/flow.c        src/jump.c          src/reload1.c     src/version.c\
src/caller-save.c  src/fold-const.c  src/local-alloc.c   src/reorg.c       src/xcoffout.c

IT=gcc
CFLAGS_EXTRA = -DHOST_WORDS_LITTLE_ENDIAN -std=gnu89
EMCCFLAGS_EXTRA_TRAIN = --preload-file data/train/input
EMCCFLAGS_EXTRA_REF = --preload-file data/ref/input

include ../Makefile.common
