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
use crate::*;

use hashbrown::HashSet;
use std::sync::Arc;
use std::thread;

// Maximum number of active handles used across all tests
// Chosen to be divisible by the number of active threads
const MAX_ACTIVE_HANDLES: u32 = NUM_ACTIVE_THREADS * MAX_ACTIVE_HANDLES_PER_THREAD;

const MAX_ACTIVE_HANDLES_PER_THREAD: u32 = 16384;

// Deliberately picking a low number of shards so that we
// are more likely to discover conflicts between threads.
const NUM_SHARDS: u8 = 4;

// Deliberately picking a higher number of threads.
const NUM_ACTIVE_THREADS: u32 = 8;

const DEFAULT_DIMENSIONS: HandleMapDimensions =
    HandleMapDimensions { num_shards: NUM_SHARDS, max_active_handles: MAX_ACTIVE_HANDLES };

fn build_handle_map<T: Send + Sync>() -> HandleMap<T> {
    HandleMap::with_dimensions(DEFAULT_DIMENSIONS)
}

/// Performs the same testing function for each thread
fn test_for_each_thread<F>(test_function_ref: Arc<F>, num_repetitions_per_thread: usize)
where
    F: Fn() + Send + Sync + 'static,
{
    let mut join_handles = Vec::new();
    for _ in 0..NUM_ACTIVE_THREADS {
        let test_function_clone = test_function_ref.clone();
        let join_handle = thread::spawn(move || {
            for _ in 0..num_repetitions_per_thread {
                test_function_clone();
            }
        });
        join_handles.push(join_handle);
    }
    for join_handle in join_handles {
        join_handle.join().unwrap()
    }
}

/// Tests the consistency of reads from the same handle across
/// multiple threads.
#[test]
fn test_read_consistency_same_address() {
    let num_repetitions_per_thread = 10000;
    let handle_map = build_handle_map::<String>();
    let handle = handle_map.allocate(|| "hello".to_string()).expect("Allocation shouldn't fail");
    let test_fn = Arc::new(move || {
        let value_ref = handle_map.get(handle).expect("Getting shouldn't fail");
        assert_eq!("hello", value_ref.deref());
    });
    test_for_each_thread(test_fn, num_repetitions_per_thread);
}

/// Tests overloading the table with allocations to ensure
/// that when all is said and done, we still haven't exceeded
/// the allocations limit.
#[test]
#[allow(unused_must_use)]
fn test_overload_with_allocations() {
    let num_repetitions_per_thread = 2 * MAX_ACTIVE_HANDLES_PER_THREAD as usize;
    let handle_map = build_handle_map::<u8>();

    let handle_map_function_ref = Arc::new(handle_map);
    let handle_map_post_function_ref = handle_map_function_ref.clone();

    let test_fn = Arc::new(move || {
        handle_map_function_ref.allocate(|| 0xFF);
    });
    test_for_each_thread(test_fn, num_repetitions_per_thread);

    let actual_num_active_handles = handle_map_post_function_ref.len();
    assert_eq!(MAX_ACTIVE_HANDLES as usize, actual_num_active_handles);
}

/// Tests deallocations and allocations near the allocation limit.
#[test]
#[allow(unused_must_use)]
fn test_overload_allocations_deallocations() {
    let num_repetitions_per_thread = 10000;

    //Pre-fill the map so that there's only one available entry.
    let handle_map = build_handle_map::<u8>();
    for i in 0..(MAX_ACTIVE_HANDLES - 1) {
        handle_map.allocate(|| (i % 256) as u8);
    }

    let handle_map_function_ref = Arc::new(handle_map);
    let handle_map_post_function_ref = handle_map_function_ref.clone();

    let test_fn = Arc::new(move || {
        let allocation_result = handle_map_function_ref.allocate(|| 0xFF);
        if let Ok(handle) = allocation_result {
            handle_map_function_ref.deallocate(handle).unwrap();
        }
    });
    test_for_each_thread(test_fn, num_repetitions_per_thread);

    // No matter what happened above, we should have the same number
    // of handles as when we started, because every successful allocation
    // should have been paired with a successful deallocation.
    let actual_num_active_handles = handle_map_post_function_ref.len();
    assert_eq!((MAX_ACTIVE_HANDLES - 1) as usize, actual_num_active_handles);

    //Verify that we still have space for one more entry after all that.
    handle_map_post_function_ref.allocate(|| 0xEE).unwrap();
}

/// Tests the progress of allocate/read/write/read/deallocate
/// to independent handles across multiple threads.
#[test]
fn test_full_lifecycle_independent_handles() {
    let num_repetitions_per_thread = 10000;
    let handle_map = build_handle_map::<String>();
    let test_fn = Arc::new(move || {
        let handle =
            handle_map.allocate(|| "Hello".to_string()).expect("Allocation shouldn't fail");
        {
            let value_ref = handle_map.get(handle).expect("Getting the value shouldn't fail");
            assert_eq!("Hello", &*value_ref);
        };
        {
            let mut value_mut_ref =
                handle_map.get_mut(handle).expect("Mutating the value shouldn't fail");
            value_mut_ref.deref_mut().push_str(" World!");
        };
        {
            let value_ref = handle_map
                .get(handle)
                .expect("Getting the value after modification shouldn't fail");
            assert_eq!("Hello World!", &*value_ref);
        };
        let removed = handle_map.deallocate(handle).expect("Deallocation shouldn't fail");
        assert_eq!("Hello World!", removed);
    });
    test_for_each_thread(test_fn, num_repetitions_per_thread);
}

