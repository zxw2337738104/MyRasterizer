#pragma once
#include "DXHelper.h"

class TAA
{
public:
    TAA(ID3D12Device* device, UINT width, UINT height);
    TAA(const TAA& rhs) = delete;
    TAA& operator=(const TAA& rhs) = delete;
    ~TAA() = default;

    UINT Width() const { return mWidth; }
    UINT Height() const { return mHeight; }

    ID3D12Resource* HistoryBuffer() const { return mHistoryBuffer.Get(); }
    ID3D12Resource* MotionVectorBuffer() const { return mMotionVectorBuffer.Get(); }
    ID3D12Resource* OutputBuffer() const { return mOutputBuffer.Get(); }

    CD3DX12_GPU_DESCRIPTOR_HANDLE HistorySrv() const { return mHistoryGpuSrv; }
    CD3DX12_GPU_DESCRIPTOR_HANDLE MotionVectorSrv() const { return mMotionVectorGpuSrv; }
    CD3DX12_GPU_DESCRIPTOR_HANDLE OutputSrv() const { return mOutputGpuSrv; }

    CD3DX12_CPU_DESCRIPTOR_HANDLE MotionVectorRtv() const { return mMotionVectorCpuRtv; }
    CD3DX12_CPU_DESCRIPTOR_HANDLE OutputRtv() const { return mOutputCpuRtv; }

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

    // 获取当前帧的抖动偏移（NDC空间）
    XMFLOAT2 GetJitterOffset(UINT frameIndex) const;

    // 交换历史缓冲区（每帧结束时调用）
    void SwapHistoryBuffer(ID3D12GraphicsCommandList* cmdList);

    static const UINT JitterSampleCount = 16; // Halton序列采样数

private:
    void BuildDescriptors();
    void BuildResources();
    void GenerateHaltonSequence();

    float HaltonSequence(UINT index, UINT base);

    ID3D12Device* md3dDevice = nullptr;

    D3D12_VIEWPORT mViewport;
    D3D12_RECT mScissorRect;

    UINT mWidth = 0;
    UINT mHeight = 0;

    DXGI_FORMAT mColorFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
    DXGI_FORMAT mMotionVectorFormat = DXGI_FORMAT_R16G16_FLOAT;

    // 历史帧缓冲区
    ComPtr<ID3D12Resource> mHistoryBuffer = nullptr;
    // 运动向量缓冲区
    ComPtr<ID3D12Resource> mMotionVectorBuffer = nullptr;
    // TAA输出缓冲区
    ComPtr<ID3D12Resource> mOutputBuffer = nullptr;

    // SRV描述符
    CD3DX12_CPU_DESCRIPTOR_HANDLE mHistoryCpuSrv;
    CD3DX12_GPU_DESCRIPTOR_HANDLE mHistoryGpuSrv;
    CD3DX12_CPU_DESCRIPTOR_HANDLE mMotionVectorCpuSrv;
    CD3DX12_GPU_DESCRIPTOR_HANDLE mMotionVectorGpuSrv;
    CD3DX12_CPU_DESCRIPTOR_HANDLE mOutputCpuSrv;
    CD3DX12_GPU_DESCRIPTOR_HANDLE mOutputGpuSrv;

    // RTV描述符
    CD3DX12_CPU_DESCRIPTOR_HANDLE mMotionVectorCpuRtv;
    CD3DX12_CPU_DESCRIPTOR_HANDLE mOutputCpuRtv;

    UINT mSrvDescriptorSize = 0;
    UINT mRtvDescriptorSize = 0;

    // Halton抖动序列
    XMFLOAT2 mJitterOffsets[JitterSampleCount];
};