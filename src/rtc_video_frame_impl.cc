#include "rtc_video_frame_impl.h"

#include "api/video/i420_buffer.h"
#include "libyuv/convert_argb.h"
#include "libyuv/convert_from.h"
#include "rtc_base/checks.h"
#include "rtc_base/logging.h"

#include "src/win/nativehandlebuffer.h"

namespace libwebrtc {

VideoFrameBufferImpl::VideoFrameBufferImpl(
    rtc::scoped_refptr<webrtc::VideoFrameBuffer> frame_buffer)
    : buffer_(frame_buffer) {}

VideoFrameBufferImpl::VideoFrameBufferImpl(
    rtc::scoped_refptr<webrtc::I420Buffer> frame_buffer)
    : buffer_(frame_buffer) {}

VideoFrameBufferImpl::VideoFrameBufferImpl(
    rtc::scoped_refptr<webrtc::NV12Buffer> frame_buffer)
    : buffer_(frame_buffer) {}

VideoFrameBufferImpl::VideoFrameBufferImpl(
    rtc::scoped_refptr<owt::base::EncodedFrameBuffer> encoded_buffer)
    : buffer_(encoded_buffer) {}

VideoFrameBufferImpl::~VideoFrameBufferImpl() {}

scoped_refptr<RTCVideoFrame> VideoFrameBufferImpl::Copy() {
  scoped_refptr<VideoFrameBufferImpl> frame =
      scoped_refptr<VideoFrameBufferImpl>(
          new RefCountedObject<VideoFrameBufferImpl>(buffer_));
  return frame;
}

// bool VideoFrameBufferImpl::IsNative() const {
//   return buffer_->type() == webrtc::VideoFrameBuffer::Type::kNative;
// }
//

void* VideoFrameBufferImpl::RawBuffer() const {
  return buffer_.get();
}

RTCVideoFrame::PixelFormat VideoFrameBufferImpl::PixFormat() const {
  if (buffer_->type() == webrtc::VideoFrameBuffer::Type::kNV12)
    return RTCVideoFrame::PixelFormat::kNV12;
  else if (buffer_->type() == webrtc::VideoFrameBuffer::Type::kNative)
    return RTCVideoFrame::PixelFormat::kNative;
  return RTCVideoFrame::PixelFormat::kYV12;
}

int VideoFrameBufferImpl::width() const {
  return buffer_->width();
}

int VideoFrameBufferImpl::height() const {
  return buffer_->height();
}

const uint8_t* VideoFrameBufferImpl::DataY() const {
  if (buffer_->type() == webrtc::VideoFrameBuffer::Type::kNV12) {
    return buffer_->GetNV12()->DataY();
  } else {
    return buffer_->GetI420()->DataY();
  }
}

const uint8_t* VideoFrameBufferImpl::DataU() const {
  if (buffer_->type() == webrtc::VideoFrameBuffer::Type::kNV12) {
    return buffer_->GetNV12()->DataUV();
  }
  return buffer_->GetI420()->DataU();
}

const uint8_t* VideoFrameBufferImpl::DataV() const {
  if (buffer_->type() == webrtc::VideoFrameBuffer::Type::kNV12) {
    return nullptr;
  }
  return buffer_->GetI420()->DataV();
}

int VideoFrameBufferImpl::StrideY() const {
  if (buffer_->type() == webrtc::VideoFrameBuffer::Type::kNV12) {
    return buffer_->GetNV12()->StrideY();
  }
  return buffer_->GetI420()->StrideY();
}

int VideoFrameBufferImpl::StrideU() const {
  if (buffer_->type() == webrtc::VideoFrameBuffer::Type::kNV12) {
    return buffer_->GetNV12()->StrideUV();
  }
  return buffer_->GetI420()->StrideU();
}

int VideoFrameBufferImpl::StrideV() const {
  if (buffer_->type() == webrtc::VideoFrameBuffer::Type::kNV12) {
    return 0;
  }
  return buffer_->GetI420()->StrideV();
}

int VideoFrameBufferImpl::ConvertToARGB(Type type,
                                        uint8_t* dst_buffer,
                                        int dst_stride,
                                        int dest_width,
                                        int dest_height) {
  rtc::scoped_refptr<webrtc::I420Buffer> i420 =
      webrtc::I420Buffer::Rotate(*buffer_.get(), rotation_);

  rtc::scoped_refptr<webrtc::I420Buffer> dest =
      webrtc::I420Buffer::Create(dest_width, dest_height);

  dest->ScaleFrom(*i420.get());
  int buf_size = dest->width() * dest->height() * (32 >> 3);
  switch (type) {
    case libwebrtc::RTCVideoFrame::Type::kARGB:
      libyuv::I420ToARGB(dest->DataY(), dest->StrideY(), dest->DataU(),
                         dest->StrideU(), dest->DataV(), dest->StrideV(),
                         dst_buffer, dest->width() * 32 / 8, dest->width(),
                         dest->height());
      break;
    case libwebrtc::RTCVideoFrame::Type::kBGRA:
      libyuv::I420ToBGRA(dest->DataY(), dest->StrideY(), dest->DataU(),
                         dest->StrideU(), dest->DataV(), dest->StrideV(),
                         dst_buffer, dest->width() * 32 / 8, dest->width(),
                         dest->height());
      break;
    case libwebrtc::RTCVideoFrame::Type::kABGR:
      libyuv::I420ToABGR(dest->DataY(), dest->StrideY(), dest->DataU(),
                         dest->StrideU(), dest->DataV(), dest->StrideV(),
                         dst_buffer, dest->width() * 32 / 8, dest->width(),
                         dest->height());
      break;
    case libwebrtc::RTCVideoFrame::Type::kRGBA:
      libyuv::I420ToRGBA(dest->DataY(), dest->StrideY(), dest->DataU(),
                         dest->StrideU(), dest->DataV(), dest->StrideV(),
                         dst_buffer, dest->width() * 32 / 8, dest->width(),
                         dest->height());
      break;
    default:
      break;
  }
  return buf_size;
}

libwebrtc::RTCVideoFrame::VideoRotation VideoFrameBufferImpl::rotation() {
  switch (rotation_) {
    case webrtc::kVideoRotation_0:
      return RTCVideoFrame::kVideoRotation_0;
    case webrtc::kVideoRotation_90:
      return RTCVideoFrame::kVideoRotation_90;
    case webrtc::kVideoRotation_180:
      return RTCVideoFrame::kVideoRotation_180;
    case webrtc::kVideoRotation_270:
      return RTCVideoFrame::kVideoRotation_270;
    default:
      break;
  }
  return RTCVideoFrame::kVideoRotation_0;
}

scoped_refptr<RTCVideoFrame> RTCVideoFrame::Create(int width,
                                                   int height,
                                                   const uint8_t* buffer,
                                                   int length) {
  int stride_y = width;
  int stride_uv = (width + 1) / 2;

  int size_y = stride_y * height;
  int size_u = stride_uv * height / 2;
  // int size_v = size_u;

  RTC_DCHECK(length == (width * height * 3) / 2);

  const uint8_t* data_y = buffer;
  const uint8_t* data_u = buffer + size_y;
  const uint8_t* data_v = buffer + size_y + size_u;

  rtc::scoped_refptr<webrtc::I420Buffer> i420_buffer = webrtc::I420Buffer::Copy(
      width, height, data_y, stride_y, data_u, stride_uv, data_v, stride_uv);

  scoped_refptr<VideoFrameBufferImpl> frame =
      scoped_refptr<VideoFrameBufferImpl>(
          new RefCountedObject<VideoFrameBufferImpl>(i420_buffer));
  return frame;
}

scoped_refptr<RTCVideoFrame> RTCVideoFrame::Create(int width,
                                                   int height,
                                                   const uint8_t* data_y,
                                                   int stride_y,
                                                   const uint8_t* data_u,
                                                   int stride_u,
                                                   const uint8_t* data_v,
                                                   int stride_v) {
  rtc::scoped_refptr<webrtc::I420Buffer> i420_buffer = webrtc::I420Buffer::Copy(
      width, height, data_y, stride_y, data_u, stride_u, data_v, stride_v);

  scoped_refptr<VideoFrameBufferImpl> frame =
      scoped_refptr<VideoFrameBufferImpl>(
          new RefCountedObject<VideoFrameBufferImpl>(i420_buffer));
  return frame;
}

scoped_refptr<RTCVideoFrame> RTCVideoFrame::Create(int width,
                                                   int height,
                                                   const uint8_t* data_y,
                                                   const uint8_t* data_uv) {
  rtc::scoped_refptr<webrtc::NV12Buffer> nv12_buffer =
      webrtc::NV12Buffer::Create(width, height, width, width);

  auto size_y = width * height;
  memcpy(nv12_buffer->MutableDataY(), data_y, size_y);
  memcpy(nv12_buffer->MutableDataY() + size_y, data_uv, size_y / 2);

  scoped_refptr<VideoFrameBufferImpl> frame =
      scoped_refptr<VideoFrameBufferImpl>(
          new RefCountedObject<VideoFrameBufferImpl>(nv12_buffer));
  return frame;
}

scoped_refptr<RTCVideoFrame> RTCVideoFrame::Create(uint8_t* data,
                                                   size_t size,
                                                   bool keyframe,
                                                   size_t w,
                                                   size_t h) {
  rtc::scoped_refptr<owt::base::EncodedFrameBuffer> encoded_buffer =
      new rtc::RefCountedObject<owt::base::EncodedFrameBuffer>(data, size,
                                                               keyframe, w, h);

  scoped_refptr<VideoFrameBufferImpl> frame =
      scoped_refptr<VideoFrameBufferImpl>(
          new RefCountedObject<VideoFrameBufferImpl>(encoded_buffer));
  return frame;
}

}  // namespace libwebrtc