/// Tests the consistency of reads+writes to the same handle,
/// where threads modify and read different parts of an
/// underlying structure.
#[test]
fn test_consistency_of_same_handle_multithreaded_modifications() {
    let num_repetitions_per_thread = 10000;
    let handle_map = Arc::new(build_handle_map::<(String, String)>());
    let handle = handle_map
        .allocate(|| ("A".to_string(), "B".to_string()))
        .expect("Allocation shouldn't fail");

    let handle_map_second_ref = handle_map.clone();

    let join_handle_a = thread::spawn(move || {
        for i in 1..num_repetitions_per_thread {
            {
                let value_ref =
                    handle_map.get(handle).expect("Getting the value from thread A shouldn't fail");
                let value = &value_ref.0;
                assert_eq!(i, value.len());
            }
            {
                let mut value_mut_ref = handle_map
                    .get_mut(handle)
                    .expect("Mutating the value from thread A shouldn't fail");
                value_mut_ref.0.push('A');
            }
        }
    });

    let join_handle_b = thread::spawn(move || {
        for i in 1..num_repetitions_per_thread {
            {
                let value_ref = handle_map_second_ref
                    .get(handle)
                    .expect("Getting the value from thread B shouldn't fail");
                let value = &value_ref.1;
                assert_eq!(i, value.len());
            }
            {
                let mut value_mut_ref = handle_map_second_ref
                    .get_mut(handle)
                    .expect("Mutating the value from thread B shouldn't fail");
                value_mut_ref.1.push('B');
            }
        }
    });

    join_handle_a.join().unwrap();
    join_handle_b.join().unwrap();
}

/// Multi-threaded test to ensure that when attempting
/// to allocate over handle IDs which are already allocated,
/// all threads eventually get distinct, unused handle IDs
/// for their own allocations.
#[test]
fn test_non_overwriting_old_handles() {
    let mut all_handles: HashSet<Handle> = HashSet::new();
    let num_repetitions_per_thread = 100;
    let mut handle_map = build_handle_map::<u8>();
    for _ in 0..(num_repetitions_per_thread * NUM_ACTIVE_THREADS) {
        let handle = handle_map.allocate(|| 0xFF).expect("Initial allocations shouldn't fail");
        all_handles.insert(handle);
    }
    // Reset the new-handle-id counter
    handle_map.set_new_handle_id_counter(0);

    let handle_map = Arc::new(handle_map);

    let mut thread_handles: Vec<thread::JoinHandle<Vec<Handle>>> = Vec::new();
    for _ in 0..NUM_ACTIVE_THREADS {
        let handle_map_reference = handle_map.clone();
        let thread_handle = thread::spawn(move || {
            let mut handles = Vec::new();
            for i in 0..num_repetitions_per_thread {
                let handle = handle_map_reference
                    .allocate(move || (i % 256) as u8)
                    .expect("No allocation should fail");
                handles.push(handle);
            }
            handles
        });
        thread_handles.push(thread_handle);
    }
    for thread_handle in thread_handles {
        let handles: Vec<Handle> = thread_handle.join().expect("Individual threads shouldn't fail");
        for handle in handles {
            let was_distinct = all_handles.insert(handle);
            assert!(was_distinct);
        }
    }
}

#[test]
fn test_id_wraparound() {
    let mut handle_map = build_handle_map::<u8>();
    handle_map.set_new_handle_id_counter(u64::MAX);
    handle_map.allocate(|| 0xAB).expect("Counter wrap-around allocation should not fail");
    handle_map.allocate(|| 0xCD).expect("Post-counter-wrap-around allocation should not fail");
}

#[test]
fn test_deallocate_unallocated_handle() {
    let handle_map = build_handle_map::<usize>();
    let handle = handle_map.allocate(|| 2).expect("Allocation shouldn't fail");
    let deallocated = handle_map.deallocate(handle).expect("Deallocation shouldn't fail");
    assert_eq!(2, deallocated);
    let double_deallocate_result = handle_map.deallocate(handle);
    assert!(double_deallocate_result.is_err());
}

#[test]
fn test_get_unallocated_handle() {
    let handle_map = build_handle_map::<u8>();
    let handle = handle_map.allocate(|| 0xFE).expect("Allocation shouldn't fail");
    let deallocated = handle_map.deallocate(handle).expect("Deallocation shouldn't fail");
    assert_eq!(0xFE, deallocated);
    let read_result = handle_map.get(handle);
    assert!(read_result.is_err());
}

#[test]
fn test_get_mut_unallocated_handle() {
    let handle_map = build_handle_map::<(usize, usize, usize)>();
    let handle = handle_map.allocate(|| (1, 2, 3)).expect("Allocation shouldn't fail");
    let deallocated = handle_map.deallocate(handle).expect("Deallocation shouldn't fail");
    assert_eq!((1, 2, 3), deallocated);
    let get_mut_result = handle_map.get_mut(handle);
    assert!(get_mut_result.is_err());
}
