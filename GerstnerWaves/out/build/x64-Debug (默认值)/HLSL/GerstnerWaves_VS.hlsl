#include "GerstnerWaves.hlsli"

VertexPosHWNormalTex VS(VertexPosNormalTex vIn)
{
    VertexPosHWNormalTex vOut;
   
    //绘制GPU水波时用到
    //if (g_GpuWavesEnabled)
    //{
    //    vIn.PosL = g_DisplacementMap.SampleLevel(g_SamPointClamp, vIn.Tex, 0).xyz;
    //    vIn.NormalL = g_NormalMap.SampleLevel(g_SamPointClamp, vIn.Tex, 0).xyz;
    //}
    
    matrix viewProj = mul(g_View, g_Proj);
    vector posW = mul(float4(vIn.PosL, 1.0f), g_World);

    vOut.PosW = posW.xyz;
    vOut.PosH = mul(posW, viewProj);
    vOut.NormalW = mul(vIn.NormalL, (float3x3) g_WorldInvTranspose);
    vOut.Tex = mul(float4(vIn.Tex, 0.0f, 1.0f), g_TexTransform).xy;
    return vOut;
}