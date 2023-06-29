#include "customizedvideoencoderfactory.h"
#include <string>
#include "absl/strings/match.h"
#include "modules/video_coding/codecs/av1/libaom_av1_encoder.h"
#include "modules/video_coding/codecs/h264/include/h264.h"
#include "modules/video_coding/codecs/vp8/include/vp8.h"
#include "modules/video_coding/codecs/vp9/include/vp9.h"
#include "src/win/codecutils.h"
#include "src/win/customizedvideoencoderproxy.h"
#include "src/win/msdkvideoencoder.h"
#include "src/win/msdkvideoencoderfactory.h"

namespace libwebrtc {
CustomizedEncodedVideoEncoderFactory::CustomizedEncodedVideoEncoderFactory()
    : is_encoded_source_(false) {
  supported_codec_types_.clear();
  auto media_capability = owt::base::MediaCapabilities::Get();
  std::vector<owt::base::VideoCodec> codecs_to_check;
  codecs_to_check.push_back(owt::base::VideoCodec::kH264);
  codecs_to_check.push_back(owt::base::VideoCodec::kVp9);
  codecs_to_check.push_back(owt::base::VideoCodec::kH265);
  codecs_to_check.push_back(owt::base::VideoCodec::kAv1);
  codecs_to_check.push_back(owt::base::VideoCodec::kVp8);
  std::vector<owt::base::VideoEncoderCapability> capabilities =
      media_capability->SupportedCapabilitiesForVideoEncoder(codecs_to_check);
  // TODO(jianlin): use the check result from MSDK.
  supported_codec_types_.push_back(webrtc::kVideoCodecH264);
  supported_codec_types_.push_back(webrtc::kVideoCodecVP9);
  supported_codec_types_.push_back(webrtc::kVideoCodecVP8);
  supported_codec_types_.push_back(webrtc::kVideoCodecH265);
  supported_codec_types_.push_back(webrtc::kVideoCodecAV1);
}

std::unique_ptr<webrtc::VideoEncoder>
CustomizedEncodedVideoEncoderFactory::CreateVideoEncoder(
    const webrtc::SdpVideoFormat& format) {
  if (is_encoded_source_) {
    is_encoded_source_ = false;
    return owt::base::CustomizedVideoEncoderProxy::Create();
  } else {
    // VP8 encoding will always use SW impl.
    if (absl::EqualsIgnoreCase(format.name, cricket::kVp8CodecName))
      return webrtc::VP8Encoder::Create();
    // VP9 encoding will only be enabled on ICL+;
    else if (absl::EqualsIgnoreCase(format.name, cricket::kVp9CodecName))
      return webrtc::VP9Encoder::Create(cricket::VideoCodec(format));
    // TODO: Replace with AV1 HW encoder post ADL.
    else if (absl::EqualsIgnoreCase(format.name, cricket::kAv1CodecName))
      return webrtc::CreateLibaomAv1Encoder();

    return owt::base::MSDKVideoEncoder::Create(cricket::VideoCodec(format));
  }
}

std::vector<webrtc::SdpVideoFormat>
CustomizedEncodedVideoEncoderFactory::GetSupportedFormats() const {
  std::vector<webrtc::SdpVideoFormat> supported_codecs;
  if (is_encoded_source_) {
    supported_codecs.push_back(
        owt::base::CodecUtils::GetConstrainedBaselineH264Codec());
  } else {
    supported_codecs.push_back(webrtc::SdpVideoFormat(cricket::kVp8CodecName));
    supported_codecs.push_back(webrtc::SdpVideoFormat(cricket::kAv1CodecName));

    for (const webrtc::SdpVideoFormat& format : webrtc::SupportedVP9Codecs())
      supported_codecs.push_back(format);
    // TODO: We should combine the codec profiles that hardware H.264 encoder
    // supports with those provided by built-in H.264 encoder
    for (const webrtc::SdpVideoFormat& format :
         owt::base::CodecUtils::SupportedH264Codecs())
      supported_codecs.push_back(format);

    for (const webrtc::SdpVideoFormat& format :
         owt::base::CodecUtils::GetSupportedH265Codecs()) {
      supported_codecs.push_back(format);
    }
  }

  return supported_codecs;
}

}  // namespace libwebrtc