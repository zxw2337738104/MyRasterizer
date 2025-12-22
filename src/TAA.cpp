#include "TAA.h"

TAA::TAA(ID3D12Device* device, UINT width, UINT height)
    : md3dDevice(device), mWidth(width), mHeight(height)
{
    mViewport = { 0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f };
    mScissorRect = { 0, 0, (int)width, (int)height };

    GenerateHaltonSequence();
    BuildResources();
}

float TAA::HaltonSequence(UINT index, UINT base)
{
    float result = 0.0f;
    float f = 1.0f;
    UINT i = index;
    while (i > 0)
    {
        f /= (float)base;
        result += f * (i % base);
        i /= base;
    }
    return result;
}

void TAA::GenerateHaltonSequence()
{
    for (UINT i = 0; i < JitterSampleCount; ++i)
    {
        // Halton(2,3)序列，偏移到[-0.5, 0.5]范围
        mJitterOffsets[i].x = HaltonSequence(i + 1, 2) - 0.5f;
        mJitterOffsets[i].y = HaltonSequence(i + 1, 3) - 0.5f;
    }
}

XMFLOAT2 TAA::GetJitterOffset(UINT frameIndex) const
{
    UINT index = frameIndex % JitterSampleCount;
    // 转换到像素空间，然后转换到NDC空间
    XMFLOAT2 jitter;
    jitter.x = mJitterOffsets[index].x / (float)mWidth * 2.0f;
    jitter.y = mJitterOffsets[index].y / (float)mHeight * 2.0f;
    return jitter;
}

void TAA::BuildResources()
{
    mHistoryBuffer = nullptr;
    mMotionVectorBuffer = nullptr;
    mOutputBuffer = nullptr;

    D3D12_RESOURCE_DESC texDesc = {};
    texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texDesc.Alignment = 0;
    texDesc.Width = mWidth;
    texDesc.Height = mHeight;
    texDesc.DepthOrArraySize = 1;
    texDesc.MipLevels = 1;
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

    // 历史缓冲区
    texDesc.Format = mColorFormat;
    texDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

    ThrowIfFailed(md3dDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &texDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&mHistoryBuffer)
    ));

    // 运动向量缓冲区
    texDesc.Format = mMotionVectorFormat;
    texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    float motionClear[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    CD3DX12_CLEAR_VALUE motionOptClear(mMotionVectorFormat, motionClear);

    ThrowIfFailed(md3dDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &texDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        &motionOptClear,
        IID_PPV_ARGS(&mMotionVectorBuffer)
    ));

    // TAA输出缓冲区
    texDesc.Format = mColorFormat;
    CD3DX12_CLEAR_VALUE outputOptClear(mColorFormat, clearColor);

    ThrowIfFailed(md3dDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &texDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        &outputOptClear,
        IID_PPV_ARGS(&mOutputBuffer)
    ));
}

void TAA::BuildDescriptors(
    CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv,
    CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv,
    CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuRtv,
    UINT srvDescriptorSize,
    UINT rtvDescriptorSize)
{
    mSrvDescriptorSize = srvDescriptorSize;
    mRtvDescriptorSize = rtvDescriptorSize;

    mHistoryCpuSrv = hCpuSrv;
    mHistoryGpuSrv = hGpuSrv;
    mMotionVectorCpuSrv = hCpuSrv.Offset(1, srvDescriptorSize);
    mMotionVectorGpuSrv = hGpuSrv.Offset(1, srvDescriptorSize);
    mOutputCpuSrv = hCpuSrv.Offset(1, srvDescriptorSize);
    mOutputGpuSrv = hGpuSrv.Offset(1, srvDescriptorSize);

    mMotionVectorCpuRtv = hCpuRtv;
    mOutputCpuRtv = hCpuRtv.Offset(1, rtvDescriptorSize);

    BuildDescriptors();
}

void TAA::BuildDescriptors()
{
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = 1;

    srvDesc.Format = mColorFormat;
    md3dDevice->CreateShaderResourceView(mHistoryBuffer.Get(), &srvDesc, mHistoryCpuSrv);
    md3dDevice->CreateShaderResourceView(mOutputBuffer.Get(), &srvDesc, mOutputCpuSrv);

    srvDesc.Format = mMotionVectorFormat;
    md3dDevice->CreateShaderResourceView(mMotionVectorBuffer.Get(), &srvDesc, mMotionVectorCpuSrv);

    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    rtvDesc.Texture2D.MipSlice = 0;

    rtvDesc.Format = mMotionVectorFormat;
    md3dDevice->CreateRenderTargetView(mMotionVectorBuffer.Get(), &rtvDesc, mMotionVectorCpuRtv);

    rtvDesc.Format = mColorFormat;
    md3dDevice->CreateRenderTargetView(mOutputBuffer.Get(), &rtvDesc, mOutputCpuRtv);
}

void TAA::SwapHistoryBuffer(ID3D12GraphicsCommandList* cmdList)
{
    // 将当前TAA输出复制到历史缓冲区
    cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
        mOutputBuffer.Get(),
        D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_SOURCE));

    cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
        mHistoryBuffer.Get(),
        D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_DEST));

    cmdList->CopyResource(mHistoryBuffer.Get(), mOutputBuffer.Get());

    cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
        mOutputBuffer.Get(),
        D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_GENERIC_READ));

    cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
        mHistoryBuffer.Get(),
        D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));
}

void TAA::OnResize(UINT width, UINT height)
{
    if (mWidth != width || mHeight != height)
    {
        mWidth = width;
        mHeight = height;

        mViewport = { 0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f };
        mScissorRect = { 0, 0, (int)width, (int)height };

        BuildResources();
        BuildDescriptors();
    }
}