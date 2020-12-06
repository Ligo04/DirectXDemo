#include "GerstnerWavesRender.h"
#include "Geometry.h"
#include "d3dUtil.h"

#include <DXGItype.h>  
#include <dxgi1_2.h>  
#include <dxgi1_3.h>  
#include <DXProgrammableCapture.h>

#pragma warning(disable:26451)

using namespace DirectX;
using namespace Microsoft::WRL;

void GerstnerWavesRender::Init(UINT rows, UINT cols, float texU, float texV, float spatialstep,
	UINT numwaves, float gradient, std::vector<GerstnerWaveParameter> parameters)
{

	m_NumRows = rows;
	m_NumCols = cols;
	m_VertexCount = rows * cols;
	m_IndexCount = 6 * (rows - 1) * (cols - 1);

	m_TexU = texU;
	m_TexV = texV;
	m_Texoffset = XMFLOAT2();
	m_SpatialStep = spatialstep;

	m_NumWaves = numwaves;
	m_Paramters = parameters;
	m_TotalGradient = gradient;
}

void GerstnerWavesRender::SetMaterial(const Material& material)
{
	m_Material = material;
}

Transform& GerstnerWavesRender::GetTransform()
{
	return m_Transform;
}

const Transform& GerstnerWavesRender::GetTransform() const
{
	return m_Transform;
}

UINT GerstnerWavesRender::RowCount() const
{
	return m_NumRows;
}

UINT GerstnerWavesRender::ColCount() const
{
	return m_NumCols;
}

void CpuGerstnerWavesRender::SetGerstnerWavesParameter(size_t wavesIndex, GerstnerWaveParameter parameter)
{
	if (wavesIndex < m_NumWaves - 1)
	{
		m_Paramters[wavesIndex] = parameter;
	}
	else
	{
		m_Paramters.push_back(parameter);
	}
	m_Paramters[wavesIndex].direction = XMConvertToRadians(m_Paramters[wavesIndex].direction);
	m_AngleFrequency[wavesIndex] = XM_2PI / parameter.waveLength;
	m_PrimaryPhase[wavesIndex] = parameter.wavespeed * sqrtf(9.8f * m_AngleFrequency[wavesIndex]);
	m_Gradient[wavesIndex] = m_TotalGradient / (m_AngleFrequency[wavesIndex] * parameter.amplitude * m_NumWaves);
}

HRESULT CpuGerstnerWavesRender::InitResource(ID3D11Device* device, const std::wstring& texFileName,
    UINT rows, UINT cols, float texU, float texV, float spatialstep, UINT numwaves, float gradient, std::vector<GerstnerWaveParameter> parameters)
{
	if (numwaves > m_MaxNumWaves)
		throw std::exception("Cannot produce more than 20 GerstnerWaves");

	// 防止重复初始化造成内存泄漏
	m_pVertexBuffer.Reset();
	m_pIndexBuffer.Reset();
	m_pTextureDiffuse.Reset();
	

	// 初始化水波数据
	Init(rows, cols, texU, texV, spatialstep, numwaves, gradient, parameters);
	
	m_AngleFrequency.resize(numwaves);
	m_PrimaryPhase.resize(numwaves);
	m_Gradient.resize(numwaves);
	//计算各个波浪的角频率,初相和陡度
	for (size_t i = 0; i < numwaves; ++i)
	{
		SetGerstnerWavesParameter(i, parameters[i]);
	}


	// 顶点行(列)数 - 1 = 网格行(列)数
	auto meshData = Geometry::CreateTerrain<VertexPosNormalTex, DWORD>(XMFLOAT2((cols - 1) * spatialstep, (rows - 1) * spatialstep),
		XMUINT2(cols - 1, rows - 1));

	HRESULT hr;
	// 创建动态顶点缓冲区
	hr = CreateVertexBuffer(device, meshData.vertexVec.data(), (UINT)meshData.vertexVec.size() * sizeof(VertexPosNormalTex),
		m_pVertexBuffer.GetAddressOf(),true);
	if (FAILED(hr))
		return hr;
	// 创建索引缓冲区
	hr = CreateIndexBuffer(device, meshData.indexVec.data(), (UINT)meshData.indexVec.size() * sizeof(DWORD),
		m_pIndexBuffer.GetAddressOf());
	if (FAILED(hr))
		return hr;

	//取出顶点数据
	m_Vertices.swap(meshData.vertexVec);

	m_OriginalPosition.resize(m_Vertices.size());
	size_t i = 0;
	for (auto& v : m_Vertices)
	{
		m_OriginalPosition[i++] = v.pos;
	}

	//读取纹理
	if (texFileName.size() > 4)
	{
		if (texFileName.substr(texFileName.size() - 3, 3) == L"dds")
		{
			hr = CreateDDSTextureFromFile(device, texFileName.c_str(), nullptr,
				m_pTextureDiffuse.GetAddressOf());
		}
		else
		{
			hr = CreateWICTextureFromFile(device, texFileName.c_str(), nullptr,
				m_pTextureDiffuse.GetAddressOf());
		}
	}
	return hr;

}

