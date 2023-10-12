// Copyright (c) 2021, Alliance for Open Media. All rights reserved
//
// This source code is subject to the terms of the BSD 2 Clause License and
// the Alliance for Open Media Patent License 1.0. If the BSD 2 Clause License
// was not distributed with this source code in the LICENSE file, you can
// obtain it at www.aomedia.org/license/software. If the Alliance for Open
// Media Patent License 1.0 was not distributed with this source code in the
// PATENTS file, you can obtain it at www.aomedia.org/license/patent.

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <filesystem>  // NOLINT
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <unordered_map>
#include <vector>

#include "avif/avif.h"
#include "avifinfo.h"

namespace {

//------------------------------------------------------------------------------

std::string GetHelpStr() {
  std::string str;
  str += "Command line tool to compare libavif and libavifinfo results.\n";
  str += "Usage:   avifparse [options] <directory>\n";
  str += "Options:\n";
  str += "  -h, --help ...... Print this help\n";
  str += "  --fast .......... Skip libavif decoding, only use libavifinfo\n";
  str += "  --min-size ...... Find minimum size to extract features per file\n";
  str += "  --validate ...... Check libavifinfo consistency on each file\n";
  str += "  --no-bad-file ... Return an error code in case of invalid file\n";
  return str;
}

//------------------------------------------------------------------------------

// Decoding result.
struct Result {
  bool success;  // True if the 'features' were correctly decoded.
  AvifInfoFeatures features;
};

// Decodes the AVIF at 'data' of 'data_size' bytes using libavif.
Result DecodeAvif(const uint8_t data[], size_t data_size) {
  Result result;
  avifImage* const image = avifImageCreateEmpty();
  avifDecoder* const decoder = avifDecoderCreate();
  decoder->strictFlags = AVIF_STRICT_DISABLED;
  const avifResult status =
      avifDecoderReadMemory(decoder, image, data, data_size);
  avifDecoderDestroy(decoder);
  if (status == AVIF_RESULT_OK) {
    const uint32_t num_channels =
        ((image->yuvFormat == AVIF_PIXEL_FORMAT_NONE)     ? 0
         : (image->yuvFormat == AVIF_PIXEL_FORMAT_YUV400) ? 1
                                                          : 3) +
        ((image->alphaPlane != nullptr) ? 1 : 0);
    result = {true, {image->width, image->height, image->depth, num_channels}};
  } else {
    result = {false};
  }
  avifImageDestroy(image);
  return result;
}

// Parses the AVIF at 'data' of 'data_size' bytes using libavifinfo.
Result ParseAvif(const uint8_t data[], size_t data_size) {
  Result result;
  result.success =
      (AvifInfoIdentify(data, data_size) == kAvifInfoOk &&
       AvifInfoGetFeatures(data, data_size, &result.features) == kAvifInfoOk);
  return result;
}

// Same as above but also returns the 'min_data_size' for which 'data' can be
// successfully parsed.
Result ParseAvifForSize(const uint8_t data[], size_t data_size,
                        size_t& min_data_size) {
  const Result result = ParseAvif(data, data_size);
  if (!result.success) {
    min_data_size = data_size;
    return result;
  }
  min_data_size = 1;
  size_t max_data_size = data_size;
  while (min_data_size < max_data_size) {
    const size_t middle = (min_data_size + max_data_size) / 2;
    if (ParseAvif(data, middle).success) {
      max_data_size = middle;
    } else {
      min_data_size = middle + 1;
    }
  }
  return result;
}

// Reuses the fuzz target for easy library validation.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t data_size);

// Aggregated stats about the decoded/parsed AVIF files.
struct Stats {
  uint32_t num_files_invalid_at_decode = 0;
  uint32_t num_files_invalid_at_parse = 0;
  uint32_t num_files_invalid_at_both = 0;
  std::unordered_map<size_t, uint32_t> min_size_to_count;
};

//------------------------------------------------------------------------------

// Recursively adds all files at 'path' to 'file_paths'.
void FindFiles(const std::string& path, std::vector<std::string>& file_paths) {
  if (std::filesystem::is_directory(path)) {
    for (const std::filesystem::directory_entry& entry :
         std::filesystem::directory_iterator(path)) {
      FindFiles(entry.path(), file_paths);
    }
  } else {
    file_paths.emplace_back(path);
  }
}

// Find the longest common prefix of all input 'paths'.
std::string FindCommonLongestPrefix(const std::vector<std::string>& paths) {
  std::string prefix = paths.empty() ? "" : paths.front();
  for (const std::string& path : paths) {
    const auto mismatch =
        std::mismatch(prefix.begin(), prefix.end(), path.begin(), path.end());
    prefix = prefix.substr(0, std::distance(prefix.begin(), mismatch.first));
  }
  return std::filesystem::path(prefix).remove_filename().string();
}

//------------------------------------------------------------------------------

// Uses libavifinfo to extract the features of an AVIF file stored in 'data' at
// 'path'. The AVIF file is 'data_size'-byte long.
void ParseFile(const std::string& path, const uint8_t* data, size_t data_size,
               Stats& stats) {
  const Result parse = ParseAvif(data, data_size);
  if (!parse.success) {
    ++stats.num_files_invalid_at_parse;
    std::cout << "parsing failure for " << path << std::endl;
  }
}

// Uses libavif then libavifinfo to extract the features of an AVIF file.
// Returns false in case of libavifinfo parsing failure or behavior
// inconsistency compared to libavif.
bool DecodeAndParseFile(const std::string& path, const uint8_t* data,
                        size_t data_size, Stats& stats) {
  const Result decode = DecodeAvif(data, data_size);
  const Result parse = ParseAvif(data, data_size);
  if (!decode.success) ++stats.num_files_invalid_at_decode;
  if (!parse.success) ++stats.num_files_invalid_at_parse;
  if (!decode.success && !parse.success) ++stats.num_files_invalid_at_both;

  if (!parse.success ||
      (decode.success &&
       (decode.features.width != parse.features.width ||
        decode.features.height != parse.features.height ||
        decode.features.bit_depth != parse.features.bit_depth ||
        decode.features.num_channels != parse.features.num_channels))) {
    if (decode.success && parse.success) {
      std::cout << "decoded " << decode.features.width << "x"
                << decode.features.height << "," << decode.features.bit_depth
                << "b*" << decode.features.num_channels << " / "
                << "parsed " << parse.features.width << "x"
                << parse.features.height << "," << parse.features.bit_depth
                << "b*" << parse.features.num_channels;
    } else {
      std::cout << "decoding " << (decode.success ? "success" : "failure")
                << " / parsing " << (parse.success ? "success" : "failure");
    }
    std::cout << " for " << path << std::endl;
    return false;
  }
  return true;
}

// Returns the minimum number of bytes of AVIF 'data' for features to be
// extracted.
void FindMinSizeOfFile(const std::string& path, const uint8_t* data,
                       size_t data_size, Stats& stats) {
  size_t min_size;
  const Result parse = ParseAvifForSize(data, data_size, min_size);
  if (parse.success) {
    ++stats.min_size_to_count[min_size];
  } else {
    ++stats.num_files_invalid_at_parse;
  }
}

// Checks the consistency of libavifinfo over an AVIF file.
// Returns false in case of error.
bool ValidateFile(const std::string& path, const uint8_t* data,
                  size_t data_size) {
  if (LLVMFuzzerTestOneInput(data, data_size) != 0) {
    std::cout << "validation failed for " << path << std::endl;
    return false;
  }
  return true;
}

}  // namespace

