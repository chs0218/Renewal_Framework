#pragma once
#include "stdafx.h"

class DirectXProgram
{
private:
	// ������ �ʺ�, ����, ������ �̸��� �����ϴ� �����̴�.
	UINT m_ProgramWidth;
	UINT m_ProgramHeight;
	std::wstring m_ProgramTitle;

	// ����� ȭ�� �ػ󵵸� �����ϴ� �����̴�.
	RECT m_DeskTopCoordinatesRect;

	//Direct3D ����̽� �������̽��� ���� �������̴�.
	ComPtr<ID3D12Device> m_pd3dDevice;

	//MSAA ���� ���ø��� Ȱ��ȭ�ϰ� ���� ���ø� ������ �����Ѵ�.
	bool m_bMsaa4xEnable = false;
	UINT m_nMsaa4xQualityLevels = 0;

	//���� ü���� �ĸ� ������ ������ ���� ���� ü���� Ǫ�� ���� �ε����̴�.
	static const UINT m_nSwapChainBuffers = 2;
	UINT m_nSwapChainBufferIndex;

	//���� ü�� �������̽��� ���� �������̴�.
	ComPtr<IDXGISwapChain3> m_pdxgiSwapChain;

	//�潺 �������̽� ������, �潺�� ��, �̺�Ʈ �ڵ��̴�.
	ComPtr<ID3D12Fence> m_pd3dFence;
	UINT64 m_nFenceValues[m_nSwapChainBuffers];
	HANDLE m_hFenceEvent;

	//��� ť, ��� �Ҵ���, ��� ����Ʈ �������̽� �������̴�.
	ComPtr<ID3D12CommandQueue> m_pd3dCommandQueue;
	ComPtr<ID3D12CommandAllocator> m_pd3dCommandAllocator;
	ComPtr<ID3D12GraphicsCommandList> m_pd3dCommandList;

	//���� Ÿ�� ����, ������ �� �������̽� ������, ���� Ÿ�� ������ ������ ũ���̴�.
	ComPtr<ID3D12Resource> m_ppd3dRenderTargetBuffers[m_nSwapChainBuffers];
	ComPtr<ID3D12DescriptorHeap> m_pd3dRtvDescriptorHeap;
	D3D12_CPU_DESCRIPTOR_HANDLE	m_pd3dSwapRTVCPUHandles[m_nSwapChainBuffers];

	//����-���ٽ� ����, ������ �� �������̽� ������, ����-���ٽ� ������ ������ ũ���̴�.
	ComPtr<ID3D12Resource> m_pd3dDepthStencilBuffer;
	ComPtr<ID3D12DescriptorHeap> m_pd3dDsvDescriptorHeap;
	D3D12_CPU_DESCRIPTOR_HANDLE m_d3dDsvDescriptorCPUHandle;
public:
	DirectXProgram(UINT width, UINT height, std::wstring title);

	const UINT& GetProgramWidth() { return m_ProgramWidth; }
	const UINT& GetProgramHeight() { return m_ProgramHeight; }
	const std::wstring& GetProgramTitle() { return m_ProgramTitle; }

	void Init();
	void WaitForGpuComplete();
};

