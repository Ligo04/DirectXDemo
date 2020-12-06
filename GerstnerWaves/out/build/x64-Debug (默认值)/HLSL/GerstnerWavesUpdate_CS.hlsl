#include "GerstnerWaves.hlsli"

[numthreads(16, 16, 1)]
void CS( uint3 DTid : SV_DispatchThreadID )
{
    uint x = DTid.x;
    uint y = DTid.y;
    
    float sinCol = 0.0f, cosCol = 0.0f;
    float3 sum = float3(0.0f, 0.0f, 0.0f);
    float3 nor = float3(0.0f, 1.0f, 0.0f);
    float2 pos = g_DiffuseMap[uint2(x, y)].xz;
    [unroll]
    for (uint i = 0; i < g_Numwaves;++i)
    {
        float angleFrequency = 2 * PI / g_WaveParameter[i].waveLength;
        float phase = g_WaveParameter[i].waveSpeed * sqrt(9.8f * angleFrequency);
        float gradient = g_Gradient / (angleFrequency * g_WaveParameter[i].amplitude * g_Numwaves);
        float direction = radians(g_WaveParameter[i].direction);//转为弧度制
        float2 dir = normalize(float2(sin(direction), cos(direction)));

        cosCol = cos(angleFrequency * dot(dir, pos) + phase * g_Time);
        sinCol = sin(angleFrequency * dot(dir, pos) + phase * g_Time);

        //计算顶点
        sum.xz += gradient * g_WaveParameter[i].amplitude * dir.xy * cosCol;
        sum.y += g_WaveParameter[i].amplitude * sinCol;

        //计算法线
        float WA = angleFrequency * g_WaveParameter[i].amplitude;
        nor.xz -= dir.xy * WA * cosCol;
        nor.y -= gradient * WA * sinCol;
    }
    float4 positon = float4(pos.x + sum.x, sum.y, pos.y + sum.z, 1.0f);
    float4 normal = float4(normalize(nor), 1.0f);
    
    g_CurrSolOut[uint2(x, y)] = positon;
    g_NorSolOut[uint2(x, y)] = normal;

}