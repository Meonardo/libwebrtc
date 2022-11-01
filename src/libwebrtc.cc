#include "libwebrtc.h"

#include "api/scoped_refptr.h"
#include "rtc_base/logging.h"
#include "rtc_base/ssl_adapter.h"
#include "rtc_base/thread.h"

#include "rtc_peerconnection_factory_impl.h"

namespace libwebrtc {

static bool g_is_initialized = false;
std::unique_ptr<rtc::Thread> worker_thread;
std::unique_ptr<rtc::Thread> signaling_thread;
std::unique_ptr<rtc::Thread> network_thread;

bool LibWebRTC::Initialize() {
  if (!g_is_initialized) {
    rtc::InitializeSSL();
    g_is_initialized = true;

    if (worker_thread.get() == nullptr) {
      worker_thread = rtc::Thread::Create();
      worker_thread->SetName("worker_thread", nullptr);
      RTC_CHECK(worker_thread->Start()) << "Failed to start thread";
    }

    if (signaling_thread.get() == nullptr) {
      signaling_thread = rtc::Thread::Create();
      signaling_thread->SetName("signaling_thread", NULL);
      RTC_CHECK(signaling_thread->Start()) << "Failed to start thread";
    }

    if (network_thread.get() == nullptr) {
      network_thread = rtc::Thread::CreateWithSocketServer();
      network_thread->SetName("network_thread", nullptr);
      RTC_CHECK(network_thread->Start()) << "Failed to start thread";
    }
  }
  return g_is_initialized;
}

void LibWebRTC::Terminate() {
  rtc::ThreadManager::Instance()->SetCurrentThread(NULL);
  rtc::CleanupSSL();

  if (worker_thread.get() != nullptr) {
    worker_thread->Stop();
    worker_thread.reset(nullptr);
  }

  if (signaling_thread.get() != nullptr) {
    signaling_thread->Stop();
    signaling_thread.reset(nullptr);
  }

  if (network_thread.get() != nullptr) {
    network_thread->Stop();
    network_thread.reset(nullptr);
  }

  g_is_initialized = false;
}

scoped_refptr<RTCPeerConnectionFactory>
LibWebRTC::CreateRTCPeerConnectionFactory() {
  scoped_refptr<RTCPeerConnectionFactory> rtc_peerconnection_factory =
      scoped_refptr<RTCPeerConnectionFactory>(
          new RefCountedObject<RTCPeerConnectionFactoryImpl>(
              worker_thread.get(), signaling_thread.get(),
              network_thread.get()));
  rtc_peerconnection_factory->Initialize();
  return rtc_peerconnection_factory;
}

// #include "rtc_base/logging.h"
// The meanings of the levels are:
//  LS_VERBOSE: This level is for data which we do not want to appear in the
//   normal debug log, but should appear in diagnostic logs.
//  LS_INFO: Chatty level used in debugging for all sorts of things, the default
//   in debug builds.
//  LS_WARNING: Something that may warrant investigation.
//  LS_ERROR: Something that should not have occurred.
//  LS_NONE: Don't log.
// enum LoggingSeverity {
//  LS_VERBOSE,
//  LS_INFO,
//  LS_WARNING,
//  LS_ERROR,
//  LS_NONE,
//};

void LibWebRTC::UpdateRTCLogLevel(int level) {
  rtc::LogMessage::LogToDebug((rtc::LoggingSeverity)level);
}

void LibWebRTC::ExecuteFuncOnWorkerThread(void (*func)(void*), void* args) {
  worker_thread->Invoke<void>(RTC_FROM_HERE, [&]() { func(args); });
}

void LibWebRTC::ExecuteFuncOnSignalingThread(void (*func)(void*), void* args) {
  signaling_thread->Invoke<void>(RTC_FROM_HERE, [&]() { func(args); });
}

void LibWebRTC::AsyncExecuteFuncOnWorkerThread(void (*func)(void*), void* args) {
  worker_thread->PostTask(RTC_FROM_HERE, [&]() { func(args); });
}

void LibWebRTC::AsyncExecuteFuncOnSignalingThread(void (*func)(void*),
                                                  void* args) {
  signaling_thread->PostTask(RTC_FROM_HERE, [&]() { func(args); });
}

}  // namespace libwebrtc
