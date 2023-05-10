// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gn/metadata.h"

#include "gn/filesystem_utils.h"

const char kMetadata_Help[] =
    R"(Metadata Collection

  Metadata is information attached to targets throughout the dependency tree. GN
  allows for the collection of this data into files written during the generation
  step, enabling users to expose and aggregate this data based on the dependency
  tree.

generated_file targets

  Similar to the write_file() function, the generated_file target type
  creates a file in the specified location with the specified content. The
  primary difference between write_file() and this target type is that the
  write_file function does the file write at parse time, while the
  generated_file target type writes at target resolution time. See
  "gn help generated_file" for more detail.

  When written at target resolution time, generated_file enables GN to
  collect and write aggregated metadata from dependents.

  A generated_file target can declare either 'contents' to write statically
  known contents to a file or 'data_keys' to aggregate metadata and write the
  result to a file. It can also specify 'walk_keys' (to restrict the metadata
  collection), 'output_conversion', and 'rebase'.


Collection and Aggregation

  Targets can declare a 'metadata' variable containing a scope, and this
  metadata may be collected and written out to a file specified by
  generated_file aggregation targets. The 'metadata' scope must contain
  only list values since the aggregation step collects a list of these values.

  During the target resolution, generated_file targets will walk their
  dependencies recursively, collecting metadata based on the specified
  'data_keys'. 'data_keys' is specified as a list of strings, used by the walk
  to identify which variables in dependencies' 'metadata' scopes to collect.

  The walk begins with the listed dependencies of the 'generated_file' target.
  The 'metadata' scope for each dependency is inspected for matching elements
  of the 'generated_file' target's 'data_keys' list.  If a match is found, the
  data from the dependent's matching key list is appended to the aggregate walk
  list. Note that this means that if more than one walk key is specified, the
  data in all of them will be aggregated into one list. From there, the walk
  will then recurse into the dependencies of each target it encounters,
  collecting the specified metadata for each.

  For example:

    group("a") {
      metadata = {
        doom_melon = [ "enable" ]
        my_files = [ "foo.cpp" ]
        my_extra_files = [ "bar.cpp" ]
      }

      deps = [ ":b" ]
    }

    group("b") {
      metadata = {
        my_files = [ "baz.cpp" ]
      }
    }

    generated_file("metadata") {
      outputs = [ "$root_build_dir/my_files.json" ]
      data_keys = [ "my_files", "my_extra_files" ]

      deps = [ ":a" ]
    }

  The above will produce the following file data:

    foo.cpp
    bar.cpp
    baz.cpp

  The dependency walk can be limited by using the 'walk_keys'. This is a list of
  labels that should be included in the walk. All labels specified here should
  also be in one of the deps lists. These keys act as barriers, where the walk
  will only recurse into the targets listed. An empty list in all specified
  barriers will end that portion of the walk.

    group("a") {
      metadata = {
        my_files = [ "foo.cpp" ]
        my_files_barrier = [ ":b" ]
      }

      deps = [ ":b", ":c" ]
    }

    group("b") {
      metadata = {
        my_files = [ "bar.cpp" ]
      }
    }

    group("c") {
      metadata = {
        my_files = [ "doom_melon.cpp" ]
      }
    }

    generated_file("metadata") {
      outputs = [ "$root_build_dir/my_files.json" ]
      data_keys = [ "my_files" ]
      walk_keys = [ "my_files_barrier" ]

      deps = [ ":a" ]
    }

  The above will produce the following file data (note that `doom_melon.cpp` is
  not included):

    foo.cpp
    bar.cpp

  A common example of this sort of barrier is in builds that have host tools
  built as part of the tree, but do not want the metadata from those host tools
  to be collected with the target-side code.

Common Uses

  Metadata can be used to collect information about the different targets in the
  build, and so a common use is to provide post-build tooling with a set of data
  necessary to do aggregation tasks. For example, if each test target specifies
  the output location of its binary to run in a metadata field, that can be
  collected into a single file listing the locations of all tests in the
  dependency tree. A local build tool (or continuous integration infrastructure)
  can then use that file to know which tests exist, and where, and run them
  accordingly.

  Another use is in image creation, where a post-build image tool needs to know
  various pieces of information about the components it should include in order
  to put together the correct image.
)";

