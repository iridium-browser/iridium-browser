// Copyright 2019 The libgav1 Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <memory>
#include <new>
#include <vector>

#include "absl/strings/numbers.h"
#include "absl/time/clock.h"
#include "absl/time/time.h"
#include "examples/file_reader_factory.h"
#include "examples/file_reader_interface.h"
#include "examples/file_writer.h"
#include "gav1/decoder.h"

#ifdef GAV1_DECODE_USE_CV_PIXEL_BUFFER_POOL
#include "examples/gav1_decode_cv_pixel_buffer_pool.h"
#endif

namespace {

struct Options {
  const char* input_file_name = nullptr;
  const char* output_file_name = nullptr;
  const char* frame_timing_file_name = nullptr;
  libgav1::FileWriter::FileType output_file_type =
      libgav1::FileWriter::kFileTypeRaw;
  uint8_t post_filter_mask = 0x1f;
  int threads = 1;
  bool frame_parallel = false;
  bool output_all_layers = false;
  int operating_point = 0;
  int limit = 0;
  int skip = 0;
  int verbose = 0;
};

struct Timing {
  absl::Duration input;
  absl::Duration dequeue;
};

struct FrameTiming {
  absl::Time enqueue;
  absl::Time dequeue;
};

void PrintHelp(FILE* const fout) {
  fprintf(fout,
          "Usage: gav1_decode [options] <input file>"
          " [-o <output file>]\n");
  fprintf(fout, "\n");
  fprintf(fout, "Options:\n");
  fprintf(fout, "  -h, --help This help message.\n");
  fprintf(fout, "  --threads <positive integer> (Default 1).\n");
  fprintf(fout, "  --frame_parallel.\n");
  fprintf(fout,
          "  --limit <integer> Stop decoding after N frames (0 = all).\n");
  fprintf(fout, "  --skip <integer> Skip initial N frames (Default 0).\n");
  fprintf(fout, "  --version.\n");
  fprintf(fout, "  --y4m (Default false).\n");
  fprintf(fout, "  --raw (Default true).\n");
  fprintf(fout, "  -v logging verbosity, can be used multiple times.\n");
  fprintf(fout, "  --all_layers.\n");
  fprintf(fout,
          "  --operating_point <integer between 0 and 31> (Default 0).\n");
  fprintf(fout,
          "  --frame_timing <file> Output per-frame timing to <file> in tsv"
          " format.\n   Yields meaningful results only when frame parallel is"
          " off.\n");
  fprintf(fout, "\nAdvanced settings:\n");
  fprintf(fout, "  --post_filter_mask <integer> (Default 0x1f).\n");
  fprintf(fout,
          "   Mask indicating which post filters should be applied to the"
          " reconstructed\n   frame. This may be given as octal, decimal or"
          " hexadecimal. From LSB:\n");
  fprintf(fout, "     Bit 0: Loop filter (deblocking filter)\n");
  fprintf(fout, "     Bit 1: Cdef\n");
  fprintf(fout, "     Bit 2: SuperRes\n");
  fprintf(fout, "     Bit 3: Loop Restoration\n");
  fprintf(fout, "     Bit 4: Film Grain Synthesis\n");
}

void ParseOptions(int argc, char* argv[], Options* const options) {
  for (int i = 1; i < argc; ++i) {
    int32_t value;
    if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
      PrintHelp(stdout);
      exit(EXIT_SUCCESS);
    } else if (strcmp(argv[i], "-o") == 0) {
      if (++i >= argc) {
        fprintf(stderr, "Missing argument for '-o'\n");
        PrintHelp(stderr);
        exit(EXIT_FAILURE);
      }
      options->output_file_name = argv[i];
    } else if (strcmp(argv[i], "--frame_timing") == 0) {
      if (++i >= argc) {
        fprintf(stderr, "Missing argument for '--frame_timing'\n");
        PrintHelp(stderr);
        exit(EXIT_FAILURE);
      }
      options->frame_timing_file_name = argv[i];
    } else if (strcmp(argv[i], "--version") == 0) {
      printf("gav1_decode, a libgav1 based AV1 decoder\n");
      printf("libgav1 %s\n", libgav1::GetVersionString());
      printf("max bitdepth: %d\n", libgav1::Decoder::GetMaxBitdepth());
      printf("build configuration: %s\n", libgav1::GetBuildConfiguration());
      exit(EXIT_SUCCESS);
    } else if (strcmp(argv[i], "-v") == 0) {
      ++options->verbose;
    } else if (strcmp(argv[i], "--raw") == 0) {
      options->output_file_type = libgav1::FileWriter::kFileTypeRaw;
    } else if (strcmp(argv[i], "--y4m") == 0) {
      options->output_file_type = libgav1::FileWriter::kFileTypeY4m;
    } else if (strcmp(argv[i], "--threads") == 0) {
      if (++i >= argc || !absl::SimpleAtoi(argv[i], &value)) {
        fprintf(stderr, "Missing/Invalid value for --threads.\n");
        PrintHelp(stderr);
        exit(EXIT_FAILURE);
      }
      options->threads = value;
    } else if (strcmp(argv[i], "--frame_parallel") == 0) {
      options->frame_parallel = true;
    } else if (strcmp(argv[i], "--all_layers") == 0) {
      options->output_all_layers = true;
    } else if (strcmp(argv[i], "--operating_point") == 0) {
      if (++i >= argc || !absl::SimpleAtoi(argv[i], &value) || value < 0 ||
          value >= 32) {
        fprintf(stderr, "Missing/Invalid value for --operating_point.\n");
        PrintHelp(stderr);
        exit(EXIT_FAILURE);
      }
      options->operating_point = value;
    } else if (strcmp(argv[i], "--limit") == 0) {
      if (++i >= argc || !absl::SimpleAtoi(argv[i], &value) || value < 0) {
        fprintf(stderr, "Missing/Invalid value for --limit.\n");
        PrintHelp(stderr);
        exit(EXIT_FAILURE);
      }
      options->limit = value;
    } else if (strcmp(argv[i], "--skip") == 0) {
      if (++i >= argc || !absl::SimpleAtoi(argv[i], &value) || value < 0) {
        fprintf(stderr, "Missing/Invalid value for --skip.\n");
        PrintHelp(stderr);
        exit(EXIT_FAILURE);
      }
      options->skip = value;
    } else if (strcmp(argv[i], "--post_filter_mask") == 0) {
      errno = 0;
      char* endptr = nullptr;
      value = (++i >= argc) ? -1
                            // NOLINTNEXTLINE(runtime/deprecated_fn)
                            : static_cast<int32_t>(strtol(argv[i], &endptr, 0));
      // Only the last 5 bits of the mask can be set.
      if ((value & ~31) != 0 || errno != 0 || endptr == argv[i]) {
        fprintf(stderr, "Invalid value for --post_filter_mask.\n");
        PrintHelp(stderr);
        exit(EXIT_FAILURE);
      }
      options->post_filter_mask = value;
    } else if (strlen(argv[i]) > 1 && argv[i][0] == '-') {
      fprintf(stderr, "Unknown option '%s'!\n", argv[i]);
      exit(EXIT_FAILURE);
    } else {
      if (options->input_file_name == nullptr) {
        options->input_file_name = argv[i];
      } else {
        fprintf(stderr, "Found invalid parameter: \"%s\".\n", argv[i]);
        PrintHelp(stderr);
        exit(EXIT_FAILURE);
      }
    }
  }

