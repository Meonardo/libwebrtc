#ifndef LIB_WEBRTC_RTC_DESKTOP_DEVICE_HXX
#define LIB_WEBRTC_RTC_DESKTOP_DEVICE_HXX

#include "rtc_types.h"

namespace libwebrtc {

class MediaSource;
class RTCDesktopCapturer;
class RTCDesktopCapturer2;
class RTCDesktopMediaList;

class LocalDesktopCapturerDataSource {
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
                                     int& dest_source) = 0;
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
    /// With this policy set, on windows, use DirectX for desktop capture if
    /// possisble.
    kEnableDirectX = 1,
    /// Use wgc capture if possible(this will show a yellow rectangle in Windows
    /// 10).
    kEnableWGC = 2,
  };
  enum class DesktopSourceType : int {
    /// Capture from whole screen
    kFullScreen = 1,
    /// Capture from application
    kApplication
  };

  LocalDesktopCapturerParameters(bool cursor_enabled)
      : cursor_enabled_(cursor_enabled),
        source_type_(DesktopSourceType::kFullScreen),
        capture_policy_(DesktopCapturePolicy::kEnableDirectX),
        fps_(30),
        width_(0),
        height_(0),
        max_bitrate_(6000),
        min_bitrate_(3000) {
    encoded_file_path_ = new char[512];
    memset(encoded_file_path_, 0, strlen(encoded_file_path_));
  }

  ~LocalDesktopCapturerParameters() { delete[] encoded_file_path_; }

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

  /// @brief scale width
  /// @param w
  void SetWidth(int w) { width_ = w; }
  int Width() const { return width_; }

  /// @brief scale height
  /// @param h
  void SetHeight(int h) { height_ = h; }
  int Height() const { return height_; }

  /// @brief the path for encoder to save the ivf file
  /// @param save_path
  void SetEncodedFilePath(const char* save_path) {
    if (save_path == nullptr)
      return;

    // reset first
    memset(encoded_file_path_, 0, strlen(encoded_file_path_));
    const size_t len = strlen(save_path);
    // copy
    strncpy_s(encoded_file_path_, len, save_path, len);
    encoded_file_path_[len] = '\0';
  }
  const char* EncodedFilePath() const { return encoded_file_path_; }
  // max bitrate for encoding
  void SetMaxBitrate(int bitrate) { max_bitrate_ = bitrate; }
  int MaxBitrate() const { return max_bitrate_; }
  // min bitrate for encoding
  void SetMinBitrate(int bitrate) { min_bitrate_ = bitrate; }
  int MinBitrate() const { return min_bitrate_; }

  DesktopSourceType SourceType() const { return source_type_; }
  DesktopCapturePolicy CapturePolicy() const { return capture_policy_; }

 private:
  bool cursor_enabled_;
  DesktopSourceType source_type_;
  DesktopCapturePolicy capture_policy_;
  // frame info
  int fps_;
  int width_;
  int height_;
  // max & min bitrate(kilobits/sec)
  int max_bitrate_;
  int min_bitrate_;
  // encoded video file saving path
  char* encoded_file_path_;
};

class RTCDesktopDevice : public RefCountInterface {
 public:
  virtual scoped_refptr<RTCDesktopCapturer> CreateDesktopCapturer(
      scoped_refptr<MediaSource> source) = 0;
  virtual scoped_refptr<RTCDesktopMediaList> GetDesktopMediaList(
      DesktopType type) = 0;

  virtual scoped_refptr<RTCDesktopCapturer2> CreateDesktopCapturer(
      LocalDesktopCapturerDataSource* datasource,
      std::shared_ptr<LocalDesktopCapturerParameters> params) = 0;

 protected:
  virtual ~RTCDesktopDevice() {}
};

}  // namespace libwebrtc

#endif  // LIB_WEBRTC_RTC_VIDEO_DEVICE_HXX