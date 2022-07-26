#include "desktop_capturer.h"

namespace webrtc {
namespace internal {

    enum { kCaptureDelay = 33, kCaptureMessageId = 1000 };

DesktopCapturer::DesktopCapturer(
    std::unique_ptr<webrtc::DesktopCapturer> desktopcapturer)
    : capturer_(std::move(desktopcapturer)) {
}

DesktopCapturer::DesktopCapturer(std::unique_ptr<webrtc::DesktopCapturer> desktopcapturer, int window_id) 
    : capturer_(std::move(desktopcapturer)) {
      windows_id_ = window_id;
}

DesktopCapturer ::~DesktopCapturer() {
  RTC_LOG(INFO) << __FUNCTION__ << ": dtor ";
  Stop();
}

CaptureState DesktopCapturer::Start(const cricket::VideoFormat& capture_format) {
  if (!capture_thread_) {
    capture_thread_ = rtc::Thread::Create();  
  }
  capture_thread_->SetName("capture_thread", nullptr);
  RTC_CHECK(capture_thread_->Start()) << "Failed to start capture thread";

  capture_thread_->PostTask(RTC_FROM_HERE,
      [this] { 
        capture_state_ = CS_RUNNING;
        capturer_->Start(this);
        CaptureFrame();
      });

  return CS_RUNNING;
}

void DesktopCapturer::Stop() {
  capture_state_ = CS_STOPPED;
  if (capture_thread_) {
    capture_thread_->Stop();
  }
}

bool DesktopCapturer::IsRunning() {
  return capture_state_ == CS_RUNNING;
}

int filterException(int code, PEXCEPTION_POINTERS ex) {
  return EXCEPTION_EXECUTE_HANDLER;
}

void DesktopCapturer::OnCaptureResult(
    webrtc::DesktopCapturer::Result result,
    std::unique_ptr<webrtc::DesktopFrame> frame) {

  if (result != webrtc::DesktopCapturer::Result::SUCCESS) {
    return;
  }

  int width = frame->size().width();
  int height = frame->size().height();

  webrtc::DesktopRect rect_ = webrtc::DesktopRect::MakeWH(width, height);

  if (windows_id_ > 0) {
    webrtc::GetWindowRect(reinterpret_cast<HWND>(windows_id_), &rect_);
    // RTC_LOG(INFO) << "GetWindowRect(): " << rect_.size().width() << " " <<  rect_.size().height();
  }

  __try {
    i420_buffer_ = webrtc::I420Buffer::Create(width, height);

    libyuv::ConvertToI420(frame->data(), 0, i420_buffer_->MutableDataY(),
                        i420_buffer_->StrideY(), i420_buffer_->MutableDataU(),
                        i420_buffer_->StrideU(), i420_buffer_->MutableDataV(),
                        i420_buffer_->StrideV(), 0, 0, rect_.width(), rect_.height(), 
                        width, height, libyuv::kRotate0, libyuv::FOURCC_ARGB);
  
    OnFrame(webrtc::VideoFrame(i420_buffer_, 0, 0, webrtc::kVideoRotation_0));
  } 
  __except (filterException(GetExceptionCode(), GetExceptionInformation())) { }
}

void DesktopCapturer::OnMessage(rtc::Message* msg) {
  if (msg->message_id == kCaptureMessageId) {
    CaptureFrame();
  }
}

void DesktopCapturer::CaptureFrame() {
  if (capture_state_ == CS_RUNNING) {
    capturer_->CaptureFrame();
    rtc::Thread::Current()->PostDelayed(RTC_FROM_HERE, kCaptureDelay, this, kCaptureMessageId);
  }
}


//
//
//

CustomizedVideoCapturer* CustomizedVideoCapturer::Create(
    std::shared_ptr<libwebrtc::LocalDesktopStreamParameters> parameters,
    std::unique_ptr<libwebrtc::LocalDesktopStreamObserver> observer) {
  std::unique_ptr<CustomizedVideoCapturer> vcm_capturer(
      new CustomizedVideoCapturer());
  if (!vcm_capturer->Init(parameters, std::move(observer)))
    return nullptr;
  return vcm_capturer.release();
}

CustomizedVideoCapturer::CustomizedVideoCapturer() : vcm_(nullptr) {}

bool CustomizedVideoCapturer::Init(
    std::shared_ptr<libwebrtc::LocalDesktopStreamParameters> parameters,
    std::unique_ptr<libwebrtc::LocalDesktopStreamObserver> observer) {

  webrtc::DesktopCaptureOptions options =
      webrtc::DesktopCaptureOptions::CreateDefault();
  options.set_allow_directx_capturer(true);
  if (parameters->SourceType() == libwebrtc::LocalDesktopStreamParameters::
                                      DesktopSourceType::kApplication) {
    vcm_ = new rtc::RefCountedObject<owt::base::BasicWindowCapturer>(
        options, std::move(observer), parameters->CursorEnabled());
  } else {
    vcm_ = new rtc::RefCountedObject<owt::base::BasicScreenCapturer>(
        options, std::move(observer), parameters->CursorEnabled());
  }

  if (!vcm_)
    return false;

  vcm_->RegisterCaptureDataCallback(this);
  capability_.maxFPS = parameters->Fps();
  capability_.videoType = webrtc::VideoType::kI420;

  if (vcm_->StartCapture(capability_) != 0) {
    Destroy();
    return false;
  }

  RTC_CHECK(vcm_->CaptureStarted());
  return true;
}

void CustomizedVideoCapturer::Destroy() {
  if (!vcm_)
    return;

  vcm_->StopCapture();
  vcm_->DeRegisterCaptureDataCallback();
  vcm_ = nullptr;
}

CustomizedVideoCapturer::~CustomizedVideoCapturer() {
  Destroy();
}

void CustomizedVideoCapturer::OnFrame(const webrtc::VideoFrame& frame) {
  VideoCapturer::OnFrame(frame);
}

}  // namespace internal
}  // namespace webrtc
