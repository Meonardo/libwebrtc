#ifndef CUSTOMIZED_ENCODEDERVIDEOFACTORY_H_
#define CUSTOMIZED_ENCODEDERVIDEOFACTORY_H_

#include <atomic>
#include <mutex>
#include <vector>

#include "api/video_codecs/sdp_video_format.h"
#include "api/video_codecs/video_encoder.h"
#include "api/video_codecs/video_encoder_factory.h"
#include "libwebrtc.h"

namespace libwebrtc {
class CustomizedEncodedVideoEncoderFactory
    : public webrtc::VideoEncoderFactory {
 public:
  CustomizedEncodedVideoEncoderFactory();
  virtual ~CustomizedEncodedVideoEncoderFactory() {}

  using webrtc::VideoEncoderFactory::CreateVideoEncoder;

  std::unique_ptr<webrtc::VideoEncoder> CreateVideoEncoder(
      const webrtc::SdpVideoFormat& format) override;
  std::vector<webrtc::SdpVideoFormat> GetSupportedFormats() const override;

  // these methods decide whether to use the customized encoder or the internal,
  // !NOTICE: it will try to lock the state until the encoder is created.
  void ForceUsingEncodedEncoder();
  void RestoreUsingNormalEncoder();

  void ForceUsingScreencastConfig() { is_screen_cast_ = true; }

 private:
  std::atomic<bool> is_encoded_source_; // default is false
  std::atomic<bool> is_encoded_mutex_locked_; // default is false
  mutable std::mutex encoded_source_mutex_;

  bool is_screen_cast_;  // default is false
  std::vector<webrtc::VideoCodecType> supported_codec_types_;
  std::unique_ptr<webrtc::VideoEncoderFactory> internal_encoder_factory_;
};

}  // namespace libwebrtc

#endif  // CUSTOMIZED_ENCODEDERVIDEOFACTORY_H_