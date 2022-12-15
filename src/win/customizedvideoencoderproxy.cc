// Copyright (C) <2018> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include <string>
#include <vector>
#include "api/video/video_frame.h"
#include "modules/include/module_common_types.h"
#include "modules/video_coding/include/video_codec_interface.h"
#include "modules/video_coding/include/video_error_codes.h"
#include "rtc_base/buffer.h"
#include "rtc_base/checks.h"
#include "rtc_base/logging.h"

#include "commontypes.h"
#include "src/win/customizedencoderbufferhandle.h"
#include "src/win/customizedvideoencoderproxy.h"
#include "src/win/mediautils.h"
#include "src/win/nativehandlebuffer.h"
// H.264 start code length.
#define H264_SC_LENGTH 4
// Maximum allowed NALUs in one output frame.
#define MAX_NALUS_PERFRAME 32
using namespace rtc;
namespace owt {
namespace base {
CustomizedVideoEncoderProxy::CustomizedVideoEncoderProxy()
    : callback_(nullptr), external_encoder_(nullptr) {
  picture_id_ = 0;
}

CustomizedVideoEncoderProxy::~CustomizedVideoEncoderProxy() {
  if (external_encoder_) {
    delete external_encoder_;
    external_encoder_ = nullptr;
  }
}

int CustomizedVideoEncoderProxy::InitEncode(
    const webrtc::VideoCodec* codec_settings,
    int number_of_cores,
    size_t max_payload_size) {
  RTC_DCHECK(codec_settings);
  codec_type_ = codec_settings->codecType;
  width_ = codec_settings->width;
  height_ = codec_settings->height;
  bitrate_ = codec_settings->startBitrate * 1000;
  picture_id_ = static_cast<uint16_t>(rand()) & 0x7FFF;
  gof_.SetGofInfoVP9(TemporalStructureMode::kTemporalStructureMode1);
  gof_idx_ = 0;
  return WEBRTC_VIDEO_CODEC_OK;
}

int32_t CustomizedVideoEncoderProxy::Encode(
    const webrtc::VideoFrame& input_image,
    const std::vector<webrtc::VideoFrameType>* frame_types) {
  auto frame_buffer = reinterpret_cast<owt::base::EncodedFrameBuffer*>(
      input_image.video_frame_buffer().get());

  auto buffer_size = frame_buffer->buffer_size();
  std::vector<uint8_t> buffer;

  std::unique_ptr<uint8_t[]> data(new uint8_t[buffer_size]);
  uint8_t* data_ptr = data.get();
  memcpy(data_ptr, frame_buffer->buffer(), buffer_size);
  uint32_t data_size = static_cast<uint32_t>(buffer_size);

  webrtc::EncodedImage encodedframe;
  encodedframe.SetEncodedData(
      EncodedImageBuffer::Create(data_ptr, buffer_size));
  encodedframe._encodedWidth = frame_buffer->width();
  encodedframe._encodedHeight = frame_buffer->height();
  encodedframe.capture_time_ms_ = input_image.render_time_ms();
  encodedframe.SetTimestamp(input_image.timestamp());

  // VP9 requires setting the frame type according to actual frame type.
  if (codec_type_ == webrtc::kVideoCodecVP9 && data_size > 2) {
    uint8_t au_key = 1;
    uint8_t first_byte = data_ptr[0], second_byte = data_ptr[1];
    uint8_t shift_bits = 4, profile = (first_byte >> shift_bits) & 0x3;
    shift_bits = (profile == 3) ? 2 : 3;
    uint8_t show_existing_frame = (first_byte >> shift_bits) & 0x1;
    if (profile == 3 && show_existing_frame) {
      au_key = (second_byte >> 6) & 0x1;
    } else if (profile == 3 && !show_existing_frame) {
      au_key = (first_byte >> 1) & 0x1;
    } else if (profile != 3 && show_existing_frame) {
      au_key = second_byte >> 7;
    } else {
      au_key = (first_byte >> 2) & 0x1;
    }
    encodedframe._frameType = (au_key == 0)
                                  ? webrtc::VideoFrameType::kVideoFrameKey
                                  : webrtc::VideoFrameType::kVideoFrameDelta;
  }
  webrtc::CodecSpecificInfo info;
  memset(&info, 0, sizeof(info));
  info.codecType = codec_type_;
  if (codec_type_ == webrtc::kVideoCodecVP8) {
    info.codecSpecific.VP8.nonReference = false;
    info.codecSpecific.VP8.temporalIdx = webrtc::kNoTemporalIdx;
    info.codecSpecific.VP8.layerSync = false;
    info.codecSpecific.VP8.keyIdx = webrtc::kNoKeyIdx;
    picture_id_ = (picture_id_ + 1) & 0x7FFF;
  } else if (codec_type_ == webrtc::kVideoCodecVP9) {
    bool key_frame =
        encodedframe._frameType == webrtc::VideoFrameType::kVideoFrameKey;
    if (key_frame) {
      gof_idx_ = 0;
    }
    info.codecSpecific.VP9.inter_pic_predicted = key_frame ? false : true;
    info.codecSpecific.VP9.flexible_mode = false;
    info.codecSpecific.VP9.ss_data_available = key_frame ? true : false;
    info.codecSpecific.VP9.temporal_idx = kNoTemporalIdx;
    // info.codecSpecific.VP9.spatial_idx = kNoSpatialIdx;
    info.codecSpecific.VP9.temporal_up_switch = true;
    info.codecSpecific.VP9.inter_layer_predicted = false;
    info.codecSpecific.VP9.gof_idx =
        static_cast<uint8_t>(gof_idx_++ % gof_.num_frames_in_gof);
    info.codecSpecific.VP9.num_spatial_layers = 1;
    info.codecSpecific.VP9.first_frame_in_picture = true;
    // info.codecSpecific.VP9.end_of_picture = true;
    info.codecSpecific.VP9.spatial_layer_resolution_present = false;
    if (info.codecSpecific.VP9.ss_data_available) {
      info.codecSpecific.VP9.spatial_layer_resolution_present = true;
      info.codecSpecific.VP9.width[0] = width_;
      info.codecSpecific.VP9.height[0] = height_;
      info.codecSpecific.VP9.gof.CopyGofInfoVP9(gof_);
    }

  } else if (codec_type_ == webrtc::kVideoCodecH264) {
    int temporal_id = 0, priority_id = 0;
    bool is_idr = false;
    bool need_frame_marking = MediaUtils::GetH264TemporalInfo(
        data_ptr, data_size, temporal_id, priority_id, is_idr);
    if (need_frame_marking) {
      info.codecSpecific.H264.temporal_idx = temporal_id;
      info.codecSpecific.H264.idr_frame = is_idr;
      info.codecSpecific.H264.base_layer_sync = (!is_idr && (temporal_id > 0));
    }
    encodedframe._frameType = is_idr ? webrtc::VideoFrameType::kVideoFrameKey
                                     : webrtc::VideoFrameType::kVideoFrameDelta;
  }
  const auto result = callback_->OnEncodedImage(encodedframe, &info);
  if (result.error != webrtc::EncodedImageCallback::Result::Error::OK) {
    RTC_LOG(LS_ERROR) << "Deliver encoded frame callback failed: "
                      << result.error;
    return WEBRTC_VIDEO_CODEC_ERROR;
  }
  return WEBRTC_VIDEO_CODEC_OK;
}

int CustomizedVideoEncoderProxy::RegisterEncodeCompleteCallback(
    webrtc::EncodedImageCallback* callback) {
  callback_ = callback;
  return WEBRTC_VIDEO_CODEC_OK;
}

void CustomizedVideoEncoderProxy::SetRates(
    const RateControlParameters& parameters) {
  if (parameters.framerate_fps < 1.0) {
    RTC_LOG(LS_WARNING) << "Unsupported framerate (must be >= 1.0";
    return;
  }
}

void CustomizedVideoEncoderProxy::OnPacketLossRateUpdate(
    float packet_loss_rate) {
  // Currently not handled.
  return;
}

void CustomizedVideoEncoderProxy::OnRttUpdate(int64_t rtt_ms) {
  // Currently not handled.
  return;
}

void CustomizedVideoEncoderProxy::OnLossNotification(
    const LossNotification& loss_notification) {
  // Currently not handled.
}

int CustomizedVideoEncoderProxy::Release() {
  callback_ = nullptr;
  if (external_encoder_ != nullptr) {
    external_encoder_->Release();
  }
  return WEBRTC_VIDEO_CODEC_OK;
}

int32_t CustomizedVideoEncoderProxy::NextNaluPosition(uint8_t* buffer,
                                                      size_t buffer_size,
                                                      size_t* sc_length) {
  if (buffer_size < H264_SC_LENGTH) {
    return -1;
  }
  uint8_t* head = buffer;
  // Set end buffer pointer to 4 bytes before actual buffer end so we can
  // access head[1], head[2] and head[3] in a loop without buffer overrun.
  uint8_t* end = buffer + buffer_size - H264_SC_LENGTH;
  while (head < end) {
    if (head[0]) {
      head++;
      continue;
    }
    if (head[1]) {  // got 00xx
      head += 2;
      continue;
    }
    if (head[2]) {  // got 0000xx
      if (head[2] == 0x01) {
        *sc_length = 3;
        return (int32_t)(head - buffer);
      }
      head += 3;
      continue;
    }
    if (head[3] != 0x01) {  // got 000000xx
      head++;               // xx != 1, continue searching.
      continue;
    }
    *sc_length = 4;
    return (int32_t)(head - buffer);
  }
  return -1;
}

webrtc::VideoEncoder::EncoderInfo CustomizedVideoEncoderProxy::GetEncoderInfo()
    const {
  EncoderInfo info;
  info.supports_native_handle = true;
  info.is_hardware_accelerated = false;
  info.has_internal_source = false;
  info.implementation_name = "OWTPassthroughEncoder";
  info.has_trusted_rate_controller = false;
  info.scaling_settings = VideoEncoder::ScalingSettings::kOff;
  return info;
}

std::unique_ptr<CustomizedVideoEncoderProxy>
CustomizedVideoEncoderProxy::Create() {
  return absl::make_unique<CustomizedVideoEncoderProxy>();
}

}  // namespace base
}  // namespace owt
