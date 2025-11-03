#include "GBuffers.h"

GBuffers::GBuffers(ID3D12Device* device, UINT width, UINT height)
	: md3dDevice(device), mWidth(width), mHeight(height)
{
	mViewport = { 0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f };
	mScissorRect = { 0, 0, (int)width, (int)height };

	BuildResource();
}

void GBuffers::BuildResource()
{
	CD3DX12_RESOURCE_DESC texDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		mAlbedoFormat, mWidth, mHeight, 1, 1);
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	ThrowIfFailed(md3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		&optClear,
		IID_PPV_ARGS(&mGBufferResources[static_cast<uint32_t>(GBufferType::Albedo)])
	));

	texDesc.Format = mNormalPosFormat;
	optClear.Format = mNormalPosFormat;
	ThrowIfFailed(md3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		&optClear,
		IID_PPV_ARGS(&mGBufferResources[static_cast<uint32_t>(GBufferType::Normal)])
	));
	ThrowIfFailed(md3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		&optClear,
		IID_PPV_ARGS(&mGBufferResources[static_cast<uint32_t>(GBufferType::Position)])
	));
}

void GBuffers::BuildDescriptors(
	CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv,
	CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv,
	CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuRtv,
	UINT srvDescriptorSize,
	UINT rtvDescriptorSize
)
{
	mCpuSrv = hCpuSrv;
	mGpuSrv = hGpuSrv;
	mCpuRtv = hCpuRtv;

	mSRVDescriptorSize = srvDescriptorSize;
	mRTVDescriptorSize = rtvDescriptorSize;

	BuildDescriptors();
}

void GBuffers::BuildDescriptors()
{
	auto cpuSrv = mCpuSrv;
	auto cpuRtv = mCpuRtv;

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;
	// Albedo
	srvDesc.Format = mAlbedoFormat;
	md3dDevice->CreateShaderResourceView(
		mGBufferResources[static_cast<uint32_t>(GBufferType::Albedo)].Get(),
		&srvDesc,
		cpuSrv);
	// Normal
	srvDesc.Format = mNormalPosFormat;
	md3dDevice->CreateShaderResourceView(
		mGBufferResources[static_cast<uint32_t>(GBufferType::Normal)].Get(),
		&srvDesc,
		cpuSrv.Offset(1, mSRVDescriptorSize));
	// Position
	md3dDevice->CreateShaderResourceView(
		mGBufferResources[static_cast<uint32_t>(GBufferType::Position)].Get(),
		&srvDesc,
		cpuSrv.Offset(1, mSRVDescriptorSize));

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	rtvDesc.Texture2D.MipSlice = 0;
	rtvDesc.Texture2D.PlaneSlice = 0;
	// Albedo
	rtvDesc.Format = mAlbedoFormat;
	md3dDevice->CreateRenderTargetView(
		mGBufferResources[static_cast<uint32_t>(GBufferType::Albedo)].Get(),
		&rtvDesc,
		cpuRtv);
	// Normal
	rtvDesc.Format = mNormalPosFormat;
	md3dDevice->CreateRenderTargetView(
		mGBufferResources[static_cast<uint32_t>(GBufferType::Normal)].Get(),
		&rtvDesc,
		cpuRtv.Offset(1, mRTVDescriptorSize));
	// Position
	md3dDevice->CreateRenderTargetView(
		mGBufferResources[static_cast<uint32_t>(GBufferType::Position)].Get(),
		&rtvDesc,
		cpuRtv.Offset(1, mRTVDescriptorSize));
}

void GBuffers::OnResize(UINT width, UINT height)
{
	if (mWidth != width || mHeight != height)
	{
		mWidth = width;
		mHeight = height;

		mViewport = { 0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f };
		mScissorRect = { 0, 0, (int)width, (int)height };
		BuildResource();
	}
}