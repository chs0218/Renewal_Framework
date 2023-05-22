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
		// ����� ���� ������ ���, ����� �������̽��� �����´�.
		ComPtr<ID3D12Debug> pd3dDebugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&pd3dDebugController))))
		{
			pd3dDebugController->EnableDebugLayer();
			dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
		}
	}
#endif

	// DXGI ��ü�� ������ �� �ִ� IDXGIFactory4 ��ü�� �����մϴ�.
	ComPtr<IDXGIFactory4> factory;
	ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)));
}
