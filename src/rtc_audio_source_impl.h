#ifndef LIB_WEBRTC_AUDIO_SOURCE_IMPL_HXX
#define LIB_WEBRTC_AUDIO_SOURCE_IMPL_HXX

#include "rtc_audio_source.h"

#include "api/media_stream_interface.h"
#include "api/peer_connection_interface.h"
#include "common_audio/resampler/include/push_resampler.h"
#include "common_audio/vad/include/webrtc_vad.h"
#include "media/engine/webrtc_video_engine.h"
#include "media/engine/webrtc_voice_engine.h"
#include "pc/media_session.h"
#include "rtc_base/logging.h"
#include "rtc_base/synchronization/mutex.h"

#include "api/notifier.h"
#include "rtc_base/timestamp_aligner.h"

namespace libwebrtc {
/// custom audio source
class RTCCustomAudioSource
    : public webrtc::Notifier<webrtc::AudioSourceInterface> {
 public:
  static rtc::scoped_refptr<RTCCustomAudioSource> Create();

  SourceState state() const override { return kLive; }
  bool remote() const override { return false; }

  ~RTCCustomAudioSource();

  void AddSink(webrtc::AudioTrackSinkInterface* sink) override;
  void RemoveSink(webrtc::AudioTrackSinkInterface* sink) override;
  void OnAudioData(uint8_t* data,
                   int64_t timestamp,
                   size_t frames,
                   uint32_t sample_rate,
                   size_t num_channels);

 protected:
  uint16_t pending_remainder;
  uint8_t* pending;

  RTCCustomAudioSource();

 private:
  webrtc::AudioTrackSinkInterface* sink_;
  rtc::TimestampAligner timestamp_aligner_;

  void Initialize();
};

class RTCAudioSourceImpl : public RTCAudioSource {
 public:
  RTCAudioSourceImpl(
      rtc::scoped_refptr<webrtc::AudioSourceInterface> rtc_audio_source);
  RTCAudioSourceImpl(rtc::scoped_refptr<RTCCustomAudioSource> audio_source);

  virtual ~RTCAudioSourceImpl();

  virtual void SetVolume(double volume) override {
    auto audio_source = rtc_audio_source();
    audio_source->SetVolume(volume);
  }

  virtual void OnAudioData(uint8_t* data,
                           int64_t timestamp,
                           size_t frames,
                           uint32_t sample_rate,
                           size_t num_channels) override {
    if (custom_audio_source_ != nullptr) {
      custom_audio_source_->OnAudioData(data, timestamp, frames, sample_rate,
                                        num_channels);
    }
  }

  rtc::scoped_refptr<webrtc::AudioSourceInterface> rtc_audio_source() {
    return rtc_audio_source_;
  }

  rtc::scoped_refptr<webrtc::AudioSourceInterface> customized_audio_source() {
    return custom_audio_source_;
  }

 private:
  rtc::scoped_refptr<webrtc::AudioSourceInterface> rtc_audio_source_;
  rtc::scoped_refptr<RTCCustomAudioSource> custom_audio_source_;
};

}  // namespace libwebrtc

#endif  // LIB_WEBRTC_AUDIO_SOURCE_IMPL_HXX
