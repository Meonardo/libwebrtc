#ifndef LIB_WEBRTC_HXX
#define LIB_WEBRTC_HXX

#include "rtc_peerconnection_factory.h"
#include "rtc_types.h"

namespace libwebrtc {

class LibWebRTC {
 public:
  LIB_WEBRTC_API static bool Initialize();

  LIB_WEBRTC_API static scoped_refptr<RTCPeerConnectionFactory>
  CreateRTCPeerConnectionFactory();

  LIB_WEBRTC_API static void Terminate();

  LIB_WEBRTC_API static void RedirectRTCLogToFile(int level,
                                                  const char* filepath);

  LIB_WEBRTC_API static void UpdateRTCLogLevel(int level);

  LIB_WEBRTC_API static void ExecuteFuncOnWorkerThread(void (*func)(void*),
                                                       void* args);

  LIB_WEBRTC_API static void ExecuteFuncOnSignalingThread(void (*func)(void*),
                                                          void* args);

  LIB_WEBRTC_API static void AsyncExecuteFuncOnWorkerThread(void (*func)(void*),
                                                            void* args);

  LIB_WEBRTC_API static void AsyncExecuteFuncOnSignalingThread(
      void (*func)(void*),
      void* args);

 public:
  enum RTCLogLevel { kVebose = 0, kInfo, kWarning, kError, kNone };
  // Wrapper for C++ RTC_LOG(sev) macros.
  // Logs the log string to the webrtc logstream for the given severity.
  LIB_WEBRTC_API static void RTCLogEx(RTCLogLevel severity,
                                      const char* file,
                                      int line,
                                      const char* format,
                                      ...);

  // Returns the filename with the path prefix removed.
  LIB_WEBRTC_API static void RTCFileName(const char* file_path, char ret[128]);
};

}  // namespace libwebrtc

// Some convenience macros for logging
#define RTCLogFormat(severity, format, ...)                              \
  do {                                                                   \
    char filename[128] = {0};                                            \
    libwebrtc::LibWebRTC::RTCFileName(__FILE__, filename);               \
    libwebrtc::LibWebRTC::RTCLogEx(severity, filename, __LINE__, format, \
                                   ##__VA_ARGS__);                       \
  } while (false)

#define RTCLogVerbose(format, ...) \
  RTCLogFormat(libwebrtc::LibWebRTC::kVebose, format, ##__VA_ARGS__)

#define RTCLogInfo(format, ...) \
  RTCLogFormat(libwebrtc::LibWebRTC::kInfo, format, ##__VA_ARGS__)

#define RTCLogWarning(format, ...) \
  RTCLogFormat(libwebrtc::LibWebRTC::kWarning, format, ##__VA_ARGS__)

#define RTCLogError(format, ...) \
  RTCLogFormat(libwebrtc::LibWebRTC::kError, format, ##__VA_ARGS__)

#if !defined(NDEBUG)
#define RTCLogDebug(format, ...) RTCLogInfo(format, ##__VA_ARGS__)
#else
#define RTCLogDebug(format, ...) \
  do {                           \
  } while (false)
#endif

#define RTCLog(format, ...) RTCLogInfo(format, ##__VA_ARGS__)


#endif  // LIB_WEBRTC_HXX
