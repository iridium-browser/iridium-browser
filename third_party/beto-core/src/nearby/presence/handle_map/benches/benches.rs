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
use criterion::{black_box, criterion_group, criterion_main, Criterion};
use handle_map::*;
use std::sync::Arc;
use std::thread;
use std::time::{Duration, Instant};

// For the sake of these benchmarks, we just want
// to be able to allocate an almost-arbitrary number of handles
// if needed by the particular benchmark.
const MAX_ACTIVE_HANDLES: u32 = u32::MAX - 1;

// The default number of threads+shards to use
// in multi-threaded benchmarks, chosen
// to represent a typical multi-threaded use-case.
const DEFAULT_SHARDS_AND_THREADS: u8 = 8;

/// We're using u8's for the wrapped object type to deliberately ignore
/// costs related to accessing the underlying data - we just care about
/// benchmarking handle-map operations.
type BenchHandleMap = HandleMap<u8>;

fn build_handle_map(num_shards: u8) -> BenchHandleMap {
    let dimensions = HandleMapDimensions { num_shards, max_active_handles: MAX_ACTIVE_HANDLES };
    HandleMap::with_dimensions(dimensions)
}

/// Benchmark for repeated reads on a single thread
fn single_threaded_read_benchmark(c: &mut Criterion) {
    // The number of shards should not matter much here.
    let handle_map = build_handle_map(8);
    // Pre-populate the map with a single handle.
    let handle = handle_map.allocate(|| 0xFF).unwrap();
    let handle_map_ref = &handle_map;
    // Perform repeated reads
    c.bench_function("single-threaded reads", |b| {
        b.iter(|| {
            let guard = handle_map_ref.get(black_box(handle)).unwrap();
            black_box(*guard)
        })
    });
}

/// Benchmark for repeated writes on a single thread
fn single_threaded_write_benchmark(c: &mut Criterion) {
    // The number of shards should not matter much here.
    let handle_map = build_handle_map(8);
    // Pre-populate the map with a single handle.
    let handle = handle_map.allocate(|| 0xFF).unwrap();
    let handle_map_ref = &handle_map;
    // Perform repeated writes
    c.bench_function("single-threaded writes", |b| {
        b.iter(|| {
            let mut guard = handle_map_ref.get_mut(black_box(handle)).unwrap();
            *guard *= black_box(0);
        })
    });
}

/// Benchmark for repeated allocations on a single thread
#[allow(unused_must_use)]
fn single_threaded_allocate_benchmark(c: &mut Criterion) {
    // The number of shards here is chosen to be quasi-reasonable.
    // Realistically, performance will fluctuate somewhat with this
    // due to the difference in the number of internal hash-map collisions,
    // but ultimately we only care about the asymptotic behavior.
    let handle_map = build_handle_map(8);
    let handle_map_ref = &handle_map;
    // Perform repeated allocations
    c.bench_function("single-threaded allocations", |b| {
        b.iter(|| {
            handle_map_ref.allocate(|| black_box(0xEE));
        })
    });
}

/// Benchmark for repeated allocation->deallocation on a single thread
/// starting from an empty map.
#[allow(unused_must_use)]
fn single_threaded_allocate_deallocate_near_empty_benchmark(c: &mut Criterion) {
    // The number of shards should not matter much here.
    let handle_map = build_handle_map(8);
    let handle_map_ref = &handle_map;
    // Perform repeated allocation/deallocation pairs
    c.bench_function("single-threaded allocate/deallocate pairs (empty init state)", |b| {
        b.iter(|| {
            let handle = handle_map_ref.allocate(|| black_box(0xDD)).unwrap();
            handle_map_ref.deallocate(handle);
        })
    });
}

/// Benchmark for repeated allocation->deallocation starting
/// from a map with a thousand elements per shard.
#[allow(unused_must_use)]
fn single_threaded_allocate_deallocate_one_thousand_benchmark(c: &mut Criterion) {
    let handle_map = build_handle_map(8);
    for _ in 0..(8 * 1000) {
        handle_map.allocate(|| black_box(0xFF)).unwrap();
    }
    let handle_map_ref = &handle_map;

    // Perform repeated allocation/deallocation pairs
    c.bench_function(
        "single-threaded allocate/deallocate pairs (init one thousand elements per shard)",
        |b| {
            b.iter(|| {
                let handle = handle_map_ref.allocate(|| black_box(0xDD)).unwrap();
                handle_map_ref.deallocate(handle);
            })
        },
    );
}