bool Metadata::WalkStep(const BuildSettings* settings,
                        const std::vector<std::string>& keys_to_extract,
                        const std::vector<std::string>& keys_to_walk,
                        const SourceDir& rebase_dir,
                        std::vector<Value>* next_walk_keys,
                        std::vector<Value>* result,
                        Err* err) const {
  // If there's no metadata, there's nothing to find, so quick exit.
  if (contents_.empty()) {
    next_walk_keys->emplace_back(nullptr, "");
    return true;
  }

  // Pull the data from each specified key.
  for (const auto& key : keys_to_extract) {
    auto iter = contents_.find(key);
    if (iter == contents_.end())
      continue;
    assert(iter->second.type() == Value::LIST);

    if (!rebase_dir.is_null()) {
      for (const auto& val : iter->second.list_value()) {
        std::pair<Value, bool> pair =
            this->RebaseValue(settings, rebase_dir, val, err);
        if (!pair.second)
          return false;
        result->push_back(pair.first);
      }
    } else {
      result->insert(result->end(), iter->second.list_value().begin(),
                     iter->second.list_value().end());
    }
  }

  // Get the targets to look at next. If no keys_to_walk are present, we push
  // the empty string to the list so that the target knows to include its deps
  // and data_deps. The values used here must be lists of strings.
  bool found_walk_key = false;
  for (const auto& key : keys_to_walk) {
    auto iter = contents_.find(key);
    if (iter != contents_.end()) {
      found_walk_key = true;
      assert(iter->second.type() == Value::LIST);
      for (const auto& val : iter->second.list_value()) {
        if (!val.VerifyTypeIs(Value::STRING, err))
          return false;
        next_walk_keys->emplace_back(val);
      }
    }
  }

  if (!found_walk_key)
    next_walk_keys->emplace_back(nullptr, "");

  return true;
}

std::pair<Value, bool> Metadata::RebaseValue(const BuildSettings* settings,
                                             const SourceDir& rebase_dir,
                                             const Value& value,
                                             Err* err) const {
  switch (value.type()) {
    case Value::STRING:
      return this->RebaseStringValue(settings, rebase_dir, value, err);
    case Value::LIST:
      return this->RebaseListValue(settings, rebase_dir, value, err);
    case Value::SCOPE:
      return this->RebaseScopeValue(settings, rebase_dir, value, err);
    default:
      return std::make_pair(value, true);
  }
}

std::pair<Value, bool> Metadata::RebaseStringValue(
    const BuildSettings* settings,
    const SourceDir& rebase_dir,
    const Value& value,
    Err* err) const {
  if (!value.VerifyTypeIs(Value::STRING, err))
    return std::make_pair(value, false);
  std::string filename = source_dir_.ResolveRelativeAs(
      /*as_file = */ true, value, err, settings->root_path_utf8());
  if (err->has_error())
    return std::make_pair(value, false);
  Value rebased_value(value.origin(), RebasePath(filename, rebase_dir,
                                                 settings->root_path_utf8()));
  return std::make_pair(rebased_value, true);
}

std::pair<Value, bool> Metadata::RebaseListValue(const BuildSettings* settings,
                                                 const SourceDir& rebase_dir,
                                                 const Value& value,
                                                 Err* err) const {
  if (!value.VerifyTypeIs(Value::LIST, err))
    return std::make_pair(value, false);

  Value rebased_list_value(value.origin(), Value::LIST);
  for (auto& val : value.list_value()) {
    std::pair<Value, bool> pair = RebaseValue(settings, rebase_dir, val, err);
    if (!pair.second)
      return std::make_pair(value, false);
    rebased_list_value.list_value().push_back(pair.first);
  }
  return std::make_pair(rebased_list_value, true);
}

std::pair<Value, bool> Metadata::RebaseScopeValue(const BuildSettings* settings,
                                                  const SourceDir& rebase_dir,
                                                  const Value& value,
                                                  Err* err) const {
  if (!value.VerifyTypeIs(Value::SCOPE, err))
    return std::make_pair(value, false);
  Value rebased_scope_value(value);
  Scope::KeyValueMap scope_values;
  value.scope_value()->GetCurrentScopeValues(&scope_values);
  for (auto& value_pair : scope_values) {
    std::pair<Value, bool> pair =
        RebaseValue(settings, rebase_dir, value_pair.second, err);
    if (!pair.second)
      return std::make_pair(value, false);

    rebased_scope_value.scope_value()->SetValue(value_pair.first, pair.first,
                                                value.origin());
  }
  return std::make_pair(rebased_scope_value, true);
}
