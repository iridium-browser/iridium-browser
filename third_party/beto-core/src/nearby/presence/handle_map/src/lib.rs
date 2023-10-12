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
//! A thread-safe implementation of a map for managing object handles,
//! a safer alternative to raw pointers for FFI interop.
#![cfg_attr(not(test), no_std)]
#![forbid(unsafe_code)]
#![deny(missing_docs)]
extern crate alloc;
extern crate core;

use alloc::boxed::Box;
use alloc::vec::Vec;
use core::ops::{Deref, DerefMut};
use core::sync::atomic::Ordering;
use hashbrown::hash_map::EntryRef;
use portable_atomic::{AtomicU32, AtomicU64};
use spin::lock_api::*;

#[cfg(test)]
mod tests;

/// A RAII read lock guard for an object in a [`HandleMap`]
/// pointed-to by a given [`Handle`]. When this struct is
/// dropped, the underlying read lock on the associated
/// shard will be dropped.
pub struct ObjectReadGuard<'a, T> {
    guard: lock_api::MappedRwLockReadGuard<'a, spin::RwLock<()>, T>,
}

impl<'a, T> Deref for ObjectReadGuard<'a, T> {
    type Target = T;

    fn deref(&self) -> &Self::Target {
        self.guard.deref()
    }
}

/// A RAII read-write lock guard for an object in a [`HandleMap`]
/// pointed-to by a given [`Handle`]. When this struct is
/// dropped, the underlying read-write lock on the associated
/// shard will be dropped.
pub struct ObjectReadWriteGuard<'a, T> {
    guard: lock_api::MappedRwLockWriteGuard<'a, spin::RwLock<()>, T>,
}

impl<'a, T> Deref for ObjectReadWriteGuard<'a, T> {
    type Target = T;

    fn deref(&self) -> &Self::Target {
        self.guard.deref()
    }
}

impl<'a, T> DerefMut for ObjectReadWriteGuard<'a, T> {
    fn deref_mut(&mut self) -> &mut Self::Target {
        self.guard.deref_mut()
    }
}

/// FFI-transmissible structure expressing the dimensions
/// (max # of allocatable slots, number of shards) of a handle-map
/// to be used upon initialization.
#[repr(C)]
#[derive(Clone, Copy)]
pub struct HandleMapDimensions {
    /// The number of shards which are employed
    /// by the associated handle-map.
    pub num_shards: u8,
    /// The maximum number of active handles which may be
    /// stored within the associated handle-map.
    pub max_active_handles: u32,
}

/// Trait for marker structs containing information about a
/// given [`SingletonHandleMap`], including the underlying
/// object type and the dimensions of the map.
/// Used primarily to enforce type-level constraints
/// on `SingletoneHandleMap`s, including
/// ensuring that they are truly singletons.
pub trait SingletonHandleMapInfo {
    /// The underlying kind of object pointed-to by handles
    /// derived from the associated handle-map.
    type Object: Send + Sync;

    /// Gets the dimensions of the corresponding handle-map.
    fn dimensions(&self) -> HandleMapDimensions;

    /// Gets a static reference to the global handle-map
    fn get_handle_map() -> &'static HandleMap<Self::Object>;
}

/// An individual handle to be given out by a [`HandleMap`].
/// This representation is untyped, and just a wrapper
/// around a handle-id, in contrast to implementors of `HandleLike`.
#[derive(Clone, Copy, PartialEq, Eq, Hash)]
pub struct Handle {
    handle_id: u64,
}

impl From<&Handle> for Handle {
    fn from(handle: &Handle) -> Self {
        *handle
    }
}

