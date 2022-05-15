// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/tracing/public/mojom/data_source_config_mojom_traits.h"

#include <utility>

#include "services/tracing/public/mojom/chrome_config_mojom_traits.h"

namespace mojo {
bool StructTraits<tracing::mojom::DataSourceConfigDataView,
                  perfetto::DataSourceConfig>::
    Read(tracing::mojom::DataSourceConfigDataView data,
         perfetto::DataSourceConfig* out) {
  std::string name, legacy_config;
  perfetto::ChromeConfig config;
  if (!data.ReadName(&name) || !data.ReadChromeConfig(&config) ||
      !data.ReadLegacyConfig(&legacy_config)) {
    return false;
  }
  out->set_name(name);
  out->set_target_buffer(data.target_buffer());
  out->set_trace_duration_ms(data.trace_duration_ms());
  out->set_tracing_session_id(data.tracing_session_id());
  *out->mutable_chrome_config() = std::move(config);
  out->set_legacy_config(legacy_config);
  return true;
}
}  // namespace mojo
