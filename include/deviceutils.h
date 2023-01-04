// Copyright (C) <2018> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0
#ifndef OWT_BASE_DEVICEUTILS_H_
#define OWT_BASE_DEVICEUTILS_H_
#include <deque>
#include <string>
#include <vector>
#include "commontypes.h"
#include "rtc_types.h"

namespace owt {
namespace base {

struct AudioDevice {
  std::string name;
  std::string unique_id;

  AudioDevice(const std::string& name, const std::string& id)
      : name(name), unique_id(id) {}
  ~AudioDevice() = default;
};

// Only works for Windows
enum DefaultDeviceType {
  kUndefined = -1,
  kDefault = 0,
  kDefaultCommunications = 1,
  kDefaultDeviceTypeMaxCount = kDefaultCommunications + 1,
};

class DeviceUtils {
 public:
  /// Get video capturer IDs.
  LIB_WEBRTC_API static std::vector<std::string> VideoCapturerIds();
  /**
   Get supported resolutions for a specific video capturer.
   @param id The ID of specific video capturer.
   @return Supported resolution for the specific video capturer. If the name is
   invalid, it returns an empty vector.
   */
  LIB_WEBRTC_API static std::vector<Resolution>
  VideoCapturerSupportedResolutions(const std::string& id);
  /// Get the camera device index by its device id.
  LIB_WEBRTC_API static int GetVideoCaptureDeviceIndex(const std::string& id);
  /// Get camera device's user friendly name by index.
  LIB_WEBRTC_API static std::string GetDeviceNameByIndex(int index);
  LIB_WEBRTC_API static std::vector<VideoTrackCapabilities>
  VideoCapturerSupportedCapabilities(const std::string& id);

  // Audio Device Enum Only Works for Windows Platform, please see the
  // definition.
  /// Get audio capturer IDs.
  LIB_WEBRTC_API static std::vector<AudioDevice> AudioCapturerDevices();
  LIB_WEBRTC_API static std::vector<AudioDevice> AudioPlaybackDevices();

  /// Get the playback & recording device id.
  LIB_WEBRTC_API static void GetDefaultAudioCapturerDeviceId(char* id,
                                                             size_t size);
  LIB_WEBRTC_API static void GetDefaultAudioPlaybackDeviceId(char* id,
                                                             size_t size);

  /// Get the microphone device index by its device id.
  LIB_WEBRTC_API static int GetAudioCapturerDeviceIndex(const std::string& id);
  /// Get microphone device by index.
  LIB_WEBRTC_API static AudioDevice GetAudioCapturerDeviceByIndex(int index);

  LIB_WEBRTC_API static int NumberOfActiveDevices(bool is_capture);
  LIB_WEBRTC_API static int NumberOfEnumeratedDevices(bool is_capture);

  LIB_WEBRTC_API static bool IsDefaultDevice(int index);
  LIB_WEBRTC_API static bool IsDefaultCommunicationsDevice(int index);
  // Returns true if |device_id| corresponds to the id of the default
  // device. Note that, if only one device is available (or if the user has not
  // explicitly set a default device), |device_id| will also math
  // IsDefaultCommunicationsDeviceId().
  LIB_WEBRTC_API static bool IsDefaultDeviceId(const std::string& device_id,
                                               bool is_capture);
  // Returns true if |device_id| corresponds to the id of the default
  // communication device. Note that, if only one device is available (or if
  // the user has not explicitly set a communication device), |device_id| will
  // also math IsDefaultDeviceId().
  LIB_WEBRTC_API static bool IsDefaultCommunicationsDeviceId(
      const std::string& device_id,
      bool is_capture);
};
}  // namespace base
}  // namespace owt
#endif  // OWT_BASE_DEVICEUTILS_H_
