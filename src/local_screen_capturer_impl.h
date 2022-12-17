#ifndef LIB_WEBRTC_LOCAL_SCREEN_CAPTURER_IMPL_HXX
#define LIB_WEBRTC_LOCAL_SCREEN_CAPTURER_IMPL_HXX

#include "local_screen_capturer.h"
#include "src/internal/base_desktop_capturer.h"
#include "src/internal/video_capturer.h"

namespace libwebrtc {
class LocalScreenCapturerImpl
    : public LocalScreenCapturer,
      public rtc::VideoSinkInterface<webrtc::VideoFrame> {
 public:
  LocalScreenCapturerImpl(webrtc::DesktopCaptureOptions options,
                          webrtc::VideoCaptureCapability capability,
                          LocalDesktopCapturerObserver* observer,
                          bool cursor_enabled);
  virtual ~LocalScreenCapturerImpl();

  virtual bool StartCapturing() override;
  virtual bool StopCapturing() override;

  void OnFrame(const webrtc::VideoFrame& frame) override;

 private:
  rtc::scoped_refptr<owt::base::BasicScreenCapturer> capturer_;
  webrtc::VideoCaptureCapability capability_;
};

}  // namespace libwebrtc

#endif