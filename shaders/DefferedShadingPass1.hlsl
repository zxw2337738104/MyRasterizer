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
    float3 PosW : POSITION;
    float3 NormalW : NORMAL;
    float3 TangentW : TANGENT;
    float2 TexC : TEXCOORD;
    
    nointerpolation uint MatIndex : MATINDEX;
};

struct PixelOut
{
    float4 Albedo : SV_Target0;
    float4 Normal : SV_Target1;
    float4 Pos : SV_Target2;
};

VertexOut VS(VertexIn vin, uint instanceID : SV_InstanceID)
{
    VertexOut vout = (VertexOut) 0.0f;
    
    InstanceData instData = gInstanceData[instanceID];
    float4x4 gWorld = instData.World;
    float4x4 gTexTransform = instData.TexTransform;
    uint gMaterialIndex = instData.MaterialIndex;
    
    vout.MatIndex = gMaterialIndex;
    
    MaterialData matData = gMaterialData[gMaterialIndex];
    
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    vout.PosW = posW.xyz;
    
    vout.NormalW = mul(vin.NormalL, (float3x3) gWorld);
    
    vout.TangentW = mul(vin.TangentU, (float3x3) gWorld);
    
    vout.PosH = mul(posW, gViewProj);
    
    float4 texC = mul(float4(vin.TexC, 0.0f, 1.0f), gTexTransform);
    vout.TexC = mul(texC, matData.MatTransform).xy;
    
    return vout;
}

PixelOut PS(VertexOut pin)
{
    MaterialData matData = gMaterialData[pin.MatIndex];
    
    float4 diffuseAlbedo = matData.DiffuseAlbedo;
    float gRoughness = matData.Roughness;
    float3 gFresnelR0 = matData.FresnelR0;
    uint diffuseTexIndex = matData.DiffuseMapIndex;
    uint normalTexIndex = matData.NormalMapIndex;
    uint cubeMapIndex = matData.CubeMapIndex;
    
    pin.NormalW = normalize(pin.NormalW);
    
    float4 normalMapSample = gTextureMap[normalTexIndex].Sample(gsamAnisotropicWrap, pin.TexC);
    float3 bumpedNormalW = NormalSampleToWorldSpace(normalMapSample.rgb, pin.NormalW, pin.TangentW);
    
    diffuseAlbedo *= gTextureMap[diffuseTexIndex].Sample(gsamAnisotropicWrap, pin.TexC);
    
#ifdef ALPHA_TEST
        clip(diffuseAlbedo.a - 0.1f);
#endif
    
    const float shininess = (1.0f - gRoughness) * (normalMapSample.a != 0.0f ? normalMapSample.a : 0.0001f);

    PixelOut pout;
    pout.Pos = float4(pin.PosW, gFresnelR0.x);
    pout.Normal = float4(bumpedNormalW, shininess);
    pout.Albedo = diffuseAlbedo;
    
    //float3 toEyeW = normalize(gEyePosW - pin.PosW);
    //float3 r = reflect(-toEyeW, pin.NormalW);
    //float4 reflectionColor = gCubeMap[cubeMapIndex].Sample(gsamAnisotropicWrap, r);
    //float3 fresnelFactor = SchlickFresnel(gFresnelR0, bumpedNormalW, r);
    
    //pout.Albedo += float4(shininess * fresnelFactor * reflectionColor.rgb, 0.0f);
    
    return pout;
}