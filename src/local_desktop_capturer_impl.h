#ifndef LIB_WEBRTC_LOCAL_SCREEN_CAPTURER_IMPL_HXX
#define LIB_WEBRTC_LOCAL_SCREEN_CAPTURER_IMPL_HXX

#include "local_desktop_capturer.h"
#include "src/internal/base_desktop_capturer.h"
#include "src/internal/video_capturer.h"

namespace owt {
namespace base {
class MSDKVideoEncoder;
}
}  // namespace owt

namespace libwebrtc {
class LocalDesktopCapturerImpl
    : public LocalDesktopCapturer,
      public rtc::VideoSinkInterface<webrtc::VideoFrame>,
      public webrtc::EncodedImageCallback {
 public:
  LocalDesktopCapturerImpl(LocalDesktopCapturerDataSource* capturer_observer);
  virtual ~LocalDesktopCapturerImpl();

  virtual bool StartCapturing(
      LocalDesktopEncodedImageCallback* image_callback,
      std::shared_ptr<LocalDesktopCapturerParameters>) override;
  virtual bool StartCapturing(
      LocalDesktopRawFrameCallback* frame_callback,
      std::shared_ptr<LocalDesktopCapturerParameters>) override;
  virtual bool StopCapturing(bool release_encoder) override;

  void OnFrame(const webrtc::VideoFrame& frame) override;
  virtual webrtc::EncodedImageCallback::Result OnEncodedImage(
      const webrtc::EncodedImage& encoded_image,
      const webrtc::CodecSpecificInfo* codec_specific_info) override;

 private:
  // capturer
  LocalDesktopCapturerDataSource* capturer_datasource_;
  rtc::scoped_refptr<owt::base::BasicScreenCapturer> capturer_;
  // encoder
  std::unique_ptr<owt::base::MSDKVideoEncoder> encoder_;
  LocalDesktopEncodedImageCallback* image_callback_;
  LocalDesktopRawFrameCallback* frame_callback_;
  bool encoder_initialized_;
  // frame type copied from `video_stream_encoder.h`
  std::vector<webrtc::VideoFrameType> next_frame_types_;
  uint32_t number_cores_ = 0;
  // path to save the encoded video data
  std::string encoded_file_save_path_;
  // max & min bitrate(kilobits/sec)
  int max_bitrate_;
  int min_bitrate_;
  uint32_t max_framerate_;

  bool InitEncoder(int width, int height);
  void ReleaseEncoder();
};

}  // namespace libwebrtc

#endif