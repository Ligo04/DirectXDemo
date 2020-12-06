#include "LightHelper.hlsli"

Texture2D g_DiffuseMap : register(t0); // ��������

SamplerState g_SamLinearWrap : register(s0); // ���Թ���+Wrap������

RWTexture2D<float4> g_CurrSolOut : register(u0); //�����Ķ���UAV
RWTexture2D<float4> g_NorSolOut : register(u1); //�����ķ���UAV

#define MAXWaveNums 10
#define PI 3.141592654f


struct GerstnerWave
{
    float waveLength;
    float amplitude;
    float waveSpeed;
    float direction;
};

cbuffer CBChangesEveryInstanceDrawing : register(b0)
{
    matrix g_World;
    matrix g_WorldInvTranspose;
    matrix g_TexTransform;
}

cbuffer CBChangesEveryObjectDrawing : register(b1)
{
    Material g_Material;
}

cbuffer CBChangesEveryFrame : register(b2)
{
    matrix g_View;
    float3 g_EyePosW;
    float g_Pad;
}

cbuffer CBDrawingStates : register(b3)
{
    int g_TextureUsed;
    
    int g_GpuWavesEnabled; // �������˻���
    uint g_NumCols; 
    uint g_NumRows; 
}

cbuffer CBChangesOnResize : register(b4)
{
    matrix g_Proj;
}

cbuffer CBChangesRarely : register(b5)
{
    DirectionalLight g_DirLight[5];
    PointLight g_PointLight[5];
    SpotLight g_SpotLight[5];
}

cbuffer CBWaveSetting : register(b6)
{
    uniform uint g_Numwaves;
    float g_Time;
    float g_Gradient;
    float g_pad1;
    GerstnerWave g_WaveParameter[MAXWaveNums];
}


struct VertexPosNormalTex
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
    float2 Tex : TEXCOORD;
};

struct VertexPosHWNormalTex
{
    float4 PosH : SV_POSITION;
    float3 PosW : POSITION; // �������е�λ��
    float3 NormalW : NORMAL; // �������������еķ���
    float2 Tex : TEXCOORD;
};