#include "DirectXProgram.h"
#include "Win32Application.h"

DirectXProgram::DirectXProgram(UINT width, UINT height, std::wstring title)
{
	m_ProgramWidth = width;
	m_ProgramHeight = height;
	m_ProgramTitle = title;

	m_AspectRatio = static_cast<float>(m_ProgramWidth) / static_cast<float>(m_ProgramHeight);
	m_Viewport = { 0, 0, static_cast<float>(m_ProgramWidth) ,  static_cast<float>(m_ProgramHeight), 0.0f, 1.0f };
	m_ScissorRect = { 0, 0, static_cast<LONG>(m_ProgramWidth) ,  static_cast<LONG>(m_ProgramHeight) };
}
void DirectXProgram::Init()
{
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
	ComPtr<IDXGIFactory4> pdxgiFactory;
	ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&pdxgiFactory)));

	CreateDirectXDevice(pdxgiFactory.Get());
	CheckMsaa4xLevels();
	CreateFence();
	CreateCommandQueueAndList();
	CreateDescriptorHeaps();
	CreateSwapChain(pdxgiFactory.Get());
	CreateRenderTargetViews();
	CreateDepthStencilView();

	BuildLevel();
}
void DirectXProgram::CreateDirectXDevice(IDXGIFactory4* pdxgiFactory)
{
	// 사용 가능한 디스플레이 어답터(그래픽카드)가 있는지 확인하고 
	// 있다면 해당 어답터로 Direct3D Device 인터페이스 객체를 생성한다.
	ComPtr<IDXGIAdapter1> pd3dAdapter = NULL;
	for (UINT i = 0;
		DXGI_ERROR_NOT_FOUND != pdxgiFactory->EnumAdapters1(i, pd3dAdapter.GetAddressOf());
		i++)
	{
		ComPtr<IDXGIOutput> pd3dOutput = NULL;
		DXGI_OUTPUT_DESC pd3dOutputDesc;

		pd3dAdapter->EnumOutputs(0, pd3dOutput.GetAddressOf());
		pd3dOutput->GetDesc(&pd3dOutputDesc);

		//사용자 데스크탑의 해상도를 가져옵니다.
		m_DeskTopCoordinatesRect = pd3dOutputDesc.DesktopCoordinates;

		DXGI_ADAPTER_DESC1 dxgiAdapterDesc;
		pd3dAdapter->GetDesc1(&dxgiAdapterDesc);
		if (dxgiAdapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
			continue;

		if (SUCCEEDED(D3D12CreateDevice(
			pd3dAdapter.Get(),
			D3D_FEATURE_LEVEL_12_0,
			_uuidof(ID3D12Device),
			(void**)m_pd3dDevice.GetAddressOf())))
			break;
	}

	// 사용 가능한 디스플레이 어답터(그래픽카드)가 없다면
	// WARP 어댑터라는 물리적 그래픽 카드의 동작을 소프트웨어에서 
	// 에뮬레이트하는 소프트웨어 기반 어댑터로 Direct3D Device 인터페이스 객체를 생성한다.
	if (!m_pd3dDevice)
	{
		pdxgiFactory->EnumWarpAdapter(
			_uuidof(IDXGIAdapter1),
			(void**)pd3dAdapter.GetAddressOf());

		D3D12CreateDevice(
			pd3dAdapter.Get(),
			D3D_FEATURE_LEVEL_11_0,
			_uuidof(ID3D12Device),
			(void**)m_pd3dDevice.GetAddressOf());
	}

	// 디바이스를 생성한뒤 디스크립터핸들의 증가 크기를 설정해준다.
	gnRtvDescriptorIncrementSize =
		m_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	gnCbvSrvDescriptorIncrementSize =
		m_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}
void DirectXProgram::CheckMsaa4xLevels()
{
	//디바이스가 지원하는 다중 샘플의 품질 수준을 확인한다. 
	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS d3dMsaaQualityLevels;
	d3dMsaaQualityLevels.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	d3dMsaaQualityLevels.SampleCount = 4; //Msaa4x 다중 샘플링
	d3dMsaaQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	d3dMsaaQualityLevels.NumQualityLevels = 0;
	m_pd3dDevice->CheckFeatureSupport(
		D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
		&d3dMsaaQualityLevels,
		sizeof(D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS));

	m_nMsaa4xQualityLevels = d3dMsaaQualityLevels.NumQualityLevels;

	//다중 샘플의 품질 수준이 1보다 크면 다중 샘플링을 활성화한다. 
	m_bMsaa4xEnable = (m_nMsaa4xQualityLevels > 1) ? true : false;
}
void DirectXProgram::CreateFence()
{
	//펜스를 생성하고 펜스 값을 0으로 설정한다. 
	ThrowIfFailed(m_pd3dDevice->CreateFence(
		0,
		D3D12_FENCE_FLAG_NONE,
		__uuidof(ID3D12Fence),
		(void**)m_pd3dFence.GetAddressOf()));

	for (UINT i = 0; i < m_nSwapChainBuffers; i++) m_nFenceValues[i] = 1;

	// 펜스와 동기화를 위한 이벤트 객체를 생성한다(이벤트 객체의 초기값을 FALSE이다).
	// 이벤트가 실행되면(Signal) 이벤트의 값을 자동적으로 FALSE가 되도록 생성한다.
	m_hFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (m_hFenceEvent == nullptr)
		ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
}
void DirectXProgram::CreateCommandQueueAndList()
{
	// 명령 큐를 서술하고 생성한다.
	D3D12_COMMAND_QUEUE_DESC d3dCommandQueueDesc;
	ZeroMemory(&d3dCommandQueueDesc, sizeof(D3D12_COMMAND_QUEUE_DESC));
	d3dCommandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	d3dCommandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	ThrowIfFailed(m_pd3dDevice->CreateCommandQueue(
		&d3dCommandQueueDesc,
		_uuidof(ID3D12CommandQueue),
		(void**)m_pd3dCommandQueue.GetAddressOf()));

	// 명령 할당자를 생성한다.
	ThrowIfFailed(m_pd3dDevice->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		__uuidof(ID3D12CommandAllocator),
		(void**)m_pd3dCommandAllocator.GetAddressOf()));

	//명령 리스트를 생성한다.
	ThrowIfFailed(m_pd3dDevice->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		m_pd3dCommandAllocator.Get(),
		NULL,
		__uuidof(ID3D12GraphicsCommandList),
		(void**)m_pd3dCommandList.GetAddressOf()));

	//명령 리스트는 생성되면 열린(Open) 상태이므로 닫힌(Closed) 상태로 만든다.
	m_pd3dCommandList->Close();
}
void DirectXProgram::CreateDescriptorHeaps()
{
	// 렌더 타겟 서술자 힙을 생성한다.
	D3D12_DESCRIPTOR_HEAP_DESC d3dDescriptorHeapDesc;
	ZeroMemory(&d3dDescriptorHeapDesc, sizeof(D3D12_DESCRIPTOR_HEAP_DESC));
	d3dDescriptorHeapDesc.NumDescriptors = m_nSwapChainBuffers;
	d3dDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	d3dDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	d3dDescriptorHeapDesc.NodeMask = 0;

	ThrowIfFailed(m_pd3dDevice->CreateDescriptorHeap(
		&d3dDescriptorHeapDesc,
		__uuidof(ID3D12DescriptorHeap),
		(void**)m_pd3dRtvDescriptorHeap.GetAddressOf()));

	// 깊이-스텐실 서술자 힙을 생성한다.
	d3dDescriptorHeapDesc.NumDescriptors = 1;
	d3dDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	ThrowIfFailed(m_pd3dDevice->CreateDescriptorHeap(
		&d3dDescriptorHeapDesc,
		__uuidof(ID3D12DescriptorHeap),
		(void**)m_pd3dDsvDescriptorHeap.GetAddressOf()));
}
void DirectXProgram::CreateSwapChain(IDXGIFactory4* pdxgiFactory)
{
	// 스왑 체인을 생성한다. DXGI 버전 1.2에서 도입된 DXGI_SWAP_CHAIN_DESC1를 사용했다.
	DXGI_SWAP_CHAIN_DESC1 dxgiSwapChainDesc;
	ZeroMemory(&dxgiSwapChainDesc, sizeof(dxgiSwapChainDesc));
	dxgiSwapChainDesc.BufferCount = m_nSwapChainBuffers;
	dxgiSwapChainDesc.Width = m_ProgramWidth;
	dxgiSwapChainDesc.Height = m_ProgramHeight;
	dxgiSwapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	dxgiSwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	dxgiSwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	dxgiSwapChainDesc.SampleDesc.Count = (m_bMsaa4xEnable) ? 4 : 1;
	dxgiSwapChainDesc.SampleDesc.Quality = (m_bMsaa4xEnable) ? (m_nMsaa4xQualityLevels - 1) : 0;

	dxgiSwapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	ThrowIfFailed(pdxgiFactory->CreateSwapChainForHwnd(
		m_pd3dCommandQueue.Get(),
		Win32Application::GetHwnd(),
		&dxgiSwapChainDesc,
		nullptr,
		nullptr,
		(IDXGISwapChain1**)m_pdxgiSwapChain.GetAddressOf()));

	ThrowIfFailed(pdxgiFactory->MakeWindowAssociation(
		Win32Application::GetHwnd(),
		DXGI_MWA_NO_ALT_ENTER));

	m_nSwapChainBufferIndex = m_pdxgiSwapChain->GetCurrentBackBufferIndex();
}
void DirectXProgram::CreateRenderTargetViews()
{
	//스왑체인의 후면 버퍼에 대한 렌더 타겟 뷰를 생성한다. 
	D3D12_RENDER_TARGET_VIEW_DESC d3dRenderTargetViewDesc;
	d3dRenderTargetViewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	d3dRenderTargetViewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	d3dRenderTargetViewDesc.Texture2D.MipSlice = 0;
	d3dRenderTargetViewDesc.Texture2D.PlaneSlice = 0;

	D3D12_CPU_DESCRIPTOR_HANDLE d3dRtvCPUDescriptorHandle =
		m_pd3dRtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

	for (UINT i = 0; i < m_nSwapChainBuffers; i++)
	{
		m_pdxgiSwapChain->GetBuffer(
			i,
			__uuidof(ID3D12Resource),
			(void**)m_ppd3dRenderTargetBuffers[i].GetAddressOf());

		m_pd3dDevice->CreateRenderTargetView(
			m_ppd3dRenderTargetBuffers[i].Get(),
			&d3dRenderTargetViewDesc,
			d3dRtvCPUDescriptorHandle);

		m_pd3dSwapRTVCPUHandles[i] = d3dRtvCPUDescriptorHandle;
		d3dRtvCPUDescriptorHandle.ptr += ::gnRtvDescriptorIncrementSize;
	}
}
void DirectXProgram::CreateDepthStencilView()
{
	//깊이-스텐실 버퍼를 생성한다. 
	D3D12_RESOURCE_DESC d3dResourceDesc;
	d3dResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	d3dResourceDesc.Alignment = 0;
	d3dResourceDesc.Width = m_ProgramWidth;
	d3dResourceDesc.Height = m_ProgramHeight;
	d3dResourceDesc.DepthOrArraySize = 1;
	d3dResourceDesc.MipLevels = 1;
	d3dResourceDesc.Format = DXGI_FORMAT_D32_FLOAT;
	d3dResourceDesc.SampleDesc.Count = (m_bMsaa4xEnable) ? 4 : 1;
	d3dResourceDesc.SampleDesc.Quality =
		(m_bMsaa4xEnable) ? (m_nMsaa4xQualityLevels - 1) : 0;
	d3dResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	d3dResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_HEAP_PROPERTIES d3dHeapProperties;
	ZeroMemory(&d3dHeapProperties, sizeof(D3D12_HEAP_PROPERTIES));
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


	//깊이-스텐실 버퍼 뷰를 생성한다. 
	m_d3dDsvDescriptorCPUHandle = m_pd3dDsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	m_pd3dDevice->CreateDepthStencilView(
		m_pd3dDepthStencilBuffer.Get(),
		NULL,
		m_d3dDsvDescriptorCPUHandle);
}
void DirectXProgram::BuildLevel()
{
	m_pd3dCommandList->Reset(m_pd3dCommandAllocator.Get(), NULL);

	{
		ComPtr<ID3DBlob> vertexShader;
		ComPtr<ID3DBlob> pixelShader;
#if defined(_DEBUG)
		UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
		UINT compileFlags = 0;
#endif
		// 버텍스 쉐이더, 픽셀 쉐이더를 컴파일
		ThrowIfFailed(D3DCompileFromFile(
			L"shaders.hlsl", 
			nullptr, 
			nullptr, 
			"VSMain", 
			"vs_5_0", 
			compileFlags, 
			0, 
			&vertexShader, 
			nullptr));

		ThrowIfFailed(D3DCompileFromFile(
			L"shaders.hlsl", 
			nullptr, 
			nullptr, 
			"PSMain", 
			"ps_5_0", 
			compileFlags, 
			0, 
			&pixelShader, 
			nullptr));

		// 입력 레이아웃을 정의
		D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, 
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },

			{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};


		//루트 시그니쳐를 생성
		D3D12_ROOT_PARAMETER pd3dRootParameters;
		D3D12_STATIC_SAMPLER_DESC d3dSamplerDesc;

		D3D12_ROOT_SIGNATURE_FLAGS d3dRootSignatureFlags = 
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | 
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_STREAM_OUTPUT | 
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS | 
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;

		D3D12_ROOT_SIGNATURE_DESC d3dRootSignatureDesc;
		::ZeroMemory(&d3dRootSignatureDesc, sizeof(D3D12_ROOT_SIGNATURE_DESC));
		d3dRootSignatureDesc.NumParameters = 0;
		d3dRootSignatureDesc.pParameters = &pd3dRootParameters;
		d3dRootSignatureDesc.NumStaticSamplers = 0;
		d3dRootSignatureDesc.pStaticSamplers = &d3dSamplerDesc;
		d3dRootSignatureDesc.Flags = d3dRootSignatureFlags;

		ID3DBlob* pd3dSignatureBlob = nullptr;
		ID3DBlob* pd3dErrorBlob = nullptr;

		ThrowIfFailed(D3D12SerializeRootSignature(
			&d3dRootSignatureDesc, 
			D3D_ROOT_SIGNATURE_VERSION_1, 
			&pd3dSignatureBlob, 
			&pd3dErrorBlob));

		ThrowIfFailed(m_pd3dDevice->CreateRootSignature(
			0, 
			pd3dSignatureBlob->GetBufferPointer(), 
			pd3dSignatureBlob->GetBufferSize(), 
			__uuidof(ID3D12RootSignature), 
			(void**)&m_RootSignature));

		if (pd3dSignatureBlob) pd3dSignatureBlob->Release();
		if (pd3dErrorBlob) pd3dErrorBlob->Release();

		// 그래픽스 파이프라인 상태를 정의
		{
			D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
			psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
			psoDesc.pRootSignature = m_RootSignature.Get();

			// 버텍스 쉐이더와 픽셀 쉐이더를 CD3DX12_SHADER_BYTECODE를 이용해 설정
			psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
			psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());

			// 래스터라이저와 블렌딩을 d3dx12.h 에서 제공하는 기본 상태로 설정
			psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
			psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

			psoDesc.DepthStencilState.DepthEnable = FALSE;
			psoDesc.DepthStencilState.StencilEnable = FALSE;
			psoDesc.SampleMask = UINT_MAX;
			psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			psoDesc.NumRenderTargets = 1;
			psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
			psoDesc.SampleDesc.Count = 1;

			// 파이프라인 스테이트를 생성
			ThrowIfFailed(m_pd3dDevice->CreateGraphicsPipelineState(
				&psoDesc,
				__uuidof(ID3D12PipelineState),
				(void**)m_pd3dPipelineState.GetAddressOf()));
		}
	}


	{
		//렌더링에 사용할 삼각형의 정점 위치와 색상을 정의, 버퍼를 생성해 VertexBufferView로 저장
		Vertex triangleVertices[] =
		{
			{ { 0.0f, 0.25f * m_AspectRatio, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
			{ { 0.25f, -0.25f * m_AspectRatio, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
			{ { -0.25f, -0.25f * m_AspectRatio, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } }
		};

		const UINT vertexBufferSize = sizeof(triangleVertices);

		m_pd3dPosColBuffer = CreateBufferResource(
			m_pd3dDevice.Get(), 
			m_pd3dCommandList.Get(), 
			triangleVertices, 
			sizeof(Vertex) * 3, 
			D3D12_HEAP_TYPE_DEFAULT, 
			D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, 
			&m_pd3dPosColUploadBuffer);

		m_pd3dVertexBufferView.BufferLocation = m_pd3dPosColBuffer->GetGPUVirtualAddress();
		m_pd3dVertexBufferView.StrideInBytes = sizeof(Vertex);
		m_pd3dVertexBufferView.SizeInBytes = sizeof(Vertex) * 3;
	}

	//레벨에 사용될 객체를 생성하기 위하여 필요한 그래픽 명령 리스트들을 명령 큐에 추가한다. 
	m_pd3dCommandList->Close();

	// 레벨을 빌드하는 명령리스트들을 실행한다.
	ComPtr<ID3D12CommandList> ppd3dCommandLists[] = { m_pd3dCommandList };
	m_pd3dCommandQueue->ExecuteCommandLists(1, ppd3dCommandLists->GetAddressOf());

	// 명령리스트에 담긴 명령들이 모두 동작할 때까지 기다린다.
	WaitForGpuComplete();
}
void DirectXProgram::RenderLevel()	
{
	// 명령할당자를 초기화한다.
	ThrowIfFailed(m_pd3dCommandAllocator->Reset());

	// 명령리스트를 초기화한다.
	ThrowIfFailed(m_pd3dCommandList->Reset(m_pd3dCommandAllocator.Get(), NULL));

	// 파이프라인 상태를 설정한다.
	m_pd3dCommandList->SetPipelineState(m_pd3dPipelineState.Get());

	// 루트시그니쳐, 뷰포트, 시저사각형을 설정한다.
	m_pd3dCommandList->SetGraphicsRootSignature(m_RootSignature.Get());
	m_pd3dCommandList->RSSetViewports(1, &m_Viewport);
	m_pd3dCommandList->RSSetScissorRects(1, &m_ScissorRect);

	SynchronizeResourceTransition(
		m_pd3dCommandList.Get(), 
		m_ppd3dRenderTargetBuffers[m_nSwapChainBufferIndex].Get(), 
		D3D12_RESOURCE_STATE_PRESENT, 
		D3D12_RESOURCE_STATE_RENDER_TARGET);

	const float clear_color_with_alpha[4] = { 0.f, 0.f, 0.f, 0.f };

	// 렌더타겟과 깊이스텐실뷰를 초기화
	m_pd3dCommandList->ClearRenderTargetView(
		m_pd3dSwapRTVCPUHandles[m_nSwapChainBufferIndex], 
		clear_color_with_alpha, 
		0, 
		NULL);

	m_pd3dCommandList->ClearDepthStencilView(
		m_d3dDsvDescriptorCPUHandle, 
		D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 
		1.0f, 
		0, 
		0, 
		NULL);

	// 렌더링할 렌더타겟을 출력병합기에 설정
	m_pd3dCommandList->OMSetRenderTargets(
		1, 
		&m_pd3dSwapRTVCPUHandles[m_nSwapChainBufferIndex], 
		FALSE, 
		&m_d3dDsvDescriptorCPUHandle);

	// 그릴 삼각형의 PrimitiveTopology를 설정, VertexBufferView를 연결해 삼각형을 렌더링
	m_pd3dCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_pd3dCommandList->IASetVertexBuffers(0, 1, &m_pd3dVertexBufferView);
	m_pd3dCommandList->DrawInstanced(3, 1, 0, 0);

	SynchronizeResourceTransition(
		m_pd3dCommandList.Get(), 
		m_ppd3dRenderTargetBuffers[m_nSwapChainBufferIndex].Get(), 
		D3D12_RESOURCE_STATE_RENDER_TARGET, 
		D3D12_RESOURCE_STATE_PRESENT);

	// 명령 리스트를 닫힌 상태로 만든다. 
	ThrowIfFailed(m_pd3dCommandList->Close());

	// 명령 리스트를 명령 큐에 추가하여 실행한다.
	ID3D12CommandList* ppd3dCommandLists[] = { m_pd3dCommandList.Get() };
	m_pd3dCommandQueue->ExecuteCommandLists(1, ppd3dCommandLists);

	// GPU가 모든 명령 리스트를 실행할 때 까지 기다린다.
	WaitForGpuComplete();

	// GPU가 모든 동작을 끝내면 Present를 호출해 렌더타겟을 보여준다.
	m_pdxgiSwapChain->Present(0, 0);

	m_nSwapChainBufferIndex = m_pdxgiSwapChain->GetCurrentBackBufferIndex();

	// Present가 완료되는 것을 기다린다.
	WaitForGpuComplete();
}
void DirectXProgram::WaitForGpuComplete()
{
	UINT64 nFenceValue = ++m_nFenceValues[m_nSwapChainBufferIndex];
	ThrowIfFailed(m_pd3dCommandQueue->Signal(m_pd3dFence.Get(), nFenceValue));

	if (m_pd3dFence->GetCompletedValue() < nFenceValue)
	{
		ThrowIfFailed(m_pd3dFence->SetEventOnCompletion(nFenceValue, m_hFenceEvent));
		::WaitForSingleObject(m_hFenceEvent, INFINITE);
	}
}
