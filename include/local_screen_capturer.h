#ifndef LIB_WEBRTC_LOCAL_SCREEN_CAPTURER_HXX
#define LIB_WEBRTC_LOCAL_SCREEN_CAPTURER_HXX

#include "rtc_desktop_device.h"
#include "rtc_video_frame.h"

namespace libwebrtc {
class LocalScreenEncodedImageCallback {
 public:
  virtual void OnEncodedImage(uint8_t* data,
                              size_t size,
                              bool keyframe,
                              size_t w,
                              size_t h) = 0;
  /// @brief contains already encoded data handle `EncodedImageHandle`
  /// @param image `EncodedImageHandle` pointer
  virtual void OnEncodedImage(void* image) = 0;
};

class LocalScreenRawFrameCallback {
 public:
  // the frame is i420 type
  virtual void OnFrame(scoped_refptr<RTCVideoFrame> frame) = 0;
};

class LocalScreenCapturer : public RefCountInterface {
 public:
  LIB_WEBRTC_API static scoped_refptr<LocalScreenCapturer>
  CreateLocalScreenCapturer(LocalDesktopCapturerObserver* observer,
                            LocalDesktopCapturerParameters* parameters);

  // start capturing screen and register encoded image callback
  virtual bool StartCapturing(
      LocalScreenEncodedImageCallback* image_callback) = 0;
  // start capturing screen and register raw frame callbacl
  virtual bool StartCapturing(LocalScreenRawFrameCallback* frame_callback) = 0;
  // stop capturing screen
  virtual bool StopCapturing() = 0;

 protected:
  virtual ~LocalScreenCapturer() {}
};

}  // namespace libwebrtc

#endif