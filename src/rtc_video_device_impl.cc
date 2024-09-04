#include "rtc_video_device_impl.h"

#include "modules/video_capture/video_capture_factory.h"

namespace libwebrtc {

RTCVideoDeviceImpl::RTCVideoDeviceImpl(rtc::Thread* worker_thread)
    : device_info_(webrtc::VideoCaptureFactory::CreateDeviceInfo()),
      worker_thread_(worker_thread) {}

uint32_t RTCVideoDeviceImpl::NumberOfDevices() {
  if (!device_info_) {
    return 0;
  }
  return device_info_->NumberOfDevices();
}

int32_t RTCVideoDeviceImpl::GetDeviceName(
    uint32_t deviceNumber, char* deviceNameUTF8, uint32_t deviceNameLength,
    char* deviceUniqueIdUTF8, uint32_t deviceUniqueIdUTF8Length,
    char* productUniqueIdUTF8 /*= 0*/,
    uint32_t productUniqueIdUTF8Length /*= 0*/) {
  if (!device_info_) {
    return -1;
  }

  if (device_info_->GetDeviceName(deviceNumber, deviceNameUTF8,
                                  deviceNameLength, deviceUniqueIdUTF8,
                                  deviceUniqueIdUTF8Length) != -1) {
    return 0;
  }
  return 0;
}

uint32_t RTCVideoDeviceImpl::GetCaptureDeviceCapabilityCount(
    const char* deviceUniqueIdUTF8) {
  if (!device_info_) {
    return 0;
  }
  auto ret = device_info_->NumberOfCapabilities(deviceUniqueIdUTF8);
  if (ret < 0) {
    return 0;
  }
  return ret;
}

int32_t RTCVideoDeviceImpl::GetCaptureDeviceCapability(
    const char* deviceUniqueIdUTF8,
    uint32_t deviceCapabilityNumber,
    VideoCaptureCapability& capability) {
  if (!device_info_) {
    return -1;
  }
  webrtc::VideoCaptureCapability c;
  auto ret = device_info_->GetCapability(deviceUniqueIdUTF8,
                                         deviceCapabilityNumber, c);
  if (ret != 0) {
    return ret;
  }

  capability.width = c.width;
  capability.height = c.height;
  capability.maxFPS = c.maxFPS;
  capability.videoType = static_cast<VideoType>(c.videoType);
  capability.interlaced = c.interlaced;

  return 0;
}

scoped_refptr<RTCVideoCapturer> RTCVideoDeviceImpl::Create(const char* name,
                                                           uint32_t index,
                                                           size_t width,
                                                           size_t height,
                                                           size_t target_fps) {
  auto vcm = worker_thread_->BlockingCall([&, width, height, target_fps]{
    return webrtc::internal::VcmCapturer::Create(worker_thread_, width, height,
                                                 target_fps, index);
   });

  if (vcm == nullptr) {
    return nullptr;
  }

  return worker_thread_->BlockingCall([vcm] {
    return scoped_refptr<RTCVideoCapturerImpl>(
        new RefCountedObject<RTCVideoCapturerImpl>(vcm));
  });
}

}  // namespace libwebrtc
