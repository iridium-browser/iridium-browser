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

#include "./stats.h"

#include <algorithm>
#include <map>
#include <numeric>
#include <string>

#include "absl/container/flat_hash_set.h"
#include "absl/types/span.h"
#include "./environment.h"
#include "./logging.h"

namespace centipede {

namespace {
// Helper for PrintExperimentStats().
// Prints the experiment summary for the `field`.
void PrintExperimentStatsForOneStatValue(absl::Span<const Stats> stats_vec,
                                         absl::Span<const Environment> env_vec,
                                         std::ostream &os,
                                         std::atomic<uint64_t> Stats::*field) {
  CHECK_EQ(stats_vec.size(), env_vec.size());
  // Maps experiment names to indices in env_vec/stats_vec.
  // We use std::map because we want lexicographic order of experiment names.
  std::map<std::string_view, std::vector<size_t>> experiment_to_indices;
  for (size_t i = 0; i < env_vec.size(); ++i) {
    experiment_to_indices[env_vec[i].experiment_name].push_back(i);
  }

  // Iterate over every experiment_name.
  for (const auto &[experiment_name, experiment_indices] :
       experiment_to_indices) {
    os << experiment_name << ": ";
    std::vector<uint64_t> stat_values;
    CHECK_NE(experiment_indices.size(), 0);
    // Get the required stat fields into a vector `stat_values`.
    stat_values.reserve(experiment_indices.size());
    for (const auto idx : experiment_indices) {
      // stat_values.push_back(extract_value(stats_vec[idx]));
      stat_values.push_back((stats_vec[idx].*field));
    }
    // Print min/max/avg and the full sorted contents of `stat_values`.
    std::sort(stat_values.begin(), stat_values.end());
    os << "min:\t" << stat_values.front() << "\t";
    os << "max:\t" << stat_values.back() << "\t";
    os << "avg:\t"
       << (std::accumulate(stat_values.begin(), stat_values.end(), 0.) /
           static_cast<double>(stat_values.size()))
       << "\t";
    os << "--";
    for (const auto value : stat_values) {
      os << "\t" << value;
    }
    os << std::endl;
  }
}

}  // namespace

void PrintExperimentStats(absl::Span<const Stats> stats_vec,
                          absl::Span<const Environment> env_vec,
                          std::ostream &os) {
  os << "Coverage:\n";
  PrintExperimentStatsForOneStatValue(stats_vec, env_vec, os,
                                      &Stats::num_covered_pcs);

  os << "Corpus size:\n";
  PrintExperimentStatsForOneStatValue(stats_vec, env_vec, os,
                                      &Stats::corpus_size);
  os << "Flags:\n";
  absl::flat_hash_set<std::string> printed_names;
  for (const auto &env : env_vec) {
    if (!printed_names.insert(env.experiment_name).second) continue;
    os << env.experiment_name << ": " << env.experiment_flags << "\n";
  }
}

}  // namespace centipede
