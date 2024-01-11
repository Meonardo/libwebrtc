/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
#ifndef INTERNAL_VCM_CAPTURER_H_
#define INTERNAL_VCM_CAPTURER_H_

#include <memory>
#include <vector>
#include <functional>

#include "api/scoped_refptr.h"
#include "modules/video_capture/video_capture.h"
#include "rtc_base/thread.h"
#include "src/internal/video_capturer.h"

namespace libwebrtc {
class RTCJpegCapturerCallback;
}

namespace webrtc {
namespace internal {

class BasicJPEGCaptureThread;

class VcmCapturer : public VideoCapturer,
                    public rtc::VideoSinkInterface<VideoFrame> {
 public:
  static VcmCapturer* Create(rtc::Thread* worker_thread,
                             size_t width,
                             size_t height,
                             size_t target_fps,
                             size_t capture_device_index);
  virtual ~VcmCapturer();

  void OnFrame(const VideoFrame& frame) override;

  bool StartCapture() override;

  bool CaptureStarted() override;

  void StopCapture() override;

  void EncodeJpeg();

  // change the capturing device dynamically
  bool UpdateCaptureDevice(size_t width,
                               size_t height,
                               size_t target_fps,
                               size_t capture_device_index);

  // for encode jpeg
  void StartEncodeJpeg(const char* id, uint16_t delay_timeinterval,
                       libwebrtc::RTCJpegCapturerCallback* data_cb);
  void StopEncodeJpeg();

 private:
  VcmCapturer(rtc::Thread* worker_thread);
  bool Init(size_t width,
            size_t height,
            size_t target_fps,
            size_t capture_device_index);
  void Destroy();

  rtc::scoped_refptr<VideoCaptureModule> vcm_;
  int last_capture_device_index_ = -1;
  rtc::Thread* worker_thread_ = nullptr;
  VideoCaptureCapability capability_;

  // for encode jpeg
  rtc::scoped_refptr<webrtc::VideoFrameBuffer> video_buffer_;
  std::unique_ptr<BasicJPEGCaptureThread> jpeg_thread_;
  libwebrtc::RTCJpegCapturerCallback* jpeg_data_cb_;
  std::string jpeg_data_cb_id_;
};

class CapturerTrackSource : public webrtc::VideoTrackSource {
 public:
  static rtc::scoped_refptr<CapturerTrackSource> Create(
      rtc::Thread* worker_thread);

 public:
  explicit CapturerTrackSource(std::unique_ptr<VideoCapturer> capturer)
      : VideoTrackSource(/*remote=*/false), capturer_(std::move(capturer)) {}

  VideoCapturer* CapturerSource() const { return capturer_.get(); }

 private:
  rtc::VideoSourceInterface<webrtc::VideoFrame>* source() override {
    return capturer_.get();
  }
  std::unique_ptr<VideoCapturer> capturer_;
};
}  // namespace internal
}  // namespace webrtc

#endif  // INTERNAL_VCM_CAPTURER_H_
