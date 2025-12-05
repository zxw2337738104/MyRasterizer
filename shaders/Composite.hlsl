Texture2D gBasePassMap : register(t0);
Texture2D gBrightPassMap : register(t1);

SamplerState gsamPointWrap : register(s0);
SamplerState gsamPointClamp : register(s1);
SamplerState gsamLinearWrap : register(s2);
SamplerState gsamLinearClamp : register(s3);
SamplerState gsamAnisotropicWrap : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);
SamplerComparisonState gsamShadow : register(s6);

cbuffer cbSettings : register(b0)
{
    int gEnableBloom;
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

struct VertexOut
{
    float4 PosH : SV_Position;
    float2 TexC : TEXCOORD;
    int Index : TEXCOORD1;
};

VertexOut VS(uint vid : SV_VertexID)
{
    VertexOut vout;
    
    vout.TexC = gTexCoords[vid % 6];
    
    vout.PosH = float4(2.0f * vout.TexC.x - 1.0f, 1.0f - 2.0f * vout.TexC.y, 0.0f, 1.0f);
    vout.Index = 0;
    
    if(vid >= 6)
    {
        vout.PosH.xy = vout.PosH.xy * 0.25f + float2(-0.75f, -0.75f);
        vout.Index = 1;
    }
    return vout;
}

float3 ToneMapping(float3 color)
{
    float exposure = 0.5f;
    return 1.0f - exp(-color * exposure);
}

float3 GammaCorrection(float3 color)
{
    return pow(color, 1.0f / 2.2f);
}

float4 PS(VertexOut pin) : SV_Target
{
    float3 BaseColor = gBasePassMap.Sample(gsamPointClamp, pin.TexC).rgb;
    
    
    float3 BrightColor = gBrightPassMap.Sample(gsamPointClamp, pin.TexC).rgb;
    if (pin.Index == 1)
        return float4(BrightColor, 1.0f);
    
    if (gEnableBloom == 1)
    {
        BaseColor += BrightColor;
    }
    
    float3 result = ToneMapping(BaseColor);
    result = GammaCorrection(result);
    
    return float4(result, 1.0f);
}