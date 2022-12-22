#ifndef LIB_WEBRTC_LOCAL_SCREEN_CAPTURER_IMPL_HXX
#define LIB_WEBRTC_LOCAL_SCREEN_CAPTURER_IMPL_HXX

#include "local_screen_capturer.h"
#include "src/internal/base_desktop_capturer.h"
#include "src/internal/video_capturer.h"

namespace owt {
namespace base {
class MSDKVideoEncoder;
}
}  // namespace owt

namespace libwebrtc {
class LocalScreenCapturerImpl
    : public LocalScreenCapturer,
      public rtc::VideoSinkInterface<webrtc::VideoFrame>,
      public webrtc::EncodedImageCallback {
 public:
  LocalScreenCapturerImpl(webrtc::VideoCaptureCapability capability,
                          LocalDesktopCapturerObserver* observer,
                          bool cursor_enabled);
  virtual ~LocalScreenCapturerImpl();

  virtual bool StartCapturing(
      LocalScreenEncodedImageCallback* image_callback) override;
  virtual bool StartCapturing(
      LocalScreenRawFrameCallback* frame_callback) override;
  virtual bool StopCapturing() override;

  void OnFrame(const webrtc::VideoFrame& frame) override;
  virtual webrtc::EncodedImageCallback::Result OnEncodedImage(
      const webrtc::EncodedImage& encoded_image,
      const webrtc::CodecSpecificInfo* codec_specific_info) override;

 private:
  // capturer
  LocalDesktopCapturerObserver* capturer_observer_;
  rtc::scoped_refptr<owt::base::BasicScreenCapturer> capturer_;
  webrtc::VideoCaptureCapability capability_;
  bool cursor_enabled_;
  // encoder
  std::unique_ptr<owt::base::MSDKVideoEncoder> encoder_;
  LocalScreenEncodedImageCallback* image_callback_;
  LocalScreenRawFrameCallback* frame_callback_;
  bool encoder_initialized_;
  // frame type copied from `video_stream_encoder.h`
  std::vector<webrtc::VideoFrameType> next_frame_types_;
  uint32_t number_cores_ = 0;

  bool InitEncoder(int width, int height);
  void ReleaseEncoder();
};

}  // namespace libwebrtc

#endif