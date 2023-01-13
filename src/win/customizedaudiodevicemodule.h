// Copyright (C) <2018> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0
#ifndef OWT_BASE_CUSTOMIZEDAUDIODEVICEMODULE_H_
#define OWT_BASE_CUSTOMIZEDAUDIODEVICEMODULE_H_

#include <memory>
#include "api/scoped_refptr.h"
#include "api/task_queue/default_task_queue_factory.h"
#include "framegeneratorinterface.h"
#include "modules/audio_device/audio_device_generic.h"
#include "modules/audio_device/include/audio_device.h"
#include "rtc_base/synchronization/mutex.h"

namespace owt {
namespace base {
using namespace webrtc;
/**
 @brief CustomizedADM is able to create customized audio device use customized
 audio input.
 @details CustomizedADM does not support audio output yet.
 */
class CustomizedAudioDeviceModule : public webrtc::AudioDeviceModule,
                                    public AudioFrameReceiverInterface {
 public:
  CustomizedAudioDeviceModule(
      std::shared_ptr<AudioFrameGeneratorInterface> frame_generator);
  virtual ~CustomizedAudioDeviceModule();
  // Factory methods (resource allocation/deallocation)
  static rtc::scoped_refptr<AudioDeviceModule> Create(
      std::shared_ptr<AudioFrameGeneratorInterface> frame_generator);
  // Retrieve the currently utilized audio layer
  int32_t ActiveAudioLayer(AudioLayer* audioLayer) const override;
  // Full-duplex transportation of PCM audio
  int32_t RegisterAudioCallback(AudioTransport* audioCallback) override;
  // Main initialization and termination
  int32_t Init() override;
  int32_t Terminate() override;
  bool Initialized() const override;
  // Device enumeration
  int16_t PlayoutDevices() override;
  int16_t RecordingDevices() override;
  int32_t PlayoutDeviceName(uint16_t index,
                            char name[kAdmMaxDeviceNameSize],
                            char guid[kAdmMaxGuidSize]) override;
  int32_t RecordingDeviceName(uint16_t index,
                              char name[kAdmMaxDeviceNameSize],
                              char guid[kAdmMaxGuidSize]) override;
  // Device selection
  int32_t SetPlayoutDevice(uint16_t index) override;
  int32_t SetPlayoutDevice(WindowsDeviceType device) override;
  int32_t SetRecordingDevice(uint16_t index) override;
  int32_t SetRecordingDevice(WindowsDeviceType device) override;
  // Audio transport initialization
  int32_t PlayoutIsAvailable(bool* available) override;
  int32_t InitPlayout() override;
  bool PlayoutIsInitialized() const override;
  int32_t RecordingIsAvailable(bool* available) override;
  int32_t InitRecording() override;
  bool RecordingIsInitialized() const override;
  // Audio transport control
  int32_t StartPlayout() override;
  int32_t StopPlayout() override;
  bool Playing() const override;
  int32_t StartRecording() override;
  int32_t StopRecording() override;
  bool Recording() const override;
  // Audio mixer initialization
  int32_t InitSpeaker() override;
  bool SpeakerIsInitialized() const override;
  int32_t InitMicrophone() override;
  bool MicrophoneIsInitialized() const override;
  // Speaker volume controls
  int32_t SpeakerVolumeIsAvailable(bool* available) override;
  int32_t SetSpeakerVolume(uint32_t volume) override;
  int32_t SpeakerVolume(uint32_t* volume) const override;
  int32_t MaxSpeakerVolume(uint32_t* maxVolume) const override;
  int32_t MinSpeakerVolume(uint32_t* minVolume) const override;
  // Microphone volume controls
  int32_t MicrophoneVolumeIsAvailable(bool* available) override;
  int32_t SetMicrophoneVolume(uint32_t volume) override;
  int32_t MicrophoneVolume(uint32_t* volume) const override;
  int32_t MaxMicrophoneVolume(uint32_t* maxVolume) const override;
  int32_t MinMicrophoneVolume(uint32_t* minVolume) const override;
  // Speaker mute control
  int32_t SpeakerMuteIsAvailable(bool* available) override;
  int32_t SetSpeakerMute(bool enable) override;
  int32_t SpeakerMute(bool* enabled) const override;
  // Microphone mute control
  int32_t MicrophoneMuteIsAvailable(bool* available) override;
  int32_t SetMicrophoneMute(bool enable) override;
  int32_t MicrophoneMute(bool* enabled) const override;
  // Stereo support
  int32_t StereoPlayoutIsAvailable(bool* available) const override;
  int32_t SetStereoPlayout(bool enable) override;
  int32_t StereoPlayout(bool* enabled) const override;
  int32_t StereoRecordingIsAvailable(bool* available) const override;
  int32_t SetStereoRecording(bool enable) override;
  int32_t StereoRecording(bool* enabled) const override;
  // Delay information and control
  int32_t PlayoutDelay(uint16_t* delayMS) const override;
  // Only supported on Android.
  bool BuiltInAECIsAvailable() const override;
  bool BuiltInAGCIsAvailable() const override;
  bool BuiltInNSIsAvailable() const override;
  // Enables the built-in audio effects. Only supported on Android.
  int32_t EnableBuiltInAEC(bool enable) override;
  int32_t EnableBuiltInAGC(bool enable) override;
  int32_t EnableBuiltInNS(bool enable) override;

  int32_t SetAudioDeviceSink(AudioDeviceSink* sink) const override;

  void OnFrame(const uint8_t* data, size_t samples_per_channel) override;

 private:
  void CreateOutputAdm();
  webrtc::Mutex crit_sect_;
  webrtc::Mutex crit_sect_event_cb_;
  webrtc::Mutex crit_sect_audio_cb_;

  std::unique_ptr<webrtc::TaskQueueFactory> task_queue_factory_;
  int64_t last_process_time_;
  bool initialized_;
  // Default internal adm for playout.
  rtc::scoped_refptr<webrtc::AudioDeviceModule> output_adm_;

  // Audio transport
  std::shared_ptr<AudioFrameGeneratorInterface> frame_generator_;
  AudioTransport* audio_transport_;
  uint8_t pending_[640 * 2 * 2];
  size_t pending_length_;
  bool recording_ = false;
};
}  // namespace base
}  // namespace owt
#endif  // OWT_BASE_CUSTOMIZEDAUDIODEVICEMODULE_H_
