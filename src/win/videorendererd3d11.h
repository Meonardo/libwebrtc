// Copyright (C) <2020> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef OWT_BASE_WIN_VIDEORENDERERD3D11_H
#define OWT_BASE_WIN_VIDEORENDERERD3D11_H

#include <atlbase.h>
#include <combaseapi.h>
#include <windows.h>

#include <d3d11.h>
#include <d3d11_1.h>
#include <d3d11_2.h>
#include <dcomp.h>
#include <dxgi1_2.h>

#ifdef ALLOW_TEARING
#include <dxgi1_4.h>
#include <dxgi1_5.h>
#endif

#include <dxgiformat.h>
#include <mfidl.h>
#include <psapi.h>
#include <wrl.h>
#include <vector>

#include "api/video/video_frame.h"
#include "rtc_base/synchronization/mutex.h"
#include "system_wrappers/include/clock.h"

#include "base/refcount.h"
#include "commontypes.h"

enum ScalingMode : int {
  SCALING_MODE_DEFAULT = 0,     // Default
  SCALING_MODE_QUALITY,         // LowerPower
  SCALING_MODE_SUPERRESOLUTION  // SuperREsolution
};

enum VPEMode : int {
  VPE_MODE_NONE = 0x0,
  VPE_MODE_PREPROC = 0x1,
  VPE_MODE_CPU_GPU_COPY = 0x3,
};

enum VPE_VERSION_ENUM : int {
  VPE_VERSION_1_0 = 0x0001,
  VPE_VERSION_2_0 = 0x0002,
  VPE_VERSION_3_0 = 0x0003,
  VPE_VERSION_UNKNOWN = 0xffff,
};

enum VPE_SUPER_RESOLUTION_MODE : int {
  DEFAULT_SCENARIO_MODE = 0,
  CAMERA_SCENARIO_MODE = 1,
};

enum VPE_CPU_GPU_COPY_DIRECTION : int {
  VPE_CPU_GPU_COPY_DIRECTION_CPU_TO_GPU,
  VPE_CPU_GPU_COPY_DIRECTION_GPU_TO_CPU,
  VPE_CPU_GPU_COPY_DIRECTION_MMC_IN,
  VPE_CPU_GPU_COPY_DIRECTION_NOW_MMC_IN,
  VPE_CPU_GPU_COPY_DIRECTION_MMC_OUT,
  VPE_CPU_GPU_COPY_DIRECTION_NOW_MMC_OUT,
};

typedef struct _SR_SCALING_MODE {
  UINT Fastscaling;
} SR_SCALING_MODE, *PSR_SCALING_MODE;

typedef struct _VPE_VERSION {
  UINT Version;
} VPE_VERSION, *PVPE_VERSION;

typedef struct _VPE_MODE {
  UINT Mode;
} VPE_MODE, *PVPE_MODE;

typedef struct _VPE_FUNCTION {
  UINT Function;
  union {
    void* pSrCalingmode;
    void* pVPEMode;
    void* pVPEVersion;
    void* pSRParams;
    void* pCpuGpuCopyParam;
  };
} VPE_FUNCTION, *PVPE_FUNCTION;

typedef struct _VPE_SR_PARAMS {
  UINT bEnable : 1;  // [in], Enable SR
  UINT ReservedBits : 31;
  VPE_SUPER_RESOLUTION_MODE SRMode;
  UINT Reserved[4];
} VPE_SR_PARAMS, *PVPE_SR_PARAMS;

typedef struct _VPE_CPU_GPU_COPY_PARAM {
  VPE_CPU_GPU_COPY_DIRECTION Direction;
  UINT MemSize;
  void* pSystemMem;
} VPE_CPU_GPU_COPY_PARAM, *PVPE_CPU_GPU_COPY_PARAM;

namespace owt {
namespace base {

// class VideoFrameSizeChangeObserver;

class WebrtcVideoRendererD3D11Impl : public libwebrtc::RefCountInterface {
 public:
  WebrtcVideoRendererD3D11Impl(HWND wnd);
  // virtual void OnFrame(const webrtc::VideoFrame& frame) override;
  void OnFrame(webrtc::VideoFrameBuffer* buffer);
  virtual ~WebrtcVideoRendererD3D11Impl();

  Resolution GetFrameSize() const;
  // void AddVideoFrameChangeObserver(VideoFrameSizeChangeObserver* o);

 private:
  bool InitMPO(int width, int height);
  void RenderNativeHandleFrame(webrtc::VideoFrameBuffer* buffer);
  void RenderNV12DXGIMPO(int width, int height);
  bool CreateVideoProcessor(int width, int height, bool reset);
  void RenderD3D11Texture(int width, int height);
  void RenderI420Frame_DX11(webrtc::VideoFrameBuffer* buffer);
  bool InitD3D11(int width, int height);
  bool InitSwapChain(int widht, int height, bool reset);
  bool CreateStagingTexture(int width, int height);
  bool GetWindowSizeForSwapChain(int& width, int& height);
  bool SupportSuperResolution();

  // Render window objects
  HWND wnd_ = nullptr;
  int window_width_ = 0;
  int window_height_ = 0;

  unsigned long frame_width_ = 0;
  unsigned long frame_height_ = 0;
  // VideoFrameSizeChangeObserver* frame_observer_;

  // D3D11 objects
  ID3D10Multithread* p_mt = nullptr;
  ID3D11Device* d3d11_device_ = nullptr;
  ID3D11Device2* d3d11_device2_ = nullptr;
  ID3D11VideoDevice* d3d11_video_device_ = nullptr;
  ID3D11DeviceContext* d3d11_device_context_ = nullptr;
  ID3D11VideoContext* d3d11_video_context_ = nullptr;
  // ID3D11DeviceContext1* dx11_device_context1_ = nullptr;

  CComPtr<IDXGIFactory2> dxgi_factory_;
  CComPtr<IDXGISwapChain1> swap_chain_for_hwnd_;

  // MPO objects
  CComPtr<IDXGIDevice> dxgi_device2_;
  CComPtr<IDCompositionDevice2> comp_device2_;
  CComPtr<IDCompositionTarget> comp_target_;
  CComPtr<IDCompositionVisual2> root_visual_;
  CComPtr<IDCompositionVisual2> visual_preview_;

  // VideoProcessor objects
  CComPtr<ID3D11VideoProcessor> video_processor_;
  CComPtr<ID3D11VideoProcessorEnumerator> video_processor_enum_;

  // Handle of texture holding decoded image.
  webrtc::Mutex d3d11_texture_lock_;
  ID3D11Texture2D* d3d11_texture_ = nullptr;
  ID3D11Texture2D* d3d11_staging_texture_ = nullptr;
  D3D11_TEXTURE2D_DESC d3d11_texture_desc_;
  // Local view is using normal d3d11 swapchain.
  bool d3d11_raw_inited_ = false;
  // Remote view is using MPO swapchain.
  bool d3d11_mpo_inited_ = false;
  // Support super resolution
  bool sr_enabled_ = false;
  webrtc::Clock* clock_;
};

}  // namespace base
}  // namespace owt
#endif  // OWT_BASE_WIN_VIDEORENDERERD3D11_H