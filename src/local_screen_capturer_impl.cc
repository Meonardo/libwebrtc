#include "src/local_screen_capturer_impl.h"
#include "src/rtc_video_frame_impl.h"
#include "src/win/msdkvideoencoder.h"
#include "system_wrappers/include/cpu_info.h"

namespace libwebrtc {
// static method
scoped_refptr<LocalScreenCapturer>
LocalScreenCapturer::CreateLocalScreenCapturer(
    LocalDesktopCapturerObserver* capturer_observer) {
  scoped_refptr<LocalScreenCapturerImpl> capturer =
      scoped_refptr<LocalScreenCapturerImpl>(
          new RefCountedObject<LocalScreenCapturerImpl>(capturer_observer));
  return capturer;
}

LocalScreenCapturerImpl::LocalScreenCapturerImpl(
    LocalDesktopCapturerObserver* capturer_observer)
    : capturer_observer_(capturer_observer),
      encoder_(nullptr),
      image_callback_(nullptr),
      frame_callback_(nullptr),
      encoder_initialized_(false),
      encoded_file_save_path_(""),
      max_bitrate_(6000),
      min_bitrate_(3000),
      max_framerate_(30) {}

LocalScreenCapturerImpl::~LocalScreenCapturerImpl() {
  capturer_observer_ = nullptr;
  image_callback_ = nullptr;
  frame_callback_ = nullptr;

  StopCapturing(true);
}

bool LocalScreenCapturerImpl::StartCapturing(
    LocalScreenEncodedImageCallback* image_callback,
    std::shared_ptr<LocalDesktopCapturerParameters> parameters) {
  if (parameters == nullptr || image_callback == nullptr) {
    RTC_LOG(LS_ERROR) << "Must set `LocalDesktopCapturerParameters`!";
    return false;
  }

  if (capturer_ == nullptr) {
    webrtc::DesktopCaptureOptions options =
        webrtc::DesktopCaptureOptions::CreateDefault();
    // tmp set directX to true
    options.set_allow_directx_capturer(true);
    capturer_ = new rtc::RefCountedObject<owt::base::BasicScreenCapturer>(
        options, capturer_observer_, parameters->CursorEnabled());
  }

  capturer_->RegisterCaptureDataCallback(this);

  // encode file path
  encoded_file_save_path_ = parameters->EncodedFilePath();
  if (!encoded_file_save_path_.empty())
    RTC_LOG(LS_ERROR) << "Save encoded video to " << encoded_file_save_path_;

  // assign capture capability
  webrtc::VideoCaptureCapability capability;
  capability.videoType = webrtc::VideoType::kI420;
  capability.height = parameters->Height();
  capability.width = parameters->Width();
  capability.maxFPS = parameters->Fps();

  if (parameters->Fps() > 0)
    max_framerate_ = parameters->Fps();
  if (parameters->MaxBitrate() > 0)
    max_bitrate_ = parameters->MaxBitrate();
  if (parameters->MinBitrate() > 0)
    min_bitrate_ = parameters->MinBitrate();

  if (capturer_->StartCapture(capability) != 0) {
    return StopCapturing(true);
  }

  number_cores_ = webrtc::CpuInfo::DetectNumberOfCores();
  next_frame_types_.push_back(webrtc::VideoFrameType::kVideoFrameDelta);
  image_callback_ = image_callback;
  return capturer_->CaptureStarted();
}

bool LocalScreenCapturerImpl::StartCapturing(
    LocalScreenRawFrameCallback* frame_callback,
    std::shared_ptr<LocalDesktopCapturerParameters> parameters) {
  if (parameters == nullptr || frame_callback == nullptr) {
    RTC_LOG(LS_ERROR) << "Must set `LocalDesktopCapturerParameters`!";
    return false;
  }

  if (capturer_ == nullptr) {
    webrtc::DesktopCaptureOptions options =
        webrtc::DesktopCaptureOptions::CreateDefault();
    // tmp set directX to true
    options.set_allow_directx_capturer(true);
    capturer_ = new rtc::RefCountedObject<owt::base::BasicScreenCapturer>(
        options, capturer_observer_, parameters->CursorEnabled());
  }

  capturer_->RegisterCaptureDataCallback(this);
  // assign capture capability
  webrtc::VideoCaptureCapability capability;
  capability.videoType = webrtc::VideoType::kI420;
  capability.height = parameters->Height();
  capability.width = parameters->Width();
  capability.maxFPS = parameters->Fps();

  if (capturer_->StartCapture(capability) != 0) {
    return StopCapturing(true);
  }

  frame_callback_ = frame_callback;
  return capturer_->CaptureStarted();
}

bool LocalScreenCapturerImpl::StopCapturing(bool release_encoder) {
  if (capturer_ == nullptr)
    return false;

  image_callback_ = nullptr;
  frame_callback_ = nullptr;
  capturer_->StopCapture();
  capturer_->DeRegisterCaptureDataCallback();
  capturer_ = nullptr;

  if (release_encoder) {  // release encoder
    ReleaseEncoder();
  }
  return true;
}

void LocalScreenCapturerImpl::OnFrame(const webrtc::VideoFrame& frame) {
  // if registered raw-frame callback then it will not encode any frame
  if (frame_callback_ != nullptr) {
    auto frame_impl = scoped_refptr<VideoFrameBufferImpl>(
        new RefCountedObject<VideoFrameBufferImpl>(frame.video_frame_buffer()));
    if (frame_callback_ != nullptr) {
      frame_callback_->OnFrame(frame_impl);
    }
    return;
  }

  if (encoder_ == nullptr) {
    encoder_initialized_ = InitEncoder(frame.width(), frame.height());
  }
  if (encoder_initialized_) {
    encoder_->Encode(frame, &next_frame_types_);
  }
}

bool LocalScreenCapturerImpl::InitEncoder(int width, int height) {
  // these settings now are const
  cricket::VideoCodec format = {0, "H264"};
  format.clockrate = 90000;
  format.SetParam("level-asymmetry-allowed", "1");
  format.SetParam("packetization-mode", "1");
  format.SetParam("profile-level-id", "42e01f");
  // format.SetParam("profile-id", "1");
  // format.SetParam("level-id", "120");
  // format.SetParam("profile-space", "0");
  // format.SetParam("level-id", "1");

  encoder_ =
      owt::base::MSDKVideoEncoder::Create(format, encoded_file_save_path_);

  webrtc::VideoCodec codec;
  codec.maxFramerate = max_framerate_;
  codec.startBitrate = min_bitrate_;
  codec.minBitrate = min_bitrate_;
  codec.maxBitrate = max_bitrate_;
  codec.width = width;
  codec.height = height;
  codec.codecType = webrtc::PayloadStringToCodecType(format.name);
  // codec.mode = webrtc::VideoCodecMode::kScreensharing;
  // codec.timing_frame_thresholds = {200, 500};
  // codec.qpMax = 57;

  // init encoder
  if (encoder_->InitEncode(&codec, number_cores_, 1200) != 0) {
    RTC_LOG(LS_ERROR) << "Failed to initialize the encoder associated with "
                         "codec type: "
                      << CodecTypeToPayloadString(codec.codecType) << " ("
                      << codec.codecType << ")";
    ReleaseEncoder();
    return false;
  }

  encoder_->RegisterEncodeCompleteCallback(this);
  return true;
}

void LocalScreenCapturerImpl::ReleaseEncoder() {
  if (!encoder_ || !encoder_initialized_) {
    return;
  }
  encoder_->Release();
  encoder_initialized_ = false;
  encoder_.reset();
  RTC_LOG(LS_ERROR) << "Encoder released";
}

webrtc::EncodedImageCallback::Result LocalScreenCapturerImpl::OnEncodedImage(
    const webrtc::EncodedImage& encoded_image,
    const webrtc::CodecSpecificInfo* codec_specific_info) {
  if (image_callback_ != nullptr) {
    bool keyframe =
        encoded_image._frameType == webrtc::VideoFrameType::kVideoFrameKey;
    image_callback_->OnEncodedImage(
        encoded_image.GetEncodedData()->data(), encoded_image.size(), keyframe,
        encoded_image._encodedWidth, encoded_image._encodedHeight);
  }
  return Result(Result::Error::OK);
}

}  // namespace libwebrtc