SRC=\
src/accum.c \
src/alpha.c \
src/alphabuf.c \
src/api1.c \
src/api2.c \
src/attrib.c \
src/bitmap.c \
src/blend.c \
src/clip.c \
src/colortab.c \
src/context.c \
src/copypix.c \
src/depth.c \
src/dlist.c \
src/drawpix.c \
src/enable.c \
src/eval.c \
src/feedback.c \
src/fog.c \
src/get.c \
src/hash.c \
src/image.c \
src/light.c \
src/lines.c \
src/logic.c \
src/masking.c \
src/mesa4.c \
src/matrix.c \
src/misc.c \
src/mmath.c \
src/osmesa.c \
src/pb.c \
src/pixel.c \
src/pointers.c \
src/points.c \
src/polygon.c \
src/quads.c \
src/rastpos.c \
src/readpix.c \
src/rect.c \
src/scissor.c \
src/shade.c \
src/span.c \
src/stencil.c \
src/teximage.c \
src/texobj.c \
src/texstate.c \
src/texture.c \
src/triangle.c \
src/varray.c \
src/vb.c \
src/vbfill.c \
src/vbrender.c \
src/vbxform.c \
src/winpos.c \
src/xform.c

IT=mesa
CFLAGS_EXTRA = -DSPEC_CPU2000
EMCCFLAGS_EXTRA_TRAIN = --preload-file mesa.in --preload-file numbers
EMCCFLAGS_EXTRA_REF = --preload-file mesa.in --preload-file numbers

EXTRA_CLEAN= ref.* train.*

include ../Makefile.common
