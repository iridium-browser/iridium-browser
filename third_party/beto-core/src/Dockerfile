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

FROM ubuntu:22.10

# install system deps
RUN apt-get update && apt-get install -y build-essential cmake gcc wget vim \
clang git checkinstall zlib1g-dev libjsoncpp-dev libbenchmark-dev curl \
protobuf-compiler pkg-config libdbus-1-dev libssl-dev ninja-build
RUN apt upgrade -y

# install cargo with default settings
RUN curl https://sh.rustup.rs -sSf | sh -s -- -y --default-toolchain 1.68.1
ENV PATH="/root/.cargo/bin:${PATH}"
RUN cargo install --locked cargo-deny --color never 2>&1
RUN cargo install cargo-fuzz --color never 2>&1
# unreleased PR https://github.com/ehuss/cargo-prefetch/pull/6
RUN cargo install cargo-prefetch \
    --git https://github.com/marshallpierce/cargo-prefetch.git \
    --rev f6affa68e950275f9fd773f2646ab7ee4db82897 \
    --color never 2>&1
# needed for generating boringssl bindings
# Must use 0.64.0, as version >= 0.65.0 removes the option "--size_t-is-usize", an option
# used by boringssl when generating rust bindings
RUN cargo install bindgen-cli --version 0.64.0
RUN cargo install wasm-pack --color never 2>&1
RUN rustup toolchain add nightly
# boringssl build wants go
RUN curl -L https://go.dev/dl/go1.20.2.linux-amd64.tar.gz | tar -C /usr/local -xz
ENV PATH="$PATH:/usr/local/go/bin"

# needed for boringssl git operations
RUN git config --global user.email "docker@example.com"
RUN git config --global user.name "NP Docker"

RUN mkdir -p /google
COPY . /google

WORKDIR /google/nearby

# prefetch dependencies so later build steps don't re-download on source changes
RUN cargo prefetch --lockfile Cargo.lock

# when the image runs build and test everything to ensure env is setup correctly
CMD ["cargo", "run", "--", "check-everything"]