impl Handle {
    /// Constructs a handle wrapping the given ID.
    ///
    /// No validity checks are done on the wrapped ID
    /// to ensure that the given ID is active in
    /// any specific handle-map, and the type
    /// of the handle is not represented.
    ///
    /// As a result, this method is only useful for
    /// allowing access to handles from an FFI layer.
    pub fn from_id(handle_id: u64) -> Self {
        Self { handle_id }
    }
    /// Gets the ID for this handle.
    ///
    /// Since the underlying handle is un-typed,`
    /// this method is only suitable for
    /// transmitting handles across an FFI layer.
    pub fn get_id(&self) -> u64 {
        self.handle_id
    }
    /// Derives the shard index from the handle id
    fn get_shard_index(&self, num_shards: u8) -> usize {
        (self.handle_id % (num_shards as u64)) as usize
    }
}

/// Error raised when attempting to allocate into a full handle-map.
#[derive(Debug)]
pub struct HandleMapFullError;

/// Internal error enum for failed allocations into a given shard.
enum ShardAllocationError<T, F: FnOnce() -> T> {
    /// Error for when the entry for the handle is occupied,
    /// in which case we spit out the object-provider to try again
    /// with a new handle-id.
    EntryOccupied(F),
    /// Error for when we would exceed the maximum number of allocations.
    ExceedsAllocationLimit,
}

/// Error raised when the entry for a given [`Handle`] doesn't exist.
#[derive(Debug)]
pub struct HandleNotPresentError;

/// Wrapper around a `HandleMap` which is specifically tailored for maintaining
/// a global singleton `'static` reference to a handle-map.
/// The wrapper handles initialization of the singleton in a const-context
/// and subsequent accesses to ensure that the underlying map is always
/// initialized upon attempts to access it.
pub struct SingletonHandleMap<I: SingletonHandleMapInfo> {
    info: I,
    wrapped: spin::once::Once<HandleMap<I::Object>, spin::relax::Spin>,
}

impl<I: SingletonHandleMapInfo> SingletonHandleMap<I> {
    /// Constructs a new global handle-map using the given
    /// singleton handle-map info. This method is callable
    /// from a `const`-context to allow it to be used to initialize
    /// `static` variables.
    pub const fn with_info(info: I) -> Self {
        Self { info, wrapped: spin::once::Once::new() }
    }
    /// Initialize the handle-map if it's not already initialized,
    /// and return a reference to the result.
    pub fn get(&self) -> &HandleMap<I::Object> {
        self.wrapped.call_once(|| {
            let dimensions = self.info.dimensions();
            HandleMap::with_dimensions(dimensions)
        })
    }
}

/// A thread-safe mapping from "handle"s [like pointers, but safer]
/// to underlying structures, supporting allocations, reads, writes,
/// and deallocations of objects behind handles.
pub struct HandleMap<T: Send + Sync> {
    /// The dimensions of this handle-map
    dimensions: HandleMapDimensions,

    /// The individually-lockable "shards" of the handle-map,
    /// among which the keys will be roughly uniformly-distributed.
    handle_map_shards: Box<[HandleMapShard<T>]>,

    /// An atomically-incrementing counter which tracks the
    /// next handle ID which allocations will attempt to use.
    new_handle_id_counter: AtomicU64,

    /// An atomic integer roughly tracking the number of
    /// currently-outstanding allocated entries in this
    /// handle-map among all [`HandleMapShard`]s.
    outstanding_allocations_counter: AtomicU32,
}

impl<T: Send + Sync> HandleMap<T> {
    /// Creates a new handle-map with the given `HandleMapDimensions`.
    pub fn with_dimensions(dimensions: HandleMapDimensions) -> Self {
        let mut handle_map_shards = Vec::with_capacity(dimensions.num_shards as usize);
        for _ in 0..dimensions.num_shards {
            handle_map_shards.push(HandleMapShard::default());
        }
        let handle_map_shards = handle_map_shards.into_boxed_slice();
        Self {
            dimensions,
            handle_map_shards,
            new_handle_id_counter: AtomicU64::new(0),
            outstanding_allocations_counter: AtomicU32::new(0),
        }
    }
}

