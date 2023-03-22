#ifndef LIB_WEBRTC_AUDIO_DEVICE_IMPL_HXX
#define LIB_WEBRTC_AUDIO_DEVICE_IMPL_HXX

#include "modules/audio_device/audio_device_impl.h"
#include "modules/audio_device/include/audio_device.h"
#include "rtc_audio_device.h"
#include "rtc_base/ref_count.h"
#include "rtc_base/thread.h"

namespace libwebrtc {
class AudioDeviceImpl : public RTCAudioDevice, public webrtc::AudioDeviceSink {
 public:
  AudioDeviceImpl(
      rtc::scoped_refptr<webrtc::AudioDeviceModule> audio_device_module,
      rtc::Thread* worker_thread);

  virtual ~AudioDeviceImpl();

 public:
  int16_t PlayoutDevices() override;

  int16_t RecordingDevices() override;

  int32_t PlayoutDeviceName(uint16_t index,
                            char name[kAdmMaxDeviceNameSize],
                            char guid[kAdmMaxGuidSize]) override;

  int32_t RecordingDeviceName(uint16_t index,
                              char name[kAdmMaxDeviceNameSize],
                              char guid[kAdmMaxGuidSize]) override;

  int32_t SetPlayoutDevice(uint16_t index) override;

  int32_t SetRecordingDevice(uint16_t index) override;

  int32_t OnDeviceChange(OnDeviceChangeCallback listener) override;

  // Windows only
  int RestartPlayoutDevice() override;
  bool RecordingIsInitialized() const override;
  bool RestartRecorder() const override;
  bool ToggleRecordingMute(bool mute) override;
  // Microphone volume controls
  virtual int32_t MicrophoneVolumeIsAvailable(bool* available) override;
  virtual int32_t SetMicrophoneVolume(uint32_t volume) override;
  virtual int32_t MicrophoneVolume(uint32_t* volume) const override;
  // Speaker volume controls
  virtual int32_t SpeakerVolumeIsAvailable(bool* available) override;
  virtual int32_t SetSpeakerVolume(uint32_t volume) override;
  virtual int32_t SpeakerVolume(uint32_t* volume) const override;

 protected:
  void OnDevicesUpdated() override;
  void OnDevicesChanged(AudioDeviceSink::EventType e,
                        AudioDeviceSink::DeviceType t,
                        const char* device_id) override;

 private:
  rtc::scoped_refptr<webrtc::AudioDeviceModule> audio_device_module_;
  rtc::Thread* worker_thread_ = nullptr;
  OnDeviceChangeCallback listener_ = nullptr;
};

}  // namespace libwebrtc

#endif  // LIB_WEBRTC_AUDIO_DEVICE_IMPL_HXX
