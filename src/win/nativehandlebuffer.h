// Copyright (C) <2018> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0
#ifndef OWT_NATIVE_HANDLE_BUFFER_H_
#define OWT_NATIVE_HANDLE_BUFFER_H_

#include <stdint.h>
#include "api/scoped_refptr.h"
#include "api/video/i420_buffer.h"
#include "api/video/video_frame_buffer.h"
#include "rtc_base/checks.h"
#include "third_party/libyuv/include/libyuv/convert.h"

#include <d3d11.h>

namespace owt {
namespace base {
using namespace webrtc;

#if defined(WEBRTC_WIN)
struct D3D11ImageHandle {
  ID3D11Device* d3d11_device;
  ID3D11Texture2D* texture;  // The DX texture or texture array.
  int texture_array_index;  // When >=0, indicate the index within texture array
};
#endif

class NativeHandleBuffer : public VideoFrameBuffer {
 public:
  NativeHandleBuffer(void* native_handle, int width, int height)
      : native_handle_(native_handle), width_(width), height_(height) {}
  Type type() const override { return Type::kNative; }
  int width() const override { return width_; }
  int height() const override { return height_; }
  rtc::scoped_refptr<I420BufferInterface> ToI420() override {
    RTC_DCHECK_NOTREACHED();
    return nullptr;
  }

  void* native_handle() { return native_handle_; }

 private:
  void* native_handle_;
  const int width_;
  const int height_;
};

class NativeNV12Buffer : public NV12BufferInterface {
 public:
  NativeNV12Buffer(const uint8_t* data_y,
                   int stride_y,
                   const uint8_t* data_uv,
                   int stride_uv,
                   int width,
                   int height)
      : data_y_(data_y),
        data_uv_(data_uv),
        stride_y_(stride_y),
        stride_uv_(stride_uv),
        width_(width),
        height_(height) {}

  Type type() const override { return Type::kNV12; }
  int width() const override { return width_; }
  int height() const override { return height_; }

  const uint8_t* DataY() const override { return data_y_; }
  const uint8_t* DataUV() const override { return data_uv_; }
  int StrideY() const override { return stride_y_; }
  int StrideUV() const override { return stride_uv_; }

  rtc::scoped_refptr<I420BufferInterface> ToI420() override {
    rtc::scoped_refptr<I420Buffer> i420_buffer =
        I420Buffer::Create(width(), height());
    libyuv::NV12ToI420(DataY(), StrideY(), DataUV(), StrideUV(),
                       i420_buffer->MutableDataY(), i420_buffer->StrideY(),
                       i420_buffer->MutableDataU(), i420_buffer->StrideU(),
                       i420_buffer->MutableDataV(), i420_buffer->StrideV(),
                       width(), height());
    return i420_buffer;
  }

 private:
  const uint8_t* data_y_;
  const uint8_t* data_uv_;
  const int stride_y_;
  const int stride_uv_;
  const int width_;
  const int height_;
};
}  // namespace base
}  // namespace owt
#endif  // OWT_NATIVE_HANDLE_BUFFER_H_