impl<T: Send + Sync> HandleMap<T> {
    /// Allocates a new object within the given handle-map, returning
    /// a handle to the location it was stored at. This operation
    /// may fail if attempting to allocate over the `dimensions.max_active_handles`
    /// limit imposed on the handle-map, in which case this method
    /// will return a `HandleMapFullError`.
    pub fn allocate(
        &self,
        initial_value_provider: impl FnOnce() -> T,
    ) -> Result<Handle, HandleMapFullError> {
        let mut initial_value_provider = initial_value_provider;
        loop {
            // Increment the new-handle-ID counter using relaxed memory ordering,
            // since the only invariant that we want to enforce is that concurrently-running
            // threads always get distinct new handle-ids.
            let new_handle_id = self.new_handle_id_counter.fetch_add(1, Ordering::Relaxed);
            let new_handle = Handle::from_id(new_handle_id);
            let shard_index = new_handle.get_shard_index(self.dimensions.num_shards);

            // Now, check the shard to see if we can actually allocate into it.
            let shard_allocate_result = self.handle_map_shards[shard_index].try_allocate(
                new_handle,
                initial_value_provider,
                &self.outstanding_allocations_counter,
                self.dimensions.max_active_handles,
            );
            match shard_allocate_result {
                Ok(_) => {
                    return Ok(new_handle);
                }
                Err(ShardAllocationError::ExceedsAllocationLimit) => {
                    return Err(HandleMapFullError);
                }
                Err(ShardAllocationError::EntryOccupied(thrown_back_provider)) => {
                    // We need to do the whole thing again with a new ID
                    initial_value_provider = thrown_back_provider;
                }
            }
        }
    }

    /// Gets a read-only reference to an object within the given handle-map,
    /// if the given handle is present. Otherwise, returns [`HandleNotPresentError`].
    pub fn get(&self, handle: Handle) -> Result<ObjectReadGuard<T>, HandleNotPresentError> {
        let shard_index = handle.get_shard_index(self.dimensions.num_shards);
        self.handle_map_shards[shard_index].get(handle)
    }

    /// Gets a read+write reference to an object within the given handle-map,
    /// if the given handle is present. Otherwise, returns [`HandleNotPresentError`].
    pub fn get_mut(
        &self,
        handle: Handle,
    ) -> Result<ObjectReadWriteGuard<T>, HandleNotPresentError> {
        let shard_index = handle.get_shard_index(self.dimensions.num_shards);
        self.handle_map_shards[shard_index].get_mut(handle)
    }

    /// Removes the object pointed to by the given handle in
    /// the handle-map, returning the removed object if it
    /// exists. Otherwise, returns [`HandleNotPresentError`].
    pub fn deallocate(&self, handle: Handle) -> Result<T, HandleNotPresentError> {
        let shard_index = handle.get_shard_index(self.dimensions.num_shards);
        self.handle_map_shards[shard_index]
            .deallocate(handle, &self.outstanding_allocations_counter)
    }

    /// Gets the actual number of elements stored in the entire map.
    /// Only suitable for single-threaded sections of tests.
    #[cfg(test)]
    pub(crate) fn len(&self) -> usize {
        self.handle_map_shards.iter().map(|s| s.len()).sum()
    }

    /// Sets the new-handle-id counter to the given value.
    /// Only suitable for tests.
    #[cfg(test)]
    pub(crate) fn set_new_handle_id_counter(&mut self, value: u64) {
        self.new_handle_id_counter = AtomicU64::new(value);
    }
}

// Bunch o' type aliases to make talking about them much easier in the shard code.
type ShardMapType<T> = hashbrown::HashMap<Handle, T>;
type ShardReadWriteLock<T> = RwLock<ShardMapType<T>>;
type ShardReadGuard<'a, T> = RwLockReadGuard<'a, ShardMapType<T>>;
type ShardUpgradableReadGuard<'a, T> = RwLockUpgradableReadGuard<'a, ShardMapType<T>>;
type ShardReadWriteGuard<'a, T> = RwLockWriteGuard<'a, ShardMapType<T>>;

/// An individual handle-map shard, which is ultimately
/// just a hash-map behind a lock.
pub(crate) struct HandleMapShard<T: Send + Sync> {
    data: RwLock<ShardMapType<T>>,
}

