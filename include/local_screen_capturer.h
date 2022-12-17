#ifndef LIB_WEBRTC_LOCAL_SCREEN_CAPTURER_HXX
#define LIB_WEBRTC_LOCAL_SCREEN_CAPTURER_HXX

#include "rtc_desktop_device.h"

namespace libwebrtc {
class LocalScreenCapturer : public RefCountInterface {
 public:
  LIB_WEBRTC_API static scoped_refptr<LocalScreenCapturer>
  CreateLocalScreenCapturer(LocalDesktopCapturerObserver* observer,
                            LocalDesktopCapturerParameters* parameters);

  virtual bool StartCapturing() = 0;
  virtual bool StopCapturing() = 0;

 protected:
  virtual ~LocalScreenCapturer() {}
};

}  // namespace libwebrtc

#endif