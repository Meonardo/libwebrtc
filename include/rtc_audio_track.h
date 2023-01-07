#ifndef LIB_WEBRTC_RTC_AUDIO_TRACK_HXX
#define LIB_WEBRTC_RTC_AUDIO_TRACK_HXX

#include "rtc_audio_source.h"
#include "rtc_media_track.h"
#include "rtc_types.h"

namespace libwebrtc {

class RTCAudioTrack : public RTCMediaTrack {
 public:
  virtual scoped_refptr<RTCAudioSource> Source() = 0;

 protected:
  virtual ~RTCAudioTrack() {}
};
}  // namespace libwebrtc

#endif  // LIB_WEBRTC_RTC_AUDIO_TRACK_HXX
