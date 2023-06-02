#pragma once
#include "stdafx.h"

struct Vertex
{
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT4 color;
};

class DirectXProgram
{
private:
	// 윈도우 너비, 높이, 윈도우 이름을 저장하는 변수이다.
	UINT m_ProgramWidth;
	UINT m_ProgramHeight;
	std::wstring m_ProgramTitle;

	// 뷰포트와 시저 사각형을 정의
	D3D12_VIEWPORT m_Viewport;
	D3D12_RECT m_ScissorRect;

	// 윈도우 종횡비를 계산해 저장하고 있는 변수이다.
	float m_AspectRatio;

	// 사용자 화면 해상도를 저장하는 변수이다.
	RECT m_DeskTopCoordinatesRect;

	//Direct3D 디바이스 인터페이스에 대한 포인터이다.
	ComPtr<ID3D12Device> m_pd3dDevice;

	//MSAA 다중 샘플링을 활성화하고 다중 샘플링 레벨을 설정한다.
	bool m_bMsaa4xEnable = false;
	UINT m_nMsaa4xQualityLevels = 0;

	//스왑 체인의 후면 버퍼의 개수와 현재 스왑 체인의 푸면 버퍼 인덱스이다.
	static const UINT m_nSwapChainBuffers = 2;
	UINT m_nSwapChainBufferIndex;

	//스왑 체인 인터페이스에 대한 포인터이다.
	ComPtr<IDXGISwapChain3> m_pdxgiSwapChain;

	//펜스 인터페이스 포인터, 펜스의 값, 이벤트 핸들이다.
	ComPtr<ID3D12Fence> m_pd3dFence;
	UINT64 m_nFenceValues[m_nSwapChainBuffers];
	HANDLE m_hFenceEvent;

	//명령 큐, 명령 할당자, 명령 리스트 인터페이스 포인터이다.
	ComPtr<ID3D12CommandQueue> m_pd3dCommandQueue;
	ComPtr<ID3D12CommandAllocator> m_pd3dCommandAllocator;
	ComPtr<ID3D12GraphicsCommandList> m_pd3dCommandList;

	//렌더 타겟 버퍼, 서술자 힙 인터페이스 포인터, 렌더 타겟 서술자 원소의 크기이다.
	ComPtr<ID3D12Resource> m_ppd3dRenderTargetBuffers[m_nSwapChainBuffers];
	ComPtr<ID3D12DescriptorHeap> m_pd3dRtvDescriptorHeap;
	D3D12_CPU_DESCRIPTOR_HANDLE	m_pd3dSwapRTVCPUHandles[m_nSwapChainBuffers];

	//깊이-스텐실 버퍼, 서술자 힙 인터페이스 포인터, 깊이-스텐실 서술자 원소의 크기이다.
	ComPtr<ID3D12Resource> m_pd3dDepthStencilBuffer;
	ComPtr<ID3D12DescriptorHeap> m_pd3dDsvDescriptorHeap;
	D3D12_CPU_DESCRIPTOR_HANDLE m_d3dDsvDescriptorCPUHandle;

	// 그래픽스파이프라인 상태를 정의
	ComPtr<ID3D12PipelineState> m_pd3dPipelineState;

	// 루트 시그니쳐를 정의
	ComPtr<ID3D12RootSignature> m_RootSignature;

	// 삼각형을 그리기 위한 포지션, 색상 정보를 저장하는 버퍼이다.
	ComPtr<ID3D12Resource> m_pd3dPosColBuffer = NULL;
	ComPtr<ID3D12Resource> m_pd3dPosColUploadBuffer = NULL;

	D3D12_VERTEX_BUFFER_VIEW m_pd3dVertexBufferView;
public:
	DirectXProgram(UINT width, UINT height, std::wstring title);

	const UINT& GetProgramWidth() { return m_ProgramWidth; }
	const UINT& GetProgramHeight() { return m_ProgramHeight; }
	const std::wstring& GetProgramTitle() { return m_ProgramTitle; }

	void Init();
	void BuildLevel();
	void RenderLevel();
	void WaitForGpuComplete();
};

