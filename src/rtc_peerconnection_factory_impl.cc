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
#include "modules/audio_device/audio_device_impl.h"
#include "pc/peer_connection_factory.h"
#include "pc/peer_connection_factory_proxy.h"

#include "modules/audio_device/include/audio_device_factory.h"

#include "src/customizedvideoencoderfactory.h"
#include "src/win/customizedframescapturer.h"
#include "src/win/customizedvideosource.h"
#include "src/win/mediacapabilities.h"
#include "src/win/msdkvideodecoderfactory.h"
#include "src/win/msdkvideoencoderfactory.h"

#if defined(WEBRTC_IOS)
#include "engine/sdk/objc/Framework/Classes/videotoolboxvideocodecfactory.h"
#endif
#include <api/task_queue/default_task_queue_factory.h>

namespace libwebrtc {

static bool hardware_acceleration_enabled_;
static bool customized_video_encoder_enabled_;
static bool customized_audio_input_enabled_;
static std::shared_ptr<owt::base::AudioFrameGeneratorInterface> audio_framer_ =
    nullptr;

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

void GlobalConfiguration::SetCustomizedAudioInputEnabled(bool enabled) {
  customized_audio_input_enabled_ = enabled;
}

bool GlobalConfiguration::GetCustomizedAudioInputEnabled() {
  return customized_audio_input_enabled_;
}

std::unique_ptr<webrtc::VideoEncoderFactory> CreateCustomVideoEncoderFactory() {
  return std::make_unique<CustomizedEncodedVideoEncoderFactory>();
}

webrtc::VideoEncoderFactory*
RTCPeerConnectionFactoryImpl::GetVideoEncoderFactory() const {
  if (rtc_peerconnection_factory_ == nullptr)
    return nullptr;
  auto pcf_proxy = static_cast<webrtc::PeerConnectionFactoryProxy*>(
      rtc_peerconnection_factory_.get());
  if (pcf_proxy == nullptr)
    return nullptr;

  auto pcf = static_cast<webrtc::PeerConnectionFactory*>(pcf_proxy->internal());
  if (pcf == nullptr)
    return nullptr;

  auto encoder_factory = pcf->media_engine()->video().encoder_factory();
  return encoder_factory;
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
          audio_device_module_, webrtc::CreateBuiltinAudioEncoderFactory(),
          webrtc::CreateBuiltinAudioDecoderFactory(),
          CreateCustomVideoEncoderFactory(), CreateIntelVideoDecoderFactory(),
          nullptr, nullptr);
    } else if (GlobalConfiguration::GetVideoHardwareAccelerationEnabled()) {
      rtc_peerconnection_factory_ = webrtc::CreatePeerConnectionFactory(
          network_thread_, worker_thread_, signaling_thread_,
          audio_device_module_, webrtc::CreateBuiltinAudioEncoderFactory(),
          webrtc::CreateBuiltinAudioDecoderFactory(),
          CreateIntelVideoEncoderFactory(), CreateIntelVideoDecoderFactory(),
          nullptr, nullptr);
    } else {
      rtc_peerconnection_factory_ = webrtc::CreatePeerConnectionFactory(
          network_thread_, worker_thread_, signaling_thread_,
          audio_device_module_, webrtc::CreateBuiltinAudioEncoderFactory(),
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
    if (GlobalConfiguration::GetCustomizedAudioInputEnabled()) {
      audio_device_module_ =
          worker_thread_->Invoke<rtc::scoped_refptr<webrtc::AudioDeviceModule>>(
              RTC_FROM_HERE, [] {
                return owt::base::CustomizedAudioDeviceModule::Create(nullptr);
              });
    } else {
      com_initializer_ = std::make_unique<webrtc::ScopedCOMInitializer>(
          webrtc::ScopedCOMInitializer::kMTA);
      if (com_initializer_->Succeeded()) {
        audio_device_module_ = webrtc::CreateWindowsCoreAudioAudioDeviceModule(
            task_queue_factory_.get());
      }
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

scoped_refptr<RTCAudioSource>
RTCPeerConnectionFactoryImpl::CreateCustomAudioSource(
    const string audio_source_label) {
  rtc::scoped_refptr<RTCCustomAudioSource> audio_source =
      RTCCustomAudioSource::Create();

  scoped_refptr<RTCAudioSourceImpl> source = scoped_refptr<RTCAudioSourceImpl>(
      new RefCountedObject<RTCAudioSourceImpl>(audio_source));
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

scoped_refptr<RTCDesktopDevice>
RTCPeerConnectionFactoryImpl::CreateDesktopDevice() {
  return scoped_refptr<RTCDesktopDeviceImpl>(
      new RefCountedObject<RTCDesktopDeviceImpl>(signaling_thread_));
}
#endif

scoped_refptr<RTCVideoSource> RTCPeerConnectionFactoryImpl::CreateVideoSource(
    scoped_refptr<RTCVideoCapturer> capturer,
    const string video_source_label,
    scoped_refptr<RTCMediaConstraints> constraints) {
  if (rtc::Thread::Current() != signaling_thread_) {
    scoped_refptr<RTCVideoSource> source =
        signaling_thread_->Invoke<scoped_refptr<RTCVideoSource>>(
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

  auto capturer_source_track =
      new rtc::RefCountedObject<webrtc::internal::CapturerTrackSource>(
          capturer_impl->video_capturer());
  // save the capturer source to the capturer its self
  capturer_impl->SaveVideoSourceTrack(
      rtc::scoped_refptr<webrtc::internal::CapturerTrackSource>(
          capturer_source_track));

  rtc::scoped_refptr<webrtc::VideoTrackSourceInterface> rtc_source_track =
      rtc::scoped_refptr<webrtc::VideoTrackSourceInterface>(
          capturer_source_track);
  scoped_refptr<RTCVideoSourceImpl> source = scoped_refptr<RTCVideoSourceImpl>(
      new RefCountedObject<RTCVideoSourceImpl>(rtc_source_track));
  return source;
}

#ifdef RTC_DESKTOP_DEVICE
scoped_refptr<RTCVideoSource> RTCPeerConnectionFactoryImpl::CreateDesktopSource(
    scoped_refptr<RTCDesktopCapturer> capturer,
    const string video_source_label,
    scoped_refptr<RTCMediaConstraints> constraints) {
  if (rtc::Thread::Current() != signaling_thread_) {
    scoped_refptr<RTCVideoSource> source =
        signaling_thread_->Invoke<scoped_refptr<RTCVideoSource>>(
            RTC_FROM_HERE, [this, capturer, video_source_label, constraints] {
              return CreateDesktopSource_d(
                  capturer, to_std_string(video_source_label).c_str(),
                  constraints);
            });
    return source;
  }

  return CreateDesktopSource_d(
      capturer, to_std_string(video_source_label).c_str(), constraints);
}

scoped_refptr<RTCVideoSource>
RTCPeerConnectionFactoryImpl::CreateDesktopSource_d(
    scoped_refptr<RTCDesktopCapturer> capturer,
    const char* video_source_label,
    scoped_refptr<RTCMediaConstraints> constraints) {
  rtc::scoped_refptr<webrtc::VideoTrackSourceInterface> rtc_source_track =
      rtc::scoped_refptr<webrtc::VideoTrackSourceInterface>(
          new rtc::RefCountedObject<ScreenCapturerTrackSource>(capturer));

  scoped_refptr<RTCVideoSourceImpl> source = scoped_refptr<RTCVideoSourceImpl>(
      new RefCountedObject<RTCVideoSourceImpl>(rtc_source_track));

  return source;
}

scoped_refptr<RTCVideoSource> RTCPeerConnectionFactoryImpl::CreateDesktopSource(
    scoped_refptr<RTCDesktopCapturer2> capturer,
    const string video_source_label,
    scoped_refptr<RTCMediaConstraints> constraints) {
  if (rtc::Thread::Current() != signaling_thread_) {
    scoped_refptr<RTCVideoSource> source =
        signaling_thread_->Invoke<scoped_refptr<RTCVideoSource>>(
            RTC_FROM_HERE, [this, capturer, video_source_label, constraints] {
              return CreateDesktopSource_d(
                  capturer, to_std_string(video_source_label).c_str(),
                  constraints);
            });
    return source;
  }

  return CreateDesktopSource_d(
      capturer, to_std_string(video_source_label).c_str(), constraints);
}

scoped_refptr<RTCVideoSource>
RTCPeerConnectionFactoryImpl::CreateDesktopSource_d(
    scoped_refptr<RTCDesktopCapturer2> capturer,
    const char* video_source_label,
    scoped_refptr<RTCMediaConstraints> constraints) {
  RTCDesktopCapturerImpl2* capturer_impl =
      static_cast<RTCDesktopCapturerImpl2*>(capturer.get());

  auto capturer_source_track = new rtc::RefCountedObject<
      webrtc::internal::LocalDesktopCaptureTrackSource>(
      capturer_impl->desktop_capturer());
  // save the capturer source to the capturer its self
  capturer_impl->SaveVideoSourceTrack(
      rtc::scoped_refptr<webrtc::internal::LocalDesktopCaptureTrackSource>(
          capturer_source_track));

  rtc::scoped_refptr<webrtc::VideoTrackSourceInterface> rtc_source_track =
      rtc::scoped_refptr<webrtc::VideoTrackSourceInterface>(
          capturer_source_track);

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
          to_std_string(track_id), source_adapter->rtc_source_track().get());

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
    owt::base::VideoFrameGeneratorInterface* video_frame_genrator,
    const string track_id) {
  rtc::scoped_refptr<owt::base::LocalRawCaptureTrackSource> video_device =
      owt::base::LocalRawCaptureTrackSource::Create(video_frame_genrator);
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
          to_std_string(track_id), source_impl->rtc_audio_source().get()));

  scoped_refptr<AudioTrackImpl> track = scoped_refptr<AudioTrackImpl>(
      new RefCountedObject<AudioTrackImpl>(audio_track));
  return track;
}

scoped_refptr<RTCAudioTrack>
RTCPeerConnectionFactoryImpl::CreateCustomAudioTrack(
    scoped_refptr<RTCAudioSource> source,
    const string track_id) {
  RTCAudioSourceImpl* source_impl =
      static_cast<RTCAudioSourceImpl*>(source.get());

  rtc::scoped_refptr<webrtc::AudioTrackInterface> audio_track(
      rtc_peerconnection_factory_->CreateAudioTrack(
          to_std_string(track_id),
          source_impl->customized_audio_source().get()));

  scoped_refptr<AudioTrackImpl> track = scoped_refptr<AudioTrackImpl>(
      new RefCountedObject<AudioTrackImpl>(audio_track));
  return track;
}

RTCVideoRenderer<scoped_refptr<RTCVideoFrame>>*
RTCPeerConnectionFactoryImpl::CreateVideoD3D11Renderer(
    HWND hwnd,
    VideoFrameSizeChangeObserver* observer) {
  if (rtc::Thread::Current() == worker_thread_) {
    return new VideoD3D11Renderer(hwnd, observer);
  }
  return worker_thread_
      ->Invoke<RTCVideoRenderer<scoped_refptr<RTCVideoFrame>>*>(
          RTC_FROM_HERE,
          [hwnd, observer] { return new VideoD3D11Renderer(hwnd, observer); });
}

void RTCPeerConnectionFactoryImpl::DestroyVideoD3D11Renderer(
    RTCVideoRenderer<scoped_refptr<RTCVideoFrame>>* renderer) {
  if (renderer == nullptr)
    return;
  if (rtc::Thread::Current() == worker_thread_) {
    delete renderer;
    renderer = nullptr;
  } else {
    worker_thread_->Invoke<void>(RTC_FROM_HERE,
                                 [renderer] { delete renderer; });
  }
}

bool RTCPeerConnectionFactoryImpl::ForceUsingEncodedVideoEncoder() {
  if (!GlobalConfiguration::GetCustomizedVideoEncoderEnabled()) {
    return false;
  }
  auto video_encoder_factory = GetVideoEncoderFactory();
  if (video_encoder_factory == nullptr) {
    return false;
  }

  auto customized_factory =
      static_cast<CustomizedEncodedVideoEncoderFactory*>(video_encoder_factory);
  if (customized_factory == nullptr) {
    return false;
  }
  customized_factory->ForceUsingEncodedEncoder();

  return true;
}

bool RTCPeerConnectionFactoryImpl::RestoreNormalVideoEncoder() {
  if (!GlobalConfiguration::GetCustomizedVideoEncoderEnabled()) {
    return false;
  }
  auto video_encoder_factory = GetVideoEncoderFactory();
  if (video_encoder_factory == nullptr) {
    return false;
  }

  auto customized_factory =
      static_cast<CustomizedEncodedVideoEncoderFactory*>(video_encoder_factory);
  if (customized_factory == nullptr) {
    return false;
  }
  customized_factory->RestoreUsingNormalEncoder();

  return true;
}

bool RTCPeerConnectionFactoryImpl::ForceUsingScreencastConfig() {
  if (!GlobalConfiguration::GetCustomizedVideoEncoderEnabled()) {
    return false;
  }
  auto video_encoder_factory = GetVideoEncoderFactory();
  if (video_encoder_factory == nullptr) {
    return false;
  }

  auto customized_factory =
      static_cast<CustomizedEncodedVideoEncoderFactory*>(video_encoder_factory);
  if (customized_factory == nullptr) {
    return false;
  }
  customized_factory->ForceUsingScreencastConfig();

  return true;
}

}  // namespace libwebrtc
