#include "rtc_audio_device_impl.h"
#include "modules/audio_device/win/core_audio_utility_win.h"
#include "rtc_base/logging.h"

namespace libwebrtc {

AudioDeviceImpl::AudioDeviceImpl(
    rtc::scoped_refptr<webrtc::AudioDeviceModule> audio_device_module,
    rtc::Thread* worker_thread)
    : audio_device_module_(audio_device_module), worker_thread_(worker_thread) {
  audio_device_module_->SetAudioDeviceSink(this);
}

AudioDeviceImpl::~AudioDeviceImpl() {
  RTC_LOG(INFO) << __FUNCTION__ << ": dtor ";
}

int16_t AudioDeviceImpl::PlayoutDevices() {
  return worker_thread_->Invoke<int32_t>(RTC_FROM_HERE, [&] {
    RTC_DCHECK_RUN_ON(worker_thread_);
    return audio_device_module_->PlayoutDevices();
  });
}

int16_t AudioDeviceImpl::RecordingDevices() {
  return worker_thread_->Invoke<int32_t>(RTC_FROM_HERE, [&] {
    RTC_DCHECK_RUN_ON(worker_thread_);
    return audio_device_module_->RecordingDevices();
  });
}

int32_t AudioDeviceImpl::PlayoutDeviceName(uint16_t index,
                                           char name[kAdmMaxDeviceNameSize],
                                           char guid[kAdmMaxGuidSize]) {
  return worker_thread_->Invoke<int32_t>(RTC_FROM_HERE, [&] {
    RTC_DCHECK_RUN_ON(worker_thread_);
    return audio_device_module_->PlayoutDeviceName(index, name, guid);
  });
}

int32_t AudioDeviceImpl::RecordingDeviceName(uint16_t index,
                                             char name[kAdmMaxDeviceNameSize],
                                             char guid[kAdmMaxGuidSize]) {
  return worker_thread_->Invoke<int32_t>(RTC_FROM_HERE, [&] {
    RTC_DCHECK_RUN_ON(worker_thread_);
    return audio_device_module_->RecordingDeviceName(index, name, guid);
  });
}

int32_t AudioDeviceImpl::SetPlayoutDevice(uint16_t index) {
  return worker_thread_->Invoke<int32_t>(RTC_FROM_HERE, [&] {
    RTC_DCHECK_RUN_ON(worker_thread_);
    if (audio_device_module_->Playing()) {
      audio_device_module_->StopPlayout();
      audio_device_module_->SetPlayoutDevice(index);
      audio_device_module_->InitPlayout();
      return audio_device_module_->StartPlayout();
    } else {
      return audio_device_module_->SetPlayoutDevice(index);
    }
  });
}

int32_t AudioDeviceImpl::SetRecordingDevice(uint16_t index) {
  return worker_thread_->Invoke<int32_t>(RTC_FROM_HERE, [&] {
    RTC_DCHECK_RUN_ON(worker_thread_);
    if (audio_device_module_->Recording()) {
      audio_device_module_->StopRecording();
      audio_device_module_->SetRecordingDevice(index);
      audio_device_module_->InitRecording();
      return audio_device_module_->StartRecording();
    } else {
      return audio_device_module_->SetRecordingDevice(index);
    }
  });
}

int AudioDeviceImpl::RestartPlayoutDevice() {
  return worker_thread_->Invoke<int16_t>(RTC_FROM_HERE, [&] {
    RTC_DCHECK_RUN_ON(worker_thread_);
    auto adm = reinterpret_cast<webrtc::AudioDeviceModuleForTest*>(
        audio_device_module_.get());
    if (adm != nullptr)
      return adm->RestartPlayoutInternally();
    return -1;
  });
}

bool AudioDeviceImpl::RecordingIsInitialized() const {
  return worker_thread_->Invoke<bool>(RTC_FROM_HERE, [&] {
    RTC_DCHECK_RUN_ON(worker_thread_);

    return audio_device_module_->RecordingIsInitialized();
  });
}

bool AudioDeviceImpl::RestartRecorder() const {
  return worker_thread_->Invoke<bool>(RTC_FROM_HERE, [&] {
    RTC_DCHECK_RUN_ON(worker_thread_);

    if (!audio_device_module_->Recording()) {
      if (audio_device_module_->InitRecording() == 0) {
#if defined(WEBRTC_WIN)
        if (audio_device_module_->BuiltInAECIsAvailable() &&
            !audio_device_module_->Playing()) {
          if (!audio_device_module_->PlayoutIsInitialized()) {
            audio_device_module_->InitPlayout();
          }
          audio_device_module_->StartPlayout();
        }
#endif
        audio_device_module_->StartRecording();
      } else {
        RTC_DLOG_F(LS_ERROR) << "Failed to initialize recording.";
        return false;
      }
    }
    return audio_device_module_->StartRecording() == 0;
  });
}

bool AudioDeviceImpl::ToggleRecordingMute(bool mute) {
  if (mute) {
    return worker_thread_->Invoke<bool>(RTC_FROM_HERE, [&] {
      RTC_DCHECK_RUN_ON(worker_thread_);

      if (!audio_device_module_->Recording()) {
        return false;
      }
      return audio_device_module_->StopRecording() == 0;
    });
  } else {
    return RestartRecorder();
  }
}

void AudioDeviceImpl::OnDevicesUpdated() {}

void AudioDeviceImpl::OnDevicesChanged(AudioDeviceSink::EventType e,
                                       AudioDeviceSink::DeviceType t,
                                       const char* device_id) {
  if (listener_)
    listener_((RTCAudioDevice::EventType)e, (RTCAudioDevice::DeviceType)t,
              device_id);
}

int32_t AudioDeviceImpl::OnDeviceChange(OnDeviceChangeCallback listener) {
  listener_ = listener;
  return 0;
}

int32_t AudioDeviceImpl::MicrophoneVolumeIsAvailable(bool* available) {
  return audio_device_module_->MicrophoneVolumeIsAvailable(available);
}

int32_t AudioDeviceImpl::SetMicrophoneVolume(uint32_t volume) {
  return audio_device_module_->SetMicrophoneVolume(volume);
}

int32_t AudioDeviceImpl::MicrophoneVolume(uint32_t* volume) const {
  return audio_device_module_->MicrophoneVolume(volume);
}

int32_t AudioDeviceImpl::SpeakerVolumeIsAvailable(bool* available) {
  return audio_device_module_->SpeakerVolumeIsAvailable(available);
}

int32_t AudioDeviceImpl::SetSpeakerVolume(uint32_t volume) {
  return audio_device_module_->SetSpeakerVolume(volume);
}

int32_t AudioDeviceImpl::SpeakerVolume(uint32_t* volume) const {
  return audio_device_module_->SpeakerVolume(volume);
}

}  // namespace libwebrtc
