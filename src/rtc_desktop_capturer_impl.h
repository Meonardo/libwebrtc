/*
 * Copyright 2022 LiveKit
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef LIBWEBRTC_RTC_DESKTOP_CAPTURER_IMPL_HXX
#define LIBWEBRTC_RTC_DESKTOP_CAPTURER_IMPL_HXX

#include "include/rtc_types.h"

#include "api/video/i420_buffer.h"
#include "api/video/video_frame.h"
#include "modules/desktop_capture/desktop_and_cursor_composer.h"
#include "modules/desktop_capture/desktop_capture_options.h"
#include "modules/desktop_capture/desktop_capturer.h"
#include "modules/desktop_capture/desktop_frame.h"
#include "rtc_base/thread.h"

#include "src/internal/desktop_capturer.h"

namespace libwebrtc {
class RTCDesktopCapturerImpl2 : public RTCDesktopCapturer2 {
 public:
  RTCDesktopCapturerImpl2(
      std::unique_ptr<webrtc::internal::LocalDesktopCapturer> video_capturer);
  ~RTCDesktopCapturerImpl2();

  std::unique_ptr<webrtc::internal::LocalDesktopCapturer> desktop_capturer();

  virtual bool StartCapture() override;
  virtual bool CaptureStarted() override;
  virtual void StopCapture() override;

  void SaveVideoSourceTrack(
      rtc::scoped_refptr<webrtc::internal::LocalDesktopCaptureTrackSource> source);
 private:
  std::unique_ptr<webrtc::internal::LocalDesktopCapturer> desktop_capturer_;
  rtc::scoped_refptr<webrtc::internal::LocalDesktopCaptureTrackSource>
      capturer_source_track_;
};

class RTCDesktopCapturerImpl : public RTCDesktopCapturer,
                               public webrtc::DesktopCapturer::Callback,
                               public rtc::MessageHandler,
                               public webrtc::internal::VideoCapturer {
 public:
  RTCDesktopCapturerImpl(DesktopType type,
                         webrtc::DesktopCapturer::SourceId source_id,
                         rtc::Thread* signaling_thread,
                         scoped_refptr<MediaSource> source);
  ~RTCDesktopCapturerImpl();

  void RegisterDesktopCapturerObserver(
      DesktopCapturerObserver* observer) override {
    observer_ = observer;
  }

  void DeRegisterDesktopCapturerObserver() override { observer_ = nullptr; }
  CaptureState Start(uint32_t fps) override;

  CaptureState Start(uint32_t fps,
                     uint32_t x,
                     uint32_t y,
                     uint32_t w,
                     uint32_t h) override;

  void Stop() override;

  bool IsRunning() override;

  scoped_refptr<MediaSource> source() override { return source_; }

 protected:
  virtual void OnCaptureResult(
      webrtc::DesktopCapturer::Result result,
      std::unique_ptr<webrtc::DesktopFrame> frame) override;
  virtual void OnMessage(rtc::Message* msg) override;

 private:
  void CaptureFrame();
  webrtc::DesktopCaptureOptions options_;
  std::unique_ptr<webrtc::DesktopCapturer> capturer_;
  std::unique_ptr<rtc::Thread> thread_;
  rtc::scoped_refptr<webrtc::I420Buffer> i420_buffer_;
  CaptureState capture_state_ = CS_STOPPED;
  DesktopType type_;
  webrtc::DesktopCapturer::SourceId source_id_;
  DesktopCapturerObserver* observer_ = nullptr;
  uint32_t capture_delay_ = 1000;  // 1s
  webrtc::DesktopCapturer::Result result_ =
      webrtc::DesktopCapturer::Result::SUCCESS;
  rtc::Thread* signaling_thread_ = nullptr;
  scoped_refptr<MediaSource> source_;
  uint32_t x_ = 0;
  uint32_t y_ = 0;
  uint32_t w_ = 0;
  uint32_t h_ = 0;
};

class ScreenCapturerTrackSource : public webrtc::VideoTrackSource {
 public:
  static rtc::scoped_refptr<ScreenCapturerTrackSource> Create(
      scoped_refptr<RTCDesktopCapturer> capturer) {
    if (capturer) {
      return rtc::make_ref_counted<ScreenCapturerTrackSource>(capturer);
    }
    return nullptr;
  }

 public:
  explicit ScreenCapturerTrackSource(scoped_refptr<RTCDesktopCapturer> capturer)
      : VideoTrackSource(/*remote=*/false), capturer_(std::move(capturer)) {}
  virtual ~ScreenCapturerTrackSource() { capturer_->Stop(); }

 private:
  rtc::VideoSourceInterface<webrtc::VideoFrame>* source() override {
    return static_cast<RTCDesktopCapturerImpl*>(capturer_.get());
  }

  scoped_refptr<RTCDesktopCapturer> capturer_;
};

}  // namespace libwebrtc

#endif  // LIBWEBRTC_RTC_DESKTOP_CAPTURER_IMPL_HXX