  if (argc < 2 || options->input_file_name == nullptr) {
    fprintf(stderr, "Input file is required!\n");
    PrintHelp(stderr);
    exit(EXIT_FAILURE);
  }
}

using InputBuffer = std::vector<uint8_t>;

class InputBuffers {
 public:
  ~InputBuffers() {
    for (auto buffer : free_buffers_) {
      delete buffer;
    }
  }
  InputBuffer* GetFreeBuffer() {
    if (free_buffers_.empty()) {
      auto* const buffer = new (std::nothrow) InputBuffer();
      if (buffer == nullptr) {
        fprintf(stderr, "Failed to create input buffer.\n");
        return nullptr;
      }
      free_buffers_.push_back(buffer);
    }
    InputBuffer* const buffer = free_buffers_.front();
    free_buffers_.pop_front();
    return buffer;
  }

  void ReleaseInputBuffer(InputBuffer* buffer) {
    free_buffers_.push_back(buffer);
  }

 private:
  std::deque<InputBuffer*> free_buffers_;
};

void ReleaseInputBuffer(void* callback_private_data,
                        void* buffer_private_data) {
  auto* const input_buffers = static_cast<InputBuffers*>(callback_private_data);
  input_buffers->ReleaseInputBuffer(
      static_cast<InputBuffer*>(buffer_private_data));
}

int CloseFile(FILE* stream) { return (stream == nullptr) ? 0 : fclose(stream); }

}  // namespace