impl<T: Send + Sync> Default for HandleMapShard<T> {
    fn default() -> Self {
        Self { data: RwLock::new(hashbrown::HashMap::new()) }
    }
}

impl<T: Send + Sync> HandleMapShard<T> {
    fn get(&self, handle: Handle) -> Result<ObjectReadGuard<T>, HandleNotPresentError> {
        let map_read_guard = ShardReadWriteLock::<T>::read(&self.data);
        let read_only_map_ref = map_read_guard.deref();
        if read_only_map_ref.contains_key(&handle) {
            let object_read_guard = ShardReadGuard::<T>::map(map_read_guard, move |map_ref| {
                // We know that the entry exists, since we've locked the
                // shard and already checked that it exists prior to
                // handing out this new, mapped read-lock.
                map_ref.get(&handle).unwrap()
            });
            Ok(ObjectReadGuard { guard: object_read_guard })
        } else {
            // Auto-drop the read guard, and return an error
            Err(HandleNotPresentError)
        }
    }
    /// Gets a read-write guard on the entire shard map if an entry for the given
    /// handle exists, but if not, yield [`HandleNotPresentError`].
    fn get_read_write_guard_if_entry_exists(
        &self,
        handle: Handle,
    ) -> Result<ShardReadWriteGuard<T>, HandleNotPresentError> {
        // Start with an upgradable read lock and then upgrade to a write lock.
        // By doing this, we prevent new readers from entering (see `spin` documentation)
        let map_upgradable_read_guard = ShardReadWriteLock::<T>::upgradable_read(&self.data);
        let read_only_map_ref = map_upgradable_read_guard.deref();
        if read_only_map_ref.contains_key(&handle) {
            // If we know that the entry exists, and we're currently
            // holding a read-lock, we know that we're safe to request
            // an upgrade to a write lock, since only one write or
            // upgradable read lock can be outstanding at any one time.
            let map_read_write_guard =
                ShardUpgradableReadGuard::<T>::upgrade(map_upgradable_read_guard);
            Ok(map_read_write_guard)
        } else {
            // Auto-drop the read guard, we don't need to allow a write.
            Err(HandleNotPresentError)
        }
    }

    fn get_mut(&self, handle: Handle) -> Result<ObjectReadWriteGuard<T>, HandleNotPresentError> {
        let map_read_write_guard = self.get_read_write_guard_if_entry_exists(handle)?;
        // Expose only the pointed-to object with a mapped read-write guard
        let object_read_write_guard =
            ShardReadWriteGuard::<T>::map(map_read_write_guard, move |map_ref| {
                // Already checked that the entry exists while holding the lock
                map_ref.get_mut(&handle).unwrap()
            });
        Ok(ObjectReadWriteGuard { guard: object_read_write_guard })
    }
    fn deallocate(
        &self,
        handle: Handle,
        outstanding_allocations_counter: &AtomicU32,
    ) -> Result<T, HandleNotPresentError> {
        let mut map_read_write_guard = self.get_read_write_guard_if_entry_exists(handle)?;
        // We don't need to worry about double-decrements, since the above call
        // got us an upgradable read guard for our read, which means it's the only
        // outstanding upgradeable guard on the shard. See `spin` documentation.
        // Remove the pointed-to object from the map, and return it,
        // releasing the lock when the guard goes out of scope.
        let removed_object = map_read_write_guard.deref_mut().remove(&handle).unwrap();
        // Decrement the allocations counter. Release ordering because we want
        // to ensure that clearing the map entry never gets re-ordered to after when
        // this counter gets decremented.
        outstanding_allocations_counter.sub(1, Ordering::Release);
        Ok(removed_object)
    }

