// Copyright 2021 The libgav1 Authors
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

#include "src/gav1/decoder.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <new>

#include "gtest/gtest.h"
#include "src/decoder_test_data.h"

namespace libgav1 {
namespace {

constexpr uint8_t kFrame1[] = {OBU_TEMPORAL_DELIMITER, OBU_SEQUENCE_HEADER,
                               OBU_FRAME_1};

constexpr uint8_t kFrame2[] = {OBU_TEMPORAL_DELIMITER, OBU_FRAME_2};

constexpr uint8_t kFrame1WithHdrCllAndHdrMdcv[] = {
    OBU_TEMPORAL_DELIMITER, OBU_SEQUENCE_HEADER, OBU_METADATA_HDR_CLL,
    OBU_METADATA_HDR_MDCV, OBU_FRAME_1};

constexpr uint8_t kFrame2WithItutT35[] = {OBU_TEMPORAL_DELIMITER,
                                          OBU_METADATA_ITUT_T35, OBU_FRAME_2};

class DecoderTest : public testing::Test {
 public:
  void SetUp() override;
  void IncrementFramesInUse() { ++frames_in_use_; }
  void DecrementFramesInUse() { --frames_in_use_; }
  void SetBufferPrivateData(void* buffer_private_data) {
    buffer_private_data_ = buffer_private_data;
  }
  void SetReleasedInputBuffer(void* released_input_buffer) {
    released_input_buffer_ = released_input_buffer;
  }

