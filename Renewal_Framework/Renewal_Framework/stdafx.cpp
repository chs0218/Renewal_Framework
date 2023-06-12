#include "stdafx.h"

UINT gnCbvSrvDescriptorIncrementSize = 0;
UINT gnRtvDescriptorIncrementSize = 0;

ComPtr<ID3D12Resource> CreateBufferResource(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, void* pData, UINT nBytes, D3D12_HEAP_TYPE d3dHeapType, D3D12_RESOURCE_STATES d3dResourceStates, ID3D12Resource** ppd3dUploadBuffer)
{
	ComPtr<ID3D12Resource> pd3dBuffer;

	D3D12_RESOURCE_STATES d3dResourceInitialStates = D3D12_RESOURCE_STATE_COPY_DEST;
	if (d3dHeapType == D3D12_HEAP_TYPE_UPLOAD) d3dResourceInitialStates = D3D12_RESOURCE_STATE_GENERIC_READ;
	else if (d3dHeapType == D3D12_HEAP_TYPE_READBACK) d3dResourceInitialStates = D3D12_RESOURCE_STATE_COPY_DEST;

	ThrowIfFailed(pd3dDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(d3dHeapType),
		D3D12_HEAP_FLAG_NONE, 
		&CD3DX12_RESOURCE_DESC::Buffer(nBytes),
		d3dResourceInitialStates, 
		NULL, 
		__uuidof(ID3D12Resource), 
		(void**)pd3dBuffer.GetAddressOf()));

	if (pData)
	{
		switch (d3dHeapType)
		{
		case D3D12_HEAP_TYPE_DEFAULT:
		{
			if (ppd3dUploadBuffer)
			{
				pd3dDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
					D3D12_HEAP_FLAG_NONE, 
					&CD3DX12_RESOURCE_DESC::Buffer(nBytes),
					D3D12_RESOURCE_STATE_GENERIC_READ, 
					NULL, 
					__uuidof(ID3D12Resource), 
					(void**)ppd3dUploadBuffer);

				UINT8* pBufferDataBegin = NULL;
				CD3DX12_RANGE d3dReadRange = { 0, 0 };
				ThrowIfFailed((*ppd3dUploadBuffer)->Map(0, &d3dReadRange, reinterpret_cast<void**>(&pBufferDataBegin)));
				memcpy(pBufferDataBegin, pData, nBytes);
				(*ppd3dUploadBuffer)->Unmap(0, NULL);

				pd3dCommandList->CopyResource(pd3dBuffer.Get(), *ppd3dUploadBuffer);

				pd3dCommandList->ResourceBarrier(1, 
					&CD3DX12_RESOURCE_BARRIER::Transition(
						pd3dBuffer.Get(),
						D3D12_RESOURCE_STATE_COPY_DEST,
						d3dResourceStates));
			}
			break;
		}
		case D3D12_HEAP_TYPE_UPLOAD:
		{
			D3D12_RANGE d3dReadRange = { 0, 0 };
			UINT8* pBufferDataBegin = NULL;
			pd3dBuffer->Map(0, &d3dReadRange, reinterpret_cast<void**>(&pBufferDataBegin));
			memcpy(pBufferDataBegin, pData, nBytes);
			pd3dBuffer->Unmap(0, NULL);
			break;
		}
		case D3D12_HEAP_TYPE_READBACK:
			break;
		}
	}
	return(pd3dBuffer);
}