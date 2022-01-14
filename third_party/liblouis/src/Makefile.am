SUBDIRS = gnulib liblouis tools tables man tests python windows

# only build the documentation if we have makeinfo 5
if HAVE_MAKEINFO_5
SUBDIRS += doc
endif

ACLOCAL_AMFLAGS = -I m4 -I gnulib/m4 -I tools/gnulib/m4

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = liblouis.pc

EXTRA_DIST = liblouis.pc README.windows HACKING

distwin32 : distwin32-zip

distwin32-zip : distwin32-dir
	rm -f liblouis-$(VERSION)-win32.zip
	zip -r liblouis-$(VERSION)-win32.zip liblouis-$(VERSION)-win32

distwin32-dir :
	rm -rf liblouis-$(VERSION)-win32/*
	@if which i586-mingw32msvc-gcc >/dev/null; then \
	  ./configure --host i586-mingw32msvc --prefix=$(CURDIR)/liblouis-$(VERSION)-win32 && \
	  make LDFLAGS="$$LDFLAGS -avoid-version -Xcompiler -static-libgcc" && \
	  make check WINE=wine32&& \
	  make install; \
	elif which i686-w64-mingw32-gcc >/dev/null; then \
	  ./configure --host i686-w64-mingw32 --prefix=$(CURDIR)/liblouis-$(VERSION)-win32 && \
	  make LDFLAGS="$$LDFLAGS -avoid-version -Xcompiler -static-libgcc" && \
	  make install; \
	elif which docker >/dev/null; then \
	  gtar --transform='s/Dockerfile.dev/Dockerfile/' --exclude-from=.dockerignore -cz * | docker build -t liblouis-dev - && \
	  mkdir -p liblouis-$(VERSION)-win32 && \
	  docker run --rm -v $(CURDIR)/liblouis-$(VERSION)-win32:/root/src/liblouis/liblouis-$(VERSION)-win32 liblouis-dev \
	    make CPPFLAGS='-I/root/build/win32/libyaml/include/' \
	         LDFLAGS='-L/root/build/win32/libyaml/lib/' \
	         distwin32-dir; \
	fi

distwin64 : distwin64-zip

distwin64-zip : distwin64-dir
	rm -f liblouis-$(VERSION)-win64.zip
	zip -r liblouis-$(VERSION)-win64.zip liblouis-$(VERSION)-win64

distwin64-dir :
	rm -rf liblouis-$(VERSION)-win64/*
	@if which x86_64-w64-mingw32-gcc >/dev/null; then \
	  ./configure --host x86_64-w64-mingw32 --prefix=$(CURDIR)/liblouis-$(VERSION)-win64 && \
	  make LDFLAGS='-avoid-version -Xcompiler -static-libgcc' && \
	  make check WINE=wine64&& \
	  make install; \
	elif which docker >/dev/null; then \
	  gtar --transform='s/Dockerfile.dev/Dockerfile/' --exclude-from=.dockerignore -cz * | docker build -t liblouis-dev - && \
	  mkdir -p liblouis-$(VERSION)-win64 && \
	  docker run --rm -v $(CURDIR)/liblouis-$(VERSION)-win64:/root/src/liblouis/liblouis-$(VERSION)-win64 liblouis-dev \
	    make distwin64-dir; \
	fi

# .dockerignore
.dockerignore : .gitignore
	sed -e 's|^\([^/#]\)|# \1|' -e 's|^/||' -e 's|\+|\\+|g' $< >$@

