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
};

}  // namespace libwebrtc

#endif  // LIB_WEBRTC_HXX
