#ifndef LIB_WEBRTC_LOCAL_SCREEN_CAPTURER_HXX
#define LIB_WEBRTC_LOCAL_SCREEN_CAPTURER_HXX

#include "rtc_desktop_device.h"
#include "rtc_video_frame.h"

namespace libwebrtc {
class LocalDesktopEncodedImageCallback {
 public:
  virtual void OnEncodedImage(uint8_t* data,
                              size_t size,
                              bool keyframe,
                              size_t w,
                              size_t h) = 0;
};

class LocalDesktopRawFrameCallback {
 public:
  // the frame is i420 type
  virtual void OnFrame(scoped_refptr<RTCVideoFrame> frame) = 0;
};

class LocalDesktopCapturer : public RefCountInterface {
 public:
  LIB_WEBRTC_API static scoped_refptr<LocalDesktopCapturer>
  CreateLocalDesktopCapturer(LocalDesktopCapturerDataSource* datasource);

  // start capturing screen and register encoded image callback
  virtual bool StartCapturing(
      LocalDesktopEncodedImageCallback* image_callback,
      std::shared_ptr<LocalDesktopCapturerParameters> parameters) = 0;
  // start capturing screen and register raw frame callback
  virtual bool StartCapturing(
      LocalDesktopRawFrameCallback* frame_callback,
      std::shared_ptr<LocalDesktopCapturerParameters> parameters) = 0;
  // stop capturing screen
  // `release_encoder` default is `false`
  virtual bool StopCapturing(bool release_encoder) = 0;

 protected:
  virtual ~LocalDesktopCapturer() {}
};

}  // namespace libwebrtc

#endif