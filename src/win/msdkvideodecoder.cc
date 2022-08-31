// Copyright (C) <2018> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "msdkvideobase.h"
#include "mfxadapter.h"
#include "src/win/d3d11_allocator.h"
#include "src/win/msdkvideodecoder.h"
#include "api/scoped_refptr.h"
#include <wrl.h>

using namespace rtc;

#define MSDK_BS_INIT_SIZE (1024*1024)
enum { kMSDKCodecPollMs = 10 };
enum { MSDK_MSG_HANDLE_INPUT = 0 };

namespace owt {
namespace base {

int32_t MSDKVideoDecoder::Release() {
    WipeMfxBitstream(&m_mfx_bs_);

    delete m_pmfx_dec_;
    m_pmfx_dec_ = nullptr;

    /*delete m_pmfx_vpp_;
    m_pmfx_vpp_ = nullptr;*/

    if (m_mfx_session_) {
      MSDKFactory* factory = MSDKFactory::Get();
      if (factory) {
        factory->UnloadMSDKPlugin(m_mfx_session_, &m_plugin_id_);
        factory->DestroySession(m_mfx_session_);
      }

      m_mfx_session_ = nullptr;
    }

    m_pmfx_allocator_.reset();
    MSDK_SAFE_DELETE_ARRAY(m_pinput_surfaces_);
    //MSDK_SAFE_DELETE_ARRAY(m_pinput_surfaces1_);
    inited_ = false;
    return WEBRTC_VIDEO_CODEC_OK;
}

MSDKVideoDecoder::MSDKVideoDecoder()
    : width_(0)
      ,height_(0)
      ,decoder_thread_(rtc::Thread::Create())
{
  decoder_thread_->SetName("MSDKVideoDecoderThread", nullptr);
  RTC_CHECK(decoder_thread_->Start())
      << "Failed to start MSDK video decoder thread";
  MSDK_ZERO_MEMORY(m_pmfx_video_params_);
  MSDK_ZERO_MEMORY(m_mfx_response_);
  MSDK_ZERO_MEMORY(m_mfx_bs_);
  m_pinput_surfaces_ = nullptr;
  m_video_param_extracted = false;
  m_dec_bs_offset_ = 0;
  inited_ = false;
  surface_handle_.reset(new D3D11ImageHandle());
}

MSDKVideoDecoder::~MSDKVideoDecoder() {
  ntp_time_ms_.clear();
  timestamps_.clear();
  if (decoder_thread_.get() != nullptr){
    decoder_thread_->Stop();
  }
}

void MSDKVideoDecoder::CheckOnCodecThread() {
  RTC_CHECK(decoder_thread_.get() ==
            rtc::ThreadManager::Instance()->CurrentThread())
      << "Running on wrong thread!";
}

bool MSDKVideoDecoder::CreateD3D11Device() {
  HRESULT hr = S_OK;

  static D3D_FEATURE_LEVEL feature_levels[] = {
      D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1,
      D3D_FEATURE_LEVEL_10_1};
  D3D_FEATURE_LEVEL feature_levels_out;

  mfxU8 headers[] = {0x00, 0x00, 0x00, 0x01, 0x67, 0x42, 0xE0, 0x0A, 0x96,
                     0x52, 0x85, 0x89, 0xC8, 0x00, 0x00, 0x00, 0x01, 0x68,
                     0xC9, 0x23, 0xC8, 0x00, 0x00, 0x00, 0x01, 0x09, 0x10};
  mfxBitstream bs = {};
  bs.Data = headers;
  bs.DataLength = bs.MaxLength = sizeof(headers);

  mfxStatus sts = MFX_ERR_NONE;
  mfxU32 num_adapters;
  sts = MFXQueryAdaptersNumber(&num_adapters);

  if (sts != MFX_ERR_NONE)
    return false;

  std::vector<mfxAdapterInfo> display_data(num_adapters);
  mfxAdaptersInfo adapters = {display_data.data(), mfxU32(display_data.size()),
                              0u};
  sts = MFXQueryAdaptersDecode(&bs, MFX_CODEC_AVC, &adapters);
  if (sts != MFX_ERR_NONE) {
    RTC_LOG(LS_ERROR) << "Failed to query adapter with hardware acceleration";
    return false;
  }
  mfxU32 adapter_idx = adapters.Adapters[0].Number;

  hr = CreateDXGIFactory(__uuidof(IDXGIFactory2), (void**)(&m_pdxgi_factory_));
  if (FAILED(hr)) {
    RTC_LOG(LS_ERROR)
        << "Failed to create dxgi factory for adatper enumeration.";
    return false;
  }

  hr = m_pdxgi_factory_->EnumAdapters(adapter_idx, &m_padapter_);
  if (FAILED(hr)) {
    RTC_LOG(LS_ERROR) << "Failed to enum adapter for specified adapter index.";
    return false;
  }

  // On DG1 this setting driver type to hardware will result-in device
  // creation failure.
  hr = D3D11CreateDevice(
      m_padapter_, D3D_DRIVER_TYPE_UNKNOWN, nullptr, 0, feature_levels,
      sizeof(feature_levels) / sizeof(feature_levels[0]), D3D11_SDK_VERSION,
      &d3d11_device_, &feature_levels_out, &d3d11_device_context_);
  if (FAILED(hr)) {
    RTC_LOG(LS_ERROR) << "Failed to create d3d11 device for decoder";
    return false;
  }
  if (d3d11_device_) {
    hr = d3d11_device_->QueryInterface(__uuidof(ID3D11VideoDevice),
                                      (void**)&d3d11_video_device_);
    if (FAILED(hr)) {
      RTC_LOG(LS_ERROR) << "Failed to get d3d11 video device.";
      return false;
    }
  }
  if (d3d11_device_context_) {
    hr = d3d11_device_context_->QueryInterface(__uuidof(ID3D11VideoContext),
                                              (void**)&d3d11_video_context_);
    if (FAILED(hr)) {
      RTC_LOG(LS_ERROR) << "Failed to get d3d11 video context.";
      return false;
    }
  }
  // Turn on multi-threading for the context
  {
    CComQIPtr<ID3D10Multithread> p_mt(d3d11_device_);
    if (p_mt) {
      p_mt->SetMultithreadProtected(true);
    }
  }

  return true;
}

bool MSDKVideoDecoder::Configure(const Settings& settings) {

  RTC_LOG(LS_INFO) << "InitDecode enter";

  codec_type_  = settings.codec_type();
  timestamps_.clear();
  ntp_time_ms_.clear();

  settings_ = settings;

  //return decoder_thread_->Invoke<int32_t>(RTC_FROM_HERE,
  //    Bind(&MSDKVideoDecoder::InitDecodeOnCodecThread, this));
  return decoder_thread_->Invoke<bool>(
      RTC_FROM_HERE, [this] {
    return InitDecodeOnCodecThread() == WEBRTC_VIDEO_CODEC_OK;
      });
}

int32_t MSDKVideoDecoder::Reset() {
  delete m_pmfx_dec_;
  m_pmfx_dec_ = nullptr;
  m_pmfx_dec_ = new MFXVideoDECODE(*m_mfx_session_);
  return WEBRTC_VIDEO_CODEC_OK;
}

int32_t MSDKVideoDecoder::InitDecodeOnCodecThread() {
  RTC_LOG(LS_INFO) << "InitDecodeOnCodecThread enter";
  CheckOnCodecThread();

  // Set video_param_extracted flag to false to make sure the delayed 
  // DecoderHeader call will happen after Init.
  m_video_param_extracted = false;

  mfxStatus sts;
  width_ = settings_.max_render_resolution().Width();
  height_ = settings_.max_render_resolution().Height();
  uint32_t codec_id = MFX_CODEC_AVC;

  if (inited_) {
    if (m_pmfx_dec_ != nullptr) {
      delete m_pmfx_dec_;
      m_pmfx_dec_ = nullptr;
    }

    MSDK_SAFE_DELETE_ARRAY(m_pinput_surfaces_);
    //MSDK_SAFE_DELETE_ARRAY(m_pinput_surfaces1_);

    if (m_pmfx_allocator_) {
      m_pmfx_allocator_->Free(m_pmfx_allocator_->pthis, &m_mfx_response_);
    }
  } else {
    MSDKFactory* factory = MSDKFactory::Get();
    m_mfx_session_ = factory->CreateSession();
    if (!m_mfx_session_) {
      return WEBRTC_VIDEO_CODEC_ERROR;
    }
    if (settings_.codec_type() == webrtc::kVideoCodecVP8) {
      codec_id = MFX_CODEC_VP8;
    } else if (settings_.codec_type() == webrtc::kVideoCodecVP9) {
      codec_id = MFX_CODEC_VP9;
    } else if (settings_.codec_type() == webrtc::kVideoCodecAV1) {
      codec_id = MFX_CODEC_AV1;
    } else if (settings_.codec_type() == webrtc::kVideoCodecH265) {
      codec_id = MFX_CODEC_HEVC;
    }

    //if (!factory->LoadDecoderPlugin(codec_id, m_mfx_session_, &m_plugin_id_)) {
    //  return WEBRTC_VIDEO_CODEC_ERROR;
    //}

    if (!CreateD3D11Device()) {
      return WEBRTC_VIDEO_CODEC_ERROR;
    }

    mfxHandleType handle_type = MFX_HANDLE_D3D11_DEVICE;
    m_mfx_session_->SetHandle(handle_type, d3d11_device_.p);

    // Allocate and initalize the D3D11 frame allocator with current device.
    m_pmfx_allocator_ = MSDKFactory::CreateD3D11FrameAllocator(d3d11_device_.p);
    if (nullptr == m_pmfx_allocator_) {
      return WEBRTC_VIDEO_CODEC_ERROR;
    }

    // Set allocator to the session.
    sts = m_mfx_session_->SetFrameAllocator(m_pmfx_allocator_.get());
    if (MFX_ERR_NONE != sts) {
      return WEBRTC_VIDEO_CODEC_ERROR;
    }

    // Prepare the bitstream
    MSDK_ZERO_MEMORY(m_mfx_bs_);
    m_mfx_bs_.Data = new mfxU8[MSDK_BS_INIT_SIZE];
    m_mfx_bs_.MaxLength = MSDK_BS_INIT_SIZE;
    RTC_LOG(LS_ERROR) << "Creating underlying MSDK decoder.";
    m_pmfx_dec_ = new MFXVideoDECODE(*m_mfx_session_);
    if (m_pmfx_dec_ == nullptr) {
      return WEBRTC_VIDEO_CODEC_ERROR;
    }
  }

  m_pmfx_video_params_.mfx.CodecId = codec_id;
  if (codec_id == MFX_CODEC_VP9 || codec_id == MFX_CODEC_AV1)
    m_pmfx_video_params_.mfx.EnableReallocRequest = MFX_CODINGOPTION_ON;
  inited_ = true;
  RTC_LOG(LS_ERROR) << "InitDecodeOnCodecThread --";
  return WEBRTC_VIDEO_CODEC_OK;
}

int32_t MSDKVideoDecoder::Decode(
    const webrtc::EncodedImage& inputImage,
    bool missingFrames,
    int64_t renderTimeMs) {

  mfxStatus sts = MFX_ERR_NONE;
  mfxFrameSurface1 *pOutputSurface = nullptr;

  m_pmfx_video_params_.IOPattern = MFX_IOPATTERN_OUT_VIDEO_MEMORY;
  m_pmfx_video_params_.AsyncDepth = 4;

  ReadFromInputStream(&m_mfx_bs_, inputImage.data(), inputImage.size());

dec_header:
  if (inited_ && !m_video_param_extracted) {
    if (m_pmfx_dec_ == nullptr) {
      RTC_LOG(LS_ERROR) << "MSDK decoder not created.";
    }

    mfxU16 nSurfNum = 0;
    sts = m_pmfx_dec_->DecodeHeader(&m_mfx_bs_, &m_pmfx_video_params_);

    if (MFX_ERR_NONE == sts || MFX_WRN_PARTIAL_ACCELERATION == sts) {
      mfxFrameAllocRequest request;
      MSDK_ZERO_MEMORY(request);

      sts = m_pmfx_dec_->QueryIOSurf(&m_pmfx_video_params_, &request);
      if (MFX_WRN_PARTIAL_ACCELERATION == sts) {
        sts = MFX_ERR_NONE;
      }
      if (MFX_ERR_NONE != sts) {
        return WEBRTC_VIDEO_CODEC_ERROR;
      }

      mfxIMPL impl = 0;
      sts = m_mfx_session_->QueryIMPL(&impl);

      if ((request.NumFrameSuggested < m_pmfx_video_params_.AsyncDepth) &&
          (impl & MFX_IMPL_HARDWARE_ANY)) {
        RTC_LOG(LS_ERROR) << "Invalid num suggested.";
        return WEBRTC_VIDEO_CODEC_ERROR;
      }
      nSurfNum = MSDK_MAX(request.NumFrameSuggested, nSurfNum);

      request.Type |= MFX_MEMTYPE_VIDEO_MEMORY_DECODER_TARGET;
      sts = m_pmfx_allocator_->Alloc(m_pmfx_allocator_->pthis, &request,
                                   &m_mfx_response_);

      if (MFX_ERR_NONE != sts) {
        RTC_LOG(LS_ERROR) << "Failed on allocator's alloc method";
        return WEBRTC_VIDEO_CODEC_ERROR;
      }
      nSurfNum = m_mfx_response_.NumFrameActual;
      // Allocate both the input and output surfaces.
      m_pinput_surfaces_ = new mfxFrameSurface1[nSurfNum];
      if (nullptr == m_pinput_surfaces_) {
        RTC_LOG(LS_ERROR) << "Failed allocating input surfaces.";
        return WEBRTC_VIDEO_CODEC_ERROR;
      }

      for (int i = 0; i < nSurfNum; i++) {
        memset(&(m_pinput_surfaces_[i]), 0, sizeof(mfxFrameSurface1));
        MSDK_MEMCPY_VAR(m_pinput_surfaces_[i].Info, &(request.Info),
                        sizeof(mfxFrameInfo));
        m_pinput_surfaces_[i].Data.MemId = m_mfx_response_.mids[i];
        m_pinput_surfaces_[i].Data.MemType = request.Type;
      }

      if (!m_pmfx_video_params_.mfx.FrameInfo.FrameRateExtN ||
          m_pmfx_video_params_.mfx.FrameInfo.FrameRateExtD) {
        m_pmfx_video_params_.mfx.FrameInfo.FrameRateExtN = 30;
        m_pmfx_video_params_.mfx.FrameInfo.FrameRateExtD = 1;
      }

      if (!m_pmfx_video_params_.mfx.FrameInfo.AspectRatioH ||
          !m_pmfx_video_params_.mfx.FrameInfo.AspectRatioW) {
        m_pmfx_video_params_.mfx.FrameInfo.AspectRatioH = 1;
        m_pmfx_video_params_.mfx.FrameInfo.AspectRatioW = 1;
      }
      // Finally we're done with all configurations and we're OK to init the
      // decoder.
      sts = m_pmfx_dec_->Init(&m_pmfx_video_params_);
      if (MFX_ERR_NONE != sts) {
        RTC_LOG(LS_ERROR) << "Failed to init the decoder.";
        return WEBRTC_VIDEO_CODEC_ERROR;
      }

      m_video_param_extracted = true;
    } else {
      // With current bitstream, if we're not able to extract the video param
      // and thus not able to continue decoding. return directly.
      return WEBRTC_VIDEO_CODEC_ERROR;
    }
  }

  m_mfx_bs_.DataFlag = MFX_BITSTREAM_COMPLETE_FRAME;
  mfxSyncPoint syncp;
  //mfxSyncPoint syncpV;

  // If we get video param changed, that means we need to continue with
  // decoding.
  while (true) {
more_surface:
    mfxU16 moreIdx =
        DecGetFreeSurface(m_pinput_surfaces_, m_mfx_response_.NumFrameActual);
    if (moreIdx == MSDK_INVALID_SURF_IDX) {
      MSDK_SLEEP(1);
      continue;
    }
    mfxFrameSurface1* moreFreeSurf = &m_pinput_surfaces_[moreIdx];

retry:
    m_dec_bs_offset_ = m_mfx_bs_.DataOffset;
    sts = m_pmfx_dec_->DecodeFrameAsync(&m_mfx_bs_, moreFreeSurf, &pOutputSurface,
                                      &syncp);
    
    if (sts == MFX_ERR_NONE && syncp != nullptr) {
      sts = m_mfx_session_->SyncOperation(syncp, MSDK_DEC_WAIT_INTERVAL);

      //if (sts == MFX_ERR_NONE) {
      //  mfxU16 nIndex2 = DecGetFreeSurface(m_pinput_surfaces1_, nSurfNumVpp);

      //  for (;;) {
      //    // Process a frame asychronously (returns immediately)
      //    sts = m_pmfx_vpp_->RunFrameVPPAsync(
      //        pOutputSurface, &m_pinput_surfaces1_[nIndex2], NULL, &syncpV);

      //    if (MFX_ERR_NONE < sts &&
      //        !syncpV) {  // repeat the call if warning and no output
      //      if (MFX_WRN_DEVICE_BUSY == sts)
      //        MSDK_SLEEP(1);  // wait if device is busy
      //    } else if (MFX_ERR_NONE < sts && syncpV) {
      //      sts = MFX_ERR_NONE;  // ignore warnings if output is available
      //      break;
      //    } else {
      //      break;  // not a warning
      //    }
      //  }

      //  // VPP needs more data, let decoder decode another frame as input
      //  if (MFX_ERR_MORE_DATA == sts) {
      //    continue;
      //  }
      //}
      //
      //if (sts == MFX_ERR_NONE) {
      //  sts = m_mfx_session_->SyncOperation(syncp, 60000);
      //}

      if (sts >= MFX_ERR_NONE) {
#if 1
        mfxMemId dxMemId = pOutputSurface->Data.MemId;
        mfxFrameInfo frame_info = pOutputSurface->Info;
        mfxHDLPair pair = {nullptr};
        // Maybe we should also send the allocator as part of the frame
        // handle for locking/unlocking purpose.
        m_pmfx_allocator_->GetFrameHDL(dxMemId, (mfxHDL*)&pair);
        if (callback_) {
          surface_handle_->texture =
              reinterpret_cast<ID3D11Texture2D*>(pair.first);
          surface_handle_->d3d11_device = d3d11_device_.p;

          // Texture_array_index not used when decoding with MSDK.
          surface_handle_->texture_array_index = 0;
          D3D11_TEXTURE2D_DESC texture_desc;
          memset(&texture_desc, 0, sizeof(texture_desc));
          surface_handle_->texture->GetDesc(&texture_desc);
          
          /*ID3D11Texture2D* outTexture = ConvertD3D11Texture(
              inTexture, frame_info.CropW, frame_info.CropH);
          if (outTexture == nullptr) {
            RTC_LOG(LS_ERROR) << "Failed to convert texture.";
            return WEBRTC_VIDEO_CODEC_ERROR;
          }
          surface_handle_->texture = outTexture;*/

          // TODO(johny): we should extend the buffer structure to include
          // not only the CropW|CropH value, but also the CropX|CropY for the
          // renderer to correctly setup the video processor input view.
          rtc::scoped_refptr<owt::base::NativeHandleBuffer> buffer =
              new rtc::RefCountedObject<owt::base::NativeHandleBuffer>(
                  (void*)surface_handle_.get(), frame_info.CropW,
                  frame_info.CropH);
          webrtc::VideoFrame decoded_frame(buffer, inputImage.Timestamp(), 0,
                                           webrtc::kVideoRotation_0);
          decoded_frame.set_ntp_time_ms(inputImage.ntp_time_ms_);
          decoded_frame.set_timestamp(inputImage.Timestamp());
          callback_->Decoded(decoded_frame);

          /*outTexture->Release();
          outTexture = nullptr;*/
        }
#else
        mfxFrameData frame_data = pOutputSurface->Data;
        mfxMemId dxMemId = frame_data.MemId;
        mfxFrameInfo frame_info = pOutputSurface->Info;

        m_pmfx_allocator_->LockFrame(dxMemId, &frame_data);

        // always NV12
        rtc::scoped_refptr<owt::base::NativeNV12Buffer> buffer =
            new rtc::RefCountedObject<owt::base::NativeNV12Buffer>(
                frame_data.Y, frame_data.Pitch,
                frame_data.UV, frame_data.Pitch,
                frame_info.Width, frame_info.Height);

        if (callback_) {
          webrtc::VideoFrame decoded_frame(buffer, inputImage.Timestamp(),
                                           0,
                                                webrtc::kVideoRotation_0);
            decoded_frame.set_ntp_time_ms(inputImage.ntp_time_ms_);
            decoded_frame.set_timestamp(inputImage.Timestamp());
            callback_->Decoded(decoded_frame);
        }

        m_pmfx_allocator_->UnlockFrame(dxMemId, &frame_data);
#endif
     }
    } else if (MFX_ERR_MORE_DATA == sts) {
      return WEBRTC_VIDEO_CODEC_OK;
    } else if (sts == MFX_WRN_DEVICE_BUSY) {
      MSDK_SLEEP(1);
      goto retry;
    } else if (sts == MFX_ERR_MORE_SURFACE) {
      goto more_surface;
    } else if (sts == MFX_WRN_VIDEO_PARAM_CHANGED) {
      goto retry;
    } else if (sts != MFX_ERR_NONE) {
      Reset();
      m_mfx_bs_.DataLength += m_mfx_bs_.DataOffset - m_dec_bs_offset_;
      m_mfx_bs_.DataOffset = m_dec_bs_offset_;
      m_video_param_extracted = false;
      goto dec_header;
	}
  }
  return WEBRTC_VIDEO_CODEC_OK;
}
mfxStatus MSDKVideoDecoder::ExtendMfxBitstream(mfxBitstream* pBitstream, mfxU32 nSize) {
  mfxU8* pData = new mfxU8[nSize];
  memmove(pData, pBitstream->Data + pBitstream->DataOffset, pBitstream->DataLength);

  WipeMfxBitstream(pBitstream);

  pBitstream->Data = pData;
  pBitstream->DataOffset = 0;
  pBitstream->MaxLength = nSize;

  return MFX_ERR_NONE;
}

void MSDKVideoDecoder::ReadFromInputStream(mfxBitstream* pBitstream, const uint8_t *data, size_t len) {
  if (m_mfx_bs_.MaxLength < len){
      // Remaining BS size is not enough to hold current image, we enlarge it the gap*2.
      mfxU32 newSize = static_cast<mfxU32>(m_mfx_bs_.MaxLength > len ? m_mfx_bs_.MaxLength * 2 : len * 2);
      ExtendMfxBitstream(&m_mfx_bs_, newSize);
  }
  memmove(m_mfx_bs_.Data + m_mfx_bs_.DataLength, data, len);
  m_mfx_bs_.DataLength += static_cast<mfxU32>(len);
  m_mfx_bs_.DataOffset = 0;
  return;
}

void MSDKVideoDecoder::WipeMfxBitstream(mfxBitstream* pBitstream) {
  // Free allocated memory
  MSDK_SAFE_DELETE_ARRAY(pBitstream->Data);
}

mfxU16 MSDKVideoDecoder::DecGetFreeSurface(mfxFrameSurface1* pSurfacesPool, mfxU16 nPoolSize) {
  mfxU32 SleepInterval = 10; // milliseconds
  mfxU16 idx = MSDK_INVALID_SURF_IDX;

  // Wait if there's no free surface
  for (mfxU32 i = 0; i < MSDK_WAIT_INTERVAL; i += SleepInterval) {
    idx = DecGetFreeSurfaceIndex(pSurfacesPool, nPoolSize);

    if (MSDK_INVALID_SURF_IDX != idx) {
      break;
    } else {
      MSDK_SLEEP(SleepInterval);
    }
  }
  return idx;
}

mfxU16 MSDKVideoDecoder::DecGetFreeSurfaceIndex(mfxFrameSurface1* pSurfacesPool, mfxU16 nPoolSize) {
  if (pSurfacesPool) {
    for (mfxU16 i = 0; i < nPoolSize; i++) {
      if (0 == pSurfacesPool[i].Data.Locked) {
          return i;
      }
    }
  }
  return MSDK_INVALID_SURF_IDX;
}

int32_t MSDKVideoDecoder::RegisterDecodeCompleteCallback(webrtc::DecodedImageCallback* callback) {
  callback_ = callback;
  return WEBRTC_VIDEO_CODEC_OK;
}

std::unique_ptr<MSDKVideoDecoder> MSDKVideoDecoder::Create(
    cricket::VideoCodec format) {
  return absl::make_unique<MSDKVideoDecoder>();
}

const char* MSDKVideoDecoder::ImplementationName() const {
  return "IntelMediaSDK";
}

//bool MSDKVideoDecoder::CreateVideoProcessor(int width, int height, bool reset) {
//  HRESULT hr = S_OK;
//  if (width < 0 || height < 0)
//    return false;
//
//  D3D11_VIDEO_PROCESSOR_CONTENT_DESC content_desc;
//  ZeroMemory(&content_desc, sizeof(content_desc));
//
//  if (video_processor_.p && video_processor_enum_.p) {
//    hr = video_processor_enum_->GetVideoProcessorContentDesc(&content_desc);
//    if (FAILED(hr))
//      return false;
//
//    if (content_desc.InputWidth != (unsigned int)width ||
//        content_desc.InputHeight != (unsigned int)height || reset) {
//      video_processor_enum_.Release();
//      video_processor_.Release();
//    } else {
//      return true;
//    }
//  }
//
//  ZeroMemory(&content_desc, sizeof(content_desc));
//  content_desc.InputFrameFormat = D3D11_VIDEO_FRAME_FORMAT_PROGRESSIVE;
//  content_desc.InputFrameRate.Numerator = 30;
//  content_desc.InputFrameRate.Denominator = 1;
//  content_desc.InputWidth = width;
//  content_desc.InputHeight = height;
//  content_desc.OutputWidth = width;
//  content_desc.OutputHeight = height;
//  content_desc.OutputFrameRate.Numerator = 30;
//  content_desc.OutputFrameRate.Denominator = 1;
//  content_desc.Usage = D3D11_VIDEO_USAGE_OPTIMAL_SPEED;
//
//  hr = d3d11_video_device_->CreateVideoProcessorEnumerator(
//      &content_desc, &video_processor_enum_);
//  if (FAILED(hr))
//    return false;
//
//  hr = d3d11_video_device_->CreateVideoProcessor(video_processor_enum_, 0,
//                                                 &video_processor_);
//  if (FAILED(hr))
//    return false;
//
//  return true;
//}
//
//ID3D11Texture2D* MSDKVideoDecoder::ConvertD3D11Texture(
//    ID3D11Texture2D* pInTexture2D, int w, int h) {
//  HRESULT hr = S_OK;
//  ID3D11Texture2D* pOutTexture2D = nullptr;
//  ID3D11VideoProcessorOutputView* output_view;
//  ID3D11VideoProcessorInputView* input_view;
//
//  D3D11_TEXTURE2D_DESC desc2D;
//  pInTexture2D->GetDesc(&desc2D);
//  desc2D.MiscFlags = D3D11_RESOURCE_MISC_SHARED;
//  desc2D.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
//  desc2D.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
//
//  D3D11_VIDEO_PROCESSOR_INPUT_VIEW_DESC pInDesc;
//  ZeroMemory(&pInDesc, sizeof(pInDesc));
//  pInDesc.FourCC = 0;
//  pInDesc.ViewDimension = D3D11_VPIV_DIMENSION_TEXTURE2D;
//  pInDesc.Texture2D.MipSlice = 0;
//  pInDesc.Texture2D.ArraySlice = 0;
//
//  hr = d3d11_video_device_->CreateVideoProcessorInputView(
//      pInTexture2D, video_processor_enum_, &pInDesc, &input_view);
//  if (FAILED(hr))
//    return nullptr;
//
//  hr = d3d11_device_->CreateTexture2D(&desc2D, NULL, &pOutTexture2D);
//  if (FAILED(hr))
//    return nullptr;
//
//  D3D11_VIDEO_PROCESSOR_OUTPUT_VIEW_DESC pOutDesc;
//  ZeroMemory(&pOutDesc, sizeof(pOutDesc));
//  pOutDesc.ViewDimension = D3D11_VPOV_DIMENSION_TEXTURE2D;
//  pOutDesc.Texture2D.MipSlice = 0;
//
//  hr = d3d11_video_device_->CreateVideoProcessorOutputView(
//      pOutTexture2D, video_processor_enum_, &pOutDesc, &output_view);
//  if (FAILED(hr))
//    return nullptr;
//
//  D3D11_VIDEO_PROCESSOR_STREAM StreamData;
//  ZeroMemory(&StreamData, sizeof(StreamData));
//  StreamData.Enable = TRUE;
//  StreamData.OutputIndex = 0;
//  StreamData.InputFrameOrField = 0;
//  StreamData.PastFrames = 0;
//  StreamData.FutureFrames = 0;
//  StreamData.ppPastSurfaces = NULL;
//  StreamData.ppFutureSurfaces = NULL;
//  StreamData.pInputSurface = input_view;
//  StreamData.ppPastSurfacesRight = NULL;
//  StreamData.ppFutureSurfacesRight = NULL;
//
//  hr = d3d11_video_context_->VideoProcessorBlt(video_processor_, output_view,
//                                               0, 1, &StreamData);
//  if (FAILED(hr))
//    return nullptr;
//
//  input_view->Release();
//  input_view = nullptr;
//  output_view->Release();
//  output_view = nullptr;
//
//  return pOutTexture2D;
//}


}  // namespace base
}  // namespace owt