//------------------------------------------------------------------------------

int main(int argc, char** argv) {
  std::vector<std::string> file_paths;
  bool only_parse = false;
  bool find_min_size = false;
  bool validate = false;
  bool error_on_bad_file = false;

  for (int arg = 1; arg < argc; ++arg) {
    if (!std::strcmp(argv[arg], "-h")) {
      std::cout << GetHelpStr();
      return 0;
    } else if (!std::strcmp(argv[arg], "--fast")) {
      only_parse = true;
    } else if (!std::strcmp(argv[arg], "--min-size")) {
      find_min_size = true;
      only_parse = true;
    } else if (!std::strcmp(argv[arg], "--validate")) {
      validate = true;
    } else if (!std::strcmp(argv[arg], "--no-bad-file")) {
      error_on_bad_file = true;
    } else {
      FindFiles(argv[arg], file_paths);
    }
  }
  if (file_paths.empty()) {
    std::cerr << "No input specified" << std::endl;
    return 1;
  }
  std::cout << "Found " << file_paths.size() << " files" << std::endl;
  const std::string prefix = FindCommonLongestPrefix(file_paths);
  for (std::string& file_path : file_paths) {
    file_path = file_path.substr(prefix.size());
  }

  Stats stats;
  bool success = true;
  for (const std::string& file_path : file_paths) {
    std::vector<uint8_t> bytes(std::filesystem::file_size(prefix + file_path));
    std::ifstream file(prefix + file_path, std::ios::binary);
    file.read(reinterpret_cast<char*>(bytes.data()), bytes.size());
    if (find_min_size) {
      FindMinSizeOfFile(file_path, bytes.data(), bytes.size(), stats);
    } else if (only_parse) {
      ParseFile(file_path, bytes.data(), bytes.size(), stats);
    } else if (!DecodeAndParseFile(file_path, bytes.data(), bytes.size(),
                                   stats)) {
      success = false;
    }
    if (validate && !ValidateFile(file_path, bytes.data(), bytes.size())) {
      success = false;
    }
  }

  std::cout << stats.num_files_invalid_at_parse << " files failed to parse"
            << std::endl;
  if (!only_parse) {
    std::cout << stats.num_files_invalid_at_decode << " files failed to decode"
              << std::endl;
    std::cout << stats.num_files_invalid_at_both
              << " files failed to parse and decode" << std::endl;
  }

  if (find_min_size) {
    std::cout << std::endl;
    for (const auto& it : stats.min_size_to_count) {
      std::cout << it.second << " files need " << it.first
                << " bytes to extract features" << std::endl;
    }
  }

  if (error_on_bad_file && (stats.num_files_invalid_at_parse > 0 ||
                            stats.num_files_invalid_at_decode > 0)) {
    success = false;
  }
  return success ? 0 : 1;
}
