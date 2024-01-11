/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "src/internal/vcm_capturer.h"

#include <stdint.h>
#include <memory>

#include "modules/video_capture/video_capture_factory.h"
#include "rtc_base/checks.h"
#include "rtc_base/internal/default_socket_server.h"
#include "rtc_base/logging.h"

#include "rtc_video_device.h"

#include "turbojpeg.h"

namespace webrtc {
namespace internal {

class BasicJPEGCaptureThread : public rtc::Thread, public rtc::MessageHandler {
 public:
  explicit BasicJPEGCaptureThread(VcmCapturer* capturer,
                                  uint16_t delay_timeinterval)
      : rtc::Thread(rtc::CreateDefaultSocketServer()),
        capturer_(capturer),
        finished_(false),
        waiting_time_ms_(delay_timeinterval) {
    this->SetName("JPEGEncodeInvokeThread", nullptr);
  }

  virtual ~BasicJPEGCaptureThread() { Stop(); }
  // Override virtual method of parent Thread. Context: Worker Thread.
  virtual void Run() {
    // Read the first frame and start the message pump. The pump runs until
    // Stop() is called externally or Quit() is called by OnMessage().
    if (capturer_) {
      capturer_->EncodeJpeg();
      rtc::Thread::Current()->Post(RTC_FROM_HERE, this);
      rtc::Thread::Current()->ProcessMessages(kForever);
    }
    webrtc::MutexLock lock(&mutex_);
    finished_ = true;
  }
  // Override virtual method of parent MessageHandler. Context: Worker Thread.
  virtual void OnMessage(rtc::Message* /*pmsg*/) {
    if (capturer_) {
      capturer_->EncodeJpeg();
      // set capture time interval
      if (waiting_time_ms_ > 0) {
        rtc::Thread::Current()->PostDelayed(RTC_FROM_HERE, waiting_time_ms_,
                                            this);
      } else {
        rtc::Thread::Current()->Post(RTC_FROM_HERE, this);
      }
    } else {
      rtc::Thread::Current()->Quit();
    }
  }
  // Check if Run() is finished.
  bool Finished() const {
    webrtc::MutexLock lock(&mutex_);
    return finished_;
  }

