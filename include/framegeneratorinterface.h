// Copyright (C) <2018> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0
#ifndef OWT_BASE_FRAMEGENERATORINTERFACE_H_
#define OWT_BASE_FRAMEGENERATORINTERFACE_H_
#include "rtc_video_frame.h"
#include "stdint.h"
namespace owt {
namespace base {

// `owt::base::CustomizedAudioCapturer` impls this interface
class AudioFrameReceiverInterface {
 public:
  virtual void OnFrame(const uint8_t* data, size_t samples_per_channel) = 0;
};

/**
 @brief frame generator interface for audio
 @details Sample rate and channel numbers cannot be changed once the generator
 is created. Currently, only 16 bit little-endian PCM is supported.
*/
class AudioFrameGeneratorInterface {
 public:
  /**
   @brief Generate frames for next 10ms.
   @param buffer Points to the start address for frame data. The memory is
   allocated and owned by SDK. Implementations should fill frame data to the
   memory starts from |buffer|.
   @param capacity Buffer's capacity. It will be equal or greater to expected
   frame buffer size.
   @return The size of actually frame buffer size.
   */
  virtual uint32_t GenerateFramesForNext10Ms(uint8_t* buffer,
                                             const uint32_t capacity) = 0;
  /// Get sample rate for frames generated.
  virtual int GetSampleRate() = 0;
  /// Get numbers of channel for frames generated.
  virtual int GetChannelNumber() = 0;
  /// pass the receiver to whom will send audio frames
  virtual void SetAudioFrameReceiver(AudioFrameReceiverInterface* receiver) = 0;

  virtual ~AudioFrameGeneratorInterface() {}
};

class AudioFrameFeeder : public AudioFrameGeneratorInterface {
  virtual uint32_t GenerateFramesForNext10Ms(uint8_t* buffer,
                                             const uint32_t capacity) override {
    return 0;
  }
};

// `owt::base::CustomizedFramesCapturer` impls this interface
class VideoFrameReceiverInterface {
 public:
  virtual void OnFrame(
      libwebrtc::scoped_refptr<libwebrtc::RTCVideoFrame> frame) = 0;
};

/**
 @brief frame generator interface for users to generates frame.
 FrameGeneratorInterface is the virtual class to implement its own frame
 generator.
*/
class VideoFrameGeneratorInterface {
 public:
  enum VideoFrameCodec {
    I420,
    NV12,
    VP8,
    H264,
  };
  /**
   @brief This function generates one frame data.
   @param buffer Points to the start address for frame data. The memory is
   allocated and owned by SDK. Implementations should fill frame data to the
   memory starts from |buffer|.
   @param capacity Buffer's capacity. It will be equal or greater to expected
   frame buffer size.
   @return The size of actually frame buffer size.
   */
  VideoFrameGeneratorInterface() {}
  virtual uint32_t GenerateNextFrame(uint8_t* buffer,
                                     const uint32_t capacity) = 0;
  virtual ~VideoFrameGeneratorInterface() {}
  /**
   @brief This function gets the size of next video frame.
   */
  virtual uint32_t GetNextFrameSize() = 0;
  /**
   @brief This function gets the height of video frame.
   */
  virtual int GetHeight() = 0;
  /**
   @brief This function gets the width of video frame.
   */
  virtual int GetWidth() = 0;
  /**
   @brief This function gets the fps of video frame generator.
   */
  virtual int GetFps() = 0;
  /**
   @brief This function gets the video frame type of video frame generator.
   */
  virtual VideoFrameCodec GetType() = 0;

  // pass the receiver to whom will send video frames
  virtual void SetFrameReceiver(VideoFrameReceiverInterface* receiver) = 0;

  /**
   @brief This function can perform any cleanup that must be done on the same
   thread as GenerateNextFrame(). Default implementation provided for backwards
   compatibility.
   */
  virtual void Cleanup() {}
};

class VideoFrameFeeder : public VideoFrameGeneratorInterface {
 public:
  virtual uint32_t GenerateNextFrame(uint8_t* buffer,
                                     const uint32_t capacity) override {
    return 0;
  }
  virtual uint32_t GetNextFrameSize() override { return 0; }
  virtual int GetHeight() override { return 1920; }
  virtual int GetWidth() override { return 1080; }
  virtual int GetFps() override { return 30; }
  virtual VideoFrameCodec GetType() override {
    return VideoFrameGeneratorInterface::VideoFrameCodec::NV12;
  }
};

}  // namespace base
}  // namespace owt
#endif  // OWT_BASE_FRAMEGENERATORINTERFACE_H_