/// Benchmark for repeated allocation->deallocation starting
/// from a map with a million elements per shard.
#[allow(unused_must_use)]
fn single_threaded_allocate_deallocate_one_million_benchmark(c: &mut Criterion) {
    let handle_map = build_handle_map(8);
    for _ in 0..(8 * 1000000) {
        handle_map.allocate(|| black_box(0xFF)).unwrap();
    }
    let handle_map_ref = &handle_map;

    // Perform repeated allocation/deallocation pairs
    c.bench_function(
        "single-threaded allocate/deallocate pairs (init one million elements per shard)",
        |b| {
            b.iter(|| {
                let handle = handle_map_ref.allocate(|| black_box(0xDD)).unwrap();
                handle_map_ref.deallocate(handle);
            })
        },
    );
}

/// Helper function for creating a multi-threaded benchmark
/// with the set number of threads. Untimed Initialization of state
/// should occur prior to this call. This method will repeatedly
/// execute the benchmark code on all threads
fn bench_multithreaded<F>(name: &str, num_threads: u8, benchable_fn_ref: Arc<F>, c: &mut Criterion)
where
    F: Fn() + Send + Sync + 'static,
{
    c.bench_function(name, move |b| {
        b.iter_custom(|iters| {
            // The returned results from the threads will be their timings
            let mut join_handles = Vec::new();
            for _ in 0..num_threads {
                let benchable_fn_ref_clone = benchable_fn_ref.clone();
                let join_handle = thread::spawn(move || {
                    let start = Instant::now();
                    for _ in 0..iters {
                        benchable_fn_ref_clone();
                    }
                    start.elapsed()
                });
                join_handles.push(join_handle);
            }
            //Threads spawned, now collect the results
            let mut total_duration = Duration::from_nanos(0);
            for join_handle in join_handles {
                let thread_duration = join_handle.join().unwrap();
                total_duration += thread_duration;
            }
            total_duration /= num_threads as u32;
            total_duration
        })
    });
}

/// Defines a number of reads/writes to perform [relative
/// to other operations performed in a given test]
#[derive(Debug, Clone, Copy)]
struct ReadWriteCount {
    num_reads: usize,
    num_writes: usize,
}

const READS_ONLY: ReadWriteCount = ReadWriteCount { num_reads: 4, num_writes: 0 };

const READ_LEANING: ReadWriteCount = ReadWriteCount { num_reads: 3, num_writes: 1 };

const BALANCED: ReadWriteCount = ReadWriteCount { num_reads: 2, num_writes: 2 };

const WRITE_LEANING: ReadWriteCount = ReadWriteCount { num_reads: 1, num_writes: 3 };

const WRITES_ONLY: ReadWriteCount = ReadWriteCount { num_reads: 0, num_writes: 4 };

const READ_WRITE_COUNTS: [ReadWriteCount; 5] =
    [READS_ONLY, READ_LEANING, BALANCED, WRITE_LEANING, WRITES_ONLY];

/// Benchmarks a repeated allocate/[X writes]/[Y reads]/deallocate workflow across
/// the default number of threads and shards.
fn lifecycle_read_and_write_multithreaded(per_object_rw_count: ReadWriteCount, c: &mut Criterion) {
    let name = format!(
        "lifecycle_read_and_write_multithreaded(reads/object: {}, writes/object:{})",
        per_object_rw_count.num_reads, per_object_rw_count.num_writes
    );
    // We need an Arc to be able to pass this off to `bench_multithreaded`.
    let handle_map = Arc::new(build_handle_map(DEFAULT_SHARDS_AND_THREADS));
    bench_multithreaded(
        &name,
        DEFAULT_SHARDS_AND_THREADS,
        Arc::new(move || {
            let handle = handle_map.allocate(|| black_box(0xFF)).unwrap();
            for _ in 0..per_object_rw_count.num_writes {
                let mut write_guard = handle_map.get_mut(handle).unwrap();
                *write_guard *= black_box(1);
            }
            for _ in 0..per_object_rw_count.num_reads {
                let read_guard = handle_map.get_mut(handle).unwrap();
                black_box(*read_guard);
            }
            handle_map.deallocate(handle).unwrap();
        }),
        c,
    );
}

