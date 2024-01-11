#ifndef LIB_WEBRTC_RTC_VIDEO_DEVICE_HXX
#define LIB_WEBRTC_RTC_VIDEO_DEVICE_HXX

#include "rtc_types.h"

#include <functional>

namespace libwebrtc {

class RTCJpegCapturerCallback {
 public:
  virtual ~RTCJpegCapturerCallback() {}
  virtual void OnEncodeJpeg(const char* id,
                            const uint8_t* data,
                            size_t size) = 0;
};

enum class VideoType {
  kUnknown,
  kI420,
  kIYUV,
  kRGB24,
  kARGB,
  kRGB565,
  kYUY2,
  kYV12,
  kUYVY,
  kMJPEG,
  kBGRA,
  kNV12,
};

struct VideoCaptureCapability {
  int32_t width = 0;
  int32_t height = 0;
  int32_t maxFPS = 0;
  VideoType videoType = VideoType::kUnknown;
  bool interlaced = false;
};

class RTCVideoCapturer : public RefCountInterface {
 public:
  virtual ~RTCVideoCapturer() {}

  virtual bool StartCapture() = 0;

  virtual bool CaptureStarted() = 0;

  virtual void StopCapture() = 0;

  virtual bool UpdateCaptureDevice(size_t width,
                                   size_t height,
                                   size_t target_fps,
                                   size_t capture_device_index) = 0;

  virtual void StartEncodeJpeg(const char* id,
                               uint16_t delay_timeinterval,
                               RTCJpegCapturerCallback* data_cb) = 0;
  virtual void StopEncodeJpeg() = 0;
};

class RTCVideoDevice : public RefCountInterface {
 public:
  virtual uint32_t NumberOfDevices() = 0;

  virtual int32_t GetDeviceName(uint32_t deviceNumber,
                                char* deviceNameUTF8,
                                uint32_t deviceNameLength,
                                char* deviceUniqueIdUTF8,
                                uint32_t deviceUniqueIdUTF8Length,
                                char* productUniqueIdUTF8 = 0,
                                uint32_t productUniqueIdUTF8Length = 0) = 0;

  virtual uint32_t GetCaptureDeviceCapabilityCount(
      const char* deviceUniqueIdUTF8) = 0;
  virtual int32_t GetCaptureDeviceCapability(
      const char* deviceUniqueIdUTF8,
      uint32_t deviceCapabilityNumber,
      VideoCaptureCapability& capability) = 0;

  virtual scoped_refptr<RTCVideoCapturer> Create(const char* name,
                                                 uint32_t index,
                                                 size_t width,
                                                 size_t height,
                                                 size_t target_fps) = 0;

 protected:
  virtual ~RTCVideoDevice() {}
};

}  // namespace libwebrtc

#endif  // LIB_WEBRTC_RTC_VIDEO_DEVICE_HXX
