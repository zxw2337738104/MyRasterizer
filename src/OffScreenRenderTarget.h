#pragma once
#include "DXHelper.h"

class OffScreenRenderTarget
{
public:
	OffScreenRenderTarget(ID3D12Device* device, UINT width, UINT height);
	OffScreenRenderTarget(const OffScreenRenderTarget& rhs) = delete;
	OffScreenRenderTarget& operator=(const OffScreenRenderTarget& rhs) = delete;
	~OffScreenRenderTarget() = default;

	ID3D12Resource* BasePassResource();
	ID3D12Resource* BrightPassResource();

	CD3DX12_GPU_DESCRIPTOR_HANDLE BasePassSrv();
	CD3DX12_GPU_DESCRIPTOR_HANDLE BrightPassSrv();

	CD3DX12_CPU_DESCRIPTOR_HANDLE BasePassRtv();
	CD3DX12_CPU_DESCRIPTOR_HANDLE BrightPassRtv();

	void BuildDescriptors(
		CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv,
		CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv,
		CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuRtv,
		UINT srvSize, UINT rtvSize);

	void OnResize(UINT newWidth, UINT newHeight);

	DXGI_FORMAT Format();

	FLOAT* ClearColor();
private:
	void BuildResource();
	void BuildDescriptors();

	ID3D12Device* md3dDevice = nullptr;

	UINT mWidth = 0;
	UINT mHeight = 0;
	DXGI_FORMAT mFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	float Clear[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	CD3DX12_CLEAR_VALUE optClear{ mFormat, Clear };

	CD3DX12_CPU_DESCRIPTOR_HANDLE mBasePassCpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE mBasePassGpuSrv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE mBasePassCpuRtv;

	CD3DX12_CPU_DESCRIPTOR_HANDLE mBrightPassCpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE mBrightPassGpuSrv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE mBrightPassCpuRtv;

	ComPtr<ID3D12Resource> mBasePass = nullptr;
	ComPtr<ID3D12Resource> mBrightPass = nullptr;
};