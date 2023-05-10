#include "src/local_desktop_capturer_impl.h"
#include "src/rtc_video_frame_impl.h"
#include "src/win/msdkvideoencoder.h"
#include "system_wrappers/include/cpu_info.h"

namespace libwebrtc {
// static method
scoped_refptr<LocalDesktopCapturer>
LocalDesktopCapturer::CreateLocalDesktopCapturer(
    LocalDesktopCapturerDataSource* datasource) {
  scoped_refptr<LocalDesktopCapturerImpl> capturer =
      scoped_refptr<LocalDesktopCapturerImpl>(
          new RefCountedObject<LocalDesktopCapturerImpl>(datasource));
  return capturer;
}

LocalDesktopCapturerImpl::LocalDesktopCapturerImpl(
    LocalDesktopCapturerDataSource* datasource)
    : capturer_datasource_(datasource),
      encoder_(nullptr),
      image_callback_(nullptr),
      frame_callback_(nullptr),
      encoder_initialized_(false),
      encoded_file_save_path_(""),
      max_bitrate_(6000),
      min_bitrate_(3000),
      max_framerate_(30),
      qsv_encoder_quality_(
          LocalDesktopCapturerParameters::IntelQSVEncoderQuality::kBalanced) {}

LocalDesktopCapturerImpl::~LocalDesktopCapturerImpl() {
  capturer_datasource_ = nullptr;
  image_callback_ = nullptr;
  frame_callback_ = nullptr;

  StopCapturing(true);
}

bool LocalDesktopCapturerImpl::StartCapturing(
    LocalDesktopEncodedImageCallback* image_callback,
    std::shared_ptr<LocalDesktopCapturerParameters> parameters) {
  if (parameters == nullptr || image_callback == nullptr) {
    RTC_LOG(LS_ERROR) << "Must set `LocalDesktopCapturerParameters`!";
    return false;
  }

  if (capturer_ == nullptr) {
    webrtc::DesktopCaptureOptions options =
        webrtc::DesktopCaptureOptions::CreateDefault();
    // update capture method
    if (parameters->CapturePolicy() ==
        LocalDesktopCapturerParameters::DesktopCapturePolicy::kEnableDirectX) {
      options.set_allow_directx_capturer(true);
    } else if (parameters->CapturePolicy() ==
               LocalDesktopCapturerParameters::DesktopCapturePolicy::
                   kEnableWGC) {
      options.set_allow_wgc_capturer(true);
    }

    capturer_ = new rtc::RefCountedObject<owt::base::BasicScreenCapturer>(
        options, capturer_datasource_, parameters->CursorEnabled());
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

  if (parameters->IntelEncoderQuality() !=
      LocalDesktopCapturerParameters::IntelQSVEncoderQuality::kUnknown) {
    qsv_encoder_quality_ = parameters->IntelEncoderQuality();
  }
  number_cores_ = webrtc::CpuInfo::DetectNumberOfCores();
  next_frame_types_.push_back(webrtc::VideoFrameType::kVideoFrameDelta);
  image_callback_ = image_callback;
  return capturer_->CaptureStarted();
}

bool LocalDesktopCapturerImpl::StartCapturing(
    LocalDesktopRawFrameCallback* frame_callback,
    std::shared_ptr<LocalDesktopCapturerParameters> parameters) {
  if (parameters == nullptr || frame_callback == nullptr) {
    RTC_LOG(LS_ERROR) << "Must set `LocalDesktopCapturerParameters`!";
    return false;
  }

  if (capturer_ == nullptr) {
    webrtc::DesktopCaptureOptions options =
        webrtc::DesktopCaptureOptions::CreateDefault();
    // update capture method
    if (parameters->CapturePolicy() ==
        LocalDesktopCapturerParameters::DesktopCapturePolicy::kEnableDirectX) {
      options.set_allow_directx_capturer(true);
    } else if (parameters->CapturePolicy() ==
               LocalDesktopCapturerParameters::DesktopCapturePolicy::
                   kEnableWGC) {
      options.set_allow_wgc_capturer(true);
    }
    capturer_ = new rtc::RefCountedObject<owt::base::BasicScreenCapturer>(
        options, capturer_datasource_, parameters->CursorEnabled());
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

  max_framerate_ = parameters->Fps();
  frame_callback_ = frame_callback;
  return capturer_->CaptureStarted();
}

bool LocalDesktopCapturerImpl::StopCapturing(bool release_encoder) {
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

void LocalDesktopCapturerImpl::OnFrame(const webrtc::VideoFrame& frame) {
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

bool LocalDesktopCapturerImpl::InitEncoder(int width, int height) {
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

  encoder_ = owt::base::MSDKVideoEncoder::Create(
      format, encoded_file_save_path_, (mfxU16)qsv_encoder_quality_);

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

void LocalDesktopCapturerImpl::ReleaseEncoder() {
  if (!encoder_ || !encoder_initialized_) {
    return;
  }
  encoder_->Release();
  encoder_initialized_ = false;
  encoder_.reset();
  RTC_LOG(LS_ERROR) << "Encoder released";
}

webrtc::EncodedImageCallback::Result LocalDesktopCapturerImpl::OnEncodedImage(
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