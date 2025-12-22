Texture2D gCurrentFrame : register(t0); // 当前帧颜色
Texture2D gHistoryFrame : register(t1); // 历史帧颜色
Texture2D gMotionVector : register(t2); // 运动向量

SamplerState gsamPointClamp : register(s1);
SamplerState gsamLinearClamp : register(s3);

cbuffer cbTAAResolve : register(b0)
{
    float2 gTexelSize;
    float gBlendFactor; // 通常0.05-0.1
    float gVarianceClipGamma; // 通常1.0-1.5
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
};

VertexOut VS(uint vid : SV_VertexID)
{
    VertexOut vout;
    vout.TexC = gTexCoords[vid];
    vout.PosH = float4(2.0f * vout.TexC.x - 1.0f, 1.0f - 2.0f * vout.TexC.y, 0.0f, 1.0f);
    return vout;
}

// RGB到YCoCg颜色空间转换（用于更好的颜色裁剪）
float3 RGBToYCoCg(float3 rgb)
{
    return float3(
        0.25f * rgb.r + 0.5f * rgb.g + 0.25f * rgb.b,
        0.5f * rgb.r - 0.5f * rgb.b,
        -0.25f * rgb.r + 0.5f * rgb.g - 0.25f * rgb.b
    );
}

float3 YCoCgToRGB(float3 ycocg)
{
    return float3(
        ycocg.x + ycocg.y - ycocg.z,
        ycocg.x + ycocg.z,
        ycocg.x - ycocg.y - ycocg.z
    );
}

// 3x3邻域颜色裁剪（Variance Clipping）
float3 ClipAABB(float3 aabbMin, float3 aabbMax, float3 prevSample)
{
    float3 center = 0.5f * (aabbMax + aabbMin);
    float3 extents = 0.5f * (aabbMax - aabbMin);
    
    float3 offset = prevSample - center;
    float3 ts = abs(extents) / (abs(offset) + 0.0001f);
    float t = saturate(min(ts.x, min(ts.y, ts.z)));
    
    return center + offset * t;
}

float Luminance(float3 color)
{
    return dot(color, float3(0.299, 0.587, 0.114));
}

// Tonemap权重：高亮度 → 低权重
float HDRWeight(float3 color)
{
    return rcp(1.0f + Luminance(color) * 12.0f);
}

float4 PS(VertexOut pin) : SV_Target
{
    float2 uv = pin.TexC;
    
    // 采样运动向量
    float2 velocity = gMotionVector.Sample(gsamPointClamp, uv).xy;
    
    // 计算上一帧UV
    float2 prevUV = uv - velocity;
    
    // 采样当前帧
    float3 currentColor = gCurrentFrame.Sample(gsamPointClamp, uv).rgb;
    
    // 边界检查
    bool isValidHistory = all(prevUV >= 0.0f) && all(prevUV <= 1.0f);
    
    if (!isValidHistory)
    {
        return float4(currentColor, 1.0f);
    }
    
    // 采样历史帧
    float3 historyColor = gHistoryFrame.Sample(gsamLinearClamp, prevUV).rgb;
    
    // 采样3x3邻域计算颜色包围盒
    float3 m1 = float3(0.0f, 0.0f, 0.0f);
    float3 m2 = float3(0.0f, 0.0f, 0.0f);
    
    [unroll]
    for (int y = -1; y <= 1; ++y)
    {
        [unroll]
        for (int x = -1; x <= 1; ++x)
        {
            float2 sampleUV = uv + float2(x, y) * gTexelSize;
            float3 sampleColor = gCurrentFrame.Sample(gsamPointClamp, sampleUV).rgb;
            sampleColor = RGBToYCoCg(sampleColor);
            m1 += sampleColor;
            m2 += sampleColor * sampleColor;
        }
    }
    
    // 计算均值和标准差
    m1 /= 9.0f;
    m2 /= 9.0f;
    float3 sigma = sqrt(max(m2 - m1 * m1, 0.0f));
    
    // 构建AABB
    float3 aabbMin = m1 - gVarianceClipGamma * sigma;
    float3 aabbMax = m1 + gVarianceClipGamma * sigma;
    
    // 将历史颜色裁剪到AABB内
    float3 historyYCoCg = RGBToYCoCg(historyColor);
    float3 clippedHistory = ClipAABB(aabbMin, aabbMax, historyYCoCg);
    float3 clippedHistoryRGB = YCoCgToRGB(clippedHistory);
    
    // ===== Luminance Weighting =====
    float weightCurrent = HDRWeight(currentColor);
    float weightHistory = HDRWeight(clippedHistoryRGB);
    
    // 加权混合
    float3 result = (currentColor * weightCurrent * gBlendFactor +
                     clippedHistoryRGB * weightHistory * (1.0f - gBlendFactor)) /
                    (weightCurrent * gBlendFactor +
                     weightHistory * (1.0f - gBlendFactor) + 0.0001f);
    
    return float4(result, 1.0f);
}