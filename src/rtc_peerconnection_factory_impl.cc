#include "rtc_peerconnection_factory_impl.h"
#include "rtc_audio_source_impl.h"
#include "rtc_media_stream_impl.h"
#include "rtc_mediaconstraints_impl.h"
#include "rtc_peerconnection_impl.h"
#include "rtc_video_device_impl.h"
#include "rtc_video_source_impl.h"

#include "api/audio_codecs/builtin_audio_decoder_factory.h"
#include "api/audio_codecs/builtin_audio_encoder_factory.h"
#include "api/create_peerconnection_factory.h"
#include "api/media_stream_interface.h"
#include "api/video_codecs/builtin_video_decoder_factory.h"
#include "api/video_codecs/builtin_video_encoder_factory.h"
// #include "modules/audio_device/audio_device_impl.h"

#include "modules/audio_device/include/audio_device_factory.h"

#include "src/win/customizedframescapturer.h"
#include "src/win/customizedvideosource.h"
#include "src/win/mediacapabilities.h"
#include "src/win/msdkvideodecoderfactory.h"
#include "src/win/msdkvideoencoderfactory.h"
#include "src/win/encodedvideoencoderfactory.h"

#if defined(WEBRTC_IOS)
#include "engine/sdk/objc/Framework/Classes/videotoolboxvideocodecfactory.h"
#endif
#include <api/task_queue/default_task_queue_factory.h>

