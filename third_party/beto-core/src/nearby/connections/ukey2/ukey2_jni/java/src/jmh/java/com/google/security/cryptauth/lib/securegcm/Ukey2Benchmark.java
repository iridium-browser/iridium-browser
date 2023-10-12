/*
 * Copyright 2023 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.google.security.cryptauth.lib.securegcm;

import org.openjdk.jmh.annotations.*;
import org.openjdk.jmh.infra.Blackhole;
import org.openjdk.jmh.profile.GCProfiler;
import org.openjdk.jmh.runner.Runner;
import org.openjdk.jmh.runner.RunnerException;
import org.openjdk.jmh.runner.options.Options;
import org.openjdk.jmh.runner.options.OptionsBuilder;

import java.nio.charset.StandardCharsets;
import java.time.Duration;
import java.util.concurrent.TimeUnit;
import java.util.Random;

/**
 * Benchmark for encoding and decoding UKEY2 messages over the JNI, analogous to
 * `ukey2_benches.rs`. The parameters and the operations also roughly matches the that of the Rust
 * Criterion benchmark. That said, since the benchmark infrastructure is different, there will
 * inevitably be differences the skews the number in certain ways – comparison of numbers from the
 * different benchmarks should compared on order-of-magnitudes only. To get the JNI overhead, for
 * example, it would be better use this JMH infra to measure a call into a no-op Rust function,
 * which is a more apples-to-apples comparison.
 *
 * To run this benchmark, run
 *   cargo build -p ukey2_jni --release && ./gradlew jmh
 */
@State(Scope.Benchmark)
@OutputTimeUnit(TimeUnit.SECONDS)
@BenchmarkMode(Mode.Throughput)
public class Ukey2Benchmark {

    @State(Scope.Thread)
    public static class ConnectionState {
        D2DConnectionContextV1 connContext;
        D2DConnectionContextV1 serverConnContext;
        @Param({"10", "1024"})
        int sizeKibs;
        byte[] inputBytes;

        @Setup
        public void setup() throws Exception {
            D2DHandshakeContext initiatorContext =
                new D2DHandshakeContext(D2DHandshakeContext.Role.Initiator);
            D2DHandshakeContext serverContext =
                new D2DHandshakeContext(D2DHandshakeContext.Role.Responder);
            serverContext.parseHandshakeMessage(initiatorContext.getNextHandshakeMessage());
            initiatorContext.parseHandshakeMessage(serverContext.getNextHandshakeMessage());
            serverContext.parseHandshakeMessage(initiatorContext.getNextHandshakeMessage());
            connContext = initiatorContext.toConnectionContext();
            serverConnContext = serverContext.toConnectionContext();
            Random random = new Random();
            inputBytes = new byte[sizeKibs * 1024];
            random.nextBytes(inputBytes);
        }
    }

    @Benchmark
    @Fork(3)
    @Warmup(iterations = 2, time = 500, timeUnit = TimeUnit.MILLISECONDS)
    @Measurement(iterations = 5, time = 500, timeUnit = TimeUnit.MILLISECONDS)
    public void encodeAndDecode(ConnectionState state, Blackhole blackhole) throws Exception {
        byte[] encoded = state.connContext.encodeMessageToPeer(state.inputBytes, null);
        byte[] decoded = state.serverConnContext.decodeMessageFromPeer(encoded, null);
        blackhole.consume(decoded);
    }
}
