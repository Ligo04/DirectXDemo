#include "Effects.h"
#include "d3dUtil.h"
#include "EffectHelper.h"	// 必须晚于Effects.h和d3dUtil.h包含
#include "DXTrace.h"
#include "Vertex.h"
using namespace DirectX;

//
// GerstnerWavesEffect::Impl 需要先于GerstnerWavesEffect的定义
//

class GerstnerWavesEffect::Impl
{
public:
	// 必须显式指定
	Impl() {}
	~Impl() = default;

public:
	std::unique_ptr<EffectHelper> m_pEffectHelper;
	std::shared_ptr<IEffectPass> m_pCurrEffectPass;

	ComPtr<ID3D11InputLayout> m_pVertexPosNormalTexLayout;

	XMFLOAT4X4 m_World{}, m_View{}, m_Proj{};
};

//
// GerstnerWavesEffect
//
namespace
{
	//GerstnerWavesEffect单例
	static GerstnerWavesEffect* g_Instance = nullptr;
}


GerstnerWavesEffect::GerstnerWavesEffect()
{
	if(g_Instance)
		throw std::exception("GerstnerWavesEffect is a singleton!");
	g_Instance = this;
	pImpl = std::make_unique<GerstnerWavesEffect::Impl>();
}

GerstnerWavesEffect::~GerstnerWavesEffect()
{
}

GerstnerWavesEffect::GerstnerWavesEffect(GerstnerWavesEffect&& moveFrom) noexcept
{
	pImpl.swap(moveFrom.pImpl);
}

GerstnerWavesEffect& GerstnerWavesEffect::operator=(GerstnerWavesEffect&& moveFrom) noexcept
{
	pImpl.swap(moveFrom.pImpl);
	return *this;
}

GerstnerWavesEffect& GerstnerWavesEffect::Get()
{
	if (!g_Instance)
		throw std::exception("GerstnerWavesEffect is a singleton!");
	return *g_Instance;
}

void GerstnerWavesEffect::SetTextureDiffuse(ID3D11ShaderResourceView* textureDiffuse)
{
	pImpl->m_pEffectHelper->SetShaderResourceByName("g_DiffuseMap", textureDiffuse);
}

void GerstnerWavesEffect::SetTextureInput(ID3D11ShaderResourceView* textureInput)
{
	SetTextureDiffuse(textureInput);
}

