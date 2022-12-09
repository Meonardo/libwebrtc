// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "src/win/customizedvideosource.h"
#include "src/win/customizedframescapturer.h"

namespace owt {
namespace base {

rtc::scoped_refptr<webrtc::VideoCaptureModule>
CustomizedVideoCapturerFactory::Create(
    std::unique_ptr<VideoFrameGeneratorInterface> framer) {
  return new rtc::RefCountedObject<CustomizedFramesCapturer>(std::move(framer));
}

rtc::scoped_refptr<webrtc::VideoCaptureModule>
CustomizedVideoCapturerFactory::Create(VideoEncoderInterface* encoder) {
  return new rtc::RefCountedObject<CustomizedFramesCapturer>(1920, 1080, 30,
                                                             4096, encoder);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

CustomizedVideoSource::CustomizedVideoSource() = default;
CustomizedVideoSource::~CustomizedVideoSource() = default;

void CustomizedVideoSource::OnFrame(const webrtc::VideoFrame& frame) {
  // TODO(johny): We need to adapt frame for yuv input here, but not for
  // encoded input.
  broadcaster_.OnFrame(frame);
}

void CustomizedVideoSource::AddOrUpdateSink(
    rtc::VideoSinkInterface<webrtc::VideoFrame>* sink,
    const rtc::VideoSinkWants& wants) {
  broadcaster_.AddOrUpdateSink(sink, wants);
  UpdateVideoAdapter();
}
void CustomizedVideoSource::RemoveSink(
    rtc::VideoSinkInterface<webrtc::VideoFrame>* sink) {
  broadcaster_.RemoveSink(sink);
  UpdateVideoAdapter();
}

rtc::VideoSinkWants CustomizedVideoSource::GetSinkWants() {
  return broadcaster_.wants();
}

void CustomizedVideoSource::UpdateVideoAdapter() {
  // NOT Implemented.
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

CustomizedCapturer* CustomizedCapturer::Create(
    std::unique_ptr<VideoFrameGeneratorInterface> framer) {
  std::unique_ptr<CustomizedCapturer> vcm_capturer(new CustomizedCapturer());
  if (!vcm_capturer->Init(std::move(framer)))
    return nullptr;
  return vcm_capturer.release();
}

CustomizedCapturer* CustomizedCapturer::Create(VideoEncoderInterface* encoder) {
  std::unique_ptr<CustomizedCapturer> vcm_capturer(new CustomizedCapturer());
  if (!vcm_capturer->Init(encoder))
    return nullptr;
  return vcm_capturer.release();
}

CustomizedCapturer::CustomizedCapturer() : vcm_(nullptr) {}

bool CustomizedCapturer::Init(
    std::unique_ptr<VideoFrameGeneratorInterface> framer) {
  vcm_ = CustomizedVideoCapturerFactory::Create(std::move(framer));

  if (!vcm_)
    return false;

  vcm_->RegisterCaptureDataCallback(this);
  capability_.width = 1920;
  capability_.height = 1080;
  capability_.maxFPS = 30;
  capability_.videoType = webrtc::VideoType::kI420;

  if (vcm_->StartCapture(capability_) != 0) {
    Destroy();
    return false;
  }

  RTC_CHECK(vcm_->CaptureStarted());
  return true;
}

bool CustomizedCapturer::Init(VideoEncoderInterface* encoder) {
  vcm_ = CustomizedVideoCapturerFactory::Create(encoder);

  if (!vcm_)
    return false;

  vcm_->RegisterCaptureDataCallback(this);
  capability_.width = 1920;
  capability_.height = 1080;
  capability_.maxFPS = 30;

  if (vcm_->StartCapture(capability_) != 0) {
    Destroy();
    return false;
  }

  RTC_CHECK(vcm_->CaptureStarted());
  return true;
}

void CustomizedCapturer::Destroy() {
  if (!vcm_)
    return;

  vcm_->StopCapture();
  vcm_->DeRegisterCaptureDataCallback();
  vcm_ = nullptr;
}

CustomizedCapturer::~CustomizedCapturer() {
  Destroy();
}

void CustomizedCapturer::OnFrame(const webrtc::VideoFrame& frame) {
  CustomizedVideoSource::OnFrame(frame);
}

}  // namespace base
}  // namespace owt