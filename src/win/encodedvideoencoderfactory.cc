// Copyright (C) <2018> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "src/win/encodedvideoencoderfactory.h"
#include "absl/strings/match.h"
#include "api/video_codecs/h264_profile_level_id.h"
#include "modules/video_coding/codecs/h264/include/h264.h"
#include "modules/video_coding/codecs/vp8/include/vp8.h"
#include "modules/video_coding/codecs/vp9/include/vp9.h"
#include "rtc_base/string_utils.h"
#include "src/win/codecutils.h"
#include "src/win/customizedvideoencoderproxy.h"

namespace owt {
namespace base {

EncodedVideoEncoderFactory::EncodedVideoEncoderFactory() {}

std::unique_ptr<webrtc::VideoEncoder>
EncodedVideoEncoderFactory::CreateVideoEncoder(
    const webrtc::SdpVideoFormat& format) {
  if (absl::EqualsIgnoreCase(format.name, cricket::kVp8CodecName) ||
      absl::EqualsIgnoreCase(format.name, cricket::kVp9CodecName) ||
      absl::EqualsIgnoreCase(format.name, cricket::kH264CodecName) ||
      absl::EqualsIgnoreCase(format.name, cricket::kH265CodecName)) {
    return CustomizedVideoEncoderProxy::Create();
  }
  return nullptr;
}

std::vector<webrtc::SdpVideoFormat>
EncodedVideoEncoderFactory::GetSupportedFormats() const {
  std::vector<webrtc::SdpVideoFormat> supported_codecs;
  // supported_codecs.push_back(webrtc::SdpVideoFormat(cricket::kVp8CodecName));
  // for (const webrtc::SdpVideoFormat& format : webrtc::SupportedVP9Codecs())
  //   supported_codecs.push_back(format);
  //// TODO: We should combine the codec profiles that hardware H.264 encoder
  //// supports with those provided by built-in H.264 encoder
  // for (const webrtc::SdpVideoFormat& format :
  //      owt::base::CodecUtils::SupportedH264Codecs())
  //   supported_codecs.push_back(format);
  // for (const webrtc::SdpVideoFormat& format :
  //      CodecUtils::GetSupportedH265Codecs()) {
  //   supported_codecs.push_back(format);
  // }

  // NOTICE: only use h264 codec for now
  supported_codecs.push_back(
      owt::base::CodecUtils::GetConstrainedBaselineH264Codec());

  return supported_codecs;
}

}  // namespace base
}  // namespace owt
