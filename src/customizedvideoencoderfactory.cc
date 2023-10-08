#include "customizedvideoencoderfactory.h"
#include <string>
#include "absl/strings/match.h"
#include "modules/video_coding/codecs/av1/libaom_av1_encoder.h"
#include "modules/video_coding/codecs/h264/include/h264.h"
#include "modules/video_coding/codecs/vp8/include/vp8.h"
#include "modules/video_coding/codecs/vp9/include/vp9.h"
#include "rtc_base/logging.h"
#include "src/win/codecutils.h"
#include "src/win/customizedvideoencoderproxy.h"
#include "src/win/msdkvideoencoder.h"
#include "src/win/msdkvideoencoderfactory.h"

namespace libwebrtc {
CustomizedEncodedVideoEncoderFactory::CustomizedEncodedVideoEncoderFactory()
    : is_encoded_source_(false), is_screen_cast_(false) {
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
  for (const auto& capability : capabilities) {
    if (capability.codec_type == owt::base::VideoCodec::kH264)
      supported_codec_types_.push_back(webrtc::kVideoCodecH264);
    else if (capability.codec_type == owt::base::VideoCodec::kVp9)
      supported_codec_types_.push_back(webrtc::kVideoCodecVP9);
    else if (capability.codec_type == owt::base::VideoCodec::kH265)
      supported_codec_types_.push_back(webrtc::kVideoCodecH265);
    else if (capability.codec_type == owt::base::VideoCodec::kAv1)
      supported_codec_types_.push_back(webrtc::kVideoCodecAV1);
    else if (capability.codec_type == owt::base::VideoCodec::kVp8)
      supported_codec_types_.push_back(webrtc::kVideoCodecVP8);
  }
}

std::unique_ptr<webrtc::VideoEncoder>
CustomizedEncodedVideoEncoderFactory::CreateVideoEncoder(
    const webrtc::SdpVideoFormat& format) {
  if (is_encoded_source_) {
    is_encoded_source_ = false;
    return owt::base::CustomizedVideoEncoderProxy::Create();
  } else {
    bool vp9_hw = false, vp8_hw = false, av1_hw = false, h264_hw = false;
    bool h265_hw = false;
    for (auto& codec : supported_codec_types_) {
      if (codec == webrtc::kVideoCodecAV1)
        av1_hw = true;
      else if (codec == webrtc::kVideoCodecH264)
        h264_hw = true;
      else if (codec == webrtc::kVideoCodecVP8)
        vp8_hw = true;
      else if (codec == webrtc::kVideoCodecVP9)
        vp9_hw = true;
      else if (codec == webrtc::kVideoCodecH265)
        h265_hw = true;
    }
    // VP8 encoding will always use SW impl.
    if (absl::EqualsIgnoreCase(format.name, cricket::kVp8CodecName) && !vp8_hw)
      return webrtc::VP8Encoder::Create();
    // VP9 encoding will only be enabled on ICL+;
    else if (absl::EqualsIgnoreCase(format.name, cricket::kVp9CodecName) &&
             !vp9_hw)
      return webrtc::VP9Encoder::Create(cricket::VideoCodec(format));
    // TODO: Replace with AV1 HW encoder post ADL.
    else if (absl::EqualsIgnoreCase(format.name, cricket::kAv1CodecName) &&
             !av1_hw)
      return webrtc::CreateLibaomAv1Encoder();
    else if (absl::EqualsIgnoreCase(format.name, cricket::kH265CodecName) &&
             !h265_hw) {
    } else if (absl::EqualsIgnoreCase(format.name, cricket::kH264CodecName) &&
               !h264_hw) {
      RTC_LOG(LS_APP)
          << "----- "
          << "not support hardware H264 encoder, fallback to software encoder";
      return webrtc::H264Encoder::Create(cricket::VideoCodec(format));
    }

    if (is_screen_cast_) {
      RTC_LOG(LS_APP) << "----- using hardware encoder screencast";

      is_screen_cast_ = false;
      return owt::base::MSDKVideoEncoder::Create(cricket::VideoCodec(format),
                                                 "", MFX_TARGETUSAGE_BEST_SPEED,
                                                 MFX_RATECONTROL_CBR);
    } else {
      RTC_LOG(LS_APP) << "----- using hardware encoder";

      return owt::base::MSDKVideoEncoder::Create(cricket::VideoCodec(format));
    }
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