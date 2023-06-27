#include "libwebrtc.h"

#include "api/scoped_refptr.h"
#include "rtc_base/helpers.h"
#include "rtc_base/logging.h"
#include "rtc_base/ssl_adapter.h"
#include "rtc_base/string_utils.h"
#include "rtc_base/thread.h"

#include "rtc_peerconnection_factory_impl.h"

#include <filesystem>
#include <fstream>

#define MAX_LOG_BUF_SIZE 5120

namespace libwebrtc {
class CustomnLogSink;
// global values
static bool g_is_initialized = false;
std::unique_ptr<rtc::Thread> worker_thread;
std::unique_ptr<rtc::Thread> signaling_thread;
std::unique_ptr<rtc::Thread> network_thread;
std::unique_ptr<CustomnLogSink> log_sink;

// customized log sink
class CustomnLogSink : public rtc::LogSink {
 public:
  CustomnLogSink(const std::string& filepath) { file_stream_.open(filepath); }

  virtual void OnLogMessage(const std::string& message) override {
    file_stream_ << message;
  }

 private:
  std::ofstream file_stream_;
};

// LibWebRTC class defines
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

  if (log_sink.get() != nullptr) {
    rtc::LogMessage::RemoveLogToStream(log_sink.get());
    log_sink.reset(nullptr);
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

void LibWebRTC::RedirectRTCLogToFile(int level, const char* filepath) {
  std::string path(filepath);
  if (path.empty()) {
    return;
  }

  if (log_sink.get() == nullptr) {
    log_sink = std::make_unique<CustomnLogSink>(path);
  }

  rtc::LogMessage::AddLogToStream(log_sink.get(), (rtc::LoggingSeverity)level);
}

// log level see enum: `RTCLogLevel`
void LibWebRTC::UpdateRTCLogLevel(int level) {
  rtc::LogMessage::LogToDebug((rtc::LoggingSeverity)level);
}

void LibWebRTC::ExecuteFuncOnWorkerThread(void (*func)(void*), void* args) {
  worker_thread->Invoke<void>(RTC_FROM_HERE, [&]() { func(args); });
}

void LibWebRTC::ExecuteFuncOnSignalingThread(void (*func)(void*), void* args) {
  signaling_thread->Invoke<void>(RTC_FROM_HERE, [&]() { func(args); });
}

void LibWebRTC::AsyncExecuteFuncOnWorkerThread(void (*func)(void*),
                                               void* args) {
  worker_thread->PostTask([func, args]() { func(args); });
}

void LibWebRTC::AsyncExecuteFuncOnSignalingThread(void (*func)(void*),
                                                  void* args) {
  signaling_thread->PostTask([func, args]() { func(args); });
}

// custom log methods
void LibWebRTC::RTCLogEx(RTCLogLevel severity,
                         const char* file,
                         int line,
                         const char* format,
                         ...) {
  va_list args;

  va_start(args, format);

  char log_string[MAX_LOG_BUF_SIZE];
  vsnprintf(log_string, sizeof(log_string), format, args);
  RTC_LOG_N_LINE((rtc::LoggingSeverity)severity, file, line) << log_string;

  va_end(args);
}

void LibWebRTC::RTCFileName(const char* file_path, char ret[128]) {
  std::filesystem::path path(file_path);
  auto std_path = rtc::ToUtf8(path.filename());

  strcpy(ret, std_path.c_str());
}

string LibWebRTC::CreateRandomUuid() {
  return rtc::CreateRandomUuid();
}

}  // namespace libwebrtc
