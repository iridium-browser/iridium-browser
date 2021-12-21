./autogen.sh &&
export CPPFLAGS="-I/$HOME/build/win$ARCH/libyaml/include/" &&
export LDFLAGS="-L/$HOME/build/win$ARCH/libyaml/lib/" &&
./configure --host $(test $ARCH == 32 && echo i586-mingw32msvc || echo x86_64-w64-mingw32) \
            $ENABLE_UCS4 --with-yaml --prefix=$(pwd)/win$ARCH-install &&
make LDFLAGS="$LDFLAGS -avoid-version -Xcompiler -static-libgcc"

# FIXME:
# make CPPFLAGS='-I/$HOME/build/win32/libyaml/include/' \
#      LDFLAGS='-L/$HOME/build/win32/libyaml/lib/' \
#      distwin32
