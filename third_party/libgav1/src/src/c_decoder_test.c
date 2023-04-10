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

// Import the test frame #defines.
#include "src/decoder_test_data.h"
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

static const uint8_t kFrame1[] = {OBU_TEMPORAL_DELIMITER, OBU_SEQUENCE_HEADER,
                                  OBU_FRAME_1};

static const uint8_t kFrame2[] = {OBU_TEMPORAL_DELIMITER, OBU_FRAME_2};

static const uint8_t kFrame1WithHdrCllAndHdrMdcv[] = {
    OBU_TEMPORAL_DELIMITER, OBU_SEQUENCE_HEADER, OBU_METADATA_HDR_CLL,
    OBU_METADATA_HDR_MDCV, OBU_FRAME_1};

static const uint8_t kFrame2WithItutT35[] = {
    OBU_TEMPORAL_DELIMITER, OBU_METADATA_ITUT_T35, OBU_FRAME_2};

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

static void DecoderTestMetadataObu(void) {
  DecoderTest test;
  DecoderTestInit(&test);
  DecoderTestSetUp(&test);

  Libgav1StatusCode status;
  const Libgav1DecoderBuffer* buffer;

  // Enqueue frame1 for decoding.
  status = Libgav1DecoderEnqueueFrame(test.decoder, kFrame1WithHdrCllAndHdrMdcv,
                                      sizeof(kFrame1WithHdrCllAndHdrMdcv), 0,
                                      (uint8_t*)&kFrame1WithHdrCllAndHdrMdcv);
  ASSERT_EQ(status, kLibgav1StatusOk);
  ASSERT_EQ(test.frames_in_use, 0);

  // Dequeue the output of frame1.
  status = Libgav1DecoderDequeueFrame(test.decoder, &buffer);
  ASSERT_EQ(status, kLibgav1StatusOk);
  ASSERT_NE(buffer, NULL);
  ASSERT_EQ(buffer->has_hdr_cll, 1);
  ASSERT_EQ(buffer->has_hdr_mdcv, 1);
  ASSERT_EQ(buffer->has_itut_t35, 0);
  ASSERT_EQ(test.released_input_buffer, &kFrame1WithHdrCllAndHdrMdcv);

  ASSERT_EQ(test.frames_in_use, 1);
  ASSERT_EQ(test.buffer_private_data, buffer->buffer_private_data);

  // Enqueue frame2 for decoding.
  status = Libgav1DecoderEnqueueFrame(test.decoder, kFrame2WithItutT35,
                                      sizeof(kFrame2WithItutT35), 0,
                                      (uint8_t*)&kFrame2WithItutT35);
  ASSERT_EQ(status, kLibgav1StatusOk);

  ASSERT_EQ(test.frames_in_use, 1);

  // Dequeue the output of frame2.
  status = Libgav1DecoderDequeueFrame(test.decoder, &buffer);
  ASSERT_EQ(status, kLibgav1StatusOk);
  ASSERT_NE(buffer, NULL);
  ASSERT_EQ(buffer->has_hdr_cll, 0);
  ASSERT_EQ(buffer->has_hdr_mdcv, 0);
  ASSERT_EQ(buffer->has_itut_t35, 1);
  ASSERT_NE(buffer->itut_t35.payload_bytes, NULL);
  ASSERT_NE(buffer->itut_t35.payload_size, 0);
  ASSERT_EQ(test.released_input_buffer, &kFrame2WithItutT35);

  ASSERT_EQ(test.frames_in_use, 2);
  ASSERT_EQ(test.buffer_private_data, buffer->buffer_private_data);

  status = Libgav1DecoderSignalEOS(test.decoder);
  ASSERT_EQ(test.frames_in_use, 0);

  Libgav1DecoderDestroy(test.decoder);
}

int main(void) {
  fprintf(stderr, "C DecoderTest started\n");
  DecoderTestAPIFlowForNonFrameParallelMode();
  DecoderTestNonFrameParallelModeEnqueueMultipleFramesWithoutDequeuing();
  DecoderTestNonFrameParallelModeEOSBeforeDequeuingLastFrame();
  DecoderTestNonFrameParallelModeInvalidFrameAfterEOS();
  DecoderTestMetadataObu();
  fprintf(stderr, "C DecoderTest passed\n");
  return 0;
}