    fn try_allocate<F>(
        &self,
        handle: Handle,
        object_provider: F,
        outstanding_allocations_counter: &AtomicU32,
        max_active_handles: u32,
    ) -> Result<(), ShardAllocationError<T, F>>
    where
        F: FnOnce() -> T,
    {
        // Use an upgradeable read guard -> write guard to provide greater fairness
        // toward writers (see `spin` documentation)
        let map_upgradable_read_guard = ShardReadWriteLock::<T>::upgradable_read(&self.data);
        let mut map_read_write_guard =
            ShardUpgradableReadGuard::<T>::upgrade(map_upgradable_read_guard);
        match map_read_write_guard.entry_ref(&handle) {
            EntryRef::Occupied(_) => {
                // We've already allocated for that handle-id, so yield
                // the object provider back to the caller.
                Err(ShardAllocationError::EntryOccupied(object_provider))
            }
            EntryRef::Vacant(vacant_entry) => {
                // An entry is open, but we haven't yet checked the allocations count.
                // Try to increment the total allocations count atomically.
                // Use acquire ordering on a successful bump, because we don't want
                // to invoke the allocation closure before we have a guaranteed slot.
                // On the other hand, upon failure, we don't care about ordering
                // of surrounding operations, and so we use a relaxed ordering there.
                let allocation_count_bump_result = outstanding_allocations_counter.fetch_update(
                    Ordering::Acquire,
                    Ordering::Relaxed,
                    |old_total_allocations| {
                        if old_total_allocations >= max_active_handles {
                            None
                        } else {
                            Some(old_total_allocations + 1)
                        }
                    },
                );
                match allocation_count_bump_result {
                    Ok(_) => {
                        // We're good to actually allocate
                        let object = object_provider();
                        vacant_entry.insert(object);
                        Ok(())
                    }
                    Err(_) => {
                        // The allocation would cause us to exceed the allowed allocations,
                        // so release all locks and error.
                        Err(ShardAllocationError::ExceedsAllocationLimit)
                    }
                }
            }
        }
    }
    /// Gets the actual number of elements stored in this shard.
    /// Only suitable for single-threaded sections of tests.
    #[cfg(test)]
    fn len(&self) -> usize {
        let guard = ShardReadWriteLock::<T>::read(&self.data);
        guard.deref().len()
    }
}

/// Externally-facing trait for things which behave like handle-map handles
/// with a globally-defined handle-map for the type.
pub trait HandleLike: Sized {
    /// The underlying object type pointed-to by this handle
    type Object: Send + Sync;

    /// Tries to allocate a new handle using the given provider
    /// to construct the underlying stored object as a new
    /// entry into the global handle table for this type.
    fn allocate(
        initial_value_provider: impl FnOnce() -> Self::Object,
    ) -> Result<Self, HandleMapFullError>;

    /// Gets a RAII read-guard on the contents behind this handle.
    fn get(&self) -> Result<ObjectReadGuard<Self::Object>, HandleNotPresentError>;

    /// Gets a RAII read-write guard on the contents behind this handle.
    fn get_mut(&self) -> Result<ObjectReadWriteGuard<Self::Object>, HandleNotPresentError>;

    /// Deallocates the contents behind this handle.
    fn deallocate(self) -> Result<Self::Object, HandleNotPresentError>;
}

