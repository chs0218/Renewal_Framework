#include "DirectXProgram.h"

DirectXProgram::DirectXProgram(UINT width, UINT height, std::wstring title)
{
	m_ProgramWidth = width;
	m_ProgramHeight = height;
	m_ProgramTitle = title;
}

void DirectXProgram::Init()
{
	HRESULT hResult;
	UINT dxgiFactoryFlags = 0;

#if defined(_DEBUG)
	{
		// 디버그 모드로 빌드할 경우, 디버그 인터페이스를 가져온다.
		ComPtr<ID3D12Debug> pd3dDebugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&pd3dDebugController))))
		{
			pd3dDebugController->EnableDebugLayer();
			dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
		}
	}
#endif

	// DXGI 객체를 생성할 수 있는 IDXGIFactory4 객체를 생성합니다.
	ComPtr<IDXGIFactory4> factory;
	ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)));
}
