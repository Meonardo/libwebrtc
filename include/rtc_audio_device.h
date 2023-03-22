#ifndef LIB_WEBRTC_RTC_AUDIO_DEVICE_HXX
#define LIB_WEBRTC_RTC_AUDIO_DEVICE_HXX

#include "rtc_types.h"

namespace libwebrtc {

class RTCAudioDevice : public RefCountInterface {
 public:
  enum EventType {
    kUnknown = 0,
    kAdded,
    kRemoved,
    kDefaultChanged,
    kStateChanged
  };
  enum DeviceType { kUndefined = 0, kCapture, kPlayout };
  typedef fixed_size_function<
      void(EventType e, DeviceType t, const char* device_id)>
      OnDeviceChangeCallback;

 public:
  static const int kAdmMaxDeviceNameSize = 128;
  static const int kAdmMaxFileNameSize = 512;
  static const int kAdmMaxGuidSize = 128;

 public:
  // Device enumeration
  virtual int16_t PlayoutDevices() = 0;

  virtual int16_t RecordingDevices() = 0;

  virtual int32_t PlayoutDeviceName(uint16_t index,
                                    char name[kAdmMaxDeviceNameSize],
                                    char guid[kAdmMaxGuidSize]) = 0;

  virtual int32_t RecordingDeviceName(uint16_t index,
                                      char name[kAdmMaxDeviceNameSize],
                                      char guid[kAdmMaxGuidSize]) = 0;

  // Device selection
  virtual int32_t SetPlayoutDevice(uint16_t index) = 0;

  virtual int32_t SetRecordingDevice(uint16_t index) = 0;

  virtual int32_t OnDeviceChange(OnDeviceChangeCallback listener) = 0;

  // Windows only
  virtual int RestartPlayoutDevice() = 0;
  virtual bool RecordingIsInitialized() const = 0;
  virtual bool RestartRecorder() const = 0;
  virtual bool ToggleRecordingMute(bool mute) = 0;
  // Microphone volume controls
  virtual int32_t MicrophoneVolumeIsAvailable(bool* available) = 0;
  virtual int32_t SetMicrophoneVolume(uint32_t volume) = 0;
  virtual int32_t MicrophoneVolume(uint32_t* volume) const = 0;
  // Speaker volume controls
  virtual int32_t SpeakerVolumeIsAvailable(bool* available) = 0;
  virtual int32_t SetSpeakerVolume(uint32_t volume) = 0;
  virtual int32_t SpeakerVolume(uint32_t* volume) const = 0;

 protected:
  virtual ~RTCAudioDevice() {}
};

}  // namespace libwebrtc

#endif  // LIB_WEBRTC_RTC_AUDIO_DEVICE_HXX
