#ifndef GERSTNERWAVESRENDER_H
#define GERSTNERWAVESRENDER_H

#include <vector>
#include <string>
#include "Effects.h"
#include "Transform.h"
#include "Vertex.h"


class GerstnerWavesRender
{
public:
	template<class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	using GerstnerWaveParameter = GerstnerWavesEffect::GerstnerWaveParameter;

	void SetMaterial(const Material& material);

	Transform& GetTransform();
	const Transform& GetTransform() const;

	UINT RowCount() const;
	UINT ColCount() const;

protected:
	GerstnerWavesRender()=default;
	~GerstnerWavesRender()=default;
	//不允许拷贝,允许移动
	GerstnerWavesRender(const GerstnerWavesRender&) = delete;
	GerstnerWavesRender& operator=(const GerstnerWavesRender&) = delete;
	GerstnerWavesRender(GerstnerWavesRender&&) = default;
	GerstnerWavesRender& operator=(GerstnerWavesRender&&) = default;

	void Init(
		UINT rows,												// 顶点行数
		UINT cols,												// 顶点列数
		float texU,												// 纹理坐标U方向最大值
		float texV,												// 纹理坐标V方向最大值
		float spatialstep,										// 空间步长
		UINT numwaves,											// 波浪数目
		float gradient,											// 陡度(0-1)
		std::vector<GerstnerWaveParameter> parameters			// 对应波浪参数
	);

protected:
	UINT m_NumRows = 0;                         //顶点行数
	UINT m_NumCols = 0;							//顶点列数

	UINT m_VertexCount = 0;						//顶点数目
	UINT m_IndexCount = 0;						//索引数目

	Transform m_Transform = {};					//水面变换
	DirectX::XMFLOAT2 m_Texoffset = {};			//纹理坐标偏移
	Material m_Material = {};					//水面材质
	float m_TexU = 0.0f;						//纹理坐标U方向最大值
	float m_TexV = 0.0f;						//纹理坐标V方向最大值
	float m_SpatialStep = 0.0f;                 //空间步长

	float m_TotalGradient=0.0f;									// 陡度
	UINT m_NumWaves = 0;										// 波浪的数目
	UINT m_MaxNumWaves = 10;									// 最大波浪数目
	std::vector<GerstnerWaveParameter> m_Paramters = {};		// 波浪的参数
};

class CpuGerstnerWavesRender:public GerstnerWavesRender
{
public:
	CpuGerstnerWavesRender() = default;
	~CpuGerstnerWavesRender() = default;
	//不允许拷贝,允许移动
	CpuGerstnerWavesRender(const CpuGerstnerWavesRender&) = delete;
	CpuGerstnerWavesRender& operator=(const CpuGerstnerWavesRender&) = delete;
	CpuGerstnerWavesRender(CpuGerstnerWavesRender&&) = default;
	CpuGerstnerWavesRender& operator=(CpuGerstnerWavesRender&&) = default;

	HRESULT InitResource(ID3D11Device* device,
		const std::wstring& texFileName,				// 纹理文件名
		UINT rows,										// 顶点行数
		UINT cols,										// 顶点列数
		float texU,										// 纹理坐标U方向最大值
		float texV,										// 纹理坐标V方向最大值
		float spatialstep,								// 空间步长
		UINT numwaves,									// 波浪数目
		float gradient,									// 陡度
		std::vector<GerstnerWaveParameter> parameters	// 对应波浪参数
	);
	//修改对应波浪的参数
	void SetGerstnerWavesParameter(size_t wavesIndex, GerstnerWaveParameter parameter);

	void Update(float gametime);

	void Draw(ID3D11DeviceContext* deviceContext, GerstnerWavesEffect* gerstnerwaveseffect);



	// 设置调试对象名
	void SetDebugObjectName(const std::string& name);

private:
	std::vector<float> m_AngleFrequency = {};					// 角频率wi
	std::vector<float> m_PrimaryPhase = {};						// 初相i
	std::vector<float> m_Gradient = {};							// 陡度i 

