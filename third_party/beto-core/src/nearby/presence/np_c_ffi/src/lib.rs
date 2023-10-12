// Copyright 2023 Google LLC
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

//! NP Rust C FFI

#![cfg_attr(not(test), no_std)]
#![allow(dead_code)]
extern crate alloc;
extern crate core;

pub mod credentials;
pub mod deserialize;

/// Structure for categorized reasons for why a NP C FFI call may
/// be panicking.
#[derive(Clone, Copy)]
#[repr(u8)]
pub enum PanicReason {
    /// Some enum cast to a variant failed. Utilized
    /// for failed enum casts of all enums.
    ///
    /// (That is, this is the catch-all panic reason for enum
    /// casts where there is not a more specific reason
    /// in some other variant of this enum.)
    EnumCastFailed = 0,
    /// The panic handler is used to assert conditions are true to avoid programmer errors.
    /// If a failed assert condition is hit, this panic handler is invoked with this reason.
    AssertFailed = 1,
    /// Error returned if action bits inside of a V0Actions struct are invalid. If the struct was
    /// created by this deserializer, the bits will always be valid, they are only invalid if
    /// a user reaches in and changes them to something invalid.
    InvalidActionBits = 2,
}

/// Structure which maintains information about the panic-handler
/// for panicking calls in the NP C FFI.
#[derive(Debug)]
struct PanicHandler {
    /// Optional function-pointer to client-specified panic behavior.
    handler: Option<unsafe extern "C" fn(PanicReason) -> ()>,

    /// Fuse to prevent setting the panic-handler more than once.
    /// We do not use the presence/absence of `self.handler` for this,
    /// since it's possible for the client to explicitly pass "NULL"
    /// to set the panic-handler to the platform-default (and ensure
    /// that the panic-handler never changes from the platform-default.)
    handler_set_by_client: bool,
}

impl PanicHandler {
    pub(crate) const fn new() -> Self {
        Self { handler: None, handler_set_by_client: false }
    }
    pub(crate) fn set_handler(
        &mut self,
        handler: Option<unsafe extern "C" fn(PanicReason) -> ()>,
    ) -> bool {
        // Only allow setting the panic handler once
        if !self.handler_set_by_client {
            self.handler = handler;
            self.handler_set_by_client = true;
            true
        } else {
            false
        }
    }
    pub(crate) fn panic(&self, panic_reason: PanicReason) -> ! {
        if let Some(handler) = self.handler {
            unsafe { handler(panic_reason) }
        }
        Self::system_handler(panic_reason)
    }
    #[cfg(feature = "std")]
    fn system_handler(panic_reason: PanicReason) -> ! {
        eprintln!("NP FFI Panicked: {:?}", panic_reason);
        let backtrace = std::backtrace::Backtrace::capture();
        eprintln!("Stack trace: {}", backtrace);
        std::process::abort!();
    }
    #[cfg(not(feature = "std"))]
    #[allow(clippy::empty_loop)]
    fn system_handler(_: PanicReason) -> ! {
        // Looping is the only platform-independent thing
        // that we can really do in this scenario.
        // (Even clippy's explanation for the empty-loop
        // lint mentions platform-specific intrinsics
        // as being the only true way to avoid this
        // in a no_std environment.)
        loop {}
    }
}

static PANIC_HANDLER: spin::RwLock<PanicHandler> = spin::RwLock::new(PanicHandler::new());

pub(crate) fn panic(reason: PanicReason) -> ! {
    PANIC_HANDLER.read().panic(reason)
}

pub(crate) fn unwrap<T>(value: Option<T>, panic_reason: PanicReason) -> T {
    match value {
        Some(x) => x,
        None => panic(panic_reason),
    }
}