void CpuGerstnerWavesRender::Update(float gametime)
{
	//计算UV
	for (size_t i = 0; i < m_NumWaves; i++)
	{
		XMFLOAT2 dirFloat2 = XMFLOAT2(std::sinf(m_Paramters[i].direction), std::cosf(m_Paramters[i].direction));
		XMVECTOR direction = XMVector2Normalize(XMLoadFloat2(&dirFloat2));
		m_Texoffset.x -= XMVectorGetX(direction) * m_PrimaryPhase[i];
		m_Texoffset.y += XMVectorGetY(direction) * m_PrimaryPhase[i];
	}


	//计算P(x,y,t)
	for (size_t row = 0; row < m_NumRows; ++row)
	{
		for (size_t col = 0; col < m_NumCols; ++col)
		{
			float sinCol = 0.0f, cosCol = 0.0f;
			XMFLOAT3 sum=XMFLOAT3(0.0f,0.0f,0.0f);
			XMFLOAT3 normal = XMFLOAT3(0.0f, 1.0f, 0.0f);

			XMFLOAT2 oriPostion = XMFLOAT2(m_OriginalPosition[row * m_NumCols + col].x, m_OriginalPosition[row * m_NumCols + col].z);
			XMVECTOR P = XMLoadFloat2(&oriPostion);
			for (size_t i = 0; i < m_NumWaves; ++i)
			{
				XMFLOAT2 dirFloat2 = XMFLOAT2(std::sinf(m_Paramters[i].direction), std::cosf(m_Paramters[i].direction));
				XMVECTOR direction = XMVector2Normalize(XMLoadFloat2(&dirFloat2));

				cosCol= std::cosf(m_AngleFrequency[i] * XMVectorGetX(XMVector2Dot(direction, P)) + m_PrimaryPhase[i] * gametime);
				sinCol = std::sinf(m_AngleFrequency[i] * XMVectorGetX(XMVector2Dot(direction, P)) + m_PrimaryPhase[i] * gametime);


				//计算顶点
				sum.x += m_Gradient[i] * m_Paramters[i].amplitude * XMVectorGetX(direction) * cosCol;
				sum.y += m_Paramters[i].amplitude * sinCol;
				sum.z += m_Gradient[i] * m_Paramters[i].amplitude * XMVectorGetY(direction) * cosCol;

				//计算法线
				float WA = m_AngleFrequency[i] * m_Paramters[i].amplitude;

				normal.x -= XMVectorGetX(direction) * WA * cosCol;
				normal.y -= m_Gradient[i] * WA * sinCol;
				normal.z -= XMVectorGetY(direction) * WA * cosCol;
			}
			//顶点
			m_Vertices[row * m_NumCols + col].pos.x = m_OriginalPosition[row * m_NumCols + col].x + sum.x;
			m_Vertices[row * m_NumCols + col].pos.y = sum.y;
			m_Vertices[row * m_NumCols + col].pos.z = m_OriginalPosition[row * m_NumCols + col].z + sum.z;

			//法线

			XMStoreFloat3(&normal, XMVector3Normalize(XMLoadFloat3(&normal)));
			m_Vertices[row * m_NumCols + col].normal = normal;
		}
	}
}

