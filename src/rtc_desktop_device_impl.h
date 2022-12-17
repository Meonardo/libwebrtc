#ifndef LIB_WEBRTC_DESKTOP_DEVICE_IMPL_HXX
#define LIB_WEBRTC_DESKTOP_DEVICE_IMPL_HXX

#include "internal/desktop_capturer.h"
#include "rtc_desktop_device.h"

namespace libwebrtc {

class RTCDesktopCapturerImpl : public RTCDesktopCapturer {
 public:
  RTCDesktopCapturerImpl(
      std::unique_ptr<webrtc::internal::LocalDesktopCapturer> video_capturer);

  std::unique_ptr<webrtc::internal::LocalDesktopCapturer> desktop_capturer() {
    return std::unique_ptr<webrtc::internal::LocalDesktopCapturer>(
        std::move(desktop_capturer_));
  }

 private:
  std::unique_ptr<webrtc::internal::LocalDesktopCapturer> desktop_capturer_;
};

class RTCDesktopDeviceImpl : public RTCDesktopDevice {
 public:
  RTCDesktopDeviceImpl(rtc::Thread* signaling_thread);
  ~RTCDesktopDeviceImpl();

  virtual scoped_refptr<RTCDesktopCapturer> CreateDesktopCapturer(
      LocalDesktopCapturerObserver* source_observer,
      std::shared_ptr<LocalDesktopCapturerParameters> params) override;

 private:
  std::unique_ptr<rtc::Thread> _capture_thread;
  rtc::Thread* signaling_thread_ = nullptr;
};

}  // namespace libwebrtc

#endif