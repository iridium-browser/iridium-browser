source ./.github/workflows/scripts/emscripten-build-command.sh &&
mkdir out &&

echo "[liblouis-js] starting build process in docker image..." &&

./autogen.sh &&

echo "[liblouis-js] configuring and making UTF-16 builds..." &&
emconfigure ./configure --disable-shared &&
emmake make &&
# install to obtain a table folder which does not contain build scripts
emmake make install prefix="$(pwd)/out-emscripten-install"

#buildjs "16" "build-no-tables-utf16.js" &&
#buildjs "16" "build-no-tables-wasm-utf16.js" "-s WASM=1" &&
#buildjs "16" "build-tables-embeded-root-utf16.js" "--embed-files ./out-emscripten-install/share/liblouis/tables@/" &&

echo "[liblouis-js] configuring and making UTF-32 builds..." &&
emconfigure ./configure --enable-ucs4 --disable-shared &&
emmake make &&

echo "[liblouis-js] building UTF-32 with no tables..." &&
buildjs "32" "build-no-tables-utf32.js" &&
#buildjs "32" "build-no-tables-wasm-utf32.js" "-s WASM=1" &&
#buildjs "32" "build-tables-embeded-root-utf32.js" "--embed-files ./out-emscripten-install/share/liblouis/tables@/" &&

echo "[liblouis-js] done building in docker image..."
