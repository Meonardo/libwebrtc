#include "rtc_audio_source_impl.h"

namespace libwebrtc {

RTCAudioSourceImpl::RTCAudioSourceImpl(
    rtc::scoped_refptr<webrtc::AudioSourceInterface> rtc_audio_source)
    : rtc_audio_source_(rtc_audio_source), custom_audio_source_(nullptr) {
  RTC_LOG(LS_INFO) << __FUNCTION__ << ": ctor ";
}

RTCAudioSourceImpl::RTCAudioSourceImpl(
    rtc::scoped_refptr<RTCCustomAudioSource> audio_source)
    : rtc_audio_source_(nullptr), custom_audio_source_(audio_source) {
  RTC_LOG(LS_INFO) << __FUNCTION__ << ": ctor(Customized audio source) ";
}

RTCAudioSourceImpl::~RTCAudioSourceImpl() {
  RTC_LOG(LS_INFO) << __FUNCTION__ << ": dtor ";
}

rtc::scoped_refptr<RTCCustomAudioSource> RTCCustomAudioSource::Create() {
  rtc::scoped_refptr<RTCCustomAudioSource> source(
      new rtc::RefCountedObject<RTCCustomAudioSource>());
  source->Initialize();
  return source;
}

void RTCCustomAudioSource::AddSink(webrtc::AudioTrackSinkInterface* sink) {
  if (nullptr != sink_) {
    RTC_LOG(LS_INFO) << "Replacing audio sink";
  }

  sink_ = sink;
}

void RTCCustomAudioSource::RemoveSink(webrtc::AudioTrackSinkInterface* sink) {
  if (sink_ != sink) {
    RTC_LOG(LS_INFO) << "Attempting to remove unassigned sink";
    return;
  }

  sink_ = nullptr;
}

void RTCCustomAudioSource::OnAudioData(uint8_t* data,
                                       int64_t timestamp,
                                       size_t frames,
                                       uint32_t sample_rate,
                                       size_t num_channels) {
  webrtc::AudioTrackSinkInterface* sink = this->sink_;
  if (nullptr == sink) {
    return;
  }

  size_t chunk = (sample_rate / 100);
  size_t sample_size = 2;  // WebRTC only support 16bit sample
  size_t i = 0;
  uint8_t* position;

  const int64_t obs_timestamp_us = timestamp / rtc::kNumNanosecsPerMicrosec;

  // Align timestamps from audio capturer(OBS) with rtc::TimeMicros timebase
  const int64_t aligned_timestamp_us = timestamp_aligner_.TranslateTimestamp(
      obs_timestamp_us, rtc::TimeMicros());

  if (pending_remainder) {
    // Copy missing chunks
    i = chunk - pending_remainder;
    memcpy(pending + pending_remainder * sample_size * num_channels, data,
           i * sample_size * num_channels);

    // Send
    sink->OnData(pending, 16, sample_rate, num_channels, chunk,
                 aligned_timestamp_us);

    // No pending chunks
    pending_remainder = 0;
  }

  while (i + chunk < frames) {
    position = data + i * sample_size * num_channels;
    sink->OnData(position, 16, sample_rate, num_channels, chunk,
                 aligned_timestamp_us);
    i += chunk;
  }

  if (i != frames) {
    pending_remainder = frames - static_cast<uint16_t>(i);
    memcpy(pending, data + i * sample_size * num_channels,
           pending_remainder * sample_size * num_channels);
  }
}

RTCCustomAudioSource::RTCCustomAudioSource() : sink_(nullptr) {}

RTCCustomAudioSource::~RTCCustomAudioSource() {
  free(pending);
  pending = nullptr;
}

void RTCCustomAudioSource::Initialize() {
  size_t num_channels = 2;
  size_t pending_len = num_channels * 2 * 640;
  pending = (uint8_t*)malloc(pending_len);
  pending_remainder = 0;
}

}  // namespace libwebrtc