 protected:
  std::unique_ptr<Decoder> decoder_;
  int frames_in_use_ = 0;
  void* buffer_private_data_ = nullptr;
  void* released_input_buffer_ = nullptr;
};

struct FrameBufferPrivate {
  uint8_t* data[3];
};

extern "C" {

static Libgav1StatusCode GetFrameBuffer(
    void* callback_private_data, int bitdepth, Libgav1ImageFormat image_format,
    int width, int height, int left_border, int right_border, int top_border,
    int bottom_border, int stride_alignment, Libgav1FrameBuffer* frame_buffer) {
  Libgav1FrameBufferInfo info;
  Libgav1StatusCode status = Libgav1ComputeFrameBufferInfo(
      bitdepth, image_format, width, height, left_border, right_border,
      top_border, bottom_border, stride_alignment, &info);
  if (status != kLibgav1StatusOk) return status;

  std::unique_ptr<FrameBufferPrivate> buffer_private(new (std::nothrow)
                                                         FrameBufferPrivate);
  if (buffer_private == nullptr) return kLibgav1StatusOutOfMemory;

  for (int i = 0; i < 3; ++i) {
    const size_t size = (i == 0) ? info.y_buffer_size : info.uv_buffer_size;
    buffer_private->data[i] = new (std::nothrow) uint8_t[size];
    if (buffer_private->data[i] == nullptr) {
      return kLibgav1StatusOutOfMemory;
    }
  }

  uint8_t* const y_buffer = buffer_private->data[0];
  uint8_t* const u_buffer =
      (info.uv_buffer_size != 0) ? buffer_private->data[1] : nullptr;
  uint8_t* const v_buffer =
      (info.uv_buffer_size != 0) ? buffer_private->data[2] : nullptr;

  status = Libgav1SetFrameBuffer(&info, y_buffer, u_buffer, v_buffer,
                                 buffer_private.release(), frame_buffer);
  if (status != kLibgav1StatusOk) return status;

  auto* const decoder_test = static_cast<DecoderTest*>(callback_private_data);
  decoder_test->IncrementFramesInUse();
  decoder_test->SetBufferPrivateData(frame_buffer->private_data);
  return kLibgav1StatusOk;
}

static void ReleaseFrameBuffer(void* callback_private_data,
                               void* buffer_private_data) {
  auto* buffer_private = static_cast<FrameBufferPrivate*>(buffer_private_data);
  for (auto& data : buffer_private->data) {
    delete[] data;
  }
  delete buffer_private;
  auto* const decoder_test = static_cast<DecoderTest*>(callback_private_data);
  decoder_test->DecrementFramesInUse();
}

static void ReleaseInputBuffer(void* private_data, void* input_buffer) {
  auto* const decoder_test = static_cast<DecoderTest*>(private_data);
  decoder_test->SetReleasedInputBuffer(input_buffer);
}

}  // extern "C"

void DecoderTest::SetUp() {
  decoder_.reset(new (std::nothrow) Decoder());
  ASSERT_NE(decoder_, nullptr);
  DecoderSettings settings = {};
  settings.frame_parallel = false;
  settings.get_frame_buffer = GetFrameBuffer;
  settings.release_frame_buffer = ReleaseFrameBuffer;
  settings.callback_private_data = this;
  settings.release_input_buffer = ReleaseInputBuffer;
  ASSERT_EQ(decoder_->Init(&settings), kStatusOk);
}

TEST_F(DecoderTest, APIFlowForNonFrameParallelMode) {
  StatusCode status;
  const DecoderBuffer* buffer;

  // Enqueue frame1 for decoding.
  status = decoder_->EnqueueFrame(kFrame1, sizeof(kFrame1), 0,
                                  const_cast<uint8_t*>(kFrame1));
  ASSERT_EQ(status, kStatusOk);

  // In non-frame-parallel mode, decoding happens only in the DequeueFrame call.
  // So there should be no frames in use yet.
  EXPECT_EQ(frames_in_use_, 0);

  // Dequeue the output of frame1.
  status = decoder_->DequeueFrame(&buffer);
  ASSERT_EQ(status, kStatusOk);
  ASSERT_NE(buffer, nullptr);
  EXPECT_EQ(released_input_buffer_, &kFrame1);

  // libgav1 has decoded frame1 and is holding a reference to it.
  EXPECT_EQ(frames_in_use_, 1);
  EXPECT_EQ(buffer_private_data_, buffer->buffer_private_data);

  // Enqueue frame2 for decoding.
  status = decoder_->EnqueueFrame(kFrame2, sizeof(kFrame2), 0,
                                  const_cast<uint8_t*>(kFrame2));
  ASSERT_EQ(status, kStatusOk);

  EXPECT_EQ(frames_in_use_, 1);

  // Dequeue the output of frame2.
  status = decoder_->DequeueFrame(&buffer);
  ASSERT_EQ(status, kStatusOk);
  ASSERT_NE(buffer, nullptr);
  EXPECT_EQ(released_input_buffer_, &kFrame2);

  EXPECT_EQ(frames_in_use_, 2);
  EXPECT_EQ(buffer_private_data_, buffer->buffer_private_data);

  // Signal end of stream (method 1). This should ensure that all the references
  // are released.
  status = decoder_->SignalEOS();

  // libgav1 should have released all the reference frames now.
  EXPECT_EQ(frames_in_use_, 0);

  // Now, the decoder is ready to accept a new coded video sequence.

  // Enqueue frame1 for decoding.
  status = decoder_->EnqueueFrame(kFrame1, sizeof(kFrame1), 0,
                                  const_cast<uint8_t*>(kFrame1));
  ASSERT_EQ(status, kStatusOk);

  EXPECT_EQ(frames_in_use_, 0);

  // Dequeue the output of frame1.
  status = decoder_->DequeueFrame(&buffer);
  ASSERT_EQ(status, kStatusOk);
  ASSERT_NE(buffer, nullptr);
  EXPECT_EQ(released_input_buffer_, &kFrame1);

  EXPECT_EQ(frames_in_use_, 1);
  EXPECT_EQ(buffer_private_data_, buffer->buffer_private_data);

  // Enqueue frame2 for decoding.
  status = decoder_->EnqueueFrame(kFrame2, sizeof(kFrame2), 0,
                                  const_cast<uint8_t*>(kFrame2));
  ASSERT_EQ(status, kStatusOk);

  EXPECT_EQ(frames_in_use_, 1);

  // Dequeue the output of frame2.
  status = decoder_->DequeueFrame(&buffer);
  ASSERT_EQ(status, kStatusOk);
  ASSERT_NE(buffer, nullptr);
  EXPECT_EQ(released_input_buffer_, &kFrame2);

  EXPECT_EQ(frames_in_use_, 2);
  EXPECT_EQ(buffer_private_data_, buffer->buffer_private_data);

  // Signal end of stream (method 2). This should ensure that all the references
  // are released.
  decoder_ = nullptr;

  // libgav1 should have released all the frames now.
  EXPECT_EQ(frames_in_use_, 0);
}

TEST_F(DecoderTest, NonFrameParallelModeEnqueueMultipleFramesWithoutDequeuing) {
  StatusCode status;
  const DecoderBuffer* buffer;

  // Enqueue frame1 for decoding.
  status = decoder_->EnqueueFrame(kFrame1, sizeof(kFrame1), 0,
                                  const_cast<uint8_t*>(kFrame1));
  ASSERT_EQ(status, kStatusOk);

  // Until the output of frame1 is dequeued, no other frames can be enqueued.
  status = decoder_->EnqueueFrame(kFrame2, sizeof(kFrame2), 0,
                                  const_cast<uint8_t*>(kFrame2));
  ASSERT_EQ(status, kStatusTryAgain);

  EXPECT_EQ(frames_in_use_, 0);

  // Dequeue the output of frame1.
  status = decoder_->DequeueFrame(&buffer);
  ASSERT_EQ(status, kStatusOk);
  ASSERT_NE(buffer, nullptr);
  EXPECT_EQ(released_input_buffer_, &kFrame1);

  EXPECT_EQ(frames_in_use_, 1);

  // Delete the decoder instance.
  decoder_ = nullptr;

  EXPECT_EQ(frames_in_use_, 0);
}

TEST_F(DecoderTest, NonFrameParallelModeEOSBeforeDequeuingLastFrame) {
  StatusCode status;
  const DecoderBuffer* buffer;

  // Enqueue frame1 for decoding.
  status = decoder_->EnqueueFrame(kFrame1, sizeof(kFrame1), 0,
                                  const_cast<uint8_t*>(kFrame1));
  ASSERT_EQ(status, kStatusOk);

  EXPECT_EQ(frames_in_use_, 0);

  // Dequeue the output of frame1.
  status = decoder_->DequeueFrame(&buffer);
  ASSERT_EQ(status, kStatusOk);
  ASSERT_NE(buffer, nullptr);
  EXPECT_EQ(released_input_buffer_, &kFrame1);

  // Enqueue frame2 for decoding.
  status = decoder_->EnqueueFrame(kFrame2, sizeof(kFrame2), 0,
                                  const_cast<uint8_t*>(kFrame2));
  ASSERT_EQ(status, kStatusOk);

  EXPECT_EQ(frames_in_use_, 1);

  // Signal end of stream before dequeuing the output of frame2.
  status = decoder_->SignalEOS();
  ASSERT_EQ(status, kStatusOk);

  // In this case, the output of the last frame that was enqueued is lost (which
  // is intentional since end of stream was signaled without dequeueing it).
  EXPECT_EQ(frames_in_use_, 0);
}

TEST_F(DecoderTest, NonFrameParallelModeInvalidFrameAfterEOS) {
  StatusCode status;
  const DecoderBuffer* buffer = nullptr;

  // Enqueue frame1 for decoding.
  status = decoder_->EnqueueFrame(kFrame1, sizeof(kFrame1), 0,
                                  const_cast<uint8_t*>(kFrame1));
  ASSERT_EQ(status, kStatusOk);

  EXPECT_EQ(frames_in_use_, 0);

  // Dequeue the output of frame1.
  status = decoder_->DequeueFrame(&buffer);
  ASSERT_EQ(status, kStatusOk);
  ASSERT_NE(buffer, nullptr);
  EXPECT_EQ(released_input_buffer_, &kFrame1);

  EXPECT_EQ(frames_in_use_, 1);

  // Signal end of stream.
  status = decoder_->SignalEOS();

  // libgav1 should have released all the reference frames now.
  EXPECT_EQ(frames_in_use_, 0);

  // Now, the decoder is ready to accept a new coded video sequence. But, we
  // try to enqueue a frame that does not have a sequence header (which is not
  // allowed).

  // Enqueue frame2 for decoding.
  status = decoder_->EnqueueFrame(kFrame2, sizeof(kFrame2), 0,
                                  const_cast<uint8_t*>(kFrame2));
  ASSERT_EQ(status, kStatusOk);

  EXPECT_EQ(frames_in_use_, 0);

  // Dequeue the output of frame2 (this will fail since no sequence header has
  // been seen since the last EOS signal).
  status = decoder_->DequeueFrame(&buffer);
  ASSERT_EQ(status, kStatusBitstreamError);
  EXPECT_EQ(released_input_buffer_, &kFrame2);

  EXPECT_EQ(frames_in_use_, 0);
}

TEST_F(DecoderTest, MetadataObu) {
  StatusCode status;
  const DecoderBuffer* buffer;

  // Enqueue frame1 for decoding.
  status = decoder_->EnqueueFrame(
      kFrame1WithHdrCllAndHdrMdcv, sizeof(kFrame1WithHdrCllAndHdrMdcv), 0,
      const_cast<uint8_t*>(kFrame1WithHdrCllAndHdrMdcv));
  ASSERT_EQ(status, kStatusOk);

  // Dequeue the output of frame1.
  status = decoder_->DequeueFrame(&buffer);
  ASSERT_EQ(status, kStatusOk);
  ASSERT_NE(buffer, nullptr);
  EXPECT_EQ(buffer->has_hdr_cll, 1);
  EXPECT_EQ(buffer->has_hdr_mdcv, 1);
  EXPECT_EQ(buffer->has_itut_t35, 0);
  EXPECT_EQ(released_input_buffer_, &kFrame1WithHdrCllAndHdrMdcv);

  // libgav1 has decoded frame1 and is holding a reference to it.
  EXPECT_EQ(frames_in_use_, 1);
  EXPECT_EQ(buffer_private_data_, buffer->buffer_private_data);

  // Enqueue frame2 for decoding.
  status =
      decoder_->EnqueueFrame(kFrame2WithItutT35, sizeof(kFrame2WithItutT35), 0,
                             const_cast<uint8_t*>(kFrame2WithItutT35));
  ASSERT_EQ(status, kStatusOk);

  EXPECT_EQ(frames_in_use_, 1);

  // Dequeue the output of frame2.
  status = decoder_->DequeueFrame(&buffer);
  ASSERT_EQ(status, kStatusOk);
  ASSERT_NE(buffer, nullptr);
  EXPECT_EQ(buffer->has_hdr_cll, 0);
  EXPECT_EQ(buffer->has_hdr_mdcv, 0);
  EXPECT_EQ(buffer->has_itut_t35, 1);
  EXPECT_NE(buffer->itut_t35.payload_bytes, nullptr);
  EXPECT_GT(buffer->itut_t35.payload_size, 0);
  EXPECT_EQ(released_input_buffer_, &kFrame2WithItutT35);

  EXPECT_EQ(frames_in_use_, 2);
  EXPECT_EQ(buffer_private_data_, buffer->buffer_private_data);

  status = decoder_->SignalEOS();
  EXPECT_EQ(frames_in_use_, 0);
}

}  // namespace
}  // namespace libgav1
