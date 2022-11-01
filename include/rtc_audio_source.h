#ifndef LIB_WEBRTC_RTC_AUDIO_SOURCE_HXX
#define LIB_WEBRTC_RTC_AUDIO_SOURCE_HXX

#include "rtc_types.h"

namespace libwebrtc {

class RTCAudioSource : public RefCountInterface {
 public:
  virtual void SetVolume(double v) = 0;

 protected:
  virtual ~RTCAudioSource() {}
};

}  // namespace libwebrtc

#endif  // LIB_WEBRTC_RTC_AUDIO_TRACK_HXX
