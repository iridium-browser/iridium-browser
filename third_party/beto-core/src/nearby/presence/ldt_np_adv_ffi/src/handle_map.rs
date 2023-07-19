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

use crate::Box;
use crate::LdtAdvDecrypter;
use crate::LdtAdvEncrypter;
use core::marker::PhantomData;
use crypto_provider::{CryptoProvider, CryptoRng};
use lazy_static::lazy_static;

// Pull in the needed deps for std vs no_std
cfg_if::cfg_if! {
    // Test pulls in std which causes duplicate errors
    if #[cfg(any(feature = "std", test, feature = "boringssl", feature = "openssl"))] {
        use std::sync::{Mutex, MutexGuard};
        use std::collections::HashMap;

        /// Trait for implementing the map used to store valid handles for accessing the LDT Adv ciphers
        pub(crate) struct HandleMap<T> {
            _marker: PhantomData<T>,
            map: HashMap<u64, T>,
        }

        impl<T> HandleMap<T> {
            /// initialized the map
            pub(crate) fn init() -> Self {
                Self {
                    _marker: Default::default(),
                    map: HashMap::new(),
                }
            }
        }

        // returns a thread safe instance of the global static hashmap tracking the cipher handles
        pub (crate) fn get_enc_handle_map() -> MutexGuard<'static, HandleMap<Box<LdtAdvEncrypter>>> {
            // Note even in the case of an error, the lock is still acquired, it just means whichever
            // thread was holding it panicked, we will continue on as either way we acquired the lock
            ENCRYPT_HANDLE_MAP
                .lock()
                .unwrap_or_else(|err_guard| err_guard.into_inner())
        }

        // returns a thread safe instance of the global static hashmap tracking the cipher handles
        pub (crate) fn get_dec_handle_map() -> MutexGuard<'static, HandleMap<Box<LdtAdvDecrypter>>> {
            // Note even in the case of an error, the lock is still acquired, it just means whichever
            // thread was holding it panicked, we will continue on as either way we acquired the lock
            DECRYPT_HANDLE_MAP
                .lock()
                .unwrap_or_else(|err_guard| err_guard.into_inner())
        }
    } else {
        use spin::{Mutex, MutexGuard};
        use  alloc::collections::BTreeMap;
        /// Trait for implementing the map used to store valid handles for accessing the LDT Adv ciphers
        pub(crate) struct HandleMap<T> {
            _marker: PhantomData<T>,
            map: BTreeMap<u64, T>,
        }

        impl<T> HandleMap<T> {
            /// initialized the map
            pub(crate) fn init() -> Self {
                Self {
                    _marker: Default::default(),
                    map: BTreeMap::new(),
                }
            }
        }

        // returns a thread safe instance of the global static hashmap tracking the cipher handles
        pub (crate) fn get_enc_handle_map() -> MutexGuard<'static, HandleMap<Box<LdtAdvEncrypter>>> {
            ENCRYPT_HANDLE_MAP.lock()
        }

        // returns a thread safe instance of the global static hashmap tracking the cipher handles
        pub (crate) fn get_dec_handle_map() -> MutexGuard<'static, HandleMap<Box<LdtAdvDecrypter>>> {
            DECRYPT_HANDLE_MAP.lock()
        }
    }
}

// Global hashmap to track valid pointers, this is a safety precaution to make sure we are not
// reading from unsafe memory address's passed in by caller.
lazy_static! {
    static ref ENCRYPT_HANDLE_MAP: Mutex<HandleMap<Box<LdtAdvEncrypter>>> =
        Mutex::new(HandleMap::init());
    static ref DECRYPT_HANDLE_MAP: Mutex<HandleMap<Box<LdtAdvDecrypter>>> =
        Mutex::new(HandleMap::init());
}

impl<T> HandleMap<T> {
    /// inserts an entry into the map and returns the randomly generated handle to the entry
    pub(crate) fn insert<C: CryptoProvider>(&mut self, data: T) -> u64 {
        let mut rng = C::CryptoRng::new();
        let mut handle: u64 = rng.next_u64();

        while self.map.contains_key(&handle) {
            handle = rng.next_u64();
        }

        // insert should always be None since we checked above that the key does not already exist
        assert!(self.map.insert(handle, data).is_none());
        handle
    }

    /// Removes an entry at a given handle returning an Option of the owned value
    pub(crate) fn remove(&mut self, handle: &u64) -> Option<T> {
        self.map.remove(handle)
    }

    /// Gets a reference to the entry stored at the specified handle
    pub(crate) fn get(&self, handle: &u64) -> Option<&T> {
        self.map.get(handle)
    }
}
