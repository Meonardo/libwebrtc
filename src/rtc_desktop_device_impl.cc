#include "rtc_desktop_device_impl.h"
#include "rtc_base/thread.h"
#include "rtc_video_device_impl.h"

namespace libwebrtc {

RTCDesktopCapturerImpl::RTCDesktopCapturerImpl(
    std::unique_ptr<webrtc::internal::LocalDesktopCapturer> video_capturer)
    : desktop_capturer_(std::move(video_capturer)) {}

RTCDesktopDeviceImpl::RTCDesktopDeviceImpl(rtc::Thread* signaling_thread)
    : signaling_thread_(signaling_thread) {}

RTCDesktopDeviceImpl::~RTCDesktopDeviceImpl() {
  RTC_LOG(INFO) << __FUNCTION__ << ": dtor ";
}

scoped_refptr<RTCDesktopCapturer> RTCDesktopDeviceImpl::CreateDesktopCapturer(
    LocalDesktopCapturerObserver* source_observer,
    std::shared_ptr<LocalDesktopCapturerParameters> params) {
  webrtc::internal::LocalDesktopCapturer* desktop_capturer =
      webrtc::internal::LocalDesktopCapturer::Create(params, source_observer);

  return signaling_thread_->Invoke<scoped_refptr<RTCDesktopCapturerImpl>>(
      RTC_FROM_HERE, [desktop_capturer] {
        return scoped_refptr<RTCDesktopCapturerImpl>(
            new RefCountedObject<RTCDesktopCapturerImpl>(
                absl::WrapUnique(desktop_capturer)));
      });
}

}  // namespace libwebrtc
