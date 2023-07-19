// Copyright 2023 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

use openssl::hash::MessageDigest;
use openssl::md::{Md, MdRef};
use openssl::md_ctx::MdCtx;

use crate::OpenSslHash;

/// openssl sha256 digest algorithm
#[derive(Clone)]
pub struct OpenSslSha256 {}

impl OpenSslHash for OpenSslSha256 {
    fn get_digest() -> MessageDigest {
        MessageDigest::sha256()
    }

    fn get_md() -> &'static MdRef {
        Md::sha256()
    }
}

impl crypto_provider::sha2::Sha256 for OpenSslSha256 {
    fn sha256(input: &[u8]) -> [u8; 32] {
        let mut mdctx = MdCtx::new().unwrap();
        mdctx.digest_init(Self::get_md()).unwrap();
        mdctx.digest_update(input).unwrap();
        let mut buf = [0_u8; 32];
        mdctx.digest_final(&mut buf).unwrap();
        buf
    }
}

/// openssl sha512 digest algorithm
#[derive(Clone)]
pub struct OpenSslSha512 {}

impl OpenSslHash for OpenSslSha512 {
    fn get_digest() -> MessageDigest {
        MessageDigest::sha512()
    }

    fn get_md() -> &'static MdRef {
        Md::sha512()
    }
}

impl crypto_provider::sha2::Sha512 for OpenSslSha512 {
    fn sha512(input: &[u8]) -> [u8; 64] {
        let mut mdctx = MdCtx::new().unwrap();
        mdctx.digest_init(Self::get_md()).unwrap();
        mdctx.digest_update(input).unwrap();
        let mut buf = [0_u8; 64];
        mdctx.digest_final(&mut buf).unwrap();
        buf
    }
}
