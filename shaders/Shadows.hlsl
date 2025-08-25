#include "Common.hlsl"

struct VertexIn
{
    float3 PosL : POSITION;
    float2 TexCoord : TEXCOORD;
};

struct VertexOut
{
    float4 PosH : SV_Position;
    float2 UV : TEXCOORD;
};

VertexOut VS(VertexIn vin)
{
    VertexOut vout;
    MaterialData matData = gMaterialData[gMaterialIndex];
    
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    vout.PosH = mul(posW, gViewProj);
    
    float4 texC = mul(float4(vin.TexCoord, 0.0f, 1.0f), gTexTransform);
    vout.UV = mul(texC, matData.MatTransform).xy;
    
    return vout;
}

void PS(VertexOut pin)
{
    
}