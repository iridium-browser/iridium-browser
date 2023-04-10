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

#include "./analyze_corpora.h"

#include "absl/container/flat_hash_set.h"
#include "./feature.h"
#include "./logging.h"

namespace centipede {
void AnalyzeCorpora(const PCTable &pc_table,
                    const SymbolTable &symbols,
                    const std::vector<CorpusRecord> &a,
                    const std::vector<CorpusRecord> &b) {
  // TODO(kcc): compute the CoverageFrontier of `a`. We are only interested in
  // elements of `b` that break though that CoverageFrontier.

  // `a_pcs` will contain all PCs covered by `a`.
  absl::flat_hash_set<size_t> a_pcs;
  for (const auto &record : a) {
    for (const auto &feature : record.features) {
      if (!feature_domains::k8bitCounters.Contains(feature)) continue;
      auto pc = Convert8bitCounterFeatureToPcIndex(feature);
      a_pcs.insert(pc);
    }
  }

  // `b_only_pcs` will contain PCs covered by `b` but not by `a`.
  // `b_unique_indices` are indices of inputs that have PCs from `b_only_pcs`.
  // `b_shared_indices` are indices of all other inputs from `b`.
  absl::flat_hash_set<size_t> b_only_pcs;
  std::vector<size_t> b_shared_indices, b_unique_indices;
  for (size_t i = 0; i < b.size(); ++i) {
    const auto &record = b[i];
    bool has_b_only = false;
    for (const auto &feature : record.features) {
      if (!feature_domains::k8bitCounters.Contains(feature)) continue;
      auto pc = Convert8bitCounterFeatureToPcIndex(feature);
      if (a_pcs.contains(pc)) continue;
      b_only_pcs.insert(pc);
      has_b_only = true;
    }
    if (has_b_only)
      b_unique_indices.push_back(i);
    else
      b_shared_indices.push_back(i);
    }
    LOG(INFO) << VV(a.size()) << VV(b.size()) << VV(a_pcs.size())
              << VV(b_only_pcs.size()) << VV(b_shared_indices.size())
              << VV(b_unique_indices.size());
    CoverageLogger coverage_logger(pc_table, symbols);
    LOG(INFO) << "symbolized b-only PCs:";
    for (const auto pc : b_only_pcs) {
    auto str = coverage_logger.ObserveAndDescribeIfNew(pc);
    if (!str.empty()) LOG(INFO) << str;
  }
}

}  // namespace centipede
