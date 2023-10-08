#ifndef CUSTOMIZED_ENCODEDERVIDEOFACTORY_H_
#define CUSTOMIZED_ENCODEDERVIDEOFACTORY_H_

#include <vector>
#include "api/video_codecs/sdp_video_format.h"
#include "api/video_codecs/video_encoder.h"
#include "api/video_codecs/video_encoder_factory.h"
#include "libwebrtc.h"

namespace libwebrtc {
class CustomizedEncodedVideoEncoderFactory : public webrtc::VideoEncoderFactory {
 public:
  CustomizedEncodedVideoEncoderFactory();
  virtual ~CustomizedEncodedVideoEncoderFactory() {}

  using webrtc::VideoEncoderFactory::CreateVideoEncoder;

  std::unique_ptr<webrtc::VideoEncoder> CreateVideoEncoder(
      const webrtc::SdpVideoFormat& format) override;
  std::vector<webrtc::SdpVideoFormat> GetSupportedFormats() const override;

  // will restore after call `CreateVideoEncoder()`
  void ForceUsingEncodedEncoder() { is_encoded_source_ = true; }
  void ForceUsingScreencastConfig() { is_screen_cast_ = true; }

 private:
  bool is_encoded_source_; // default is false
  bool is_screen_cast_; // default is false
  std::vector<webrtc::VideoCodecType> supported_codec_types_;
};

} // namespace libwebrtc

#endif  // CUSTOMIZED_ENCODEDERVIDEOFACTORY_H_