bool GerstnerWavesEffect::InitAll(ID3D11Device* device)
{
	if (!device)
		return false;

	if (!RenderStates::IsInit())
		throw std::exception("RenderStates need to be initialized first!");

	pImpl->m_pEffectHelper = std::make_unique<EffectHelper>();

	ComPtr<ID3DBlob> blob;

	// ******************
	// 创建顶点着色器
	//
	HR(CreateShaderFromFile(L"HLSL\\GerstnerWaves_VS.cso", L"HLSL\\GerstnerWaves_VS.hlsl", "VS", "vs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(pImpl->m_pEffectHelper->AddShader("GerstnerWaves_VS", device, blob.Get()));
	// 创建顶点布局
	HR(device->CreateInputLayout(VertexPosNormalTex::inputLayout, ARRAYSIZE(VertexPosNormalTex::inputLayout),
		blob->GetBufferPointer(), blob->GetBufferSize(), pImpl->m_pVertexPosNormalTexLayout.GetAddressOf()));
	// ******************
	// 创建像素着色器
	//
	HR(CreateShaderFromFile(L"HLSL\\GerstnerWaves_PS.cso", L"HLSL\\GerstnerWaves_PS.hlsl", "PS", "ps_5_0", blob.ReleaseAndGetAddressOf()));
	HR(pImpl->m_pEffectHelper->AddShader("GerstnerWaves_PS", device, blob.Get()));

	// ******************
	// 创建计算着色器
	//
	HR(CreateShaderFromFile(L"HLSL\\GerstnerWavesUpdate_CS.cso", L"HLSL\\GerstnerWavesUpdate_CS.hlsl", "CS", "cs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(pImpl->m_pEffectHelper->AddShader("GerstnerWavesUpdate_CS", device, blob.Get()));

	// ******************
	// 创建通道
	//
	EffectPassDesc passDesc;
	passDesc.nameVS = "GerstnerWaves_VS";
	passDesc.namePS = "GerstnerWaves_PS";
	passDesc.nameCS = "GerstnerWavesUpdate_CS";
	pImpl->m_pEffectHelper->AddEffectPass("GerstnerWaves",device,&passDesc);
	pImpl->m_pEffectHelper->GetEffectPass("GerstnerWaves")->SetRasterizerState(nullptr);
	pImpl->m_pEffectHelper->GetEffectPass("GerstnerWaves")->SetBlendState(RenderStates::BSTransparent.Get(), nullptr, 0xFFFFFFFF);
	pImpl->m_pEffectHelper->GetEffectPass("GerstnerWaves")->SetDepthStencilState(nullptr,0);

	pImpl->m_pCurrEffectPass = pImpl->m_pEffectHelper->GetEffectPass("GerstnerWaves");

	pImpl->m_pEffectHelper->SetSamplerStateByName("g_SamLinearWrap", RenderStates::SSLinearWrap.Get());
	return true;
}


void GerstnerWavesEffect::SetRenderDefault(ID3D11DeviceContext* deviceContext)
{
	//基本物体绘制
	deviceContext->IASetInputLayout(pImpl->m_pVertexPosNormalTexLayout.Get());
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void GerstnerWavesEffect::SetEnableGpu(bool isEnable)
{
	if (isEnable)
	{
		pImpl->m_pEffectHelper->GetConstantBufferVariable("g_GpuWavesEnabled")->SetFloat(1);
	}
	else
	{
		pImpl->m_pEffectHelper->GetConstantBufferVariable("g_GpuWavesEnabled")->SetFloat(0);
	}
}

void GerstnerWavesEffect::SetRSWireframe(bool isWireframe)
{
	if (isWireframe)
	{
		pImpl->m_pEffectHelper->GetEffectPass("GerstnerWaves")->SetRasterizerState(RenderStates::RSWireframe.Get());
	}
	else
	{
		pImpl->m_pEffectHelper->GetEffectPass("GerstnerWaves")->SetRasterizerState(nullptr);
	}
}

void XM_CALLCONV GerstnerWavesEffect::SetWorldMatrix(DirectX::FXMMATRIX W)
{
	XMStoreFloat4x4(&pImpl->m_World, W);
}

void XM_CALLCONV GerstnerWavesEffect::SetViewMatrix(DirectX::FXMMATRIX V)
{
	XMStoreFloat4x4(&pImpl->m_View, V);
}

void XM_CALLCONV GerstnerWavesEffect::SetProjMatrix(DirectX::FXMMATRIX P)
{
	XMStoreFloat4x4(&pImpl->m_Proj, P);
}

void XM_CALLCONV GerstnerWavesEffect::SetTexTransformMatrix(DirectX::FXMMATRIX T)
{
	XMMATRIX T_tr = XMMatrixTranspose(T);
	pImpl->m_pEffectHelper->GetConstantBufferVariable("g_TexTransform")->SetFloatMatrix(4, 4, (const FLOAT*)&T_tr);
}

void GerstnerWavesEffect::SetDirLight(size_t pos, const DirectionalLight& dirLight)
{
	pImpl->m_pEffectHelper->GetConstantBufferVariable("g_DirLight")->SetRaw(&dirLight, (UINT)pos * sizeof(dirLight), sizeof(dirLight));
}

void GerstnerWavesEffect::SetPointLight(size_t pos, const PointLight& pointLight)
{
	pImpl->m_pEffectHelper->GetConstantBufferVariable("g_PointLight")->SetRaw(&pointLight, (UINT)pos * sizeof(pointLight), sizeof(pointLight));
}

void GerstnerWavesEffect::SetSpotLight(size_t pos, const SpotLight& spotLight)
{
	pImpl->m_pEffectHelper->GetConstantBufferVariable("g_SpotLight")->SetRaw(&spotLight, (UINT)pos * sizeof(spotLight), sizeof(spotLight));
}

void GerstnerWavesEffect::SetMaterial(const Material& material)
{
	pImpl->m_pEffectHelper->GetConstantBufferVariable("g_Material")->SetRaw(&material);
}

void GerstnerWavesEffect::SetTextureNormalMap(ID3D11ShaderResourceView* textureNormalMap)
{
	pImpl->m_pEffectHelper->SetShaderResourceByName("g_NormalMap", textureNormalMap);
}

void GerstnerWavesEffect::SetTextureDisplacementMap(ID3D11ShaderResourceView* textureDisplacementMap)
{
	pImpl->m_pEffectHelper->SetShaderResourceByName("g_DisplacementMap", textureDisplacementMap);
}

void GerstnerWavesEffect::SetTextureNormalSRV(ID3D11ShaderResourceView* textureNormalSrv)
{
	SetTextureNormalMap(textureNormalSrv);
}

void GerstnerWavesEffect::SetTexturePositionSRV(ID3D11ShaderResourceView* texturepositionSrv)
{
	SetTextureDisplacementMap(texturepositionSrv);
}

void GerstnerWavesEffect::SetTexturePositonUAV(ID3D11UnorderedAccessView* texturePositonUav)
{
	pImpl->m_pEffectHelper->SetUnorderedAccessByName("g_CurrSolOut", texturePositonUav,nullptr);
}

void GerstnerWavesEffect::SetTextureNormalUAV(ID3D11UnorderedAccessView* textureNormalUav)
{
	pImpl->m_pEffectHelper->SetUnorderedAccessByName("g_NorSolOut", textureNormalUav, nullptr);
}

void GerstnerWavesEffect::SetEyePos(const DirectX::XMFLOAT3& eyePos)
{
	pImpl->m_pEffectHelper->GetConstantBufferVariable("g_EyePosW")->SetFloatVector(3, (const FLOAT*)&eyePos);
}

void GerstnerWavesEffect::SetHeightScale(float scale)
{
	pImpl->m_pEffectHelper->GetConstantBufferVariable("g_HeightScale")->SetFloat(scale);
}

void GerstnerWavesEffect::SetTessInfo(float maxTessDistance, float minTessDistance, float minTessFactor, float maxTessFactor)
{
	pImpl->m_pEffectHelper->GetConstantBufferVariable("g_MaxTessDistance")->SetFloat(maxTessDistance);
	pImpl->m_pEffectHelper->GetConstantBufferVariable("g_MinTessDistance")->SetFloat(minTessDistance);
	pImpl->m_pEffectHelper->GetConstantBufferVariable("g_MinTessFactor")->SetFloat(minTessFactor);
	pImpl->m_pEffectHelper->GetConstantBufferVariable("g_MaxTessFactor")->SetFloat(maxTessFactor);
}


void GerstnerWavesEffect::SetTextureUsed(bool isUsed)
{
	pImpl->m_pEffectHelper->GetConstantBufferVariable("g_TextureUsed")->SetSInt(isUsed);
}

void GerstnerWavesEffect::SetNumWaves(UINT numwaves)
{
	pImpl->m_pEffectHelper->GetConstantBufferVariable("g_Numwaves")->SetUInt(numwaves);
}

void GerstnerWavesEffect::SetGameTime(float gametime)
{
	pImpl->m_pEffectHelper->GetConstantBufferVariable("g_Time")->SetFloat(gametime);
}

void GerstnerWavesEffect::SetGradient(float gradient)
{
	pImpl->m_pEffectHelper->GetConstantBufferVariable("g_Gradient")->SetFloat(gradient);
}

void GerstnerWavesEffect::SetGpuGerStnerWave(UINT pos, const GerstnerWaveParameter& parameter)
{
	pImpl->m_pEffectHelper->GetConstantBufferVariable("g_WaveParameter")->SetRaw(&parameter, (UINT)pos * sizeof(parameter), sizeof(parameter));
}

void GerstnerWavesEffect::SetNumCols(UINT numcols)
{
	pImpl->m_pEffectHelper->GetConstantBufferVariable("g_NumCols")->SetUInt(numcols);
}

void GerstnerWavesEffect::SetDebugObjectName(const std::string& name)
{
	// 设置调试对象名
	std::string layout = name + ".VertexPosNormalTexLayout";
	D3D11SetDebugObjectName(pImpl->m_pVertexPosNormalTexLayout.Get(), layout.c_str());
	pImpl->m_pEffectHelper->SetDebugObjectName(name);
}

void GerstnerWavesEffect::Apply(ID3D11DeviceContext* deviceContext)
{
	XMMATRIX W = XMLoadFloat4x4(&pImpl->m_World);
	XMMATRIX V = XMLoadFloat4x4(&pImpl->m_View);
	XMMATRIX P = XMLoadFloat4x4(&pImpl->m_Proj);
	
	XMMATRIX WInvT = InverseTranspose(W);

	W=XMMatrixTranspose(W);
	WInvT = XMMatrixTranspose(WInvT);
	V=XMMatrixTranspose(V);
	P=XMMatrixTranspose(P);

	pImpl->m_pEffectHelper->GetConstantBufferVariable("g_World")->SetFloatMatrix(4, 4, (const FLOAT*)&W);
	pImpl->m_pEffectHelper->GetConstantBufferVariable("g_WorldInvTranspose")->SetFloatMatrix(4, 4, (const FLOAT*)&WInvT);
	pImpl->m_pEffectHelper->GetConstantBufferVariable("g_View")->SetFloatMatrix(4, 4, (const FLOAT*)&V);
	pImpl->m_pEffectHelper->GetConstantBufferVariable("g_Proj")->SetFloatMatrix(4, 4, (const FLOAT*)&P);


	if (pImpl->m_pCurrEffectPass)
		pImpl->m_pCurrEffectPass->Apply(deviceContext);
}

