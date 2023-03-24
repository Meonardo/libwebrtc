#ifndef LIB_WEBRTC_VIDEO_DEVICE_IMPL_HXX
#define LIB_WEBRTC_VIDEO_DEVICE_IMPL_HXX

#include "rtc_video_device.h"

#include "modules/video_capture/video_capture.h"
#include "rtc_base/thread.h"
#include "src/internal/vcm_capturer.h"
#include "src/internal/video_capturer.h"

#include <memory>

namespace libwebrtc {

class RTCVideoCapturerImpl : public RTCVideoCapturer {
 public:
  RTCVideoCapturerImpl(
      std::unique_ptr<webrtc::internal::VideoCapturer> video_capturer)
      : video_capturer_(std::move(video_capturer)) {}
  std::unique_ptr<webrtc::internal::VideoCapturer> video_capturer() {
    return std::move(video_capturer_);
  }

  ~RTCVideoCapturerImpl() {
    RTC_LOG(LS_ERROR) << "RTCVideoCapturerImpl dtor";
    capturer_source_track_ = nullptr;
  }

  virtual bool UpdateCaptureDevice(size_t width,
                                   size_t height,
                                   size_t target_fps,
                                   size_t capture_device_index) override {
    if (capturer_source_track_ == nullptr) {
      RTC_LOG(LS_ERROR) << "capturer_source_track_ not exists";
      return false;
    }

    auto vcm = reinterpret_cast<webrtc::internal::VcmCapturer*>(
        capturer_source_track_->CapturerSource());

    return vcm->UpdateCaptureDevice(width, height, target_fps,
                                    capture_device_index);
  }

  void SaveVideoSourceTrack(
      rtc::scoped_refptr<webrtc::internal::CapturerTrackSource> source) {
    capturer_source_track_ = source;
  }

 private:
  std::unique_ptr<webrtc::internal::VideoCapturer> video_capturer_;
  rtc::scoped_refptr<webrtc::internal::CapturerTrackSource>
      capturer_source_track_;
};

class RTCVideoDeviceImpl : public RTCVideoDevice {
 public:
  RTCVideoDeviceImpl(rtc::Thread* signaling_thread, rtc::Thread* worker_thread);

 public:
  uint32_t NumberOfDevices() override;

  int32_t GetDeviceName(uint32_t deviceNumber,
                        char* deviceNameUTF8,
                        uint32_t deviceNameLength,
                        char* deviceUniqueIdUTF8,
                        uint32_t deviceUniqueIdUTF8Length,
                        char* productUniqueIdUTF8 = 0,
                        uint32_t productUniqueIdUTF8Length = 0) override;

  scoped_refptr<RTCVideoCapturer> Create(const char* name,
                                         uint32_t index,
                                         size_t width,
                                         size_t height,
                                         size_t target_fps) override;

 private:
  std::unique_ptr<webrtc::VideoCaptureModule::DeviceInfo> device_info_;
  rtc::Thread* signaling_thread_ = nullptr;
  rtc::Thread* worker_thread_ = nullptr;
};

}  // namespace libwebrtc

#endif  // LIB_WEBRTC_VIDEO_DEVICE_IMPL_HXX
