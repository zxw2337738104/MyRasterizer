#pragma once
#include "DXHelper.h"

using Microsoft::WRL::ComPtr;

class GBuffers
{
public:
	enum class GBufferType
	{
		Albedo = 0,
		Normal,
		Position,
		//MetallicRoughness,
		Count
	};
	GBuffers(ID3D12Device* device, UINT width, UINT height);
	GBuffers(const GBuffers& rhs) = delete;
	GBuffers& operator=(const GBuffers& rhs) = delete;
	~GBuffers() = default;

	UINT Width() const { return mWidth; }
	UINT Height() const { return mHeight; }

	ID3D12Resource* Resource(GBufferType type) const
	{
		return mGBufferResources[static_cast<uint32_t>(type)].Get();
	}

	CD3DX12_GPU_DESCRIPTOR_HANDLE Srv(GBufferType t) const 
	{ 
		auto gpuSrv = mGpuSrv;
		return gpuSrv.Offset(static_cast<INT>(t), mSRVDescriptorSize);
	}
	CD3DX12_CPU_DESCRIPTOR_HANDLE Rtv(GBufferType t) const 
	{ 
		auto cpuRtv = mCpuRtv;
		return cpuRtv.Offset(static_cast<INT>(t), mRTVDescriptorSize);
	}

	D3D12_VIEWPORT Viewport() const { return mViewport; }
	D3D12_RECT ScissorRect() const { return mScissorRect; }

	void BuildDescriptors(
		CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv,
		CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv,
		CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuRtv,
		UINT srvDescriptorSize,
		UINT rtvDescriptorSize
	);

	void OnResize(UINT width, UINT height);

	void CleanAll(ID3D12GraphicsCommandList* cmdList)
	{
		for (int i = 0; i < static_cast<int>(GBufferType::Count); ++i)
		{
			cmdList->ClearRenderTargetView(
				Rtv(GBufferType(i)),
				mClearColor,
				0,
				nullptr);
		}
	}

	static constexpr uint32_t GetGBufferCount()
	{
		return static_cast<uint32_t>(GBufferType::Count);
	}

private:

	void BuildDescriptors();
	void BuildResource();

	ID3D12Device* md3dDevice = nullptr;

	D3D12_VIEWPORT mViewport;
	D3D12_RECT mScissorRect;

	UINT mWidth = 0;
	UINT mHeight = 0;

	DXGI_FORMAT mAlbedoFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	DXGI_FORMAT mNormalPosFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;

	float mClearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	CD3DX12_CLEAR_VALUE optClear = { mAlbedoFormat, mClearColor };

	ComPtr<ID3D12Resource> mGBufferResources[static_cast<uint32_t>(GBufferType::Count)];
	
	UINT mRTVDescriptorSize = 0;
	UINT mSRVDescriptorSize = 0;

	CD3DX12_CPU_DESCRIPTOR_HANDLE mCpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE mGpuSrv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE mCpuRtv;
};