fn lifecycle_benchmarks(c: &mut Criterion) {
    for read_write_count in READ_WRITE_COUNTS {
        // Using 8 separate shards to roughly match the number of active threads.
        lifecycle_read_and_write_multithreaded(read_write_count, c);
    }
}

/// Benchmarks repeated cycles of accessing the given number of distinct objects, each
/// with the given number of reads and writes across the given number of threads.
fn read_and_write_contention_multithreaded(
    num_threads: u8,
    num_distinct_objects: u32,
    per_object_rw_count: ReadWriteCount,
    c: &mut Criterion,
) {
    let name = format!("read_and_write_contention_multithreaded(threads: {}, objects: {}, reads/object: {}, writes/object:{})",
                       num_threads, num_distinct_objects, per_object_rw_count.num_reads, per_object_rw_count.num_writes);
    // Default to 8 shards, ultimately doesn't matter much since we only ever access a few shards.
    // This needs to be `'static` in order to pass a ref off to `bench_multithreaded`.
    let handle_map = Arc::new(build_handle_map(8));
    let mut handles = Vec::new();
    for i in 0..num_distinct_objects {
        let handle = handle_map.allocate(|| i as u8).unwrap();
        handles.push(handle);
    }

    bench_multithreaded(
        &name,
        num_threads,
        Arc::new(move || {
            // Deliberately performing all writes first, with
            // the goal of getting staggered timings from
            // all of the threads for subsequent iterations.
            // We also perform reads and writes across all objects
            // in batches to get something resembling uniform access.
            for _ in 0..per_object_rw_count.num_writes {
                for handle in handles.iter() {
                    let mut write_guard = handle_map.get_mut(*handle).unwrap();
                    *write_guard *= black_box(1);
                }
            }
            for _ in 0..per_object_rw_count.num_reads {
                for handle in handles.iter() {
                    let read_guard = handle_map.get(*handle).unwrap();
                    black_box(*read_guard);
                }
            }
        }),
        c,
    );
}

fn read_write_contention_benchmarks(c: &mut Criterion) {
    for num_threads in [2, 4, 8].iter() {
        for num_distinct_objects in [1u32, 2u32, 4u32].iter() {
            if num_distinct_objects >= &(*num_threads as u32) {
                continue;
            }
            for read_write_count in READ_WRITE_COUNTS {
                read_and_write_contention_multithreaded(
                    *num_threads,
                    *num_distinct_objects,
                    read_write_count,
                    c,
                );
            }
        }
    }
}

/// Benchmarks allocation (starting from empty) for the default number of shards and threads
fn allocator_contention_benchmark(c: &mut Criterion) {
    let name = "allocator_contention_multithreaded";
    // We need an Arc to pass this off to `bench_multithreaded`
    let handle_map = Arc::new(build_handle_map(DEFAULT_SHARDS_AND_THREADS));
    bench_multithreaded(
        name,
        DEFAULT_SHARDS_AND_THREADS,
        Arc::new(move || {
            handle_map.allocate(|| black_box(0xFF)).unwrap();
        }),
        c,
    );
}

criterion_group!(
    benches,
    single_threaded_read_benchmark,
    single_threaded_write_benchmark,
    single_threaded_allocate_benchmark,
    single_threaded_allocate_deallocate_near_empty_benchmark,
    single_threaded_allocate_deallocate_one_thousand_benchmark,
    single_threaded_allocate_deallocate_one_million_benchmark,
    lifecycle_benchmarks,
    read_write_contention_benchmarks,
    allocator_contention_benchmark,
);
criterion_main!(benches);
