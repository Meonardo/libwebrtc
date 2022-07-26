#include "desktop_capturer.h"

namespace webrtc {
namespace internal {

LocalDesktopCapturer* LocalDesktopCapturer::Create(
    std::shared_ptr<libwebrtc::LocalDesktopStreamParameters> parameters,
    std::unique_ptr<libwebrtc::LocalDesktopStreamObserver> observer) {
  std::unique_ptr<LocalDesktopCapturer> vcm_capturer(
      new LocalDesktopCapturer());
  if (!vcm_capturer->Init(parameters, std::move(observer)))
    return nullptr;
  return vcm_capturer.release();
}

LocalDesktopCapturer::LocalDesktopCapturer() : vcm_(nullptr) {}

bool LocalDesktopCapturer::Init(
    std::shared_ptr<libwebrtc::LocalDesktopStreamParameters> parameters,
    std::unique_ptr<libwebrtc::LocalDesktopStreamObserver> observer) {

  webrtc::DesktopCaptureOptions options =
      webrtc::DesktopCaptureOptions::CreateDefault();
  options.set_allow_directx_capturer(true);
  if (parameters->SourceType() == libwebrtc::LocalDesktopStreamParameters::
                                      DesktopSourceType::kApplication) {
    vcm_ = new rtc::RefCountedObject<owt::base::BasicWindowCapturer>(
        options, std::move(observer), parameters->CursorEnabled());
  } else {
    vcm_ = new rtc::RefCountedObject<owt::base::BasicScreenCapturer>(
        options, std::move(observer), parameters->CursorEnabled());
  }

  if (!vcm_)
    return false;

  vcm_->RegisterCaptureDataCallback(this);
  capability_.maxFPS = parameters->Fps();
  capability_.videoType = webrtc::VideoType::kI420;

  if (vcm_->StartCapture(capability_) != 0) {
    Destroy();
    return false;
  }

  RTC_CHECK(vcm_->CaptureStarted());
  return true;
}

void LocalDesktopCapturer::Destroy() {
  if (!vcm_)
    return;

  vcm_->StopCapture();
  vcm_->DeRegisterCaptureDataCallback();
  vcm_ = nullptr;
}

LocalDesktopCapturer::~LocalDesktopCapturer() {
  Destroy();
}

void LocalDesktopCapturer::OnFrame(const webrtc::VideoFrame& frame) {
  VideoCapturer::OnFrame(frame);
}

}  // namespace internal
}  // namespace webrtc
