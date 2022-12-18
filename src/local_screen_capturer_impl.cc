#include "src/local_screen_capturer_impl.h"
#include "src/win/msdkvideoencoder.h"

namespace libwebrtc {
// static method
scoped_refptr<LocalScreenCapturer>
LocalScreenCapturer::CreateLocalScreenCapturer(
    LocalDesktopCapturerObserver* observer,
    LocalDesktopCapturerParameters* parameters) {
  webrtc::VideoCaptureCapability capability;
  capability.maxFPS = parameters->Fps();
  capability.videoType = webrtc::VideoType::kI420;

  scoped_refptr<LocalScreenCapturerImpl> capturer =
      scoped_refptr<LocalScreenCapturerImpl>(
          new RefCountedObject<LocalScreenCapturerImpl>(
              capability, observer, parameters->CursorEnabled()));
  return capturer;
}

LocalScreenCapturerImpl::LocalScreenCapturerImpl(
    webrtc::VideoCaptureCapability capability,
    LocalDesktopCapturerObserver* observer,
    bool cursor_enabled)
    : capturer_observer_(observer),
      capability_(capability),
      cursor_enabled_(cursor_enabled),
      encoder_(nullptr),
      image_callback_(nullptr),
      encoder_initialized_(false) {}

LocalScreenCapturerImpl::~LocalScreenCapturerImpl() {
  capturer_observer_ = nullptr;
  image_callback_ = nullptr;
  StopCapturing();
  ReleaseEncoder();
}

bool LocalScreenCapturerImpl::StartCapturing(
    LocalScreenEncodedImageCallback* image_callback) {
  if (capturer_ == nullptr) {
    webrtc::DesktopCaptureOptions options =
        webrtc::DesktopCaptureOptions::CreateDefault();
    options.set_allow_directx_capturer(true);
    capturer_ = new rtc::RefCountedObject<owt::base::BasicScreenCapturer>(
        options, capturer_observer_, cursor_enabled_);
  }

  capturer_->RegisterCaptureDataCallback(this);
  if (capturer_->StartCapture(capability_) != 0) {
    return StopCapturing();
  }

  image_callback_ = image_callback;
  return capturer_->CaptureStarted();
}

bool LocalScreenCapturerImpl::StopCapturing() {
  if (capturer_ == nullptr)
    return false;

  image_callback_ = nullptr;
  capturer_->StopCapture();
  capturer_->DeRegisterCaptureDataCallback();
  capturer_ = nullptr;
  return true;
}

void LocalScreenCapturerImpl::OnFrame(const webrtc::VideoFrame& frame) {
  if (encoder_ == nullptr) {
    encoder_initialized_ = InitEncoder(frame.width(), frame.height());
  }
  if (encoder_initialized_) {
    encoder_->Encode(frame, nullptr);
  }
}

bool LocalScreenCapturerImpl::InitEncoder(int width, int height) {
  // these settings now are const
  cricket::VideoCodec format = {0, "H264"};
  format.clockrate = 9000;
  format.SetParam("level-asymmetry-allowed", "1");
  format.SetParam("packetization-mode", "1");
  format.SetParam("profile-level-id", "42e01f");
  encoder_ = owt::base::MSDKVideoEncoder::Create(format);

  webrtc::VideoCodec codec;
  codec.maxFramerate = 30;
  codec.startBitrate = 2000;
  codec.minBitrate = 2000;
  codec.maxBitrate = 4000;
  codec.width = width;
  codec.height = height;
  codec.codecType = webrtc::PayloadStringToCodecType(format.name);

  // init encoder
  if (encoder_->InitEncode(&codec, 0, 1200) != 0) {
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
    bool keyframe = codec_specific_info->codecSpecific.H264.idr_frame;
    image_callback_->OnEncodedImage(encoded_image.data(), encoded_image.size(),
                                    keyframe, encoded_image._encodedWidth,
                                    encoded_image._encodedHeight);
  }
  return Result(Result::Error::OK);
}

}  // namespace libwebrtc