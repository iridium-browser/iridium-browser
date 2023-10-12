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

extern crate alloc;
extern crate std;

use alloc::borrow::ToOwned;
use alloc::string::String;
use alloc::vec::Vec;
use crypto_provider::ed25519::{Ed25519Provider, KeyPair, PublicKey, Signature};
use wycheproof::TestResult;

// These are test vectors from the creators of Ed25519: https://ed25519.cr.yp.to/ which are referenced
// as the SOT for the test vectors in the RFC: https://www.rfc-editor.org/rfc/rfc8032#section-7.1
// The vectors have been formatted into a easily parsable/readable format by libgcrypt which is
// also used for test cases in the above RFC:
// https://dev.gnupg.org/source/libgcrypt/browse/master/tests/t-ed25519.inp
const PATH_TO_RFC_VECTORS_FILE: &str =
    "crypto/crypto_provider_test/src/testdata/ecdsa/rfc_test_vectors.txt";

/// Runs set of Ed25519 wycheproof test vectors against a provided ed25519 implementation
/// Tests vectors from Project Wycheproof: <https://github.com/google/wycheproof>
pub fn run_wycheproof_test_vectors<E>()
where
    E: Ed25519Provider,
{
    let test_set = wycheproof::eddsa::TestSet::load(wycheproof::eddsa::TestName::Ed25519)
        .expect("should be able to load test set");

    for test_group in test_set.test_groups {
        let key_pair = test_group.key;
        let public_key = key_pair.pk;
        let secret_key = key_pair.sk;

        for test in test_group.tests {
            let tc_id = test.tc_id;
            let comment = test.comment;
            let sig = test.sig;
            let msg = test.msg;

            let valid = match test.result {
                TestResult::Invalid => false,
                TestResult::Valid | TestResult::Acceptable => true,
            };
            let result =
                run_test::<E>(public_key.clone(), secret_key.clone(), sig.clone(), msg.clone());
            if valid {
                if let Err(desc) = result {
                    panic!(
                        "\n\
                         Failed test {}: {}\n\
                         msg:\t{:?}\n\
                         sig:\t{:?}\n\
                         comment:\t{:?}\n",
                        tc_id, desc, msg, sig, comment,
                    );
                }
            } else {
                assert!(result.is_err())
            }
        }
    }
}

/// Runs the RFC specified test vectors against an Ed25519 implementation
pub fn run_rfc_test_vectors<E>()
where
    E: Ed25519Provider,
{
    let file_contents =
        std::fs::read_to_string(test_helper::get_data_file(PATH_TO_RFC_VECTORS_FILE))
            .expect("should be able to read file");

    let mut split_cases: Vec<&str> = file_contents.as_str().split("\n\n").collect();
    // remove the comments
    split_cases.remove(0);
    for case in split_cases {
        let test_case: Vec<&str> = case.split('\n').collect();

        let tc_id = extract_string(test_case[0]);
        let sk = extract_hex(test_case[1]);
        let pk = extract_hex(test_case[2]);
        let msg = extract_hex(test_case[3]);
        let sig = extract_hex(test_case[4]);

        let result = run_test::<E>(pk.clone(), sk.clone(), sig.clone(), msg.clone());
        if let Err(desc) = result {
            panic!(
                "\n\
                         Failed test {}: {}\n\
                         msg:\t{:?}\n\
                         sig:\t{:?}\n\"",
                tc_id, desc, msg, sig,
            );
        }
    }
}

fn extract_hex(line: &str) -> Vec<u8> {
    test_helper::string_to_hex(extract_string(line).as_str())
}

fn extract_string(line: &str) -> String {
    line.split(':').collect::<Vec<&str>>()[1].trim().to_owned()
}

fn run_test<E>(
    expected_pub_key: Vec<u8>,
    private_key: Vec<u8>,
    sig: Vec<u8>,
    msg: Vec<u8>,
) -> Result<(), &'static str>
where
    E: Ed25519Provider,
{
    let private_key_bytes: [u8; 32] =
        private_key.as_slice().try_into().expect("Secret key is the wrong length");
    let kp = E::KeyPair::from_private_key(&private_key_bytes);

    let sig_result = kp.sign(msg.as_slice());
    (sig.as_slice() == sig_result.to_bytes()).then_some(()).ok_or("sig not matching expected")?;
    let signature = E::Signature::from_bytes(
        sig.as_slice().try_into().expect("Test signature should be the correct length"),
    );

    let pub_key = kp.public();
    assert_eq!(pub_key.to_bytes().as_slice(), expected_pub_key.as_slice());
    pub_key.verify_strict(msg.as_slice(), &signature).map_err(|_| "verify failed")?;

    Ok(())
}
