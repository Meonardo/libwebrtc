// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef OWT_BASE_CUSTOMIZEDVIDEOSOURCE_H
#define OWT_BASE_CUSTOMIZEDVIDEOSOURCE_H

#include "api/media_stream_interface.h"
#include "api/scoped_refptr.h"
#include "api/video/video_frame.h"
#include "api/video/video_rotation.h"
#include "api/video/video_sink_interface.h"
#include "media/base/video_adapter.h"
#include "media/base/video_broadcaster.h"
#include "modules/video_capture/video_capture.h"
#include "pc/video_track_source.h"

#include "framegeneratorinterface.h"
#include "videoencoderinterface.h"

namespace owt {
namespace base {
using namespace cricket;

// Factory class for different customized capturers
class CustomizedVideoCapturerFactory {
 public:
  static rtc::scoped_refptr<webrtc::VideoCaptureModule> Create(
      VideoFrameGeneratorInterface* framer);
  static rtc::scoped_refptr<webrtc::VideoCaptureModule> Create(
      VideoEncoderInterface* encoder);
};

class CustomizedVideoSource
    : public rtc::VideoSourceInterface<webrtc::VideoFrame> {
 public:
  CustomizedVideoSource();
  ~CustomizedVideoSource() override;

  void AddOrUpdateSink(rtc::VideoSinkInterface<webrtc::VideoFrame>* sink,
                       const rtc::VideoSinkWants& wants) override;
  void RemoveSink(rtc::VideoSinkInterface<webrtc::VideoFrame>* sink) override;

 protected:
  void OnFrame(const webrtc::VideoFrame& frame);
  rtc::VideoSinkWants GetSinkWants();

 private:
  void UpdateVideoAdapter();

  rtc::VideoBroadcaster broadcaster_;
  cricket::VideoAdapter video_adapter_;
};

// The proxy capturer to actual VideoCaptureModule implementation.
class CustomizedCapturer : public CustomizedVideoSource,
                           public rtc::VideoSinkInterface<webrtc::VideoFrame> {
 public:
  static CustomizedCapturer* Create(VideoFrameGeneratorInterface* framer);
  static CustomizedCapturer* Create(VideoEncoderInterface* encoder);

  virtual ~CustomizedCapturer();

  // VideoSinkInterfaceImpl
  void OnFrame(const webrtc::VideoFrame& frame) override;

 private:
  CustomizedCapturer();
  bool Init(VideoFrameGeneratorInterface* framer);
  bool Init(VideoEncoderInterface* encoder);

  void Destroy();

  rtc::scoped_refptr<webrtc::VideoCaptureModule> vcm_;
  webrtc::VideoCaptureCapability capability_;
};

// VideoTrackSources
class LocalRawCaptureTrackSource : public webrtc::VideoTrackSource {
 public:
  static rtc::scoped_refptr<LocalRawCaptureTrackSource> Create(
      VideoFrameGeneratorInterface* framer) {
    std::unique_ptr<CustomizedCapturer> capturer;
    capturer = absl::WrapUnique(CustomizedCapturer::Create(std::move(framer)));

    if (capturer)
      return rtc::make_ref_counted<LocalRawCaptureTrackSource>(
          std::move(capturer));

    return nullptr;
  }

 protected:
  explicit LocalRawCaptureTrackSource(
      std::unique_ptr<CustomizedCapturer> capturer)
      : VideoTrackSource(/*remote=*/false), capturer_(std::move(capturer)) {}

 private:
  rtc::VideoSourceInterface<webrtc::VideoFrame>* source() override {
    return capturer_.get();
  }
  std::unique_ptr<CustomizedCapturer> capturer_;
};

class LocalEncodedCaptureTrackSource : public webrtc::VideoTrackSource {
 public:
  static rtc::scoped_refptr<LocalEncodedCaptureTrackSource> Create(
      VideoEncoderInterface* encoder) {
    std::unique_ptr<CustomizedCapturer> capturer;
    capturer = absl::WrapUnique(CustomizedCapturer::Create(encoder));

    if (capturer)
      return rtc::make_ref_counted<LocalEncodedCaptureTrackSource>(
          std::move(capturer));

    return nullptr;
  }

 protected:
  explicit LocalEncodedCaptureTrackSource(
      std::unique_ptr<CustomizedCapturer> capturer)
      : VideoTrackSource(/*remote=*/false), capturer_(std::move(capturer)) {}

 private:
  rtc::VideoSourceInterface<webrtc::VideoFrame>* source() override {
    return capturer_.get();
  }
  std::unique_ptr<CustomizedCapturer> capturer_;
};

}  // namespace base
}  // namespace owt

#endif