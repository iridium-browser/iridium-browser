/*
 * Copyright 2021 The libgav1 Authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifdef __cplusplus
#error Do not compile this file with a C++ compiler
#endif

// clang-format off
#include "src/gav1/decoder.h"
// clang-format on

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define ASSERT_EQ(a, b)                                                      \
  do {                                                                       \
    if ((a) != (b)) {                                                        \
      fprintf(stderr, "Assertion failure: (%s) == (%s), at %s:%d\n", #a, #b, \
              __FILE__, __LINE__);                                           \
      fprintf(stderr, "C DecoderTest failed\n");                             \
      exit(1);                                                               \
    }                                                                        \
  } while (0)

#define ASSERT_NE(a, b)                                                      \
  do {                                                                       \
    if ((a) == (b)) {                                                        \
      fprintf(stderr, "Assertion failure: (%s) != (%s), at %s:%d\n", #a, #b, \
              __FILE__, __LINE__);                                           \
      fprintf(stderr, "C DecoderTest failed\n");                             \
      exit(1);                                                               \
    }                                                                        \
  } while (0)

#define ASSERT_TRUE(a)                                                   \
  do {                                                                   \
    if (!(a)) {                                                          \
      fprintf(stderr, "Assertion failure: %s, at %s:%d\n", #a, __FILE__, \
              __LINE__);                                                 \
      fprintf(stderr, "C DecoderTest failed\n");                         \
      exit(1);                                                           \
    }                                                                    \
  } while (0)

#define ASSERT_FALSE(a)                                                     \
  do {                                                                      \
    if (a) {                                                                \
      fprintf(stderr, "Assertion failure: !(%s), at %s:%d\n", #a, __FILE__, \
              __LINE__);                                                    \
      fprintf(stderr, "C DecoderTest failed\n");                            \
      exit(1);                                                              \
    }                                                                       \
  } while (0)

// These two frames come from the libaom test vector av1-1-b8-01-size-32x32.ivf
static const uint8_t kFrame1[] = {
    0x12, 0x0,  0xa,  0xa,  0x0,  0x0,  0x0,  0x2,  0x27, 0xfe, 0xff, 0xfc,
    0xc0, 0x20, 0x32, 0x93, 0x2,  0x10, 0x0,  0xa8, 0x80, 0x0,  0x3,  0x0,
    0x10, 0x10, 0x30, 0x0,  0xd3, 0xc6, 0xc6, 0x82, 0xaa, 0x5e, 0xbf, 0x82,
    0xf2, 0xa4, 0xa4, 0x29, 0xab, 0xda, 0xd7, 0x1,  0x5,  0x0,  0xb3, 0xde,
    0xa8, 0x6f, 0x8d, 0xbf, 0x1b, 0xa8, 0x25, 0xc3, 0x84, 0x7c, 0x1a, 0x2b,
    0x8b, 0x0,  0xff, 0x19, 0x1f, 0x45, 0x7e, 0xe0, 0xbe, 0xe1, 0x3a, 0x63,
    0xc2, 0xc6, 0x6e, 0xf4, 0xc8, 0xce, 0x11, 0xe1, 0x9f, 0x48, 0x64, 0x72,
    0xeb, 0xbb, 0x4f, 0xf3, 0x94, 0xb4, 0xb6, 0x9d, 0x4f, 0x4,  0x18, 0x5e,
    0x5e, 0x1b, 0x65, 0x49, 0x74, 0x90, 0x13, 0x50, 0xef, 0x8c, 0xb8, 0xe8,
    0xd9, 0x8e, 0x9c, 0xc9, 0x4d, 0xda, 0x60, 0x6a, 0xa,  0xf9, 0x75, 0xd0,
    0x62, 0x69, 0xd,  0xf5, 0xdc, 0xa9, 0xb9, 0x4c, 0x8,  0x9e, 0x33, 0x15,
    0xa3, 0xe1, 0x42, 0x0,  0xe2, 0xb0, 0x46, 0xd0, 0xf7, 0xad, 0x55, 0xbc,
    0x75, 0xe9, 0xe3, 0x1f, 0xa3, 0x41, 0x11, 0xba, 0xaa, 0x81, 0xf3, 0xcb,
    0x82, 0x87, 0x71, 0x0,  0xe6, 0xb9, 0x8c, 0xe1, 0xe9, 0xd3, 0x21, 0xcc,
    0xcd, 0xe7, 0x12, 0xb9, 0xe,  0x43, 0x6a, 0xa3, 0x76, 0x5c, 0x35, 0x90,
    0x45, 0x36, 0x52, 0xb4, 0x2d, 0xa3, 0x55, 0xde, 0x20, 0xf8, 0x80, 0xe1,
    0x26, 0x46, 0x1b, 0x3f, 0x59, 0xc7, 0x2e, 0x5b, 0x4a, 0x73, 0xf8, 0xb3,
    0xf4, 0x62, 0xf4, 0xf5, 0xa4, 0xc2, 0xae, 0x9e, 0xa6, 0x9c, 0x10, 0xbb,
    0xe1, 0xd6, 0x88, 0x75, 0xb9, 0x85, 0x48, 0xe5, 0x7,  0x12, 0xf3, 0x11,
    0x85, 0x8e, 0xa2, 0x95, 0x9d, 0xed, 0x50, 0xfb, 0x6,  0x5a, 0x1,  0x37,
    0xc4, 0x8e, 0x9e, 0x73, 0x9b, 0x96, 0x64, 0xbd, 0x42, 0xb,  0x80, 0xde,
    0x57, 0x86, 0xcb, 0x7d, 0xab, 0x12, 0xb2, 0xcc, 0xe6, 0xea, 0xb5, 0x89,
    0xeb, 0x91, 0xb3, 0x93, 0xb2, 0x4f, 0x2f, 0x5b, 0xf3, 0x72, 0x12, 0x51,
    0x56, 0x75, 0xb3, 0xdd, 0x49, 0xb6, 0x5b, 0x77, 0xbe, 0xc5, 0xd7, 0xd4,
    0xaf, 0xd6, 0x6b, 0x38};

static const uint8_t kFrame2[] = {
    0x12, 0x0,  0x32, 0x33, 0x30, 0x3,  0xc3, 0x0,  0xa7, 0x2e, 0x46,
    0xa8, 0x80, 0x0,  0x3,  0x0,  0x10, 0x1,  0x0,  0xa0, 0x0,  0xed,
    0xb1, 0x51, 0x15, 0x58, 0xc7, 0x69, 0x3,  0x26, 0x35, 0xeb, 0x5a,
    0x2d, 0x7a, 0x53, 0x24, 0x26, 0x20, 0xa6, 0x11, 0x7,  0x49, 0x76,
    0xa3, 0xc7, 0x62, 0xf8, 0x3,  0x32, 0xb0, 0x98, 0x17, 0x3d, 0x80};

typedef struct DecoderTest {
  Libgav1Decoder* decoder;
  int frames_in_use;
  void* buffer_private_data;
  void* released_input_buffer;
} DecoderTest;

static void DecoderTestInit(DecoderTest* test) {
  test->decoder = NULL;
  test->frames_in_use = 0;
  test->buffer_private_data = NULL;
  test->released_input_buffer = NULL;
}

static void DecoderTestIncrementFramesInUse(DecoderTest* test) {
  ++test->frames_in_use;
}

static void DecoderTestDecrementFramesInUse(DecoderTest* test) {
  --test->frames_in_use;
}

static void DecoderTestSetReleasedInputBuffer(DecoderTest* test,
                                              void* released_input_buffer) {
  test->released_input_buffer = released_input_buffer;
}

static void DecoderTestSetBufferPrivateData(DecoderTest* test,
                                            void* buffer_private_data) {
  test->buffer_private_data = buffer_private_data;
}

typedef struct FrameBufferPrivate {
  uint8_t* data[3];
} FrameBufferPrivate;

static Libgav1StatusCode GetFrameBuffer(
    void* callback_private_data, int bitdepth, Libgav1ImageFormat image_format,
    int width, int height, int left_border, int right_border, int top_border,
    int bottom_border, int stride_alignment, Libgav1FrameBuffer* frame_buffer) {
  Libgav1FrameBufferInfo info;
  Libgav1StatusCode status = Libgav1ComputeFrameBufferInfo(
      bitdepth, image_format, width, height, left_border, right_border,
      top_border, bottom_border, stride_alignment, &info);
  if (status != kLibgav1StatusOk) return status;

  FrameBufferPrivate* buffer_private =
      (FrameBufferPrivate*)malloc(sizeof(FrameBufferPrivate));
  if (buffer_private == NULL) return kLibgav1StatusOutOfMemory;

  for (int i = 0; i < 3; ++i) {
    const size_t size = (i == 0) ? info.y_buffer_size : info.uv_buffer_size;
    buffer_private->data[i] = (uint8_t*)malloc(sizeof(uint8_t) * size);
    if (buffer_private->data[i] == NULL) {
      for (int j = 0; j < i; j++) {
        free(buffer_private->data[j]);
      }
      free(buffer_private);
      return kLibgav1StatusOutOfMemory;
    }
  }

  uint8_t* const y_buffer = buffer_private->data[0];
  uint8_t* const u_buffer =
      (info.uv_buffer_size != 0) ? buffer_private->data[1] : NULL;
  uint8_t* const v_buffer =
      (info.uv_buffer_size != 0) ? buffer_private->data[2] : NULL;

  status = Libgav1SetFrameBuffer(&info, y_buffer, u_buffer, v_buffer,
                                 buffer_private, frame_buffer);
  if (status != kLibgav1StatusOk) return status;

  DecoderTest* const decoder_test = (DecoderTest*)callback_private_data;
  DecoderTestIncrementFramesInUse(decoder_test);
  DecoderTestSetBufferPrivateData(decoder_test, frame_buffer->private_data);
  return kLibgav1StatusOk;
}

static void ReleaseFrameBuffer(void* callback_private_data,
                               void* buffer_private_data) {
  FrameBufferPrivate* buffer_private = (FrameBufferPrivate*)buffer_private_data;
  for (int i = 0; i < 3; ++i) {
    free(buffer_private->data[i]);
  }
  free(buffer_private);
  DecoderTest* const decoder_test = (DecoderTest*)callback_private_data;
  DecoderTestDecrementFramesInUse(decoder_test);
}

static void ReleaseInputBuffer(void* private_data, void* input_buffer) {
  DecoderTestSetReleasedInputBuffer((DecoderTest*)private_data, input_buffer);
}

static void DecoderTestSetUp(DecoderTest* test) {
  Libgav1DecoderSettings settings;
  Libgav1DecoderSettingsInitDefault(&settings);
  settings.frame_parallel = 0;  // false
  settings.get_frame_buffer = GetFrameBuffer;
  settings.release_frame_buffer = ReleaseFrameBuffer;
  settings.callback_private_data = test;
  settings.release_input_buffer = ReleaseInputBuffer;
  ASSERT_EQ(test->decoder, NULL);
  ASSERT_EQ(Libgav1DecoderCreate(&settings, &test->decoder), kLibgav1StatusOk);
  ASSERT_NE(test->decoder, NULL);
}

static void DecoderTestAPIFlowForNonFrameParallelMode(void) {
  DecoderTest test;
  DecoderTestInit(&test);
  DecoderTestSetUp(&test);

  Libgav1StatusCode status;
  const Libgav1DecoderBuffer* buffer;

  // Enqueue frame1 for decoding.
  status = Libgav1DecoderEnqueueFrame(test.decoder, kFrame1, sizeof(kFrame1), 0,
                                      (uint8_t*)&kFrame1);
  ASSERT_EQ(status, kLibgav1StatusOk);

  // In non-frame-parallel mode, decoding happens only in the DequeueFrame call.
  // So there should be no frames in use yet.
  ASSERT_EQ(test.frames_in_use, 0);

  // Dequeue the output of frame1.
  status = Libgav1DecoderDequeueFrame(test.decoder, &buffer);
  ASSERT_EQ(status, kLibgav1StatusOk);
  ASSERT_NE(buffer, NULL);
  ASSERT_EQ(test.released_input_buffer, &kFrame1);

  // libgav1 has decoded frame1 and is holding a reference to it.
  ASSERT_EQ(test.frames_in_use, 1);
  ASSERT_EQ(test.buffer_private_data, buffer->buffer_private_data);

  // Enqueue frame2 for decoding.
  status = Libgav1DecoderEnqueueFrame(test.decoder, kFrame2, sizeof(kFrame2), 0,
                                      (uint8_t*)&kFrame2);
  ASSERT_EQ(status, kLibgav1StatusOk);

  ASSERT_EQ(test.frames_in_use, 1);

  // Dequeue the output of frame2.
  status = Libgav1DecoderDequeueFrame(test.decoder, &buffer);
  ASSERT_EQ(status, kLibgav1StatusOk);
  ASSERT_NE(buffer, NULL);
  ASSERT_EQ(test.released_input_buffer, &kFrame2);

  ASSERT_EQ(test.frames_in_use, 2);
  ASSERT_EQ(test.buffer_private_data, buffer->buffer_private_data);

  // Signal end of stream (method 1). This should ensure that all the references
  // are released.
  status = Libgav1DecoderSignalEOS(test.decoder);

  // libgav1 should have released all the reference frames now.
  ASSERT_EQ(test.frames_in_use, 0);

  // Now, the decoder is ready to accept a new coded video sequence.

  // Enqueue frame1 for decoding.
  status = Libgav1DecoderEnqueueFrame(test.decoder, kFrame1, sizeof(kFrame1), 0,
                                      (uint8_t*)&kFrame1);
  ASSERT_EQ(status, kLibgav1StatusOk);

  ASSERT_EQ(test.frames_in_use, 0);

  // Dequeue the output of frame1.
  status = Libgav1DecoderDequeueFrame(test.decoder, &buffer);
  ASSERT_EQ(status, kLibgav1StatusOk);
  ASSERT_NE(buffer, NULL);
  ASSERT_EQ(test.released_input_buffer, &kFrame1);

  ASSERT_EQ(test.frames_in_use, 1);
  ASSERT_EQ(test.buffer_private_data, buffer->buffer_private_data);

  // Enqueue frame2 for decoding.
  status = Libgav1DecoderEnqueueFrame(test.decoder, kFrame2, sizeof(kFrame2), 0,
                                      (uint8_t*)&kFrame2);
  ASSERT_EQ(status, kLibgav1StatusOk);

  ASSERT_EQ(test.frames_in_use, 1);

  // Dequeue the output of frame2.
  status = Libgav1DecoderDequeueFrame(test.decoder, &buffer);
  ASSERT_EQ(status, kLibgav1StatusOk);
  ASSERT_NE(buffer, NULL);
  ASSERT_EQ(test.released_input_buffer, &kFrame2);

  ASSERT_EQ(test.frames_in_use, 2);
  ASSERT_EQ(test.buffer_private_data, buffer->buffer_private_data);

  // Signal end of stream (method 2). This should ensure that all the references
  // are released.
  Libgav1DecoderDestroy(test.decoder);
  test.decoder = NULL;

  // libgav1 should have released all the frames now.
  ASSERT_EQ(test.frames_in_use, 0);
}

static void
DecoderTestNonFrameParallelModeEnqueueMultipleFramesWithoutDequeuing(void) {
  DecoderTest test;
  DecoderTestInit(&test);
  DecoderTestSetUp(&test);

  Libgav1StatusCode status;
  const Libgav1DecoderBuffer* buffer;

  // Enqueue frame1 for decoding.
  status = Libgav1DecoderEnqueueFrame(test.decoder, kFrame1, sizeof(kFrame1), 0,
                                      (uint8_t*)&kFrame1);
  ASSERT_EQ(status, kLibgav1StatusOk);

  // Until the output of frame1 is dequeued, no other frames can be enqueued.
  status = Libgav1DecoderEnqueueFrame(test.decoder, kFrame2, sizeof(kFrame2), 0,
                                      (uint8_t*)&kFrame2);
  ASSERT_EQ(status, kLibgav1StatusTryAgain);

  ASSERT_EQ(test.frames_in_use, 0);

  // Dequeue the output of frame1.
  status = Libgav1DecoderDequeueFrame(test.decoder, &buffer);
  ASSERT_EQ(status, kLibgav1StatusOk);
  ASSERT_NE(buffer, NULL);
  ASSERT_EQ(test.released_input_buffer, &kFrame1);

  ASSERT_EQ(test.frames_in_use, 1);

  // Delete the decoder instance.
  Libgav1DecoderDestroy(test.decoder);
  test.decoder = NULL;

  ASSERT_EQ(test.frames_in_use, 0);
}

static void DecoderTestNonFrameParallelModeEOSBeforeDequeuingLastFrame(void) {
  DecoderTest test;
  DecoderTestInit(&test);
  DecoderTestSetUp(&test);

  Libgav1StatusCode status;
  const Libgav1DecoderBuffer* buffer;

  // Enqueue frame1 for decoding.
  status = Libgav1DecoderEnqueueFrame(test.decoder, kFrame1, sizeof(kFrame1), 0,
                                      (uint8_t*)&kFrame1);
  ASSERT_EQ(status, kLibgav1StatusOk);

  ASSERT_EQ(test.frames_in_use, 0);

  // Dequeue the output of frame1.
  status = Libgav1DecoderDequeueFrame(test.decoder, &buffer);
  ASSERT_EQ(status, kLibgav1StatusOk);
  ASSERT_NE(buffer, NULL);
  ASSERT_EQ(test.released_input_buffer, &kFrame1);

  // Enqueue frame2 for decoding.
  status = Libgav1DecoderEnqueueFrame(test.decoder, kFrame2, sizeof(kFrame2), 0,
                                      (uint8_t*)&kFrame2);
  ASSERT_EQ(status, kLibgav1StatusOk);

  ASSERT_EQ(test.frames_in_use, 1);

  // Signal end of stream before dequeuing the output of frame2.
  status = Libgav1DecoderSignalEOS(test.decoder);
  ASSERT_EQ(status, kLibgav1StatusOk);

  // In this case, the output of the last frame that was enqueued is lost (which
  // is intentional since end of stream was signaled without dequeueing it).
  ASSERT_EQ(test.frames_in_use, 0);

  Libgav1DecoderDestroy(test.decoder);
  test.decoder = NULL;
}

static void DecoderTestNonFrameParallelModeInvalidFrameAfterEOS(void) {
  DecoderTest test;
  DecoderTestInit(&test);
  DecoderTestSetUp(&test);

  Libgav1StatusCode status;
  const Libgav1DecoderBuffer* buffer = NULL;

  // Enqueue frame1 for decoding.
  status = Libgav1DecoderEnqueueFrame(test.decoder, kFrame1, sizeof(kFrame1), 0,
                                      (uint8_t*)&kFrame1);
  ASSERT_EQ(status, kLibgav1StatusOk);

  ASSERT_EQ(test.frames_in_use, 0);

  // Dequeue the output of frame1.
  status = Libgav1DecoderDequeueFrame(test.decoder, &buffer);
  ASSERT_EQ(status, kLibgav1StatusOk);
  ASSERT_NE(buffer, NULL);
  ASSERT_EQ(test.released_input_buffer, &kFrame1);

  ASSERT_EQ(test.frames_in_use, 1);

  // Signal end of stream.
  status = Libgav1DecoderSignalEOS(test.decoder);

  // libgav1 should have released all the reference frames now.
  ASSERT_EQ(test.frames_in_use, 0);

  // Now, the decoder is ready to accept a new coded video sequence. But, we
  // try to enqueue a frame that does not have a sequence header (which is not
  // allowed).

  // Enqueue frame2 for decoding.
  status = Libgav1DecoderEnqueueFrame(test.decoder, kFrame2, sizeof(kFrame2), 0,
                                      (uint8_t*)&kFrame2);
  ASSERT_EQ(status, kLibgav1StatusOk);

  ASSERT_EQ(test.frames_in_use, 0);

  // Dequeue the output of frame2 (this will fail since no sequence header has
  // been seen since the last EOS signal).
  status = Libgav1DecoderDequeueFrame(test.decoder, &buffer);
  ASSERT_EQ(status, kLibgav1StatusBitstreamError);
  ASSERT_EQ(test.released_input_buffer, &kFrame2);

  ASSERT_EQ(test.frames_in_use, 0);

  Libgav1DecoderDestroy(test.decoder);
  test.decoder = NULL;
}

int main(void) {
  fprintf(stderr, "C DecoderTest started\n");
  DecoderTestAPIFlowForNonFrameParallelMode();
  DecoderTestNonFrameParallelModeEnqueueMultipleFramesWithoutDequeuing();
  DecoderTestNonFrameParallelModeEOSBeforeDequeuingLastFrame();
  DecoderTestNonFrameParallelModeInvalidFrameAfterEOS();
  fprintf(stderr, "C DecoderTest passed\n");
  return 0;
}