/// Overrides the global panic handler to be used when NP C FFI calls panic.
/// This method will only have an effect on the global panic-handler
/// the first time it's called, and this method will return `true`
/// to indicate that the panic handler was successfully set.
/// All subsequent calls to this method
/// will simply ignore the argument and return `false`.
///
/// If the passed function pointer is non-null,
/// then we will call it upon every panic,
/// followed by the default panicking behavior for
/// the platform (in the case where the user-specified
/// function does not terminate or hang the running process.)
///
/// Otherwise, we will resort to the
/// default panicking behavior for the system, which
/// is a printed stack trace followed by an abort
/// when this crate is compiled with `std`,
/// but a bare `loop { }` when this crate is compiled without.
#[no_mangle]
pub extern "C" fn np_ffi_global_config_panic_handler(
    handler: Option<unsafe extern "C" fn(PanicReason) -> ()>,
) -> bool {
    let mut panic_handler = PANIC_HANDLER.write();
    panic_handler.set_handler(handler)
}

/// Sets an override to the number of shards to employ in the NP FFI's
/// internal handle-maps, which places an upper bound on the number
/// of writing threads which may make progress at any one time
/// when concurrently accessing handles of the same type.
///
/// By default, this value will be set to 16, or in `std` environments,
/// the minimum of 16 and the number of available hardware threads.
/// A shard value override of zero will be interpreted the same
/// as this default.
///
/// Setting this value will have no effect if the handle-maps for the
/// API have already begun being used by the client code, and any
/// values set will take effect upon the first usage of _any_ non-`np_ffi_global_config_set`
/// API call.
#[no_mangle]
pub extern "C" fn np_ffi_global_config_set_num_shards(num_shards: u8) {
    np_ffi_core::common::global_config_set_num_shards(num_shards)
}

/// Sets the maximum number of active handles to credential books
/// which may be active at any one time.
/// Default value: Max value.
/// Max value: `u32::MAX - 1`.
///
/// Useful for bounding the maximum memory used by the client application
/// on credential books in constrained-memory environments.
///
/// Setting this value will have no effect if the handle-maps for the
/// API have already begun being used by the client code, and any
/// values set will take effect upon the first usage of any API
/// call utilizing credential books.
#[no_mangle]
pub extern "C" fn np_ffi_global_config_set_max_num_credential_books(max_num_credential_books: u32) {
    np_ffi_core::common::global_config_set_max_num_credential_books(max_num_credential_books)
}

/// Sets the maximum number of active handles to deserialized v0
/// advertisements which may be active at any one time.
///
/// Useful for bounding the maximum memory used by the client application
/// on v0 advertisements in constrained-memory environments.
///
/// Default value: Max value.
/// Max value: `u32::MAX - 1`.
///
/// Setting this value will have no effect if the handle-maps for the
/// API have already begun being used by the client code, and any
/// values set will take effect upon the first usage of any API
/// call which references or returns a deserialized V0 advertisement.
#[no_mangle]
pub extern "C" fn np_ffi_global_config_set_max_num_deserialized_v0_advertisements(
    max_num_deserialized_v0_advertisements: u32,
) {
    np_ffi_core::common::global_config_set_max_num_deserialized_v0_advertisements(
        max_num_deserialized_v0_advertisements,
    )
}

/// Sets the maximum number of active handles to deserialized v1
/// advertisements which may be active at any one time.
///
/// Useful for bounding the maximum memory used by the client application
/// on v1 advertisements in constrained-memory environments.
///
/// Default value: Max value.
/// Max value: `u32::MAX - 1`.
///
/// Setting this value will have no effect if the handle-maps for the
/// API have already begun being used by the client code, and any
/// values set will take effect upon the first usage of any API
/// call which references or returns a deserialized V1 advertisement.
#[no_mangle]
pub extern "C" fn np_ffi_global_config_set_max_num_deserialized_v1_advertisements(
    max_num_deserialized_v1_advertisements: u32,
) {
    np_ffi_core::common::global_config_set_max_num_deserialized_v1_advertisements(
        max_num_deserialized_v1_advertisements,
    )
}
