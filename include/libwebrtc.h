#ifndef LIB_WEBRTC_HXX
#define LIB_WEBRTC_HXX

#include "rtc_peerconnection_factory.h"
#include "rtc_types.h"

namespace libwebrtc {

/**
 * @class LibWebRTC
 * @brief Provides static methods for initializing, creating and terminating
 * the WebRTC PeerConnectionFactory and threads.
 *
 * This class provides static methods for initializing, creating and terminating
 * the WebRTC PeerConnectionFactory and threads. These methods are thread-safe
 * and can be called from any thread. This class is not meant to be
 * instantiated.
 *
 */
class LibWebRTC {
 public:

   enum RTCLogLevel { kVebose = 0, kInfo, kWarning, kError, kNone, kApp };

  /**
   * @brief Initializes the WebRTC PeerConnectionFactory and threads.
   *
   * Initializes the WebRTC PeerConnectionFactory and threads. This method is
   * thread-safe and can be called from any thread. It initializes SSL and
   * creates three threads: worker_thread, signaling_thread and network_thread.
   *
   * @return true if initialization is successful, false otherwise.
   */
  LIB_WEBRTC_API static bool Initialize();

  /**
   * @brief Creates a new WebRTC PeerConnectionFactory.
   *
   * Creates a new WebRTC PeerConnectionFactory. This method is thread-safe and
   * can be called from any thread. It creates a new instance of the
   * RTCPeerConnectionFactoryImpl class and initializes it.
   *
   * @return A scoped_refptr object that points to the newly created
   * RTCPeerConnectionFactory.
   */
  LIB_WEBRTC_API static scoped_refptr<RTCPeerConnectionFactory>
  CreateRTCPeerConnectionFactory();

  /**
   * @brief Terminates the WebRTC PeerConnectionFactory and threads.
   *
   * Terminates the WebRTC PeerConnectionFactory and threads. This method is
   * thread-safe and can be called from any thread. It cleans up SSL and stops
   * and destroys the three threads: worker_thread, signaling_thread and
   * network_thread.
   *
   */
  LIB_WEBRTC_API static void Terminate();

  // Redirect the log stream to a file
  LIB_WEBRTC_API static void RedirectRTCLogToFile(int level,
                                                  const char* filepath);
  LIB_WEBRTC_API static void FlushLogToFile();

  // Change log severity level
  LIB_WEBRTC_API static void UpdateRTCLogLevel(int level);

  // Wrapper for C++ RTC_LOG(sev) macros.
  // Logs the log string to the webrtc logstream for the given severity.
  LIB_WEBRTC_API static void RTCLogEx(RTCLogLevel severity, const char* file,
                                      int line, const char* format, ...);

  // Returns the filename with the path prefix removed.
  LIB_WEBRTC_API static void RTCFileName(const char* file_path, char ret[128]);

  // Create random UUID
  LIB_WEBRTC_API static string CreateRandomUuid();
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

#define RTCLogApp(format, ...) \
  RTCLogFormat(libwebrtc::LibWebRTC::kApp, format, ##__VA_ARGS__)

#if !defined(NDEBUG)
#define RTCLogDebug(format, ...) RTCLogInfo(format, ##__VA_ARGS__)
#else
#define RTCLogDebug(format, ...) \
  do {                           \
  } while (false)
#endif

#define RTCLog(format, ...) RTCLogInfo(format, ##__VA_ARGS__)

#endif  // LIB_WEBRTC_HXX
