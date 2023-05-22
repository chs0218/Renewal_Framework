#include "DirectXProgram.h"
#include "Win32Application.h"

DirectXProgram::DirectXProgram(UINT width, UINT height, std::wstring title)
{
	m_ProgramWidth = width;
	m_ProgramHeight = height;
	m_ProgramTitle = title;
}

void DirectXProgram::Init()
{
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
	ComPtr<IDXGIFactory4> pdxgiFactory;
	ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&pdxgiFactory)));


	// ��� ������ ���÷��� �����(�׷���ī��)�� �ִ��� Ȯ���ϰ� 
	// �ִٸ� �ش� ����ͷ� Direct3D Device �������̽� ��ü�� �����Ѵ�.
	ComPtr<IDXGIAdapter1> pd3dAdapter = NULL;
	for (UINT i = 0; DXGI_ERROR_NOT_FOUND != pdxgiFactory->EnumAdapters1(i, pd3dAdapter.GetAddressOf()); i++)
	{
		ComPtr<IDXGIOutput> pd3dOutput = NULL;
		DXGI_OUTPUT_DESC pd3dOutputDesc;

		pd3dAdapter->EnumOutputs(0, pd3dOutput.GetAddressOf());
		pd3dOutput->GetDesc(&pd3dOutputDesc);

		//����� ����ũž�� �ػ󵵸� �����ɴϴ�.
		m_DeskTopCoordinatesRect = pd3dOutputDesc.DesktopCoordinates;

		DXGI_ADAPTER_DESC1 dxgiAdapterDesc;
		pd3dAdapter->GetDesc1(&dxgiAdapterDesc);
		if (dxgiAdapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
			continue;

		if (SUCCEEDED(D3D12CreateDevice(pd3dAdapter.Get(), D3D_FEATURE_LEVEL_12_0, _uuidof(ID3D12Device), (void**)m_pd3dDevice.GetAddressOf())))
			break;
	}

	// ��� ������ ���÷��� �����(�׷���ī��)�� ���ٸ�
	// WARP ����Ͷ�� ������ �׷��� ī���� ������ ����Ʈ����� 
	// ���ķ���Ʈ�ϴ� ����Ʈ���� ��� ����ͷ� Direct3D Device �������̽� ��ü�� �����Ѵ�.
	if (!m_pd3dDevice)
	{
		pdxgiFactory->EnumWarpAdapter(_uuidof(IDXGIAdapter1), (void**)pd3dAdapter.GetAddressOf());
		D3D12CreateDevice(pd3dAdapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), (void**)m_pd3dDevice.GetAddressOf());
	}

	//����̽��� �����ϴ� ���� ������ ǰ�� ������ Ȯ���Ѵ�. 
	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS d3dMsaaQualityLevels;
	d3dMsaaQualityLevels.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	d3dMsaaQualityLevels.SampleCount = 4; //Msaa4x ���� ���ø�
	d3dMsaaQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	d3dMsaaQualityLevels.NumQualityLevels = 0;
	m_pd3dDevice->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &d3dMsaaQualityLevels, sizeof(D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS));
	m_nMsaa4xQualityLevels = d3dMsaaQualityLevels.NumQualityLevels;

	//���� ������ ǰ�� ������ 1���� ũ�� ���� ���ø��� Ȱ��ȭ�Ѵ�. 
	m_bMsaa4xEnable = (m_nMsaa4xQualityLevels > 1) ? true : false;

	//�潺�� �����ϰ� �潺 ���� 0���� �����Ѵ�. 
	ThrowIfFailed(m_pd3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, __uuidof(ID3D12Fence), (void**)m_pd3dFence.GetAddressOf()));
	for (UINT i = 0; i < m_nSwapChainBuffers; i++) m_nFenceValues[i] = 1;

	// �潺�� ����ȭ�� ���� �̺�Ʈ ��ü�� �����Ѵ�(�̺�Ʈ ��ü�� �ʱⰪ�� FALSE�̴�).
	// �̺�Ʈ�� ����Ǹ�(Signal) �̺�Ʈ�� ���� �ڵ������� FALSE�� �ǵ��� �����Ѵ�.
	m_hFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (m_hFenceEvent == nullptr)
		ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));

	::gnRtvDescriptorIncrementSize = m_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	::gnCbvSrvDescriptorIncrementSize = m_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);


	// ��� ť�� �����ϰ� �����Ѵ�.
	D3D12_COMMAND_QUEUE_DESC d3dCommandQueueDesc;
	::ZeroMemory(&d3dCommandQueueDesc, sizeof(D3D12_COMMAND_QUEUE_DESC));
	d3dCommandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	d3dCommandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	ThrowIfFailed(m_pd3dDevice->CreateCommandQueue(&d3dCommandQueueDesc, _uuidof(ID3D12CommandQueue), (void**)m_pd3dCommandQueue.GetAddressOf()));

	// ��� �Ҵ��ڸ� �����Ѵ�.
	ThrowIfFailed(m_pd3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, __uuidof(ID3D12CommandAllocator), (void**)m_pd3dCommandAllocator.GetAddressOf()));

	//��� ����Ʈ�� �����Ѵ�.
	ThrowIfFailed(m_pd3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_pd3dCommandAllocator.Get(), NULL, __uuidof(ID3D12GraphicsCommandList), (void**)m_pd3dCommandList.GetAddressOf()));
	
	//��� ����Ʈ�� �����Ǹ� ����(Open) �����̹Ƿ� ����(Closed) ���·� �����.
	m_pd3dCommandList->Close();

	// ���� Ÿ�� ������ ���� �����Ѵ�.
	D3D12_DESCRIPTOR_HEAP_DESC d3dDescriptorHeapDesc;
	::ZeroMemory(&d3dDescriptorHeapDesc, sizeof(D3D12_DESCRIPTOR_HEAP_DESC));
	d3dDescriptorHeapDesc.NumDescriptors = m_nSwapChainBuffers;
	d3dDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	d3dDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	d3dDescriptorHeapDesc.NodeMask = 0;

	ThrowIfFailed(m_pd3dDevice->CreateDescriptorHeap(&d3dDescriptorHeapDesc, __uuidof(ID3D12DescriptorHeap), (void**)m_pd3dRtvDescriptorHeap.GetAddressOf()));

	// ����-���ٽ� ������ ���� �����Ѵ�.
	d3dDescriptorHeapDesc.NumDescriptors = 1;
	d3dDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	ThrowIfFailed(m_pd3dDevice->CreateDescriptorHeap(&d3dDescriptorHeapDesc, __uuidof(ID3D12DescriptorHeap), (void**)m_pd3dDsvDescriptorHeap.GetAddressOf()));

	// ���� ü���� �����Ѵ�. DXGI ���� 1.2���� ���Ե� DXGI_SWAP_CHAIN_DESC1�� ����ߴ�.
	DXGI_SWAP_CHAIN_DESC1 dxgiSwapChainDesc;
	::ZeroMemory(&dxgiSwapChainDesc, sizeof(dxgiSwapChainDesc));
	dxgiSwapChainDesc.BufferCount = m_nSwapChainBuffers;
	dxgiSwapChainDesc.Width = m_ProgramWidth;
	dxgiSwapChainDesc.Height = m_ProgramHeight;
	dxgiSwapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	dxgiSwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	dxgiSwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	dxgiSwapChainDesc.SampleDesc.Count = (m_bMsaa4xEnable) ? 4 : 1;
	dxgiSwapChainDesc.SampleDesc.Quality = (m_bMsaa4xEnable) ? (m_nMsaa4xQualityLevels - 1) : 0;

	dxgiSwapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	ThrowIfFailed(pdxgiFactory->CreateSwapChainForHwnd(m_pd3dCommandQueue.Get(), Win32Application::GetHwnd(), &dxgiSwapChainDesc, nullptr, nullptr, (IDXGISwapChain1**)m_pdxgiSwapChain.GetAddressOf()));

	ThrowIfFailed(pdxgiFactory->MakeWindowAssociation(Win32Application::GetHwnd(), DXGI_MWA_NO_ALT_ENTER));
	m_nSwapChainBufferIndex = m_pdxgiSwapChain->GetCurrentBackBufferIndex();

	//����ü���� �ĸ� ���ۿ� ���� ���� Ÿ�� �並 �����Ѵ�. 
	D3D12_RENDER_TARGET_VIEW_DESC d3dRenderTargetViewDesc;
	d3dRenderTargetViewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	d3dRenderTargetViewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	d3dRenderTargetViewDesc.Texture2D.MipSlice = 0;
	d3dRenderTargetViewDesc.Texture2D.PlaneSlice = 0;

	D3D12_CPU_DESCRIPTOR_HANDLE d3dRtvCPUDescriptorHandle = m_pd3dRtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

	for (UINT i = 0; i < m_nSwapChainBuffers; i++)
	{
		m_pdxgiSwapChain->GetBuffer(i, __uuidof(ID3D12Resource), (void**)m_ppd3dRenderTargetBuffers[i].GetAddressOf());
		m_pd3dDevice->CreateRenderTargetView(m_ppd3dRenderTargetBuffers[i].Get(), &d3dRenderTargetViewDesc, d3dRtvCPUDescriptorHandle);
		m_pd3dSwapRTVCPUHandles[i] = d3dRtvCPUDescriptorHandle;
		d3dRtvCPUDescriptorHandle.ptr += ::gnRtvDescriptorIncrementSize;
	}

	//����-���ٽ� ���۸� �����Ѵ�. 
	D3D12_RESOURCE_DESC d3dResourceDesc;
	d3dResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	d3dResourceDesc.Alignment = 0;
	d3dResourceDesc.Width = m_ProgramWidth;
	d3dResourceDesc.Height = m_ProgramHeight;
	d3dResourceDesc.DepthOrArraySize = 1;
	d3dResourceDesc.MipLevels = 1;
	d3dResourceDesc.Format = DXGI_FORMAT_D32_FLOAT;
	d3dResourceDesc.SampleDesc.Count = (m_bMsaa4xEnable) ? 4 : 1;
	d3dResourceDesc.SampleDesc.Quality = (m_bMsaa4xEnable) ? (m_nMsaa4xQualityLevels - 1) : 0;
	d3dResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	d3dResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_HEAP_PROPERTIES d3dHeapProperties;
	::ZeroMemory(&d3dHeapProperties, sizeof(D3D12_HEAP_PROPERTIES));
	d3dHeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
	d3dHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	d3dHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	d3dHeapProperties.CreationNodeMask = 1;
	d3dHeapProperties.VisibleNodeMask = 1;

	D3D12_CLEAR_VALUE d3dClearValue;
	d3dClearValue.Format = DXGI_FORMAT_D32_FLOAT;
	d3dClearValue.DepthStencil.Depth = 1.0f;
	d3dClearValue.DepthStencil.Stencil = 0;

	ThrowIfFailed(m_pd3dDevice->CreateCommittedResource(
		&d3dHeapProperties, 
		D3D12_HEAP_FLAG_NONE, 
		&d3dResourceDesc, 
		D3D12_RESOURCE_STATE_DEPTH_WRITE, 
		&d3dClearValue, 
		__uuidof(ID3D12Resource), 
		(void**)m_pd3dDepthStencilBuffer.GetAddressOf()));


	//����-���ٽ� ���� �並 �����Ѵ�. 
	m_d3dDsvDescriptorCPUHandle = m_pd3dDsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	m_pd3dDevice->CreateDepthStencilView(m_pd3dDepthStencilBuffer.Get(), NULL, m_d3dDsvDescriptorCPUHandle);

	//�� ��ü�� �����ϱ� ���Ͽ� �ʿ��� �׷��� ��� ����Ʈ���� ��� ť�� �߰��Ѵ�. 
	m_pd3dCommandList->Close();
	ComPtr<ID3D12CommandList> ppd3dCommandLists[] = { m_pd3dCommandList };
	m_pd3dCommandQueue->ExecuteCommandLists(1, ppd3dCommandLists->GetAddressOf());

	WaitForGpuComplete();
}

void DirectXProgram::WaitForGpuComplete()
{
	UINT64 nFenceValue = ++m_nFenceValues[m_nSwapChainBufferIndex];
	HRESULT hResult = m_pd3dCommandQueue->Signal(m_pd3dFence.Get(), nFenceValue);

	if (m_pd3dFence->GetCompletedValue() < nFenceValue)
	{
		hResult = m_pd3dFence->SetEventOnCompletion(nFenceValue, m_hFenceEvent);
		::WaitForSingleObject(m_hFenceEvent, INFINITE);
	}
}
