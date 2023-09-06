#include "desktop_capturer.h"
#include "src/internal/base_desktop_capturer.h"

namespace webrtc {
namespace internal {

LocalDesktopCapturer* LocalDesktopCapturer::Create(
    std::shared_ptr<libwebrtc::LocalDesktopCapturerParameters> parameters,
    libwebrtc::LocalDesktopCapturerDataSource* datasource) {
  std::unique_ptr<LocalDesktopCapturer> vcm_capturer(
      new LocalDesktopCapturer());
  if (!vcm_capturer->Init(parameters, datasource))
    return nullptr;
  return vcm_capturer.release();
}

LocalDesktopCapturer::LocalDesktopCapturer() : vcm_(nullptr) {}

bool LocalDesktopCapturer::Init(
    std::shared_ptr<libwebrtc::LocalDesktopCapturerParameters> parameters,
    libwebrtc::LocalDesktopCapturerDataSource* datasource) {
  webrtc::DesktopCaptureOptions options =
      webrtc::DesktopCaptureOptions::CreateDefault();
  options.set_allow_directx_capturer(true);
  if (parameters->SourceType() == libwebrtc::LocalDesktopCapturerParameters::
                                      DesktopSourceType::kApplication) {
    vcm_ = rtc::make_ref_counted<owt::base::BasicWindowCapturer>(
        options, datasource, parameters->CursorEnabled());
  } else {
    vcm_ = rtc::make_ref_counted<owt::base::BasicScreenCapturer>(
        options, datasource, parameters->CursorEnabled());
  }

  if (!vcm_)
    return false;

  vcm_->RegisterCaptureDataCallback(this);
  capability_.videoType = webrtc::VideoType::kI420;
  capability_.height = parameters->Height();
  capability_.width = parameters->Width();
  capability_.maxFPS = parameters->Fps();

  if (vcm_->StartCapture(capability_) != 0) {
    Destroy();
    return false;
  }

  RTC_CHECK(vcm_->CaptureStarted());
  return true;
}

bool LocalDesktopCapturer::StartCapture() {
  if (vcm_ == nullptr) {
    RTC_LOG(LS_ERROR) << "vcm_ is nullptr";
    return false;
  }

  int32_t result = vcm_->StartCapture(capability_);

  if (result != 0) {
    Destroy();
    return false;
  }

  return true;
}

bool LocalDesktopCapturer::CaptureStarted() {
  if (vcm_ == nullptr) {
    RTC_LOG(LS_ERROR) << "vcm_ is nullptr";
    return false;
  }

  return vcm_->CaptureStarted();
}

void LocalDesktopCapturer::StopCapture() {
  vcm_->StopCapture();
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

namespace libwebrtc {}  // namespace libwebrtc
