// Copyright 2022 The Centipede Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "./centipede_interface.h"

#include <signal.h>

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <sstream>
#include <string>
#include <thread>  // NOLINT(build/c++11)
#include <vector>

#include "absl/status/status.h"
#include "absl/strings/str_join.h"
#include "absl/strings/str_replace.h"
#include "absl/types/span.h"
#include "./analyze_corpora.h"
#include "./blob_file.h"
#include "./centipede.h"
#include "./command.h"
#include "./coverage.h"
#include "./defs.h"
#include "./environment.h"
#include "./logging.h"
#include "./remote_file.h"
#include "./shard_reader.h"
#include "./stats.h"
#include "./symbol_table.h"
#include "./util.h"

namespace centipede {

namespace {

// Sets signal handler for SIGINT.
void SetSignalHandlers() {
  struct sigaction sigact = {};
  sigact.sa_handler = [](int) {
    ABSL_RAW_LOG(INFO, "SIGINT caught; cleaning up\n");
    RequestEarlyExit(EXIT_FAILURE);
  };
  sigaction(SIGINT, &sigact, nullptr);
}

// Runs env.for_each_blob on every blob extracted from env.args.
// Returns EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
int ForEachBlob(const Environment &env) {
  auto tmpdir = TemporaryLocalDirPath();
  CreateLocalDirRemovedAtExit(tmpdir);
  std::string tmpfile = std::filesystem::path(tmpdir).append("t");

  for (const auto &arg : env.args) {
    LOG(INFO) << "Running '" << env.for_each_blob << "' on " << arg;
    auto blob_reader = DefaultBlobFileReaderFactory();
    absl::Status open_status = blob_reader->Open(arg);
    if (!open_status.ok()) {
      LOG(INFO) << "Failed to open " << arg << ": " << open_status;
      return EXIT_FAILURE;
    }
    absl::Span<uint8_t> blob;
    while (blob_reader->Read(blob) == absl::OkStatus()) {
      ByteArray bytes;
      bytes.insert(bytes.begin(), blob.data(), blob.end());
      // TODO(kcc): [impl] add a variant of WriteToLocalFile that accepts Span.
      WriteToLocalFile(tmpfile, bytes);
      std::string command_line = absl::StrReplaceAll(
          env.for_each_blob, {{"%P", tmpfile}, {"%H", Hash(bytes)}});
      Command cmd(command_line);
      // TODO(kcc): [as-needed] this creates one process per blob.
      // If this flag gets active use, we may want to define special cases,
      // e.g. if for_each_blob=="cp %P /some/where" we can do it in-process.
      cmd.Execute();
      if (EarlyExitRequested()) return ExitCode();
    }
  }
  return EXIT_SUCCESS;
}

// Runs in a dedicated thread, periodically calls PrintExperimentStats
// on `stats_vec` and `envs`.
// Stops when `continue_running` becomes false.
// Exits immediately if --experiment flag is not used.
void PrintExperimentStatsThread(const std::atomic<bool> &continue_running,
                                const std::vector<Stats> &stats_vec,
                                const std::vector<Environment> &envs) {
  CHECK(!envs.empty());
  if (envs.front().experiment.empty()) return;
  for (int i = 0; continue_running; ++i) {
    // Sleep at least a few seconds, and at most 600.
    int seconds_to_sleep = std::clamp(i, 5, 600);
    // Sleep(1) in a loop so that we check continue_running once a second.
    while (--seconds_to_sleep && continue_running) {
      sleep(1);
    }
    std::ostringstream os;
    PrintExperimentStats(stats_vec, envs, os);
    LOG(INFO) << "Experiment:\n" << os.str();
  }
}

// Loads corpora from work dirs provided in `env.args`, analyzes differences.
// Returns EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
int Analyze(const Environment &env, const PCTable &pc_table,
            const SymbolTable &symbols) {
  LOG(INFO) << "Analyze " << absl::StrJoin(env.args, ",");
  CHECK_EQ(env.args.size(), 2) << "for now, Analyze supports only 2 work dirs";
  CHECK(!env.binary.empty()) << "--binary must be used";
  std::vector<std::vector<CorpusRecord>> corpora;
  for (const auto &workdir : env.args) {
    LOG(INFO) << "Reading " << workdir;
    Environment workdir_env = env;
    workdir_env.workdir = workdir;
    corpora.emplace_back();
    auto &corpus = corpora.back();
    for (size_t shard_index = 0; shard_index < env.total_shards;
         ++shard_index) {
      auto corpus_path = workdir_env.MakeCorpusPath(shard_index);
      auto features_path = workdir_env.MakeFeaturesPath(shard_index);
      LOG(INFO) << "Loading corpus shard: " << corpus_path << " "
                << features_path;
      ReadShard(corpus_path, features_path,
                [&corpus](const ByteArray &input, FeatureVec &features) {
                  corpus.push_back({input, features});
                });
    }
    CHECK(!corpus.empty()) << "the corpus is empty, nothing to analyze";
    LOG(INFO) << "corpus size " << corpus.size();
  }
  CHECK_EQ(corpora.size(), 2);
  AnalyzeCorpora(pc_table, symbols, corpora[0], corpora[1]);
  return EXIT_SUCCESS;
}

void SavePCsToFile(const PCTable &pc_table, std::string_view file_path) {
  std::vector<uintptr_t> pcs(pc_table.size());
  for (size_t i = 0; i < pcs.size(); ++i) {
    pcs[i] = pc_table[i].pc;
  }
  WriteToLocalFile(file_path, pcs);
}

}  // namespace

int CentipedeMain(const Environment &env,
                  CentipedeCallbacksFactory &callbacks_factory) {
  SetSignalHandlers();

  if (!env.save_corpus_to_local_dir.empty())
    return Centipede::SaveCorpusToLocalDir(env, env.save_corpus_to_local_dir);

  if (!env.for_each_blob.empty()) return ForEachBlob(env);

  // Just export the corpus from a local dir and exit.
  if (!env.export_corpus_from_local_dir.empty())
    return Centipede::ExportCorpusFromLocalDir(
        env, env.export_corpus_from_local_dir);

  const auto tmpdir = TemporaryLocalDirPath();
  CreateLocalDirRemovedAtExit(tmpdir);  // creates temp dir.

  // Export the corpus from a local dir and then fuzz.
  if (!env.corpus_dir.empty()) {
    for (const auto &corpus_dir : env.corpus_dir) {
      Centipede::ExportCorpusFromLocalDir(env, corpus_dir);
    }
  }

  // Create the coverage dir once, before creating any threads.
  LOG(INFO) << "Coverage dir " << env.MakeCoverageDirPath();
  RemoteMkdir(env.MakeCoverageDirPath());

  auto one_time_callbacks = callbacks_factory.create(env);
  BinaryInfo binary_info;
  one_time_callbacks->PopulateBinaryInfo(binary_info);
  callbacks_factory.destroy(one_time_callbacks);

  std::string pcs_file_path;
  if (binary_info.uses_legacy_trace_pc_instrumentation) {
    pcs_file_path = std::filesystem::path(tmpdir).append("pcs");
    SavePCsToFile(binary_info.pc_table, pcs_file_path);
  }

  if (env.analyze)
    return Analyze(env, binary_info.pc_table, binary_info.symbols);

  if (env.use_pcpair_features) {
    CHECK(!binary_info.pc_table.empty())
        << "use_pcpair_features requires non-empty pc_table";
  }
  CoverageLogger coverage_logger(binary_info.pc_table, binary_info.symbols);

  auto thread_callback = [&](Environment &my_env, Stats &stats) {
    CreateLocalDirRemovedAtExit(TemporaryLocalDirPath());  // creates temp dir.
    my_env.seed = GetRandomSeed(env.seed);  // uses TID, call in this thread.
    my_env.pcs_file_path = pcs_file_path;   // same for all threads.

    if (env.dry_run) return;

    auto user_callbacks = callbacks_factory.create(my_env);
    my_env.ReadKnobsFileIfSpecified();
    Centipede centipede(my_env, *user_callbacks, binary_info, coverage_logger,
                        stats);
    centipede.FuzzingLoop();
    callbacks_factory.destroy(user_callbacks);
  };

  std::vector<Environment> envs(env.num_threads, env);
  std::vector<Stats> stats_vec(env.num_threads);
  std::vector<std::thread> threads(env.num_threads);
  std::atomic<bool> stats_thread_continue_running(true);

  // Create threads.
  for (size_t thread_idx = 0; thread_idx < env.num_threads; thread_idx++) {
    Environment &my_env = envs[thread_idx];
    my_env.my_shard_index = env.my_shard_index + thread_idx;
    my_env.UpdateForExperiment();
    threads[thread_idx] = std::thread(thread_callback, std::ref(my_env),
                                      std::ref(stats_vec[thread_idx]));
  }

  std::thread stats_thread(PrintExperimentStatsThread,
                           std::ref(stats_thread_continue_running),
                           std::ref(stats_vec), std::ref(envs));

  // Join threads.
  for (size_t thread_idx = 0; thread_idx < env.num_threads; thread_idx++) {
    threads[thread_idx].join();
  }
  stats_thread_continue_running = false;
  stats_thread.join();

  return ExitCode();
}

}  // namespace centipede
