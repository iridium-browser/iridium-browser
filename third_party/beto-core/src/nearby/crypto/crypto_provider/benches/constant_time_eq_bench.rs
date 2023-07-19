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

use criterion::{
    criterion_group, criterion_main, measurement::WallTime, BatchSize, BenchmarkGroup, Criterion,
};
use crypto_provider::CryptoProvider;
use crypto_provider_openssl::Openssl;
use crypto_provider_rustcrypto::RustCrypto;
use rand::{distributions::Standard, rngs::ThreadRng, Rng};

fn constant_time_eq_equals(c: &mut Criterion) {
    let mut rng = rand::thread_rng();
    let mut group = c.benchmark_group("constant_time_eq");

    /// Adds benchmarks for the given CryptoProvider.
    ///
    /// Each call adds 11 benchmarks to the group, where the two vectors to be compared are equal in
    /// the first test, differ by up to 100 bytes in the second, differ by up to 200 bytes in the
    /// third etc. The resulting graph can be used to show if there are any correlations between the
    /// number of differing bytes and the time it takes to execute the function.
    fn add_benches<C: CryptoProvider>(group: &mut BenchmarkGroup<WallTime>, rng: &mut ThreadRng) {
        const TEST_LEN: usize = 1000;
        for i in (0..=TEST_LEN).step_by(100) {
            group.bench_function(
                &format!(
                    "constant_time_eq impl {} differ by {:04} bytes",
                    std::any::type_name::<C>(),
                    i,
                ),
                |b| {
                    b.iter_batched(
                        || {
                            let a: Vec<u8> = rng.sample_iter(Standard).take(TEST_LEN).collect();
                            let mut b = a.clone();
                            b[TEST_LEN - i..].fill(0);
                            (a, b)
                        },
                        |(a, b)| C::constant_time_eq(&a, &b),
                        BatchSize::SmallInput,
                    );
                },
            );
        }
    }

    add_benches::<RustCrypto>(&mut group, &mut rng);
    add_benches::<Openssl>(&mut group, &mut rng);
}

criterion_group!(benches, constant_time_eq_equals);
criterion_main!(benches);
