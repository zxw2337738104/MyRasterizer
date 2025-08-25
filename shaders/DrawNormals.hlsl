#include "Common.hlsl"

struct VertexIn
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
    float2 TexC : TEXCOORD;
    float3 TangentU : TANGENT;
};

struct VertexOut
{
    float4 PosH : SV_Position;
    float3 NormalW : NORMAL;
    float2 TexC : TEXCOORD;
    float3 TangentW : TANGENT;
    
    
    nointerpolation uint MatIndex : MATINDEX;
};

VertexOut VS(VertexIn vin, uint instanceID : SV_InstanceID)
{
    VertexOut vout = (VertexOut) 0.0f;

    InstanceData instData = gInstanceData[instanceID];
    float4x4 gWorld = instData.World;
    float4x4 gInvTpsWorld = instData.InvTpsWorld;
    float4x4 gTexTransform = instData.TexTransform;
    uint gMaterialIndex = instData.MaterialIndex;
    
    vout.MatIndex = gMaterialIndex;
    
    MaterialData matData = gMaterialData[gMaterialIndex];
    
    vout.NormalW = mul(vin.NormalL, (float3x3) gInvTpsWorld);
    vout.TangentW = mul(vin.TangentU, (float3x3) gInvTpsWorld);
    
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    vout.PosH = mul(posW, gViewProj);
    
    
    float4 texC = mul(float4(vin.TexC, 0.0f, 1.0f), gTexTransform);
    vout.TexC = mul(texC, matData.MatTransform).xy;
    
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    MaterialData matData = gMaterialData[pin.MatIndex];
    uint normalTexIndex = matData.NormalMapIndex;
    
    pin.NormalW = normalize(pin.NormalW);
    
    float4 normalMapSample = gTextureMap[normalTexIndex].Sample(gsamAnisotropicWrap, pin.TexC);
    float3 bumpedNormalW = NormalSampleToWorldSpace(normalMapSample.rgb, pin.NormalW, pin.TangentW);
    
    float3 normalV = mul(bumpedNormalW, (float3x3) gView);
    return float4(normalV, 0.0f);
}