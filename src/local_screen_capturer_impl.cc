#include "src/local_screen_capturer_impl.h"

namespace libwebrtc {
// static method
scoped_refptr<LocalScreenCapturer>
LocalScreenCapturer::CreateLocalScreenCapturer(
    LocalDesktopCapturerObserver* observer,
    LocalDesktopCapturerParameters* parameters) {
  webrtc::VideoCaptureCapability capability;
  capability.maxFPS = parameters->Fps();
  capability.videoType = webrtc::VideoType::kI420;
  webrtc::DesktopCaptureOptions options =
      webrtc::DesktopCaptureOptions::CreateDefault();
  options.set_allow_directx_capturer(true);

  scoped_refptr<LocalScreenCapturerImpl> capturer =
      scoped_refptr<LocalScreenCapturerImpl>(
          new RefCountedObject<LocalScreenCapturerImpl>(
              options, capability, observer, parameters->CursorEnabled()));
  return capturer;
}

LocalScreenCapturerImpl::LocalScreenCapturerImpl(
    webrtc::DesktopCaptureOptions options,
    webrtc::VideoCaptureCapability capability,
    LocalDesktopCapturerObserver* observer,
    bool cursor_enabled)
    : capability_(capability) {
  capturer_ = new rtc::RefCountedObject<owt::base::BasicScreenCapturer>(
      options, observer, cursor_enabled);
}

LocalScreenCapturerImpl::~LocalScreenCapturerImpl() {
  StopCapturing();
  capturer_ = nullptr;
}

bool LocalScreenCapturerImpl::StartCapturing() {
  if (capturer_ == nullptr) {
    return false;
  }

  capturer_->RegisterCaptureDataCallback(this);
  if (capturer_->StartCapture(capability_) != 0) {
    return StopCapturing();
  }

  return capturer_->CaptureStarted();
}

bool LocalScreenCapturerImpl::StopCapturing() {
  if (capturer_ == nullptr)
    return false;

  capturer_->StopCapture();
  capturer_->DeRegisterCaptureDataCallback();
  return true;
}

void LocalScreenCapturerImpl::OnFrame(const webrtc::VideoFrame& frame) {}

}  // namespace libwebrtc