int main(int argc, char* argv[]) {
  Options options;
  ParseOptions(argc, argv, &options);

  auto file_reader =
      libgav1::FileReaderFactory::OpenReader(options.input_file_name);
  if (file_reader == nullptr) {
    fprintf(stderr, "Cannot open input file!\n");
    return EXIT_FAILURE;
  }

  std::unique_ptr<FILE, decltype(&CloseFile)> frame_timing_file(nullptr,
                                                                &CloseFile);
  if (options.frame_timing_file_name != nullptr) {
    frame_timing_file.reset(fopen(options.frame_timing_file_name, "wb"));
    if (frame_timing_file == nullptr) {
      fprintf(stderr, "Cannot open frame timing file '%s'!\n",
              options.frame_timing_file_name);
      return EXIT_FAILURE;
    }
  }

#ifdef GAV1_DECODE_USE_CV_PIXEL_BUFFER_POOL
  // Reference frames + 1 scratch frame (for either the current frame or the
  // film grain frame).
  constexpr int kNumBuffers = 8 + 1;
  std::unique_ptr<Gav1DecodeCVPixelBufferPool> cv_pixel_buffers =
      Gav1DecodeCVPixelBufferPool::Create(kNumBuffers);
  if (cv_pixel_buffers == nullptr) {
    fprintf(stderr, "Cannot create Gav1DecodeCVPixelBufferPool!\n");
    return EXIT_FAILURE;
  }
#endif

  InputBuffers input_buffers;
  libgav1::Decoder decoder;
  libgav1::DecoderSettings settings;
  settings.post_filter_mask = options.post_filter_mask;
  settings.threads = options.threads;
  settings.frame_parallel = options.frame_parallel;
  settings.output_all_layers = options.output_all_layers;
  settings.operating_point = options.operating_point;
  settings.blocking_dequeue = true;
  settings.callback_private_data = &input_buffers;
  settings.release_input_buffer = ReleaseInputBuffer;
#ifdef GAV1_DECODE_USE_CV_PIXEL_BUFFER_POOL
  settings.on_frame_buffer_size_changed = Gav1DecodeOnCVPixelBufferSizeChanged;
  settings.get_frame_buffer = Gav1DecodeGetCVPixelBuffer;
  settings.release_frame_buffer = Gav1DecodeReleaseCVPixelBuffer;
  settings.callback_private_data = cv_pixel_buffers.get();
  settings.release_input_buffer = nullptr;
  // TODO(vigneshv): Support frame parallel mode to be used with
  // CVPixelBufferPool.
  settings.frame_parallel = false;
#endif
  libgav1::StatusCode status = decoder.Init(&settings);
  if (status != libgav1::kStatusOk) {
    fprintf(stderr, "Error initializing decoder: %s\n",
            libgav1::GetErrorString(status));
    return EXIT_FAILURE;
  }

  fprintf(stderr, "decoding '%s'\n", options.input_file_name);
  if (options.verbose > 0 && options.skip > 0) {
    fprintf(stderr, "skipping %d frame(s).\n", options.skip);
  }

  int input_frames = 0;
  int decoded_frames = 0;
  Timing timing = {};
  std::vector<FrameTiming> frame_timing;
  const bool record_frame_timing = frame_timing_file != nullptr;
  std::unique_ptr<libgav1::FileWriter> file_writer;
  InputBuffer* input_buffer = nullptr;
  bool limit_reached = false;
  bool dequeue_finished = false;
  const absl::Time decode_loop_start = absl::Now();
  do {
    if (input_buffer == nullptr && !file_reader->IsEndOfFile() &&
        !limit_reached) {
      input_buffer = input_buffers.GetFreeBuffer();
      if (input_buffer == nullptr) return EXIT_FAILURE;
      const absl::Time read_start = absl::Now();
      if (!file_reader->ReadTemporalUnit(input_buffer,
                                         /*timestamp=*/nullptr)) {
        fprintf(stderr, "Error reading input file.\n");
        return EXIT_FAILURE;
      }
      timing.input += absl::Now() - read_start;
    }

    if (++input_frames <= options.skip) {
      input_buffers.ReleaseInputBuffer(input_buffer);
      input_buffer = nullptr;
      continue;
    }

    if (input_buffer != nullptr) {
      if (input_buffer->empty()) {
        input_buffers.ReleaseInputBuffer(input_buffer);
        input_buffer = nullptr;
        continue;
      }

      const absl::Time enqueue_start = absl::Now();
      status = decoder.EnqueueFrame(input_buffer->data(), input_buffer->size(),
                                    static_cast<int64_t>(frame_timing.size()),
                                    /*buffer_private_data=*/input_buffer);
      if (status == libgav1::kStatusOk) {
        if (options.verbose > 1) {
          fprintf(stderr, "enqueue frame (length %zu)\n", input_buffer->size());
        }
        if (record_frame_timing) {
          FrameTiming enqueue_time = {enqueue_start, absl::UnixEpoch()};
          frame_timing.emplace_back(enqueue_time);
        }

        input_buffer = nullptr;
        // Continue to enqueue frames until we get a kStatusTryAgain status.
        continue;
      }
      if (status != libgav1::kStatusTryAgain) {
        fprintf(stderr, "Unable to enqueue frame: %s\n",
                libgav1::GetErrorString(status));
        return EXIT_FAILURE;
      }
    }

    const libgav1::DecoderBuffer* buffer;
    status = decoder.DequeueFrame(&buffer);
    if (status == libgav1::kStatusNothingToDequeue) {
      dequeue_finished = true;
      continue;
    }
    if (status != libgav1::kStatusOk) {
      fprintf(stderr, "Unable to dequeue frame: %s\n",
              libgav1::GetErrorString(status));
      return EXIT_FAILURE;
    }
    dequeue_finished = false;
    if (buffer == nullptr) continue;
    ++decoded_frames;
    if (options.verbose > 1) {
      fprintf(stderr, "buffer dequeued\n");
    }

    if (record_frame_timing) {
      frame_timing[static_cast<int>(buffer->user_private_data)].dequeue =
          absl::Now();
    }

    if (options.output_file_name != nullptr && file_writer == nullptr) {
      libgav1::FileWriter::Y4mParameters y4m_parameters;
      y4m_parameters.width = buffer->displayed_width[0];
      y4m_parameters.height = buffer->displayed_height[0];
      y4m_parameters.frame_rate_numerator = file_reader->frame_rate();
      y4m_parameters.frame_rate_denominator = file_reader->time_scale();
      y4m_parameters.chroma_sample_position = buffer->chroma_sample_position;
      y4m_parameters.image_format = buffer->image_format;
      y4m_parameters.bitdepth = static_cast<size_t>(buffer->bitdepth);
      file_writer = libgav1::FileWriter::Open(
          options.output_file_name, options.output_file_type, &y4m_parameters);
      if (file_writer == nullptr) {
        fprintf(stderr, "Cannot open output file!\n");
        return EXIT_FAILURE;
      }
    }

    if (!limit_reached && file_writer != nullptr &&
        !file_writer->WriteFrame(*buffer)) {
      fprintf(stderr, "Error writing output file.\n");
      return EXIT_FAILURE;
    }
    if (options.limit > 0 && options.limit == decoded_frames) {
      limit_reached = true;
      if (input_buffer != nullptr) {
        input_buffers.ReleaseInputBuffer(input_buffer);
      }
      input_buffer = nullptr;
      // Clear any in progress frames to ensure the output frame limit is
      // respected.
      decoder.SignalEOS();
    }
  } while (input_buffer != nullptr ||
           (!file_reader->IsEndOfFile() && !limit_reached) ||
           !dequeue_finished);
  timing.dequeue = absl::Now() - decode_loop_start - timing.input;

  if (record_frame_timing) {
    // Note timing for frame parallel will be skewed by the time spent queueing
    // additional frames and in the output queue waiting for previous frames,
    // the values reported won't be that meaningful.
    fprintf(frame_timing_file.get(), "frame number\tdecode time us\n");
    for (size_t i = 0; i < frame_timing.size(); ++i) {
      const int decode_time_us = static_cast<int>(absl::ToInt64Microseconds(
          frame_timing[i].dequeue - frame_timing[i].enqueue));
      fprintf(frame_timing_file.get(), "%zu\t%d\n", i, decode_time_us);
    }
  }

  if (options.verbose > 0) {
    fprintf(stderr, "time to read input: %d us\n",
            static_cast<int>(absl::ToInt64Microseconds(timing.input)));
    const int decode_time_us =
        static_cast<int>(absl::ToInt64Microseconds(timing.dequeue));
    const double decode_fps =
        (decode_time_us == 0) ? 0.0 : 1.0e6 * decoded_frames / decode_time_us;
    fprintf(stderr, "time to decode input: %d us (%d frames, %.2f fps)\n",
            decode_time_us, decoded_frames, decode_fps);
  }

  return EXIT_SUCCESS;
}
