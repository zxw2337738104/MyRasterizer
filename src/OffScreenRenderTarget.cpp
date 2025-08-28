#include "OffScreenRenderTarget.h"

OffScreenRenderTarget::OffScreenRenderTarget(
	ID3D12Device* device, UINT width, UINT height)
	: md3dDevice(device), mWidth(width), mHeight(height)
{
	BuildResource();
}

ID3D12Resource* OffScreenRenderTarget::BasePassResource()
{
	return mBasePass.Get();
}

ID3D12Resource* OffScreenRenderTarget::BrightPassResource()
{
	return mBrightPass.Get();
}

CD3DX12_GPU_DESCRIPTOR_HANDLE OffScreenRenderTarget::BasePassSrv()
{
	return mBasePassGpuSrv;
}

CD3DX12_GPU_DESCRIPTOR_HANDLE OffScreenRenderTarget::BrightPassSrv()
{
	return mBrightPassGpuSrv;
}

CD3DX12_CPU_DESCRIPTOR_HANDLE OffScreenRenderTarget::BasePassRtv()
{
	return mBasePassCpuRtv;
}

CD3DX12_CPU_DESCRIPTOR_HANDLE OffScreenRenderTarget::BrightPassRtv()
{
	return mBrightPassCpuRtv;
}

DXGI_FORMAT OffScreenRenderTarget::Format()
{
	return mFormat;
}

FLOAT* OffScreenRenderTarget::ClearColor()
{
	return optClear.Color;
}

void OffScreenRenderTarget::BuildResource()
{
	// Create the texture.
	D3D12_RESOURCE_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Alignment = 0;
	texDesc.Width = mWidth;
	texDesc.Height = mHeight;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.Format = mFormat;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	ThrowIfFailed(md3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		&optClear,
		IID_PPV_ARGS(&mBasePass)));

	md3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_COPY_SOURCE,
		&optClear,
		IID_PPV_ARGS(&mBrightPass));
}

void OffScreenRenderTarget::BuildDescriptors(
	CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv,
	CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv,
	CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuRtv,
	UINT srvSize, UINT rtvSize)
{
	mBasePassCpuSrv = hCpuSrv;
	mBasePassGpuSrv = hGpuSrv;
	mBasePassCpuRtv = hCpuRtv;

	mBrightPassCpuSrv = CD3DX12_CPU_DESCRIPTOR_HANDLE(hCpuSrv, 1, srvSize);
	mBrightPassGpuSrv = CD3DX12_GPU_DESCRIPTOR_HANDLE(hGpuSrv, 1, srvSize);
	mBrightPassCpuRtv = CD3DX12_CPU_DESCRIPTOR_HANDLE(hCpuRtv, 1, rtvSize);

	BuildDescriptors();
}

void OffScreenRenderTarget::BuildDescriptors()
{
	// Create SRV to resource so we can read the texture in a shader.
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = mFormat;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	md3dDevice->CreateShaderResourceView(mBasePass.Get(), &srvDesc, mBasePassCpuSrv);
	md3dDevice->CreateShaderResourceView(mBrightPass.Get(), &srvDesc, mBrightPassCpuSrv);

	// Create RTV to resource so we can render to the texture.
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = mFormat;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	rtvDesc.Texture2D.MipSlice = 0;
	rtvDesc.Texture2D.PlaneSlice = 0;
	md3dDevice->CreateRenderTargetView(mBasePass.Get(), &rtvDesc, mBasePassCpuRtv);
	md3dDevice->CreateRenderTargetView(mBrightPass.Get(), &rtvDesc, mBrightPassCpuRtv);
}

void OffScreenRenderTarget::OnResize(UINT newWidth, UINT newHeight)
{
	if (mWidth != newWidth || mHeight != newHeight)
	{
		mWidth = newWidth;
		mHeight = newHeight;
		BuildResource();
	}
}