# Copyright 2022 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# This script contains common functions which can be used to help when building
# specific components of the beto-rust repo. To load these into your environment
# run `source ./scripts/build-script.sh` Then run the functions from root
# This can also be sourced to help when writing further build scripts

export SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

# Use to generate headers for new source code files
gen_headers() {
  set -e
  $HOME/go/bin/addlicense -c "Google LLC" -l apache -ignore=**/android/build/** -ignore=target/** \
      -ignore=**/target/** -ignore=".idea/*" -ignore=**/cmake-build/** -ignore="**/java/build/**" \
      -ignore="**/ukey2_c_ffi/cpp/build/**" .
}

# Checks the workspace 3rd party crates and makes sure they have a valid license
check_crate_licenses(){
    set -e
    cd $SCRIPT_DIR/..
    cargo deny --workspace check
}

# Checks everything in beto-rust
check_everything(){
  set -e
  cd $SCRIPT_DIR/..
  check_license_headers
  check_workspace
  check_boringssl
  check_ldt_ffi
  check_ukey2_ffi
  build_fuzzers
}

# Checks everything included in the top level workspace
check_workspace(){
  set -e
  cd $SCRIPT_DIR/..
  # ensure formatting is correct (Check for it first because it is fast compared to running tests)
  cargo fmt --check
  # make sure everything compiles
  cargo check --workspace --all-targets
  # run all the tests
  cargo test --workspace --quiet
  # ensure the docs are valid (cross-references to other code, etc)
  cargo doc --workspace --no-deps
  cargo clippy --all-targets
  cargo deny --workspace check
  # Check the build for targets without using RustCrypto dependencies
  cargo check --features=openssl --no-default-features
}

# Checks that the license auditing tool is installed and that all source files in the project contain the needed headers
check_license_headers() {
  set -e
  cd $SCRIPT_DIR/..
  # install location for those following the default instructions
  ADDLICENSE="$HOME/go/bin/addlicense"
  if [ ! -x "$ADDLICENSE" ]; then
    # if not in the default place, assume it's in PATH
    ADDLICENSE="addlicense"
  fi

  # see README for instructions on setting up addlicense tool
  if ($ADDLICENSE -h >/dev/null 2>&1); then
    echo "Add license is already installed"
  else
    echo "ERROR: addlicense tool is not installed, see instructions in README"
    exit 1
  fi

  if $ADDLICENSE -check \
      -ignore="**/android/build/**" \
      -ignore="target/**" \
      -ignore="**/target/**" \
      -ignore="**/.idea/**" \
      -ignore="**/cmake-build/**" \
      -ignore="**/java/build/**" \
      -ignore="**/java/*/build/**" \
      -ignore="**/ukey2_c_ffi/cpp/build/**" \
      .; then
    echo "License header check succeeded!"
  else
    echo "ERROR: License header missing for above files"
    exit 1
  fi
}

# Build all fuzz targets
build_fuzzers() {
  set -e
  cd $SCRIPT_DIR/..
  # rust fuzzers
  for fuzzed_crate in presence/xts_aes presence/ldt presence/ldt_np_adv connections/ukey2/ukey2_connections; do
    (cd "$fuzzed_crate" && cargo +nightly fuzz build)
  done

  # ffi fuzzers
  rm -Rf presence/ldt_np_adv_ffi_fuzz/cmake-build
  (cd presence/ldt_np_adv_ffi_fuzz && mkdir -p cmake-build && cd cmake-build && cmake ../.. -DENABLE_FUZZ=true && make)
  rm -Rf presence/ldt_np_adv_ffi_fuzz/cmake-build
}

# Builds and runs all tests for all combinations of features for the LDT FFI
check_ldt_ffi() {
  set -e
  cd $SCRIPT_DIR/..
  # We need to handle ldt_np_adv_ffi separately since it requires the nightly toolchain
  cd presence/ldt_np_adv_ffi
  cargo fmt --check
  cargo check
  # Default build, RustCrypto + no_std
  cargo build --release
  # Turn on std, still using RustCrypto
  cargo build --features=std
  # Turn off default features and try to build with std
  cargo build --no-default-features --features=std
  # Turn off RustCrypto and use openssl
  cargo build --no-default-features --features=openssl
  # Turn off RustCrypto and use boringssl
  cargo --config .cargo/config-boringssl.toml build --no-default-features --features=boringssl
  cargo doc --no-deps
  cargo clippy --release
  cargo clippy --features=std
  cargo clippy --no-default-features --features=openssl
  cargo clippy --no-default-features --features=std
  cargo deny check
  cd ../

  # build C/C++ samples, tests, and benches
  mkdir -p cmake-build && cd cmake-build
  cmake .. -DENABLE_TESTS=true
  make

  # test with default build settings (rustcrypto, no_std)
  echo "Testing default features (no_std + rustcrypto)"
  (cd ../ldt_np_adv_ffi && cargo build --release)
  (cd ldt_np_c_sample/tests && ctest)

  # test with std
  echo "Testing std feature flag"
  (cd ../ldt_np_adv_ffi && cargo build --features std --release)
  (cd ldt_np_c_sample/tests && make && ctest)

  # test with boringssl crypto feature flag
  echo "Testing boringssl"
  (cd ../ldt_np_adv_ffi && cargo --config .cargo/config-boringssl.toml build --no-default-features --features boringssl --release)
  (cd ldt_np_c_sample/tests && make && ctest)

  # test with openssl feature flag
  echo "Testing openssl"
  (cd ../ldt_np_adv_ffi && cargo build --no-default-features --features openssl --release)
  (cd ldt_np_c_sample/tests && make && ctest)

  # test with std feature flag
  echo "Testing std with no default features"
  (cd ../ldt_np_adv_ffi && cargo build --no-default-features --features std --release)
  (cd ldt_np_c_sample/tests && make && ctest)
  cd ../
}

# Builds and runs tests for the UKEY2 FFI
check_ukey2_ffi() {
  set -e
  cd $SCRIPT_DIR/..
  cd connections/ukey2/ukey2_c_ffi
  # Default build, RustCrypto
  cargo build --release --lib
  # Try to build with OpenSSL
  cargo build --no-default-features --features=openssl
  cargo doc --no-deps
  cargo clippy --release
  cargo clippy --no-default-features --features=openssl
  cargo deny check

  # build C/C++ samples, tests, and benches
  cd cpp
  mkdir -p build && cd build
  cmake ..
  make all
  ctest

  cd $SCRIPT_DIR/..
}

# Clones boringssl and uses bindgen to generate the rust crate, applies AOSP
# specific patches to the 3p `openssl` crate so that it can use a bssl backend
prepare_boringssl() {
  set -e
  cd $SCRIPT_DIR/../..
  projectroot=$PWD
  mkdir -p boringssl-build && cd boringssl-build

  if ! git -C boringssl pull origin master; then
    git clone https://boringssl.googlesource.com/boringssl
  fi
  # Snap to the AOSP commit of boringssl
   boringssl_rev=$(curl https://android.googlesource.com/platform/external/boringssl/+/master/BORINGSSL_REVISION?format=text | base64 -d)
  cd boringssl && git checkout $boringssl_rev && mkdir -p build && cd build
  target=$(rustc -vV | awk '/host/ { print $2 }')
  cmake -G Ninja .. -DRUST_BINDINGS="$target" && ninja
  # The Rust crate is in `boringssl-build/boringssl/build/rust/bssl-sys`, which depends on a
  # cmake-generated file as part of its source.

  cd $projectroot/boringssl-build
  rm -Rf rust-openssl
  git clone https://github.com/sfackler/rust-openssl.git
  git -C rust-openssl checkout 11797d9ecb73e94b7f55a49274318abc9dc074d2
  git -C rust-openssl branch -f BASE_COMMIT
  git -C rust-openssl am $projectroot/nearby/scripts/openssl-patches/*.patch

  cd $projectroot/nearby

  cat <<'EOF' >&2
==========
Preparation complete. The required repositories are downloaded to `beto-rust/boringssl-build`. If
you need to go back to a clean state, you can remove that directory and rerun this script.

You can now build and test with boringssl using the following command
  `cargo --config .cargo/config-boringssl.toml test -p crypto_provider* --features=boringssl,std`
==========
EOF
  echo
}

# Checks the build and tests for all boringssl related deps
# crypto_provider_openssl is used on AOSP
# crypto_provider_boringssl is used on Chromium
# And we want to verify that both of these are tested in our own repo
check_boringssl() {
  set -e
  cd $SCRIPT_DIR/../..
  # clones boringssl and uses bindgen to generate the sys bindings
  prepare_boringssl

  # test the openssl crate with the boringssl feature
  cargo --config .cargo/config-boringssl.toml test -p crypto_provider_openssl --features=boringssl

  # test the crypto_provider built on the new bssl crate
  cd crypto/crypto_provider_boringssl
  cargo check
  cargo fmt --check
  cargo clippy --all-targets
  cargo test
  cargo doc --no-deps
  cd ../../
}

# Helper for setting up dependencies on the build machine
setup_kokoro_macos () {
  set -e
  go install github.com/google/addlicense@latest
  curl https://sh.rustup.rs -sSf | sh -s -- -y --no-modify-path --default-toolchain 1.68.0
  cargo install --locked cargo-deny --color never 2>&1
  # Must use this version, as version >= 0.65.0 removes the option "--size_t-is-usize", an option
  # used by boringssl when generating rust bindings
  cargo install --version 0.64.0 bindgen-cli
  source "$HOME/.cargo/env"
  rustup install nightly
  brew install google-benchmark ninja jsoncpp

  # Unfortunately CMake is not smart enough to find this on its own, even though
  # it is in fact there by default on the build machines
  export OPENSSL_ROOT_DIR="/usr/local/opt/openssl@3"
}