 private:
  VcmCapturer* capturer_;
  mutable webrtc::Mutex mutex_;
  bool finished_;
  int waiting_time_ms_;
};

VcmCapturer::VcmCapturer(rtc::Thread* worker_thread)
    : vcm_(nullptr),
      worker_thread_(worker_thread),
      video_buffer_(nullptr),
      jpeg_thread_(nullptr),
      jpeg_data_cb_(nullptr) {}

bool VcmCapturer::Init(size_t width,
                       size_t height,
                       size_t target_fps,
                       size_t capture_device_index) {
  std::unique_ptr<VideoCaptureModule::DeviceInfo> device_info(
      VideoCaptureFactory::CreateDeviceInfo());

  char device_name[256];
  char unique_name[256];
  if (device_info->GetDeviceName(static_cast<uint32_t>(capture_device_index),
                                 device_name, sizeof(device_name), unique_name,
                                 sizeof(unique_name)) != 0) {
    Destroy();
    return false;
  }

  vcm_ = worker_thread_->Invoke<rtc::scoped_refptr<VideoCaptureModule>>(
      RTC_FROM_HERE,
      [&] { return webrtc::VideoCaptureFactory::Create(unique_name); });

  if (!vcm_) {
    return false;
  }

  vcm_->RegisterCaptureDataCallback(this);

  device_info->GetCapability(vcm_->CurrentDeviceName(), 0, capability_);

  capability_.width = static_cast<int32_t>(width);
  capability_.height = static_cast<int32_t>(height);
  capability_.maxFPS = static_cast<int32_t>(target_fps);
  capability_.videoType = VideoType::kI420;

  int32_t result = worker_thread_->Invoke<bool>(
      RTC_FROM_HERE, [&] { return vcm_->StartCapture(capability_); });

  if (result != 0) {
    Destroy();
    return false;
  }

  RTC_CHECK(worker_thread_->Invoke<bool>(
      RTC_FROM_HERE, [&] { return vcm_->CaptureStarted(); }));

  // save the capture device index
  last_capture_device_index_ = capture_device_index;

  return true;
}

bool VcmCapturer::UpdateCaptureDevice(size_t width,
                                      size_t height,
                                      size_t target_fps,
                                      size_t capture_device_index) {
  std::unique_ptr<VideoCaptureModule::DeviceInfo> device_info(
      VideoCaptureFactory::CreateDeviceInfo());

  if (!vcm_) {
    RTC_LOG(LS_ERROR) << "the original device not exists";
  }

  // check if the target device exists
  char device_name[256];
  char unique_name[256];
  if (device_info->GetDeviceName(static_cast<uint32_t>(capture_device_index),
                                 device_name, sizeof(device_name), unique_name,
                                 sizeof(unique_name)) != 0) {
    RTC_LOG(LS_ERROR) << "the target device not exists";
    return false;
  }

  // stop capture, release old vcm_
  Destroy();
  // reinit vcm_ with target device info
  return Init(width, height, target_fps, capture_device_index);
}

VcmCapturer* VcmCapturer::Create(rtc::Thread* worker_thread,
                                 size_t width,
                                 size_t height,
                                 size_t target_fps,
                                 size_t capture_device_index) {
  std::unique_ptr<VcmCapturer> vcm_capturer(new VcmCapturer(worker_thread));
  if (!vcm_capturer->Init(width, height, target_fps, capture_device_index)) {
    RTC_LOG(LS_ERROR) << "Failed to create VcmCapturer(w = " << width
                        << ", h = " << height << ", fps = " << target_fps
                        << ")";
    return nullptr;
  }
  return vcm_capturer.release();
}

bool VcmCapturer::StartCapture() {
  if (CaptureStarted())
    return true;
  if (vcm_ == nullptr) {
    RTC_LOG(LS_APP)
        << "vcm_ is nullptr when call StartCapture, try to init vcm_, index: " << last_capture_device_index_;
    if (last_capture_device_index_ >= 0) {
      if (!Init(capability_.width, capability_.height, capability_.maxFPS,
                last_capture_device_index_)) {
        RTC_LOG(LS_ERROR) << "Failed to init vcm_";
        return false;
      }
      RTC_LOG(LS_APP) << "Init vcm_ success at StartCapture";
      return true;
    }
  }

  int32_t result = worker_thread_->Invoke<bool>(
      RTC_FROM_HERE, [&] { return vcm_->StartCapture(capability_); });

  if (result != 0) {
    Destroy();
    return false;
  }

  return true;
}

bool VcmCapturer::CaptureStarted() {
  if (vcm_ == nullptr) {
    RTC_LOG(LS_ERROR) << "vcm_ is nullptr";
    return false;
  }

  return worker_thread_->Invoke<bool>(RTC_FROM_HERE,
                                      [&] { return vcm_->CaptureStarted(); });
}

void VcmCapturer::StopCapture() {
  if (vcm_ == nullptr) {
    RTC_LOG(LS_ERROR) << "vcm_ is nullptr";
    return;
  }
  worker_thread_->Invoke<void>(RTC_FROM_HERE, [&] { vcm_->StopCapture(); });
}

void VcmCapturer::Destroy() {
  if (!vcm_)
    return;

  vcm_->DeRegisterCaptureDataCallback();

  worker_thread_->Invoke<void>(RTC_FROM_HERE, [&] {
    vcm_->StopCapture();
    // Release reference to VCM.
    vcm_ = nullptr;
  });
}

VcmCapturer::~VcmCapturer() {
  Destroy();
  if (jpeg_thread_) {
    jpeg_thread_->Quit();
    jpeg_thread_.reset();
  }
}

void VcmCapturer::OnFrame(const VideoFrame& frame) {
  video_buffer_ = frame.video_frame_buffer();
  VideoCapturer::OnFrame(frame);
}

void VcmCapturer::EncodeJpeg() {
  if (video_buffer_) {
    int w = video_buffer_->width();
    int h = video_buffer_->height();
    const uint8_t* data = video_buffer_->GetI420()->DataY();

    uint8_t* data_copy = new uint8_t[w * h * 3 / 2];
    memcpy(data_copy, data, w * h * 3 / 2);

    // compress to jpeg by using the best quality
    unsigned char* jpeg_buf = NULL;
    unsigned long jpeg_size = 0;
    tjhandle compressor = tjInitCompress();
    int ret = tjCompressFromYUV(compressor, data_copy, w, 1, h, TJ_420,
                                &jpeg_buf, &jpeg_size, 70, TJFLAG_FASTDCT);
    tjDestroy(compressor);
    if (ret != 0) {
      RTC_LOG(LS_ERROR) << "compress failed";
      return;
    }
    if (jpeg_data_cb_) {
      jpeg_data_cb_->OnEncodeJpeg(jpeg_data_cb_id_.c_str(), jpeg_buf, jpeg_size);
    }

    tjFree(jpeg_buf);
    delete[] data_copy;
  }
}

void VcmCapturer::StartEncodeJpeg(const char* id, uint16_t delay_timeinterval,
                                  libwebrtc::RTCJpegCapturerCallback* data_cb) {
  if (jpeg_thread_) {
    jpeg_thread_->Stop();
    jpeg_thread_.reset();
  }

  jpeg_thread_.reset(new BasicJPEGCaptureThread(this, delay_timeinterval));
  bool ret = jpeg_thread_->Start();
  if (!ret) {
    RTC_LOG(LS_ERROR) << "JPEG encode thread failed to start";
    return;
  }

  jpeg_data_cb_ = data_cb;
  jpeg_data_cb_id_ = std::string(id);
}

void VcmCapturer::StopEncodeJpeg() {
  if (jpeg_thread_) {
    jpeg_thread_->Stop();
    jpeg_thread_.reset();
  }
  jpeg_data_cb_ = nullptr;
}

rtc::scoped_refptr<CapturerTrackSource> CapturerTrackSource::Create(
    rtc::Thread* worker_thread) {
  const size_t kWidth = 640;
  const size_t kHeight = 480;
  const size_t kFps = 30;
  std::unique_ptr<VcmCapturer> capturer;
  std::unique_ptr<webrtc::VideoCaptureModule::DeviceInfo> info(
      webrtc::VideoCaptureFactory::CreateDeviceInfo());
  if (!info) {
    return nullptr;
  }
  int num_devices = info->NumberOfDevices();
  for (int i = 0; i < num_devices; ++i) {
    capturer = absl::WrapUnique(
        VcmCapturer::Create(worker_thread, kWidth, kHeight, kFps, i));
    if (capturer) {
      return rtc::scoped_refptr<CapturerTrackSource>(
          new rtc::RefCountedObject<CapturerTrackSource>(std::move(capturer)));
    }
  }

  return nullptr;
}

}  // namespace internal
}  // namespace webrtc
