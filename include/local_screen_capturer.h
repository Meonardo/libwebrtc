#ifndef LIB_WEBRTC_LOCAL_SCREEN_CAPTURER_HXX
#define LIB_WEBRTC_LOCAL_SCREEN_CAPTURER_HXX

#include "rtc_desktop_device.h"

namespace libwebrtc {
class LocalScreenEncodedImageCallback {
 public:
  virtual void OnEncodedImage(const uint8_t* data,
                              size_t size,
                              bool keyframe,
                              size_t w,
                              size_t h) = 0;
};

class LocalScreenCapturer : public RefCountInterface {
 public:
  LIB_WEBRTC_API static scoped_refptr<LocalScreenCapturer>
  CreateLocalScreenCapturer(LocalDesktopCapturerObserver* observer,
                            LocalDesktopCapturerParameters* parameters);

  virtual bool StartCapturing(
      LocalScreenEncodedImageCallback* image_callback) = 0;
  virtual bool StopCapturing() = 0;

 protected:
  virtual ~LocalScreenCapturer() {}
};

}  // namespace libwebrtc

#endif