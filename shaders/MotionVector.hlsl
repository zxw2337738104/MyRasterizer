// shaders/MotionVector.hlsl
#include "Common.hlsl"

cbuffer cbTAA : register(b2)
{
    float4x4 gPrevViewProj;
    float4x4 gCurrViewProj;
    float2 gJitterOffset;
    float2 gPrevJitterOffset;
};

struct VertexIn
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
    float2 TexC : TEXCOORD;
    float3 TangentL : TANGENT;
};

struct VertexOut
{
    float4 PosH : SV_Position;
    float4 CurrPosH : POSITION0;
    float4 PrevPosH : POSITION1;
};

VertexOut VS(VertexIn vin, uint instanceID : SV_InstanceID)
{
    VertexOut vout = (VertexOut) 0.0f;
    
    InstanceData instData = gInstanceData[instanceID];
    //float4x4 gWorld = instData.World;
    
    //float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    
    //// 当前帧位置（带抖动）
    //vout.CurrPosH = mul(posW, gCurrViewProj);
    
    //// 上一帧位置（静态物体使用相同世界坐标，动态物体需要上一帧的世界矩阵）
    //// 这里假设是静态物体
    //vout.PrevPosH = mul(posW, gPrevViewProj);
    
    // 如果有动态物体，使用下面的代码
    float4 currPosW = mul(float4(vin.PosL, 1.0f), instData.World);
    float4 prevPosW = mul(float4(vin.PosL, 1.0f), instData.PrevWorld);
    
    // 当前帧位置（带抖动）
    vout.CurrPosH = mul(currPosW, gCurrViewProj);
    
    // 上一帧位置（静态物体使用相同世界坐标，动态物体需要上一帧的世界矩阵）
    vout.PrevPosH = mul(prevPosW, gPrevViewProj);
    
    vout.PosH = vout.CurrPosH;
    
    return vout;
}

float2 PS(VertexOut pin) : SV_Target
{
    // 转换到NDC空间
    float2 currNDC = pin.CurrPosH.xy / pin.CurrPosH.w;
    float2 prevNDC = pin.PrevPosH.xy / pin.PrevPosH.w;
    
    // 移除抖动
    currNDC -= gJitterOffset;
    prevNDC -= gPrevJitterOffset;
    
    // 计算运动向量（屏幕空间）
    float2 velocity = (currNDC - prevNDC) * 0.5f; // NDC到UV空间
    
    return velocity;
}