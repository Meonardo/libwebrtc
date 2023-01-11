#ifndef LIB_WEBRTC_RTC_PEERCONNECTION_FACTORY_HXX
#define LIB_WEBRTC_RTC_PEERCONNECTION_FACTORY_HXX

#include "rtc_types.h"

#include "rtc_audio_source.h"
#include "rtc_audio_track.h"
#ifdef RTC_DESKTOP_DEVICE
#include "rtc_desktop_device.h"
#endif
#include "rtc_media_stream.h"
#include "rtc_mediaconstraints.h"
#include "rtc_video_device.h"
#include "rtc_video_source.h"

#include "framegeneratorinterface.h"
#include "videoencoderinterface.h"

namespace libwebrtc {

class RTCPeerConnection;
class RTCAudioDevice;
class RTCVideoDevice;

class GlobalConfiguration {
 public:
  LIB_WEBRTC_API static void SetVideoHardwareAccelerationEnabled(bool enabled);
  LIB_WEBRTC_API static bool GetVideoHardwareAccelerationEnabled();
  LIB_WEBRTC_API static void SetCustomizedVideoEncoderEnabled(bool enabled);
  LIB_WEBRTC_API static bool GetCustomizedVideoEncoderEnabled();
  LIB_WEBRTC_API static void SetCustomizedAudioInputEnabled(
      bool enabled,
      std::shared_ptr<owt::base::AudioFrameGeneratorInterface> audio_framer);
  LIB_WEBRTC_API static bool GetCustomizedAudioInputEnabled();
  LIB_WEBRTC_API static std::shared_ptr<owt::base::AudioFrameGeneratorInterface>
  GetAudioFrameGenerator();
};

class VideoFrameSizeChangeObserver {
 public:
  virtual void OnVideoFrameSizeChanged(HWND hwnd,
                                       uint16_t width,
                                       uint16_t height) = 0;
};

class RTCPeerConnectionFactory : public RefCountInterface {
 public:
  virtual bool Initialize() = 0;

  virtual bool Terminate() = 0;

  virtual scoped_refptr<RTCPeerConnection> Create(
      const RTCConfiguration& configuration,
      scoped_refptr<RTCMediaConstraints> constraints) = 0;

  virtual void Delete(scoped_refptr<RTCPeerConnection> peerconnection) = 0;

  virtual scoped_refptr<RTCAudioDevice> GetAudioDevice() = 0;

  virtual scoped_refptr<RTCVideoDevice> GetVideoDevice() = 0;
#ifdef RTC_DESKTOP_DEVICE
  virtual scoped_refptr<RTCDesktopDevice> GetDesktopDevice() = 0;
#endif
  virtual scoped_refptr<RTCAudioSource> CreateAudioSource(
      const string audio_source_label) = 0;

  virtual scoped_refptr<RTCVideoSource> CreateVideoSource(
      scoped_refptr<RTCVideoCapturer> capturer,
      const string video_source_label,
      scoped_refptr<RTCMediaConstraints> constraints) = 0;
#ifdef RTC_DESKTOP_DEVICE
  virtual scoped_refptr<RTCVideoSource> CreateDesktopSource(
      scoped_refptr<RTCDesktopCapturer> capturer,
      const string video_source_label,
      scoped_refptr<RTCMediaConstraints> constraints) = 0;
  virtual scoped_refptr<RTCVideoSource> CreateDesktopSource(
      scoped_refptr<RTCDesktopCapturer2> capturer,
      const string video_source_label,
      scoped_refptr<RTCMediaConstraints> constraints) = 0;
#endif
  virtual scoped_refptr<RTCAudioTrack> CreateAudioTrack(
      scoped_refptr<RTCAudioSource> source,
      const string track_id) = 0;

  virtual scoped_refptr<RTCVideoTrack> CreateVideoTrack(
      scoped_refptr<RTCVideoSource> source,
      const string track_id) = 0;

  // customized raw video frame track
  virtual scoped_refptr<RTCVideoTrack> CreateVideoTrack(
      owt::base::VideoFrameGeneratorInterface* v_frame_genrator,
      const string track_id) = 0;
  // customized encoded video packet track
  virtual scoped_refptr<RTCVideoTrack> CreateVideoTrack(
      owt::base::VideoEncoderInterface* encoder,
      const string track_id) = 0;

  virtual scoped_refptr<RTCMediaStream> CreateStream(
      const string stream_id) = 0;

  virtual RTCVideoRenderer<scoped_refptr<RTCVideoFrame>>*
  CreateVideoD3D11Renderer(HWND hwnd,
                           VideoFrameSizeChangeObserver* observer) = 0;
};

}  // namespace libwebrtc

#endif  // LIB_WEBRTC_RTC_PEERCONNECTION_FACTORY_HXX
