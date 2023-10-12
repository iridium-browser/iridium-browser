// Copyright 2022 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

extern crate core;

use anyhow::anyhow;
use crypto_provider::CryptoProvider;
use crypto_provider_default::CryptoProviderImpl;
use ldt_tbc::TweakableBlockCipherDecrypter;
use ldt_tbc::TweakableBlockCipherEncrypter;
use ldt_tbc::TweakableBlockCipherKey;
use std::{collections::hash_map, fs, io, io::BufRead as _};
use xts_aes::{self, XtsAes128Key, XtsAes256Key, XtsDecrypter, XtsEncrypter, XtsKey};

#[test]
fn nist_test_vectors_data_unit_seq_128() -> Result<(), anyhow::Error> {
    run_test_cases::<XtsAes128Key, <CryptoProviderImpl as CryptoProvider>::Aes128>(
        "format tweak value input - data unit seq no/XTSGenAES128.rsp",
        // not 1000 because we skip test cases with data that isn't a multiple of 8 bits
        800,
    )
}

#[test]
fn nist_test_vectors_data_unit_seq_256() -> Result<(), anyhow::Error> {
    run_test_cases::<XtsAes256Key, <CryptoProviderImpl as CryptoProvider>::Aes256>(
        "format tweak value input - data unit seq no/XTSGenAES256.rsp",
        600,
    )
}

#[test]
fn nist_test_vectors_hex_tweak_128() -> Result<(), anyhow::Error> {
    run_test_cases::<XtsAes128Key, <CryptoProviderImpl as CryptoProvider>::Aes128>(
        "format tweak value input - 128 hex str/XTSGenAES128.rsp",
        800,
    )
}

#[test]
fn nist_test_vectors_hex_tweak_256() -> Result<(), anyhow::Error> {
    run_test_cases::<XtsAes256Key, <CryptoProviderImpl as CryptoProvider>::Aes256>(
        "format tweak value input - 128 hex str/XTSGenAES256.rsp",
        600,
    )
}

fn run_test_cases<K, A>(path: &str, expected_num_cases: usize) -> Result<(), anyhow::Error>
where
    K: XtsKey + TweakableBlockCipherKey,
    A: crypto_provider::aes::Aes<Key = K::BlockCipherKey>,
{
    let test_cases = parse_test_vector(path)?;
    let len = test_cases.len();

    let mut buf = Vec::new();
    for tc in test_cases {
        buf.clear();

        let xts_enc = XtsEncrypter::<A, _>::new(&K::try_from(tc.key.as_slice()).unwrap());
        let xts_dec = XtsDecrypter::<A, _>::new(&K::try_from(tc.key.as_slice()).unwrap());

        match tc.test_type {
            TestType::Encrypt => {
                buf.extend_from_slice(&tc.plaintext);
                xts_enc.encrypt_data_unit(tc.tweak.clone(), &mut buf).unwrap();

                // check decryption too just for fun
                xts_dec.decrypt_data_unit(tc.tweak.clone(), &mut buf).unwrap();
                assert_eq!(tc.plaintext, buf, "count {}", tc.count);
            }
            TestType::Decrypt => {
                buf.extend_from_slice(&tc.ciphertext);
                xts_dec.decrypt_data_unit(tc.tweak.clone(), &mut buf).unwrap();
                assert_eq!(tc.plaintext, buf, "count {}", tc.count);

                xts_enc.encrypt_data_unit(tc.tweak.clone(), &mut buf).unwrap();
                assert_eq!(tc.ciphertext, buf, "count {}", tc.count);
            }
        }
    }

    assert_eq!(expected_num_cases, len);
    Ok(())
}

