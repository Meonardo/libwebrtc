#include "rtc_desktop_device_impl.h"
#include "internal/desktop_capturer.h"
#include "rtc_base/thread.h"
#include "rtc_desktop_capturer.h"
#include "rtc_desktop_media_list.h"
#include "rtc_video_device_impl.h"

namespace libwebrtc {

RTCDesktopDeviceImpl::RTCDesktopDeviceImpl(rtc::Thread* signaling_thread)
    : signaling_thread_(signaling_thread) {}

RTCDesktopDeviceImpl::~RTCDesktopDeviceImpl() {}

scoped_refptr<RTCDesktopCapturer> RTCDesktopDeviceImpl::CreateDesktopCapturer(
    scoped_refptr<MediaSource> source) {
  MediaSourceImpl* source_impl = static_cast<MediaSourceImpl*>(source.get());
  return new RefCountedObject<RTCDesktopCapturerImpl>(
      source_impl->type(), source_impl->source_id(), signaling_thread_, source);
}

scoped_refptr<RTCDesktopMediaList> RTCDesktopDeviceImpl::GetDesktopMediaList(
    DesktopType type) {
  if (desktop_media_lists_.find(type) == desktop_media_lists_.end()) {
    desktop_media_lists_[type] =
        new RefCountedObject<RTCDesktopMediaListImpl>(type, signaling_thread_);
  }
  return desktop_media_lists_[type];
}

scoped_refptr<RTCDesktopCapturer2> RTCDesktopDeviceImpl::CreateDesktopCapturer(
    LocalDesktopCapturerDataSource* source_observer,
    std::shared_ptr<LocalDesktopCapturerParameters> params) {
  webrtc::internal::LocalDesktopCapturer* desktop_capturer =
      webrtc::internal::LocalDesktopCapturer::Create(params, source_observer);

  return signaling_thread_->Invoke<scoped_refptr<RTCDesktopCapturerImpl2>>(
      RTC_FROM_HERE, [desktop_capturer] {
        return scoped_refptr<RTCDesktopCapturerImpl2>(
            new RefCountedObject<RTCDesktopCapturerImpl2>(
                absl::WrapUnique(desktop_capturer)));
      });
}

}  // namespace libwebrtc
