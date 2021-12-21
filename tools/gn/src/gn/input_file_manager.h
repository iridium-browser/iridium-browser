// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_GN_INPUT_FILE_MANAGER_H_
#define TOOLS_GN_INPUT_FILE_MANAGER_H_

#include <functional>
#include <mutex>
#include <set>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/files/file_path.h"
#include "base/memory/ref_counted.h"
#include "gn/input_file.h"
#include "gn/parse_tree.h"
#include "gn/settings.h"
#include "gn/vector_utils.h"
#include "util/auto_reset_event.h"

class BuildSettings;
class Err;
class LocationRange;
class ParseNode;
class Token;

// Manages loading and parsing files from disk. This doesn't actually have
// any context for executing the results, so potentially multiple configs
// could use the same input file (saving parsing).
//
// This class is threadsafe.
//
// InputFile objects must never be deleted while the program is running since
// various state points into them.
class InputFileManager : public base::RefCountedThreadSafe<InputFileManager> {
 public:
  // Callback issued when a file is loaded. On auccess, the parse node will
  // refer to the root block of the file. On failure, this will be NULL.
  using FileLoadCallback = std::function<void(const ParseNode*)>;

  // Callback to emulate SyncLoadFile in tests.
  using SyncLoadFileCallback =
      std::function<bool(const SourceFile& file_name, InputFile* file)>;

  InputFileManager();

  // Loads the given file and executes the callback on the worker pool.
  //
  // There are two types of errors. For errors known synchronously, the error
  // will be set, it will return false, and no work will be scheduled.
  //
  // For parse errors and such that happen in the future, the error will be
  // logged to the scheduler and the callback will be invoked with a null
  // ParseNode pointer. The given |origin| will be blamed for the invocation.
  bool AsyncLoadFile(const LocationRange& origin,
                     const BuildSettings* build_settings,
                     const SourceFile& file_name,
                     const FileLoadCallback& callback,
                     Err* err);

  // Loads and parses the given file synchronously, returning the root block
  // corresponding to the parsed result. On error, return NULL and the given
  // Err is set.
  const ParseNode* SyncLoadFile(const LocationRange& origin,
                                const BuildSettings* build_settings,
                                const SourceFile& file_name,
                                Err* err);

  // Creates an entry to manage the memory associated with keeping a parsed
  // set of code in memory.
  //
  // The values pointed to by the parameters will be filled with pointers to
  // the file, tokens, and parse node that this class created. The calling
  // code is responsible for populating these values and maintaining
  // threadsafety. This class' only job is to hold onto the memory and delete
  // it when the program exits.
  //
  // This solves the problem that sometimes we need to execute something
  // dynamic and save the result, but the values all have references to the
  // nodes and file that created it. Either we need to reset the origin of
  // the values and lose context for error reporting, or somehow keep the
  // associated parse nodes, tokens, and file data in memory. This function
  // allows the latter.
  void AddDynamicInput(const SourceFile& name,
                       InputFile** file,
                       std::vector<Token>** tokens,
                       std::unique_ptr<ParseNode>** parse_root);

  // Does not count dynamic input.
  int GetInputFileCount() const;

  // Add all physical input files to a VectorSetSorter instance.
  // This allows fast merging and sorting with other file paths sets.
  //
  // This is more memory efficient than returning a vector of base::FilePath
  // instance, especially with projects with a very large number of input files,
  // but note that the VectorSetSorter only holds pointers to the
  // items recorded in this InputFileManager instance, and it is up to the
  // caller to ensure these will not change until the sorter is destroyed.
  void AddAllPhysicalInputFileNamesToVectorSetSorter(
      VectorSetSorter<base::FilePath>* sorter) const;

  void set_load_file_callback(SyncLoadFileCallback load_file_callback) {
    load_file_callback_ = load_file_callback;
  }

 private:
  friend class base::RefCountedThreadSafe<InputFileManager>;

  struct InputFileData {
    explicit InputFileData(const SourceFile& file_name);
    ~InputFileData();

    // Don't touch this outside the lock until it's marked loaded.
    InputFile file;

    bool loaded;

    bool sync_invocation;

    // Lists all invocations that need to be executed when the file completes
    // loading.
    std::vector<FileLoadCallback> scheduled_callbacks;

    // Event to signal when the load is complete (or fails). This is lazily
    // created only when a thread is synchronously waiting for this load (which
    // only happens for imports).
    std::unique_ptr<AutoResetEvent> completion_event;

    std::vector<Token> tokens;

    // Null before the file is loaded or if loading failed.
    std::unique_ptr<ParseNode> parsed_root;
    Err parse_error;
  };

  virtual ~InputFileManager();

  void BackgroundLoadFile(const LocationRange& origin,
                          const BuildSettings* build_settings,
                          const SourceFile& name,
                          InputFile* file);

  // Loads the given file. On error, sets the Err and return false.
  bool LoadFile(const LocationRange& origin,
                const BuildSettings* build_settings,
                const SourceFile& name,
                InputFile* file,
                Err* err);

  mutable std::mutex lock_;

  // Maps repo-relative filenames to the corresponding owned pointer.
  using InputFileMap =
      std::unordered_map<SourceFile, std::unique_ptr<InputFileData>>;
  InputFileMap input_files_;

  // Tracks all dynamic inputs. The data are holders for memory management
  // purposes and should not be read or modified by this class. The values
  // will be vended out to the code creating the dynamic input, who is in
  // charge of the threadsafety requirements.
  //
  // See AddDynamicInput().
  std::vector<std::unique_ptr<InputFileData>> dynamic_inputs_;

  // Used by unit tests to mock out SyncLoadFile().
  SyncLoadFileCallback load_file_callback_;

  InputFileManager(const InputFileManager&) = delete;
  InputFileManager& operator=(const InputFileManager&) = delete;
};

#endif  // TOOLS_GN_INPUT_FILE_MANAGER_H_
