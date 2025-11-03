#include "Common.hlsl"

Texture2D gAlbedoMap : register(t25);
Texture2D gNormalMap : register(t26);
Texture2D gPositionMap : register(t27);

struct VertexOut
{
    float4 PosH : SV_Position;
    float2 TexC : TEXCOORD0;
    float2 quadID : TEXCOORD1;
};

static const float2 gTexCoords[6] =
{
    float2(0.0f, 1.0f),
    float2(0.0f, 0.0f),
    float2(1.0f, 0.0f),
    float2(0.0f, 1.0f),
    float2(1.0f, 0.0f),
    float2(1.0f, 1.0f)
};

VertexOut VS(uint vid : SV_VertexID)
{
    VertexOut vout = (VertexOut) 0.0f;
    
    int quadID = vid / 6;
    vid %= 6;
    
    vout.TexC = gTexCoords[vid];
    vout.PosH = float4(vout.TexC.x * 2.0f - 1.0f, 1.0f - vout.TexC.y * 2.0f, 0.0f, 1.0f);
    
    vout.PosH.xy *= 0.25f;
    vout.PosH.x -= (-0.25f + 0.5f * quadID);
    vout.PosH.y -= 0.75f;

    
    vout.quadID.x = quadID;
    
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    float3 posW = gPositionMap.Sample(gsamPointClamp, pin.TexC).xyz;
    float3 normalW = gNormalMap.Sample(gsamPointClamp, pin.TexC).xyz;
    float3 albedo = gAlbedoMap.Sample(gsamPointClamp, pin.TexC).rgb;
    
    if (pin.quadID.x == 0.0f)
        return float4(posW, 1.0f);
    else if (pin.quadID.x == 1.0f)
        return float4(normalW, 1.0f);
    else // if(pin.quadID.x == 3.0f)
        return float4(albedo, 2.0f);
}