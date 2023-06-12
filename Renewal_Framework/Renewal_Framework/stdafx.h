#pragma once
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")

#define FIRST_WINDOW_POS_WIDTH 0
#define FIRST_WINDOW_POS_HEIGHT 0

#define FRAME_BUFFER_WIDTH 1920
#define FRAME_BUFFER_HEIGHT 1080

#define HALF_FRAME_BUFFER_WIDTH 960
#define HALF_FRAME_BUFFER_HEIGHT 540

#include <windows.h>

#include <stdexcept>
#include <string>

#include "d3dx12.h"
#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl.h>

#include <DirectXMath.h>
#include <d3dcompiler.h>

using Microsoft::WRL::ComPtr;

extern UINT	gnCbvSrvDescriptorIncrementSize;
extern UINT gnRtvDescriptorIncrementSize;

extern ComPtr<ID3D12Resource> CreateBufferResource(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, void* pData, UINT nBytes, D3D12_HEAP_TYPE d3dHeapType, D3D12_RESOURCE_STATES d3dResourceStates, ID3D12Resource** ppd3dUploadBuffer);

inline std::string HrToString(HRESULT hr)
{
    char s_str[64] = {};
    sprintf_s(s_str, "HRESULT of 0x%08X", static_cast<UINT>(hr));
    return std::string(s_str);
}

inline void ThrowIfFailed(HRESULT hr)
{
    if (FAILED(hr))
    {
        throw std::runtime_error(HrToString(hr));
    }
}