/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "src/win/customizedaudiodevicemodule.h"
#include "common_audio/signal_processing/include/signal_processing_library.h"
#include "modules/audio_device/audio_device_config.h"
#include "modules/audio_device/audio_device_impl.h"
#include "modules/audio_device/include/fake_audio_device.h"
#include "rtc_base/logging.h"
#include "rtc_base/ref_counted_object.h"
#include "rtc_base/time_utils.h"

// Code partly borrowed from WebRTC project's audio device moudule
// implementation.
#define CHECK_INITIALIZED() \
  {                         \
    if (!initialized_) {    \
      return -1;            \
    };                      \
  }
#define CHECK_INITIALIZED_BOOL() \
  {                              \
    if (!initialized_) {         \
      return false;              \
    };                           \
  }

namespace owt {
namespace base {

rtc::scoped_refptr<AudioDeviceModule> CustomizedAudioDeviceModule::Create(
    std::shared_ptr<AudioFrameGeneratorInterface> frame_generator) {
  // Create the generic ref counted implementation.
  rtc::scoped_refptr<CustomizedAudioDeviceModule> audioDevice(
      rtc::make_ref_counted<CustomizedAudioDeviceModule>(frame_generator));

  return audioDevice;
}

CustomizedAudioDeviceModule::CustomizedAudioDeviceModule(
    std::shared_ptr<AudioFrameGeneratorInterface> frame_generator)
    : last_process_time_(rtc::TimeMillis()),
      initialized_(false),
      frame_generator_(frame_generator),
      audio_transport_(nullptr),
      pending_length_(0) {
  task_queue_factory_ = webrtc::CreateDefaultTaskQueueFactory();
  CreateOutputAdm();
}

CustomizedAudioDeviceModule::~CustomizedAudioDeviceModule() {}

int32_t CustomizedAudioDeviceModule::ActiveAudioLayer(
    AudioLayer* audioLayer) const {
  return 0;
}

int32_t CustomizedAudioDeviceModule::Init() {
  if (initialized_)
    return 0;
  if (!output_adm_ || output_adm_->Init() == -1)
    return -1;
  initialized_ = true;
  return 0;
}

int32_t CustomizedAudioDeviceModule::Terminate() {
  if (!initialized_)
    return 0;
  if (!output_adm_ || output_adm_->Terminate() == -1)
    return -1;
  initialized_ = false;
  return 0;
}

bool CustomizedAudioDeviceModule::Initialized() const {
  return (initialized_ && output_adm_->Initialized());
}

int32_t CustomizedAudioDeviceModule::InitSpeaker() {
  return (output_adm_->InitSpeaker());
}

int32_t CustomizedAudioDeviceModule::InitMicrophone() {
  CHECK_INITIALIZED();
  return 0;
}

int32_t CustomizedAudioDeviceModule::SpeakerVolumeIsAvailable(bool* available) {
  return output_adm_->SpeakerVolumeIsAvailable(available);
}

int32_t CustomizedAudioDeviceModule::SetSpeakerVolume(uint32_t volume) {
  return (output_adm_->SetSpeakerVolume(volume));
}

int32_t CustomizedAudioDeviceModule::SpeakerVolume(uint32_t* volume) const {
  return output_adm_->SpeakerVolume(volume);
}

bool CustomizedAudioDeviceModule::SpeakerIsInitialized() const {
  return output_adm_->SpeakerIsInitialized();
}

bool CustomizedAudioDeviceModule::MicrophoneIsInitialized() const {
  CHECK_INITIALIZED_BOOL();
  return false;
}

int32_t CustomizedAudioDeviceModule::MaxSpeakerVolume(
    uint32_t* maxVolume) const {
  return output_adm_->MaxSpeakerVolume(maxVolume);
}

int32_t CustomizedAudioDeviceModule::MinSpeakerVolume(
    uint32_t* minVolume) const {
  return output_adm_->MinSpeakerVolume(minVolume);
}

int32_t CustomizedAudioDeviceModule::SpeakerMuteIsAvailable(bool* available) {
  return output_adm_->SpeakerMuteIsAvailable(available);
}

int32_t CustomizedAudioDeviceModule::SetSpeakerMute(bool enable) {
  return output_adm_->SetSpeakerMute(enable);
}

int32_t CustomizedAudioDeviceModule::SpeakerMute(bool* enabled) const {
  return output_adm_->SpeakerMute(enabled);
}

int32_t CustomizedAudioDeviceModule::MicrophoneMuteIsAvailable(
    bool* available) {
  CHECK_INITIALIZED()
  return -1;
}

int32_t CustomizedAudioDeviceModule::SetMicrophoneMute(bool enable) {
  CHECK_INITIALIZED();
  return -1;
}

int32_t CustomizedAudioDeviceModule::MicrophoneMute(bool* enabled) const {
  CHECK_INITIALIZED();
  return -1;
}

int32_t CustomizedAudioDeviceModule::MicrophoneVolumeIsAvailable(
    bool* available) {
  CHECK_INITIALIZED();
  return -1;
}

int32_t CustomizedAudioDeviceModule::SetMicrophoneVolume(uint32_t volume) {
  CHECK_INITIALIZED();
  return -1;
}

int32_t CustomizedAudioDeviceModule::MicrophoneVolume(uint32_t* volume) const {
  CHECK_INITIALIZED();
  return -1;
}

int32_t CustomizedAudioDeviceModule::StereoRecordingIsAvailable(
    bool* available) const {
  CHECK_INITIALIZED();
  return 0;
}

int32_t CustomizedAudioDeviceModule::SetStereoRecording(bool enable) {
  CHECK_INITIALIZED();
  return 0;
}

int32_t CustomizedAudioDeviceModule::StereoRecording(bool* enabled) const {
  CHECK_INITIALIZED();
  return 0;
}

int32_t CustomizedAudioDeviceModule::StereoPlayoutIsAvailable(
    bool* available) const {
  return output_adm_->StereoPlayoutIsAvailable(available);
}

int32_t CustomizedAudioDeviceModule::SetStereoPlayout(bool enable) {
  return output_adm_->SetStereoPlayout(enable);
}

int32_t CustomizedAudioDeviceModule::StereoPlayout(bool* enabled) const {
  return output_adm_->StereoPlayout(enabled);
}

int32_t CustomizedAudioDeviceModule::PlayoutIsAvailable(bool* available) {
  return output_adm_->PlayoutIsAvailable(available);
}

int32_t CustomizedAudioDeviceModule::RecordingIsAvailable(bool* available) {
  CHECK_INITIALIZED();
  return frame_generator_ != nullptr;
}

int32_t CustomizedAudioDeviceModule::MaxMicrophoneVolume(
    uint32_t* maxVolume) const {
  CHECK_INITIALIZED();
  return -1;
}

int32_t CustomizedAudioDeviceModule::MinMicrophoneVolume(
    uint32_t* minVolume) const {
  CHECK_INITIALIZED();
  return -1;
}

int16_t CustomizedAudioDeviceModule::PlayoutDevices() {
  return output_adm_->PlayoutDevices();
}

int32_t CustomizedAudioDeviceModule::SetPlayoutDevice(uint16_t index) {
  return output_adm_->SetPlayoutDevice(index);
}

int32_t CustomizedAudioDeviceModule::SetPlayoutDevice(
    WindowsDeviceType device) {
  return output_adm_->SetPlayoutDevice(device);
}

int32_t CustomizedAudioDeviceModule::PlayoutDeviceName(
    uint16_t index,
    char name[kAdmMaxDeviceNameSize],
    char guid[kAdmMaxGuidSize]) {
  return output_adm_->PlayoutDeviceName(index, name, guid);
}

int32_t CustomizedAudioDeviceModule::RecordingDeviceName(
    uint16_t index,
    char name[kAdmMaxDeviceNameSize],
    char guid[kAdmMaxGuidSize]) {
  CHECK_INITIALIZED();
  const char* kName = "customized_audio_device";
  const char* kGuid = "customized_audio_device_unique_id";
  if (index < 1) {
    memset(name, 0, kAdmMaxDeviceNameSize);
    memset(guid, 0, kAdmMaxGuidSize);
    memcpy(name, kName, strlen(kName));
    memcpy(guid, kGuid, strlen(guid));
    return 0;
  }
  return -1;
}

int16_t CustomizedAudioDeviceModule::RecordingDevices() {
  CHECK_INITIALIZED();
  return 1;
}

int32_t CustomizedAudioDeviceModule::SetRecordingDevice(uint16_t index) {
  CHECK_INITIALIZED();
  return -1;
}

int32_t CustomizedAudioDeviceModule::SetRecordingDevice(
    WindowsDeviceType device) {
  CHECK_INITIALIZED();
  return -1;
}

int32_t CustomizedAudioDeviceModule::InitPlayout() {
  return output_adm_->InitPlayout();
}

int32_t CustomizedAudioDeviceModule::InitRecording() {
  CHECK_INITIALIZED();
  return 0;
}

bool CustomizedAudioDeviceModule::PlayoutIsInitialized() const {
  return output_adm_->PlayoutIsInitialized();
}

bool CustomizedAudioDeviceModule::RecordingIsInitialized() const {
  CHECK_INITIALIZED_BOOL();
  return true;
}

int32_t CustomizedAudioDeviceModule::StartPlayout() {
  return (output_adm_->StartPlayout());
}

int32_t CustomizedAudioDeviceModule::StopPlayout() {
  return (output_adm_->StopPlayout());
}

bool CustomizedAudioDeviceModule::Playing() const {
  return (output_adm_->Playing());
}

int32_t CustomizedAudioDeviceModule::StartRecording() {
  CHECK_INITIALIZED();
  webrtc::MutexLock lock(&crit_sect_);
  frame_generator_->SetAudioFrameReceiver(this);
  recording_ = true;
  return 0;
}

int32_t CustomizedAudioDeviceModule::StopRecording() {
  CHECK_INITIALIZED();
  recording_ = false;
  webrtc::MutexLock lock(&crit_sect_);
  frame_generator_->SetAudioFrameReceiver(nullptr);
  return 0;
}

bool CustomizedAudioDeviceModule::Recording() const {
  CHECK_INITIALIZED_BOOL();
  return recording_;
}

int32_t CustomizedAudioDeviceModule::RegisterAudioCallback(
    AudioTransport* audioCallback) {
  webrtc::MutexLock lock(&crit_sect_audio_cb_);
  audio_transport_ = audioCallback;
  return output_adm_->RegisterAudioCallback(audioCallback);
}

int32_t CustomizedAudioDeviceModule::PlayoutDelay(uint16_t* delayMS) const {
  return output_adm_->PlayoutDelay(delayMS);
}

bool CustomizedAudioDeviceModule::BuiltInAECIsAvailable() const {
  CHECK_INITIALIZED_BOOL();
  return false;
}

int32_t CustomizedAudioDeviceModule::EnableBuiltInAEC(bool enable) {
  CHECK_INITIALIZED();
  return -1;
}

bool CustomizedAudioDeviceModule::BuiltInAGCIsAvailable() const {
  CHECK_INITIALIZED_BOOL();
  return false;
}

int32_t CustomizedAudioDeviceModule::EnableBuiltInAGC(bool enable) {
  return -1;
}

bool CustomizedAudioDeviceModule::BuiltInNSIsAvailable() const {
  CHECK_INITIALIZED_BOOL();
  return false;
}

int32_t CustomizedAudioDeviceModule::EnableBuiltInNS(bool enable) {
  CHECK_INITIALIZED();
  return -1;
}

int32_t CustomizedAudioDeviceModule::SetAudioDeviceSink(
    AudioDeviceSink* sink) const {
  return -1;
}

void CustomizedAudioDeviceModule::CreateOutputAdm() {
  if (output_adm_ == nullptr) {
#if defined(WEBRTC_INCLUDE_INTERNAL_AUDIO_DEVICE)
    output_adm_ = webrtc::AudioDeviceModuleImpl::Create(
        AudioDeviceModule::kPlatformDefaultAudio, task_queue_factory_.get());
#else
    output_adm_ = rtc::scoped_refptr<webrtc::FakeAudioDeviceModule>(
        rtc::make_ref_counted<webrtc::FakeAudioDeviceModule>());
#endif
  }
}

void CustomizedAudioDeviceModule::OnFrame(const uint8_t* data,
                                          size_t samples_per_channel) {
  {
    webrtc::MutexLock lock(&crit_sect_);
    if (!recording_ || !audio_transport_) {
      RTC_LOG(LS_WARNING) << "Recording not started yet...";
      return;
    }
  }

  // This info is set on the stream before starting capture
  size_t channels = frame_generator_->GetChannelNumber();
  uint32_t sample_rate = frame_generator_->GetSampleRate();
  size_t sample_size = 2;
  // Get chunk for 10ms
  size_t chunk = (sample_rate / 100);

  size_t i = 0;
  uint32_t level;

  // Check if we had pending chunks
  if (pending_length_) {
    // Copy missing chunks
    i = chunk - pending_length_;
    memcpy(pending_ + pending_length_ * sample_size * channels, data,
           i * sample_size * channels);
    // Add sent
    audio_transport_->RecordedDataIsAvailable(
        pending_, chunk, sample_size * channels, channels, sample_rate, 0, 0, 0,
        false, level);
    // No pending chunks
    pending_length_ = 0;
  }

  // Send all possible full chunks
  while (i + chunk < samples_per_channel) {
    // Send them
    audio_transport_->RecordedDataIsAvailable(
        data + i * sample_size * channels, chunk, sample_size * channels,
        channels, sample_rate, 0, 0, 0, false, level);
    i += chunk;
  }

  // If there are missing chunks
  if (i != samples_per_channel) {
    // Calculate pending chunks
    pending_length_ = samples_per_channel - i;
    // Copy chunks to pending buffer
    memcpy(pending_, data + i * sample_size * channels,
           pending_length_ * sample_size * channels);
  }
}

}  // namespace base
}  // namespace owt
