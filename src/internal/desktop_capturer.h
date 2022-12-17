#ifndef LIB_WEBRTC_DESKTOP_CAPTURER_IMPL_HXX
#define LIB_WEBRTC_DESKTOP_CAPTURER_IMPL_HXX

#include "rtc_types.h"
#include "src/internal/base_desktop_capturer.h"
#include "src/internal/video_capturer.h"

namespace webrtc {
namespace internal {

// The proxy capturer to actual VideoCaptureModule implementation.
class LocalDesktopCapturer
    : public VideoCapturer,
      public rtc::VideoSinkInterface<webrtc::VideoFrame> {
 public:
  static LocalDesktopCapturer* Create(
      std::shared_ptr<libwebrtc::LocalDesktopCapturerParameters> parameters,
      libwebrtc::LocalDesktopCapturerObserver* observer);

  LocalDesktopCapturer();
  virtual ~LocalDesktopCapturer();

  // VideoSinkInterfaceImpl
  void OnFrame(const webrtc::VideoFrame& frame) override;

  bool Init(
      std::shared_ptr<libwebrtc::LocalDesktopCapturerParameters> parameters,
      libwebrtc::LocalDesktopCapturerObserver* observer);
  void Destroy();

  rtc::scoped_refptr<webrtc::VideoCaptureModule> vcm_;
  webrtc::VideoCaptureCapability capability_;
};

class LocalDesktopCaptureTrackSource : public webrtc::VideoTrackSource {
 public:
  static rtc::scoped_refptr<LocalDesktopCaptureTrackSource> Create(
      std::shared_ptr<libwebrtc::LocalDesktopCapturerParameters> parameters,
      libwebrtc::LocalDesktopCapturerObserver* observer) {
    std::unique_ptr<LocalDesktopCapturer> capturer;
    capturer =
        absl::WrapUnique(LocalDesktopCapturer::Create(parameters, observer));

    if (capturer)
      return new rtc::RefCountedObject<LocalDesktopCaptureTrackSource>(
          std::move(capturer));

    return nullptr;
  }

  static rtc::scoped_refptr<LocalDesktopCaptureTrackSource> Create(
      std::unique_ptr<LocalDesktopCapturer> capturer) {
    if (capturer) {
      return new rtc::RefCountedObject<LocalDesktopCaptureTrackSource>(
          std::move(capturer));
    }
    return nullptr;
  }

 protected:
  explicit LocalDesktopCaptureTrackSource(
      std::unique_ptr<LocalDesktopCapturer> capturer)
      : webrtc::VideoTrackSource(/*remote=*/false),
        capturer_(std::move(capturer)) {}

 private:
  rtc::VideoSourceInterface<webrtc::VideoFrame>* source() override {
    return capturer_.get();
  }
  std::unique_ptr<LocalDesktopCapturer> capturer_;
};

}  // namespace internal
}  // namespace webrtc
#endif