	std::vector<DirectX::XMFLOAT3> m_OriginalPosition;		// 初始顶点位置数据
	std::vector<VertexPosNormalTex> m_Vertices;				// 保存当前模拟结果的顶点二维数组的一维展开
	ComPtr<ID3D11Buffer> m_pVertexBuffer;					// 当前模拟的顶点缓冲区
	ComPtr<ID3D11Buffer> m_pIndexBuffer;					// 当前模拟的索引缓冲区

	ComPtr<ID3D11ShaderResourceView> m_pTextureDiffuse;		// 水面纹理


};


class GpuGerstnerWavesRender:public GerstnerWavesRender
{
public:
	GpuGerstnerWavesRender()=default;
	~GpuGerstnerWavesRender()=default;
	//不允许拷贝,允许移动
	GpuGerstnerWavesRender(const GpuGerstnerWavesRender&) = delete;
	GpuGerstnerWavesRender& operator=(const GpuGerstnerWavesRender&) = delete;
	GpuGerstnerWavesRender(GpuGerstnerWavesRender&&) = default;
	GpuGerstnerWavesRender& operator=(GpuGerstnerWavesRender&&) = default;

	HRESULT InitResource(ID3D11Device* device,
		const std::wstring& texFileName,				// 纹理文件名
		UINT rows,										// 顶点行数
		UINT cols,										// 顶点列数
		float texU,										// 纹理坐标U方向最大值
		float texV,										// 纹理坐标V方向最大值
		float spatialstep,								// 空间步长
		UINT numwaves,									// 波浪数目
		float gradient,									// 陡度
		std::vector<GerstnerWaveParameter> parameters	// 对应波浪参数
	);

	void Update(ID3D11DeviceContext* deviceContext, GerstnerWavesEffect* gerstnerWavesEffect, float gametime);

	void Draw(ID3D11DeviceContext* deviceContext, GerstnerWavesEffect* gerstnerwaveseffect);

	// 设置调试对象名
	void SetDebugObjectName(const std::string& name);

private:
	std::vector<DirectX::XMFLOAT4> m_pCurrVertex;			// 当前模拟的顶点数据
	std::vector<DirectX::XMFLOAT4> m_pCurrNormal;			// 当前模拟的法线数据

	std::vector<VertexPosNormalTex> m_pVertevices;			// 顶点数据

	ComPtr<ID3D11Texture2D> m_pOriSolution;					// 保存原始顶点数据的二维数组
	ComPtr<ID3D11Texture2D> m_pCurrSolution;				// 保存当前模拟顶点结果的二维数组
	ComPtr<ID3D11Texture2D> m_pNormalSolution;				// 保存当前模拟法线结果的二维数组

	ComPtr<ID3D11Texture2D> m_pCurrOut;						// 输出当前模拟顶点结果的二维数组
	ComPtr<ID3D11Texture2D> m_pNormalOut;					// 输出当前模拟法线结果的二维数组

	ComPtr<ID3D11ShaderResourceView> m_pOriSolutionSRV;		// 缓存原始顶点的3d向量着色器资源视图

	ComPtr<ID3D11UnorderedAccessView> m_pCurrSolutionUAV;	// 缓存当前模拟结果的3d向量无序访问视图
	ComPtr<ID3D11UnorderedAccessView> m_pNormalSolutionUAV;	// 缓存当前模拟法线结果的3d向量无序访问视图

	ComPtr<ID3D11Buffer> m_pVertexBuffer;					// 当前模拟的顶点缓冲区
	ComPtr<ID3D11Buffer> m_pIndexBuffer;					// 当前模拟的索引缓冲区

	ComPtr<ID3D11ShaderResourceView> m_pTextureDiffuse;		// 水面纹理
};

#endif // !GERSTNERWAVESRENDER_H
