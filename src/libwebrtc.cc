#include "libwebrtc.h"

#include <filesystem>
#include <fstream>
#include <memory>

#include "api/scoped_refptr.h"
#include "rtc_base/logging.h"
#include "rtc_base/helpers.h"
#include "rtc_base/ssl_adapter.h"
#include "rtc_base/string_utils.h"
#include "rtc_base/thread.h"
#include "rtc_peerconnection_factory_impl.h"

#define MAX_LOG_BUF_SIZE 9000

namespace libwebrtc {
class CustomnLogSink;
// Initialize static variable g_is_initialized to false.
static bool g_is_initialized = false;
std::unique_ptr<CustomnLogSink> log_sink;

class CustomnLogSink : public rtc::LogSink {
 public:
  CustomnLogSink(const std::string& filepath) { file_stream_.open(filepath); }

  virtual void OnLogMessage(const std::string& message) override {
    file_stream_ << message;
  }

  void Close() {
    if (!file_stream_.is_open()) return;

    file_stream_.flush();
    file_stream_.close();
  }

  void Flush() {
    if (!file_stream_.is_open()) return;

    file_stream_.flush();
  }

 private:
  std::ofstream file_stream_;
};

// Initializes SSL, if not initialized.
bool LibWebRTC::Initialize() {
  if (!g_is_initialized) {
    rtc::InitializeSSL();
    g_is_initialized = true;
  }
  return g_is_initialized;
}

// Stops and cleans up the threads and SSL.
void LibWebRTC::Terminate() {
  rtc::ThreadManager::Instance()->SetCurrentThread(NULL);
  rtc::CleanupSSL();

  if (log_sink.get() != nullptr) {
    // flush immediately
    log_sink->Close();
    rtc::LogMessage::RemoveLogToStream(log_sink.get());
    log_sink.reset(nullptr);
  }

  // Resets the static variable g_is_initialized to false.
  g_is_initialized = false;
}

// Creates and returns an instance of RTCPeerConnectionFactory.
scoped_refptr<RTCPeerConnectionFactory>
LibWebRTC::CreateRTCPeerConnectionFactory() {
  scoped_refptr<RTCPeerConnectionFactory> rtc_peerconnection_factory =
      scoped_refptr<RTCPeerConnectionFactory>(
          new RefCountedObject<RTCPeerConnectionFactoryImpl>());
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

void LibWebRTC::FlushLogToFile() {
  if (log_sink.get() == nullptr) {
    return;
  }
  log_sink->Flush();
}

// log level see enum: `RTCLogLevel`
void LibWebRTC::UpdateRTCLogLevel(int level) {
  rtc::LogMessage::LogToDebug((rtc::LoggingSeverity)level);
}

// custom log methods
void LibWebRTC::RTCLogEx(RTCLogLevel severity, const char* file, int line,
                         const char* format, ...) {
  va_list args;

  va_start(args, format);

  char log_string[MAX_LOG_BUF_SIZE];
  vsnprintf(log_string, sizeof(log_string), format, args);
  RTC_LOG_N_LINE((rtc::LoggingSeverity)severity, file, line) << log_string;

  va_end(args);
}

void LibWebRTC::RTCFileName(const char* file_path, char ret[128]) {
  std::filesystem::path path(file_path);
  strcpy(ret, path.filename().c_str());
}

string LibWebRTC::CreateRandomUuid() { return rtc::CreateRandomUuid(); }

}  // namespace libwebrtc