fn parse_test_vector(path: &str) -> Result<Vec<TestCase>, anyhow::Error> {
    let mut full_path =
        test_helper::get_data_file("presence/xts_aes/resources/test/nist-test-vectors/");
    full_path.push(path);

    let file = fs::File::open(&full_path)?;

    // The file is annoyingly stateful, so we have to know what test type we've most
    // recently seen
    let mut test_type = None;
    let mut test_cases = Vec::new();
    for parse_unit in
        (TestVectorFileIterator { delegate: io::BufReader::new(file).lines().map(|r| r.unwrap()) })
    {
        match parse_unit {
            ParseUnit::SectionHeader(s) => {
                test_type = Some(match s.as_str() {
                    "ENCRYPT" => TestType::Encrypt,
                    "DECRYPT" => TestType::Decrypt,
                    _ => panic!("Unknown test type {s}"),
                })
            }
            ParseUnit::Chunk(map) => {
                if let Some(t) = test_type {
                    let data_unit_len: usize =
                        map.get("DataUnitLen").and_then(|s| s.parse().ok()).unwrap();
                    let plaintext = map
                        .get("PT")
                        .ok_or_else(|| anyhow!("missing pt"))
                        .and_then(|s| hex::decode(s).map_err(|e| e.into()))?;
                    let ciphertext = map
                        .get("CT")
                        .ok_or_else(|| anyhow!("missing ct"))
                        .and_then(|s| hex::decode(s).map_err(|e| e.into()))?;

                    // Skip test cases that aren't in multiples of 8 bits
                    if data_unit_len % 8 != 0 {
                        continue;
                    }

                    assert_eq!(data_unit_len, plaintext.len() * 8);
                    assert_eq!(data_unit_len, ciphertext.len() * 8);
                    let data_unit_seq_num: Option<xts_aes::Tweak> = map
                        .get("DataUnitSeqNumber")
                        .and_then(|s| s.parse::<u128>().ok())
                        .map(|t| t.into());
                    let tweak: Option<xts_aes::Tweak> = map
                        .get("i")
                        .and_then(|s| hex::decode(s).ok())
                        .and_then(|s| s.try_into().ok())
                        .map(|arr: crypto_provider::aes::AesBlock| arr.into());

                    test_cases.push(TestCase {
                        count: map.get("COUNT").unwrap().parse()?,
                        key: map
                            .get("Key")
                            .ok_or_else(|| anyhow!("missing key"))
                            .and_then(|s| hex::decode(s).map_err(|e| e.into()))?,
                        tweak: data_unit_seq_num
                            .or(tweak)
                            .expect("data unit seq or tweak must be present"),
                        plaintext,
                        ciphertext,
                        test_type: t,
                    });
                } else {
                    panic!("No test type set yet");
                }
            }
        }
    }

    Ok(test_cases)
}

#[derive(Debug, Copy, Clone, PartialEq, Eq)]
enum TestType {
    Encrypt,
    Decrypt,
}

struct TestCase {
    count: u32,
    key: Vec<u8>,
    tweak: xts_aes::Tweak,
    plaintext: Vec<u8>,
    ciphertext: Vec<u8>,
    test_type: TestType,
}

#[derive(Debug)]
enum ParseUnit {
    SectionHeader(String),
    Chunk(hash_map::HashMap<String, String>),
}

struct TestVectorFileIterator<I: Iterator<Item = String>> {
    delegate: I,
}
impl<I: Iterator<Item = String>> Iterator for TestVectorFileIterator<I> {
    type Item = ParseUnit;

    fn next(&mut self) -> Option<Self::Item> {
        let mut map = hash_map::HashMap::new();
        while let Some(line) = self.delegate.next().map(|l| l.trim().to_owned()) {
            if line.starts_with('#') {
                continue;
            }

            if line.is_empty() {
                if map.is_empty() {
                    // not currently in a chunk, so blank lines can be skipped
                    continue;
                } else {
                    // we were in a chunk and now it's done
                    let res = Some(ParseUnit::Chunk(map.clone()));
                    map.clear();
                    return res;
                }
            }

            // look for `[ENCRYPT]` / `[DECRYPT]`
            if let Some(captures) = regex::Regex::new("^\\[(.*)\\]$").unwrap().captures(&line) {
                return Some(ParseUnit::SectionHeader(
                    captures.get(1).unwrap().as_str().to_owned(),
                ));
            }

            // `key = value` in a test case chunk
            if let Some(captures) = regex::Regex::new("^(.*) = (.*)$").unwrap().captures(&line) {
                map.insert(
                    captures.get(1).unwrap().as_str().to_owned(),
                    captures.get(2).unwrap().as_str().to_owned(),
                );
            }
        }

        None
    }
}