void CpuGerstnerWavesRender::Draw(ID3D11DeviceContext* deviceContext, GerstnerWavesEffect* gerstnerwaveseffect)
{
	//更新动态缓冲区数据
	D3D11_MAPPED_SUBRESOURCE mappedData;
	deviceContext->Map(m_pVertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData);
	memcpy_s(mappedData.pData, m_VertexCount * sizeof(VertexPosNormalTex),
		m_Vertices.data(), m_VertexCount * sizeof(VertexPosNormalTex));
	deviceContext->Unmap(m_pVertexBuffer.Get(), 0);

	UINT strides[1] = { sizeof(VertexPosNormalTex) };
	UINT offsets[1] = { 0 };
	deviceContext->IASetVertexBuffers(0, 1, m_pVertexBuffer.GetAddressOf(), strides, offsets);
	deviceContext->IASetIndexBuffer(m_pIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

	gerstnerwaveseffect->SetEnableGpu(false);
	gerstnerwaveseffect->SetMaterial(m_Material);
	gerstnerwaveseffect->SetTextureDiffuse(m_pTextureDiffuse.Get());
	gerstnerwaveseffect->SetWorldMatrix(m_Transform.GetLocalToWorldMatrixXM());
	gerstnerwaveseffect->SetTexTransformMatrix(XMMatrixScaling(m_TexU, m_TexV, 1.0f) * XMMatrixTranslationFromVector(XMLoadFloat2(&m_Texoffset)));
	gerstnerwaveseffect->Apply(deviceContext);
	deviceContext->DrawIndexed(m_IndexCount, 0, 0);
}

void CpuGerstnerWavesRender::SetDebugObjectName(const std::string& name)
{
#if (defined(DEBUG)||defined(_DEBUG)&&(GRAPHICS_DEBUGGER_OBJECT_NAME))
	// 先清空可能存在的名称
	D3D11SetDebugObjectName(m_pTextureDiffuse.Get(), nullptr);

	D3D11SetDebugObjectName(m_pTextureDiffuse.Get(), name + ".TextureSRV");
	D3D11SetDebugObjectName(m_pVertexBuffer.Get(), name + ".VertexBuffer");
	D3D11SetDebugObjectName(m_pIndexBuffer.Get(), name + ".IndexBuffer");
#else
	UNREFERENCED_PARAMETER(name);
#endif

}

HRESULT GpuGerstnerWavesRender::InitResource(ID3D11Device* device, const std::wstring& texFileName, 
	UINT rows, UINT cols, float texU, float texV, float spatialstep, UINT numwaves, float gradient, std::vector<GerstnerWaveParameter> parameters)
{
	if (numwaves > m_MaxNumWaves)
		throw std::exception("Cannot produce more than 20 GerstnerWaves");

	// 防止重复初始化造成内存泄漏
	m_pVertexBuffer.Reset();
	m_pIndexBuffer.Reset();

	m_pTextureDiffuse.Reset();
	m_pOriSolution.Reset();
	m_pCurrSolution.Reset();
	m_pNormalSolution.Reset();
	m_pCurrOut.Reset();
	m_pNormalOut.Reset();

	m_pOriSolutionSRV.Reset();
	m_pCurrSolutionUAV.Reset();

	
	m_pCurrSolutionUAV.Reset();
	m_pNormalSolutionUAV.Reset();

	// rows和cols都需要是16的倍数，以免出现多余的部分被分配到额外的线程组当中。
	if (rows % 16 || cols % 16)
		return E_INVALIDARG;

	//初始化水波数据
	Init(rows, cols, texU, texV, spatialstep, numwaves, gradient, parameters);

	// 顶点行(列)数 - 1 = 网格行(列)数
	auto meshData = Geometry::CreateTerrain<VertexPosNormalTex, DWORD>(XMFLOAT2((cols - 1) * spatialstep, (rows - 1) * spatialstep),
		XMUINT2(cols - 1, rows - 1));

	HRESULT hr;

	// 创建顶点缓冲区
	hr = CreateVertexBuffer(device, meshData.vertexVec.data(), (UINT)meshData.vertexVec.size() * sizeof(VertexPosNormalTex),
		m_pVertexBuffer.GetAddressOf(),true);
	if (FAILED(hr))
		return hr;
	// 创建索引缓冲区
	hr = CreateIndexBuffer(device, meshData.indexVec.data(), (UINT)meshData.indexVec.size() * sizeof(DWORD),
		m_pIndexBuffer.GetAddressOf());
	if (FAILED(hr))
		return hr;

	//取出顶点数据
	m_pVertevices.swap(meshData.vertexVec);
	m_pCurrVertex.resize(m_pVertevices.size());
	m_pCurrNormal.resize(m_pVertevices.size());

    m_pCurrVertex=std::vector<XMFLOAT4>(m_pVertevices.size());
	size_t i = 0;
	for (auto& v : m_pVertevices)
	{
		m_pCurrVertex[i++] = XMFLOAT4(v.pos.x,v.pos.y,v.pos.z,1.0f);
	}

	D3D11_SUBRESOURCE_DATA initData;
	initData.pSysMem = m_pCurrVertex.data();
	initData.SysMemPitch = m_NumCols * sizeof(XMFLOAT4);
	initData.SysMemSlicePitch = 0;

	//创建缓存计算结果的资源
	D3D11_TEXTURE2D_DESC texDesc;
	texDesc.Width = cols;
	texDesc.Height = rows;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = 0;

	//一个输入,一个输出  用于计算着色器
	hr = device->CreateTexture2D(&texDesc, &initData, m_pOriSolution.GetAddressOf());
	if (FAILED(hr))
		return hr;
	hr = device->CreateTexture2D(&texDesc, nullptr, m_pCurrSolution.GetAddressOf());
	if (FAILED(hr))
		return hr;
	hr = device->CreateTexture2D(&texDesc, nullptr, m_pNormalSolution.GetAddressOf());
	if (FAILED(hr))
		return hr;

	// 
	texDesc.Width = cols;
	texDesc.Height = rows;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D11_USAGE_STAGING;
	texDesc.BindFlags = 0;
	texDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	texDesc.MiscFlags = 0;

	hr = device->CreateTexture2D(&texDesc, nullptr, m_pCurrOut.GetAddressOf());
	if (FAILED(hr))
		return hr;
	hr = device->CreateTexture2D(&texDesc, nullptr, m_pNormalOut.GetAddressOf());
	if (FAILED(hr))
		return hr;

	//创建着色器资源视图
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.MostDetailedMip = 0;
	hr = device->CreateShaderResourceView(m_pOriSolution.Get(), &srvDesc, m_pOriSolutionSRV.GetAddressOf());
	if (FAILED(hr))
		return hr;



	//创建无序访问试图
	D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
	uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	uavDesc.Texture2D.MipSlice = 0;
	hr = device->CreateUnorderedAccessView(m_pCurrSolution.Get(), &uavDesc, m_pCurrSolutionUAV.GetAddressOf());
	if (FAILED(hr))
		return hr;
	hr = device->CreateUnorderedAccessView(m_pNormalSolution.Get(), &uavDesc, m_pNormalSolutionUAV.GetAddressOf());
	if (FAILED(hr))
		return hr;


	// 读取纹理
	if (texFileName.size() > 4)
	{
		if (texFileName.substr(texFileName.size() - 3, 3) == L"dds")
		{
			hr = CreateDDSTextureFromFile(device, texFileName.c_str(), nullptr,
				m_pTextureDiffuse.GetAddressOf());
		}
		else
		{
			hr = CreateWICTextureFromFile(device, texFileName.c_str(), nullptr,
				m_pTextureDiffuse.GetAddressOf());
		}
	}
	return hr;
}

void GpuGerstnerWavesRender::Update(ID3D11DeviceContext* deviceContext,GerstnerWavesEffect* gerstnerWavesEffect ,float gametime)
{
	// 设置所用到的数据
	gerstnerWavesEffect->SetGameTime(gametime);
	gerstnerWavesEffect->SetNumWaves(m_NumWaves);
	gerstnerWavesEffect->SetGradient(m_TotalGradient);
	// 设置波浪参数
	for (UINT i = 0; i < m_NumWaves; ++i)
	{
		gerstnerWavesEffect->SetGpuGerStnerWave(i, m_Paramters[i]);
	}

	gerstnerWavesEffect->SetTextureInput(m_pOriSolutionSRV.Get());
	gerstnerWavesEffect->SetTexturePositonUAV(m_pCurrSolutionUAV.Get());
	gerstnerWavesEffect->SetTextureNormalUAV(m_pNormalSolutionUAV.Get());
	gerstnerWavesEffect->Apply(deviceContext);
	// 调度计算着色器
	deviceContext->Dispatch(m_NumCols / 16, m_NumRows / 16, 1);


	// 将计算后的结果映射回内存
	deviceContext->CopyResource(m_pCurrOut.Get(), m_pCurrSolution.Get());
	deviceContext->CopyResource(m_pNormalOut.Get(), m_pNormalSolution.Get());

	D3D11_MAPPED_SUBRESOURCE mappedData;
	deviceContext->Map(m_pCurrOut.Get(), 0, D3D11_MAP_READ, 0, &mappedData);
	float* pData = reinterpret_cast<float*>(mappedData.pData);
	memcpy_s(m_pCurrVertex.data(), m_NumRows * m_NumCols * 16, pData, m_NumRows * m_NumCols * 16);
	deviceContext->Unmap(m_pCurrOut.Get(), 0);

	deviceContext->Map(m_pNormalOut.Get(), 0, D3D11_MAP_READ, 0, &mappedData);
	pData = reinterpret_cast<float*>(mappedData.pData);
	memcpy_s(m_pCurrNormal.data(), m_NumRows * m_NumCols * 16, pData, m_NumRows * m_NumCols * 16);
	deviceContext->Unmap(m_pNormalOut.Get(), 0);

	//计算UV
	for (size_t i = 0; i < m_NumWaves; i++)
	{
		XMFLOAT2 dirFloat2 = XMFLOAT2(std::sinf(XMConvertToRadians(m_Paramters[i].direction)), std::cosf(XMConvertToRadians(m_Paramters[i].direction)));
		XMVECTOR direction = XMVector2Normalize(XMLoadFloat2(&dirFloat2));
		float phase = m_Paramters[i].wavespeed * sqrtf(9.8f * XM_2PI / m_Paramters[i].waveLength);
		m_Texoffset.x -= XMVectorGetX(direction) * phase;
		m_Texoffset.y += XMVectorGetY(direction) * phase;
	}
}

void GpuGerstnerWavesRender::Draw(ID3D11DeviceContext* deviceContext, GerstnerWavesEffect* gerstnerwaveseffect)
{
	size_t i = 0;
	for (auto& v : m_pCurrVertex)
	{
		XMStoreFloat3(&m_pVertevices[i++].pos, XMLoadFloat4(&v));
	}
	i = 0;
	for (auto& v :m_pCurrNormal)
	{
		XMStoreFloat3(&m_pVertevices[i++].normal, XMLoadFloat4(&v));
	}

	//更新动态缓冲区数据
	D3D11_MAPPED_SUBRESOURCE mappedData;
	deviceContext->Map(m_pVertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData);
	memcpy_s(mappedData.pData, m_VertexCount * sizeof(VertexPosNormalTex),
		m_pVertevices.data(), m_VertexCount * sizeof(VertexPosNormalTex));
	deviceContext->Unmap(m_pVertexBuffer.Get(), 0);

	UINT strides[1] = { sizeof(VertexPosNormalTex) };
	UINT offsets[1] = { 0 };
	deviceContext->IASetVertexBuffers(0, 1, m_pVertexBuffer.GetAddressOf(), strides, offsets);
	deviceContext->IASetIndexBuffer(m_pIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);


	gerstnerwaveseffect->SetEnableGpu(false);
	gerstnerwaveseffect->SetMaterial(m_Material);
	gerstnerwaveseffect->SetTextureDiffuse(m_pTextureDiffuse.Get());
	gerstnerwaveseffect->SetWorldMatrix(m_Transform.GetLocalToWorldMatrixXM());
	gerstnerwaveseffect->SetTexTransformMatrix(XMMatrixScaling(m_TexU, m_TexV, 1.0f) * XMMatrixTranslationFromVector(XMLoadFloat2(&m_Texoffset)));
	gerstnerwaveseffect->Apply(deviceContext);
	deviceContext->DrawIndexed(m_IndexCount, 0, 0);
}

void GpuGerstnerWavesRender::SetDebugObjectName(const std::string& name)
{
#if (defined(DEBUG)||defined(_DEBUG)&&(GRAPHICS_DEBUGGER_OBJECT_NAME))
	//向清空可能存在的名称
	D3D11SetDebugObjectName(m_pTextureDiffuse.Get(), nullptr);


	D3D11SetDebugObjectName(m_pTextureDiffuse.Get(), name + ".TextureSRV");
	D3D11SetDebugObjectName(m_pVertexBuffer.Get(), name + ".VertexBuffer");
	D3D11SetDebugObjectName(m_pIndexBuffer.Get(), name + ".IndexBuffer");

	D3D11SetDebugObjectName(m_pOriSolution.Get(), name + ".OriTexture");
	D3D11SetDebugObjectName(m_pCurrSolution.Get(), name + ".CurrTexture");
	D3D11SetDebugObjectName(m_pNormalSolution.Get(), name + ".NorTexture");
	D3D11SetDebugObjectName(m_pCurrOut.Get(), name + ".CurrOutTexture");
	D3D11SetDebugObjectName(m_pNormalOut.Get(), name + ".NorOutTexture");

	D3D11SetDebugObjectName(m_pOriSolutionSRV.Get(), name + ".OriTextureSRV");
	//D3D11SetDebugObjectName(m_pCurrSolutionSRV.Get(), name + ".CurrTextureSRV");
	//D3D11SetDebugObjectName(m_pNormalSolutionSRV.Get(), name + ".NorTextureSRV");
	D3D11SetDebugObjectName(m_pCurrSolutionUAV.Get(), name + ".CurrTextureUAV");
	D3D11SetDebugObjectName(m_pNormalSolutionUAV.Get(), name + ".NormalTextureUAV");
#else
	UNREFERENCED_PARAMETER(name);
#endif
}
