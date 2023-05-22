#pragma once

#define FIRST_WINDOW_POS_WIDTH 0
#define FIRST_WINDOW_POS_HEIGHT 0

#define FRAME_BUFFER_WIDTH 1920
#define FRAME_BUFFER_HEIGHT 1080

#include <windows.h>
#pragma once
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")

#include <stdexcept>
#include <string>

#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl.h>

using Microsoft::WRL::ComPtr;

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