namespace libwebrtc {

static bool hardware_acceleration_enabled_;
static bool customized_video_encoder_enabled_;

void GlobalConfiguration::SetVideoHardwareAccelerationEnabled(bool enabled) {
  hardware_acceleration_enabled_ = enabled;
}
bool GlobalConfiguration::GetVideoHardwareAccelerationEnabled() {
  return hardware_acceleration_enabled_;
}

void GlobalConfiguration::SetCustomizedVideoEncoderEnabled(bool enabled) {
  customized_video_encoder_enabled_ = enabled;
}
bool GlobalConfiguration::GetCustomizedVideoEncoderEnabled() {
  return customized_video_encoder_enabled_;
}

std::unique_ptr<webrtc::VideoEncoderFactory> CreateCustomVideoEncoderFactory() {
  if (!owt::base::MediaCapabilities::Get()) {
    return webrtc::CreateBuiltinVideoEncoderFactory();
  }
  return std::make_unique<owt::base::EncodedVideoEncoderFactory>();
}

std::unique_ptr<webrtc::VideoEncoderFactory> CreateIntelVideoEncoderFactory() {
  if (!owt::base::MediaCapabilities::Get()) {
    return webrtc::CreateBuiltinVideoEncoderFactory();
  }
  return std::make_unique<owt::base::MSDKVideoEncoderFactory>();
}

std::unique_ptr<webrtc::VideoDecoderFactory> CreateIntelVideoDecoderFactory() {
  if (!owt::base::MediaCapabilities::Get()) {
    return webrtc::CreateBuiltinVideoDecoderFactory();
  }
  return std::make_unique<owt::base::MSDKVideoDecoderFactory>();
}

RTCPeerConnectionFactoryImpl::RTCPeerConnectionFactoryImpl(
    rtc::Thread* worker_thread,
    rtc::Thread* signaling_thread,
    rtc::Thread* network_thread)
    : worker_thread_(worker_thread),
      signaling_thread_(signaling_thread),
      network_thread_(network_thread) {}

RTCPeerConnectionFactoryImpl::~RTCPeerConnectionFactoryImpl() {}

bool RTCPeerConnectionFactoryImpl::Initialize() {
  if (!audio_device_module_) {
    task_queue_factory_ = webrtc::CreateDefaultTaskQueueFactory();
    worker_thread_->Invoke<void>(RTC_FROM_HERE,
                                 [=] { CreateAudioDeviceModule_w(); });
  }

  if (!rtc_peerconnection_factory_) {
    if (GlobalConfiguration::GetCustomizedVideoEncoderEnabled()) {
      rtc_peerconnection_factory_ = webrtc::CreatePeerConnectionFactory(
          network_thread_, worker_thread_, signaling_thread_,
          audio_device_module_.get(),
          webrtc::CreateBuiltinAudioEncoderFactory(),
          webrtc::CreateBuiltinAudioDecoderFactory(),
          CreateCustomVideoEncoderFactory(), CreateIntelVideoDecoderFactory(),
          nullptr, nullptr);
    } else if (GlobalConfiguration::GetVideoHardwareAccelerationEnabled()) {
      rtc_peerconnection_factory_ = webrtc::CreatePeerConnectionFactory(
          network_thread_, worker_thread_, signaling_thread_,
          audio_device_module_.get(),
          webrtc::CreateBuiltinAudioEncoderFactory(),
          webrtc::CreateBuiltinAudioDecoderFactory(),
          CreateIntelVideoEncoderFactory(), CreateIntelVideoDecoderFactory(),
          nullptr, nullptr);
    } else {
      rtc_peerconnection_factory_ = webrtc::CreatePeerConnectionFactory(
          network_thread_, worker_thread_, signaling_thread_,
          audio_device_module_.get(),
          webrtc::CreateBuiltinAudioEncoderFactory(),
          webrtc::CreateBuiltinAudioDecoderFactory(),
          webrtc::CreateBuiltinVideoEncoderFactory(),
          webrtc::CreateBuiltinVideoDecoderFactory(), nullptr, nullptr);
    }
  }

  if (!rtc_peerconnection_factory_.get()) {
    Terminate();
    return false;
  }

  return true;
}

bool RTCPeerConnectionFactoryImpl::Terminate() {
  worker_thread_->Invoke<void>(RTC_FROM_HERE, [&] {
    audio_device_impl_ = nullptr;
    video_device_impl_ = nullptr;
  });
  rtc_peerconnection_factory_ = NULL;
  if (audio_device_module_) {
    worker_thread_->Invoke<void>(RTC_FROM_HERE,
                                 [this] { DestroyAudioDeviceModule_w(); });
  }

  return true;
}

void RTCPeerConnectionFactoryImpl::CreateAudioDeviceModule_w() {
  if (!audio_device_module_) {
    com_initializer_ = std::make_unique<webrtc::ScopedCOMInitializer>(
        webrtc::ScopedCOMInitializer::kMTA);
    if (com_initializer_->Succeeded()) {
      audio_device_module_ = webrtc::CreateWindowsCoreAudioAudioDeviceModule(
          task_queue_factory_.get());
    }
  }
}

void RTCPeerConnectionFactoryImpl::DestroyAudioDeviceModule_w() {
  if (audio_device_module_)
    audio_device_module_ = nullptr;
}

scoped_refptr<RTCPeerConnection> RTCPeerConnectionFactoryImpl::Create(
    const RTCConfiguration& configuration,
    scoped_refptr<RTCMediaConstraints> constraints) {
  scoped_refptr<RTCPeerConnection> peerconnection =
      scoped_refptr<RTCPeerConnectionImpl>(
          new RefCountedObject<RTCPeerConnectionImpl>(
              configuration, constraints, rtc_peerconnection_factory_));
  peerconnections_.push_back(peerconnection);
  return peerconnection;
}

void RTCPeerConnectionFactoryImpl::Delete(
    scoped_refptr<RTCPeerConnection> peerconnection) {
  peerconnections_.erase(
      std::remove_if(
          peerconnections_.begin(), peerconnections_.end(),
          [peerconnection](const scoped_refptr<RTCPeerConnection> pc_) {
            return pc_ == peerconnection;
          }),
      peerconnections_.end());
}

scoped_refptr<RTCAudioDevice> RTCPeerConnectionFactoryImpl::GetAudioDevice() {
  if (!audio_device_module_) {
    worker_thread_->Invoke<void>(RTC_FROM_HERE,
                                 [this] { CreateAudioDeviceModule_w(); });
  }

  if (!audio_device_impl_)
    audio_device_impl_ =
        scoped_refptr<AudioDeviceImpl>(new RefCountedObject<AudioDeviceImpl>(
            audio_device_module_, worker_thread_));

  return audio_device_impl_;
}

scoped_refptr<RTCVideoDevice> RTCPeerConnectionFactoryImpl::GetVideoDevice() {
  if (!video_device_impl_)
    video_device_impl_ = scoped_refptr<RTCVideoDeviceImpl>(
        new RefCountedObject<RTCVideoDeviceImpl>(signaling_thread_,
                                                 worker_thread_));

  return video_device_impl_;
}

scoped_refptr<RTCAudioSource> RTCPeerConnectionFactoryImpl::CreateAudioSource(
    const string audio_source_label) {
  rtc::scoped_refptr<webrtc::AudioSourceInterface> rtc_source_track =
      rtc_peerconnection_factory_->CreateAudioSource(cricket::AudioOptions());

  scoped_refptr<RTCAudioSourceImpl> source = scoped_refptr<RTCAudioSourceImpl>(
      new RefCountedObject<RTCAudioSourceImpl>(rtc_source_track));
  return source;
}
#ifdef RTC_DESKTOP_DEVICE
scoped_refptr<RTCDesktopDevice>
RTCPeerConnectionFactoryImpl::GetDesktopDevice() {
  if (!desktop_device_impl_) {
    desktop_device_impl_ = scoped_refptr<RTCDesktopDeviceImpl>(
        new RefCountedObject<RTCDesktopDeviceImpl>(signaling_thread_));
  }
  return desktop_device_impl_;
}
#endif
scoped_refptr<RTCVideoSource> RTCPeerConnectionFactoryImpl::CreateVideoSource(
    scoped_refptr<RTCVideoCapturer> capturer,
    const string video_source_label,
    scoped_refptr<RTCMediaConstraints> constraints) {
  if (rtc::Thread::Current() != worker_thread_) {
    scoped_refptr<RTCVideoSource> source =
        worker_thread_->Invoke<scoped_refptr<RTCVideoSource>>(
            RTC_FROM_HERE, [this, capturer, video_source_label, constraints] {
              return CreateVideoSource_s(
                  capturer, to_std_string(video_source_label).c_str(),
                  constraints);
            });
    return source;
  }

  return CreateVideoSource_s(
      capturer, to_std_string(video_source_label).c_str(), constraints);
}

scoped_refptr<RTCVideoSource> RTCPeerConnectionFactoryImpl::CreateVideoSource_s(
    scoped_refptr<RTCVideoCapturer> capturer,
    const char* video_source_label,
    scoped_refptr<RTCMediaConstraints> constraints) {
  RTCVideoCapturerImpl* capturer_impl =
      static_cast<RTCVideoCapturerImpl*>(capturer.get());
  /*RTCMediaConstraintsImpl* media_constraints =
          static_cast<RTCMediaConstraintsImpl*>(constraints.get());*/
  rtc::scoped_refptr<webrtc::VideoTrackSourceInterface> rtc_source_track =
      new rtc::RefCountedObject<webrtc::internal::CapturerTrackSource>(
          capturer_impl->video_capturer());
  scoped_refptr<RTCVideoSourceImpl> source = scoped_refptr<RTCVideoSourceImpl>(
      new RefCountedObject<RTCVideoSourceImpl>(rtc_source_track));
  return source;
}

#ifdef RTC_DESKTOP_DEVICE
scoped_refptr<RTCVideoSource> RTCPeerConnectionFactoryImpl::CreateDesktopSource(
    scoped_refptr<RTCDesktopCapturer> capturer,
    const string video_source_label,
    scoped_refptr<RTCMediaConstraints> constraints) {
  if (rtc::Thread::Current() != worker_thread_) {
    scoped_refptr<RTCVideoSource> source =
        worker_thread_->Invoke<scoped_refptr<RTCVideoSource>>(
            RTC_FROM_HERE, [this, capturer, video_source_label, constraints] {
              return CreateVideoSource_d(
                  capturer, to_std_string(video_source_label).c_str(),
                  constraints);
            });
    return source;
  }

  return CreateVideoSource_d(
      capturer, to_std_string(video_source_label).c_str(), constraints);
}

scoped_refptr<RTCVideoSource> RTCPeerConnectionFactoryImpl::CreateVideoSource_d(
    scoped_refptr<RTCDesktopCapturer> capturer,
    const char* video_source_label,
    scoped_refptr<RTCMediaConstraints> constraints) {
  RTCDesktopCapturerImpl* capturer_impl =
      static_cast<RTCDesktopCapturerImpl*>(capturer.get());

  rtc::scoped_refptr<webrtc::VideoTrackSourceInterface> rtc_source_track =
      new rtc::RefCountedObject<
          webrtc::internal::LocalDesktopCaptureTrackSource>(
          capturer_impl->desktop_capturer());

  scoped_refptr<RTCVideoSourceImpl> source = scoped_refptr<RTCVideoSourceImpl>(
      new RefCountedObject<RTCVideoSourceImpl>(rtc_source_track));

  return source;
}
#endif

scoped_refptr<RTCMediaStream> RTCPeerConnectionFactoryImpl::CreateStream(
    const string stream_id) {
  rtc::scoped_refptr<webrtc::MediaStreamInterface> rtc_stream =
      rtc_peerconnection_factory_->CreateLocalMediaStream(
          to_std_string(stream_id));

  scoped_refptr<MediaStreamImpl> stream = scoped_refptr<MediaStreamImpl>(
      new RefCountedObject<MediaStreamImpl>(rtc_stream));

  return stream;
}

scoped_refptr<RTCVideoTrack> RTCPeerConnectionFactoryImpl::CreateVideoTrack(
    scoped_refptr<RTCVideoSource> source,
    const string track_id) {
  scoped_refptr<RTCVideoSourceImpl> source_adapter(
      static_cast<RTCVideoSourceImpl*>(source.get()));

  rtc::scoped_refptr<webrtc::VideoTrackInterface> rtc_video_track =
      rtc_peerconnection_factory_->CreateVideoTrack(
          to_std_string(track_id), source_adapter->rtc_source_track());

  scoped_refptr<VideoTrackImpl> video_track = scoped_refptr<VideoTrackImpl>(
      new RefCountedObject<VideoTrackImpl>(rtc_video_track));

  // 	webrtc::VideoTrackProxyWithInternal<webrtc::VideoTrackInterface>
  // *track_proxy =
  // dynamic_cast<webrtc::VideoTrackProxyWithInternal<webrtc::VideoTrackInterface>
  // *>(video_track.get()); 	if (track_proxy) {
  // 		webrtc::MediaStreamTrack<VideoTrackInterface> *track =
  // dynamic_cast<webrtc::MediaStreamTrack<VideoTrackInterface>*>(track_proxy->internal());
  // 		LOG(INFO) << "VideoTrackInterface: " << track->id();
  // 	}

  return video_track;
}

scoped_refptr<RTCVideoTrack> RTCPeerConnectionFactoryImpl::CreateVideoTrack(
    std::unique_ptr<owt::base::VideoFrameGeneratorInterface>
        video_frame_genrator,
    const string track_id) {
  rtc::scoped_refptr<owt::base::LocalRawCaptureTrackSource> video_device =
      owt::base::LocalRawCaptureTrackSource::Create(
          std::move(video_frame_genrator));
  if (video_device == nullptr)
    return nullptr;

  rtc::scoped_refptr<webrtc::VideoTrackInterface> rtc_video_track =
      rtc_peerconnection_factory_->CreateVideoTrack(to_std_string(track_id),
                                                    video_device.get());

  scoped_refptr<VideoTrackImpl> video_track = scoped_refptr<VideoTrackImpl>(
      new RefCountedObject<VideoTrackImpl>(rtc_video_track));
  return video_track;
}

scoped_refptr<RTCVideoTrack> RTCPeerConnectionFactoryImpl::CreateVideoTrack(
    owt::base::VideoEncoderInterface* encoder,
    const string track_id) {
  rtc::scoped_refptr<owt::base::LocalEncodedCaptureTrackSource> video_device =
      owt::base::LocalEncodedCaptureTrackSource::Create(encoder);
  if (video_device == nullptr)
    return nullptr;

  rtc::scoped_refptr<webrtc::VideoTrackInterface> rtc_video_track =
      rtc_peerconnection_factory_->CreateVideoTrack(to_std_string(track_id),
                                                    video_device.get());

  scoped_refptr<VideoTrackImpl> video_track = scoped_refptr<VideoTrackImpl>(
      new RefCountedObject<VideoTrackImpl>(rtc_video_track));
  return video_track;
}

scoped_refptr<RTCAudioTrack> RTCPeerConnectionFactoryImpl::CreateAudioTrack(
    scoped_refptr<RTCAudioSource> source,
    const string track_id) {
  RTCAudioSourceImpl* source_impl =
      static_cast<RTCAudioSourceImpl*>(source.get());

  rtc::scoped_refptr<webrtc::AudioTrackInterface> audio_track(
      rtc_peerconnection_factory_->CreateAudioTrack(
          to_std_string(track_id), source_impl->rtc_audio_source()));

  scoped_refptr<AudioTrackImpl> track = scoped_refptr<AudioTrackImpl>(
      new RefCountedObject<AudioTrackImpl>(audio_track));
  return track;
}

RTCVideoRenderer<scoped_refptr<RTCVideoFrame>>*
RTCPeerConnectionFactoryImpl::CreateVideoD3D11Renderer(
    HWND hwnd,
    VideoFrameSizeChangeObserver* observer) {
  return new VideoD3D11Renderer(hwnd, observer);
}

}  // namespace libwebrtc
