#ifndef LIB_WEBRTC_RTC_DESKTOP_DEVICE_HXX
#define LIB_WEBRTC_RTC_DESKTOP_DEVICE_HXX

#include "rtc_types.h"

namespace libwebrtc {

class MediaSource;
class RTCDesktopCapturer;
class RTCDesktopMediaList;

class LocalDesktopCapturerObserver {
 public:
  /**
  @brief Event callback for local screen stream to request for a source from
  application.
  @details After local stream is started, this callback will be invoked to
  request for a source from application.
  @param sources list of windows/screen's (id, title) pair.
  @param dest_source application will set this id to be used by it.
  */
  virtual void OnCaptureSourceNeeded(const SourceList& sources,
                                     int& dest_source) {}
  virtual ~LocalDesktopCapturerObserver() {}
};

/**
@brief This class contains parameters and methods that's needed for creating a
local stream with certain screen or window as source.
When a stream is created, it will not be impacted if these parameters are
changed.
*/
class LocalDesktopCapturerParameters final {
 public:
  enum class DesktopCapturePolicy : int {
    /// Default capture policy.
    kDefault = 0,
    /// With this policy set, on windows, use DirectX for desktop capture if
    /// possisble.
    kEnableDirectX = 1,
    /// With this policy set, enable platform specific window effects if
    /// possible.
    kEnableEffects = 2,
    /// With this policy set, capturer will provide update region information to
    /// caller.
    kEnableUpdateDetection = 4,
    /// With this policy set, capturer can send out scaled captured frame.
    kEnableMagnification = 8
  };
  enum class DesktopSourceType : int {
    /// Capture from whole screen
    kFullScreen = 1,
    /// Capture from application
    kApplication
  };
  /**
  @brief Initialize a LocalDesktopCapturerParameters.
  @param audio_enabled Indicates if audio is enabled for this stream.
  @param video_anabled Indicates if video is enabled for this stream.
  @param soruce_type Indicates if capture from screen or an app.
  @param capture_policy the OR of any of the DesktopCapturePolicy options.
  */
  LocalDesktopCapturerParameters(bool audio_enabled,
                                 bool video_enabled,
                                 bool cursor_enabled)
      : video_enabled_(video_enabled),
        audio_enabled_(audio_enabled),
        cursor_enabled_(cursor_enabled),
        fps_(30),
        source_type_(DesktopSourceType::kFullScreen),
        capture_policy_(DesktopCapturePolicy::kDefault) {}

  ~LocalDesktopCapturerParameters() {}
  /**
  @brief Get video is enabled or not for this stream.
  @return true or false.
  */
  bool VideoEnabled() const { return video_enabled_; }
  /**
  @brief Get audio is enabled or not for this stream.
  @return true or false.
  */
  bool AudioEnabled() const { return audio_enabled_; }
  /// Get mouse cursor enabled state.
  bool CursorEnabled() const { return cursor_enabled_; }
  /**
    @brief Set the source type of screen/app sharing
    @param source_type Indicate if capturing the full screen
    or an application.
  */
  void SourceType(DesktopSourceType source_type) { source_type_ = source_type; }
  /**
    @brief Set the capturer features
    @params capture_policy Indicate the feature used by the capturer.
  */
  void CapturePolicy(DesktopCapturePolicy capture_policy) {
    capture_policy_ = capture_policy;
  }
  /**
    @brief Set the frame rate.
     The actual frame rate of window/screen capturing may be lower than this.
    @param fps The frame rate of the captured screen/window.
  */
  void Fps(int fps) { fps_ = fps; }
  /** @cond */
  int Fps() const { return fps_; }

  DesktopSourceType SourceType() const { return source_type_; }
  DesktopCapturePolicy CapturePolicy() const { return capture_policy_; }
  /** @endcond */
 private:
  bool video_enabled_;
  bool audio_enabled_;
  bool cursor_enabled_;
  int fps_;
  DesktopSourceType source_type_;
  DesktopCapturePolicy capture_policy_;
};

class RTCDesktopDevice : public RefCountInterface {
 public:
  virtual scoped_refptr<RTCDesktopCapturer> CreateDesktopCapturer(
      scoped_refptr<MediaSource> source) = 0;
  virtual scoped_refptr<RTCDesktopMediaList> GetDesktopMediaList(
      DesktopType type) = 0;

 protected:
  virtual ~RTCDesktopDevice() {}
};

}  // namespace libwebrtc

#endif  // LIB_WEBRTC_RTC_VIDEO_DEVICE_HXX