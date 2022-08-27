#ifndef LIB_WEBRTC_VIDEO_SINK_ADPTER_HXX
#define LIB_WEBRTC_VIDEO_SINK_ADPTER_HXX

#include "rtc_peerconnection.h"
#include "rtc_video_renderer.h"
#include "rtc_video_frame.h"

#include "api/media_stream_interface.h"
#include "api/peer_connection_interface.h"
#include "rtc_base/synchronization/mutex.h"

#include "src/win/videorendererd3d11.h"

namespace libwebrtc {

class VideoD3D11Renderer
        : public RTCVideoRenderer<scoped_refptr<RTCVideoFrame>> {
 public:
  VideoD3D11Renderer(HWND hwnd) {
    d3d11_renderer_impl_ =
        scoped_refptr<owt::base::WebrtcVideoRendererD3D11Impl>(
            new RefCountedObject<owt::base::WebrtcVideoRendererD3D11Impl>(
                hwnd));
  }

  ~VideoD3D11Renderer() {}

  void OnFrame(scoped_refptr<RTCVideoFrame> frame) override {
    webrtc::VideoFrameBuffer* video_frame_buffer =
        reinterpret_cast<webrtc::VideoFrameBuffer*>(frame->RawBuffer());
    d3d11_renderer_impl_->OnFrame(video_frame_buffer);
  }

  private:
  scoped_refptr<owt::base::WebrtcVideoRendererD3D11Impl> d3d11_renderer_impl_;
};

class VideoSinkAdapter : public rtc::VideoSinkInterface<webrtc::VideoFrame>,
                         public RefCountInterface {
 public:
  VideoSinkAdapter(rtc::scoped_refptr<webrtc::VideoTrackInterface> track);
  ~VideoSinkAdapter() override;

  virtual void AddRenderer(
      RTCVideoRenderer<scoped_refptr<RTCVideoFrame>>* renderer);

  virtual void RemoveRenderer(
      RTCVideoRenderer<scoped_refptr<RTCVideoFrame>>* renderer);

  virtual void AddRenderer(
      rtc::VideoSinkInterface<webrtc::VideoFrame>* renderer);

  virtual void RemoveRenderer(
      rtc::VideoSinkInterface<webrtc::VideoFrame>* renderer);

 protected:
  // VideoSinkInterface implementation
  void OnFrame(const webrtc::VideoFrame& frame) override;
  rtc::scoped_refptr<webrtc::VideoTrackInterface> rtc_track_;
  std::unique_ptr<webrtc::Mutex> crt_sec_;
  std::vector<RTCVideoRenderer<scoped_refptr<RTCVideoFrame>>*> renderers_;
};

}  // namespace libwebrtc

#endif  // LIB_WEBRTC_VIDEO_SINK_ADPTER_HXX
