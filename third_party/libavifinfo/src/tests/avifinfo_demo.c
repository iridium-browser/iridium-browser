// Copyright (c) 2023, Alliance for Open Media. All rights reserved
//
// This source code is subject to the terms of the BSD 2 Clause License and
// the Alliance for Open Media Patent License 1.0. If the BSD 2 Clause License
// was not distributed with this source code in the LICENSE file, you can
// obtain it at www.aomedia.org/license/software. If the Alliance for Open
// Media Patent License 1.0 was not distributed with this source code in the
// PATENTS file, you can obtain it at www.aomedia.org/license/patent.

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "avifinfo.h"

//------------------------------------------------------------------------------
// Simple API

// Returns true if the file is an AVIF file.
static int Identify(const char* file_path) {
  FILE* file = fopen(file_path, "rb");
  if (!file) return 0;
  uint8_t data[32];  // File type is likely known in the first 32 bytes.
  const size_t data_size = fread(data, 1, sizeof(data), file);
  fclose(file);
  if (AvifInfoIdentify(data, data_size) == kAvifInfoOk) {
    // The file signature identifies it as AVIF.
    return 1;
  }
  return 0;
}

// Returns true if the file is an AVIF file and if a few features were
// successfully parsed.
static int IdentifyAndGetFeatures(const char* file_path) {
  FILE* file = fopen(file_path, "rb");
  if (!file) return 0;
  uint8_t data[512];  // Features are probably available within 512 bytes.
  const size_t data_size = fread(data, 1, sizeof(data), file);
  fclose(file);
  AvifInfoFeatures features;
  if (AvifInfoGetFeatures(data, data_size, &features) == kAvifInfoOk) {
    // Use features.width, features.height etc.
    return 1;
  }
  return 0;
}

//------------------------------------------------------------------------------
// Stream API

typedef struct Stream {
  FILE* file;
  uint8_t data[AVIFINFO_MAX_NUM_READ_BYTES];
} Stream;

static const uint8_t* StreamRead(void* stream, size_t num_bytes) {
  Stream* s = (Stream*)stream;
  if (!s->file) return NULL;
  const size_t data_size = fread(s->data, 1, num_bytes, s->file);
  if (num_bytes > data_size) return NULL;
  return s->data;
}

static void StreamSkip(void* stream, size_t num_bytes) {
  Stream* s = (Stream*)stream;
  if (!s->file) return;
  if (fseek(s->file, num_bytes, SEEK_CUR)) {
    fclose(s->file);
    s->file = NULL;
  }
}

// Returns true if the file is an AVIF file.
static int IdentifyStream(const char* file_path) {
  Stream stream = {0};
  stream.file = fopen(file_path, "rb");
  if (AvifInfoIdentifyStream(&stream, StreamRead, StreamSkip) == kAvifInfoOk) {
    // The file signature identifies it as AVIF.
    fclose(stream.file);
    return 1;
  }
  fclose(stream.file);
  return 0;
}

// Returns true if the file is an AVIF file and if a few features were
// successfully parsed.
static int IdentifyAndGetFeaturesStream(const char* file_path) {
  Stream stream = {0};
  stream.file = fopen(file_path, "rb");
  if (AvifInfoIdentifyStream(&stream, StreamRead, StreamSkip) == kAvifInfoOk) {
    // The file signature identifies it as AVIF.

    AvifInfoFeatures features;
    if (AvifInfoGetFeaturesStream(&stream, StreamRead, StreamSkip, &features) ==
        kAvifInfoOk) {
      // Use features.width, features.height etc.
      fclose(stream.file);
      return 1;
    }
  }
  fclose(stream.file);
  return 0;
}

// Alternative to IdentifyAndGetFeaturesStream() where the stream object cannot
// be shared between AvifInfoIdentifyStream() and AvifInfoGetFeaturesStream().
static int IdentifyAndGetFeaturesStreams(const char* file_path) {
  {
    Stream stream = {0};
    stream.file = fopen(file_path, "rb");
    if (AvifInfoIdentifyStream(&stream, StreamRead, StreamSkip) ==
        kAvifInfoOk) {
      // The file signature identifies it as AVIF.
      fclose(stream.file);
      return 1;
    }
    fclose(stream.file);
  }
  {
    Stream stream = {0};
    stream.file = fopen(file_path, "rb");
    AvifInfoFeatures features;
    // This is allowed because AvifInfoIdentifyStream() was successful on the
    // same input file bytes.
    if (AvifInfoGetFeaturesStream(&stream, StreamRead, StreamSkip, &features) ==
        kAvifInfoOk) {
      // Use features.width, features.height etc.
      fclose(stream.file);
      return 1;
    }
    fclose(stream.file);
  }
  return 0;
}

//------------------------------------------------------------------------------

int main(int argc, char** argv) {
  if (argc >= 2 && Identify(argv[1]) && IdentifyAndGetFeatures(argv[1]) &&
      IdentifyStream(argv[1]) && IdentifyAndGetFeaturesStream(argv[1]) &&
      IdentifyAndGetFeaturesStreams(argv[1])) {
    return 0;
  }
  return 1;
}