#[macro_export]
/// `declare_handle_map! { handle_module_name, handle_type_name, wrapped_type,
/// map_dimension_provider }`
///
/// Declares a new public module with name `handle_module_name` which includes a new type
/// `handle_type_name` which is `#[repr(C)]` and represents FFI-accessible handles
/// to values of type `wrapped_type`.
///
/// Internal to the generated module, a new static `SingletonHandleMap` is created, where the
/// maximum number of active handles and the number of shards are given by
/// the dimensions returned by evaluation of the `map_dimension_provider` expression.
///
/// Note: `map_dimension_provider` will be evaluated within the defined module's scope,
/// so you will likely need to use `super` to refer to definitions in the enclosing scope.
///
/// # Example
/// The following code defines an FFI-safe type `StringHandle` which references
/// the `String` data-type, and uses it to define a (contrived)
/// function `sample` which will print "Hello World".
///
/// ```
/// mod sample {
///     use core::ops::Deref;
///     use handle_map::{declare_handle_map, HandleMapDimensions, HandleLike};
///
///     fn get_string_handle_map_dimensions() -> HandleMapDimensions {
///         HandleMapDimensions {
///             num_shards: 8,
///             max_active_handles: 100,
///         }
///     }
///
///     declare_handle_map! { string_handle, StringHandle, String,
///                           super::get_string_handle_map_dimensions() }
///
///     use string_handle::StringHandle;
///
///     fn sample() {
///         // Note: this method could panic if there are
///         // more than 99 outstanding handles.
///
///         // Allocate a new string-handle pointing to the string "Hello"
///         let handle = StringHandle::allocate(|| { "Hello".to_string() }).unwrap();
///         {
///             // Obtain a write-guard on the contents of our handle
///             let mut handle_write_guard = handle.get_mut().unwrap();
///             handle_write_guard.push_str(" World");
///             // Write guard is auto-dropped at the end of this block.
///         }
///         {
///             // Obtain a read-guard on the contents of our handle.
///             // Note that we had to ensure that the write-guard was
///             // dropped prior to doing this, or else execution
///             // could potentially hang.
///             let handle_read_guard = handle.get().unwrap();
///             println!("{}", handle_read_guard.deref());
///         }
///         // Clean up the data behind the created handle
///         handle.deallocate().unwrap();
///     }
/// }
/// ```
macro_rules! declare_handle_map {
    ($handle_module_name:ident, $handle_type_name:ident, $wrapped_type:ty,
     $map_dimension_provider:expr) => {
        #[doc = concat!("Macro-generated (via `handle_map::declare_handle_map!`) module",
                        " which defines the `", stringify!($handle_module_name), "::",
                        stringify!($handle_type_name), "` FFI-transmissible handle type ",
                        " which references values of type `", stringify!($wrapped_type), "`.")]
        pub mod $handle_module_name {
            use $crate::SingletonHandleMapInfo;

            struct SingletonInfo;

            static GLOBAL_HANDLE_MAP: $crate::SingletonHandleMap<SingletonInfo> =
                $crate::SingletonHandleMap::with_info(SingletonInfo);

            impl $crate::SingletonHandleMapInfo for SingletonInfo {
                type Object = $wrapped_type;

                fn dimensions(&self) -> $crate::HandleMapDimensions {
                    $map_dimension_provider
                }
                fn get_handle_map() -> &'static $crate::HandleMap<$wrapped_type> {
                    GLOBAL_HANDLE_MAP.get()
                }
            }

            #[doc = concat!("A `#[repr(C)]` handle to a value of type `",
                            stringify!($wrapped_type), "`.")]
            #[repr(C)]
            #[derive(Clone, Copy, PartialEq, Eq)]
            pub struct $handle_type_name {
                handle_id: u64,
            }

            impl $handle_type_name {
                fn get_as_handle(&self) -> $crate::Handle {
                    $crate::Handle::from_id(self.handle_id)
                }
            }
            impl $crate::HandleLike for $handle_type_name {
                type Object = $wrapped_type;
                fn allocate(initial_value_provider: impl FnOnce() -> $wrapped_type,
                ) -> Result<Self, $crate::HandleMapFullError> {
                    SingletonInfo::get_handle_map().allocate(initial_value_provider)
                        .map(|derived_handle| Self {
                            handle_id: derived_handle.get_id(),
                        })
                }
                fn get(&self) -> Result<$crate::ObjectReadGuard<$wrapped_type>, $crate::HandleNotPresentError> {
                    SingletonInfo::get_handle_map().get(self.get_as_handle())
                }
                fn get_mut(&self) -> Result<$crate::ObjectReadWriteGuard<$wrapped_type>, $crate::HandleNotPresentError> {
                    SingletonInfo::get_handle_map().get_mut(self.get_as_handle())
                }
                fn deallocate(self) -> Result<$wrapped_type, $crate::HandleNotPresentError> {
                    SingletonInfo::get_handle_map().deallocate(self.get_as_handle())
                }
            }
        }
    }
}
