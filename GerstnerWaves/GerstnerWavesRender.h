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
	//��������,�����ƶ�
	GerstnerWavesRender(const GerstnerWavesRender&) = delete;
	GerstnerWavesRender& operator=(const GerstnerWavesRender&) = delete;
	GerstnerWavesRender(GerstnerWavesRender&&) = default;
	GerstnerWavesRender& operator=(GerstnerWavesRender&&) = default;

	void Init(
		UINT rows,												// ��������
		UINT cols,												// ��������
		float texU,												// ��������U�������ֵ
		float texV,												// ��������V�������ֵ
		float spatialstep,										// �ռ䲽��
		UINT numwaves,											// ������Ŀ
		float gradient,											// ����(0-1)
		std::vector<GerstnerWaveParameter> parameters			// ��Ӧ���˲���
	);

protected:
	UINT m_NumRows = 0;                         //��������
	UINT m_NumCols = 0;							//��������

	UINT m_VertexCount = 0;						//������Ŀ
	UINT m_IndexCount = 0;						//������Ŀ

	Transform m_Transform = {};					//ˮ��任
	DirectX::XMFLOAT2 m_Texoffset = {};			//��������ƫ��
	Material m_Material = {};					//ˮ�����
	float m_TexU = 0.0f;						//��������U�������ֵ
	float m_TexV = 0.0f;						//��������V�������ֵ
	float m_SpatialStep = 0.0f;                 //�ռ䲽��

	float m_TotalGradient=0.0f;									// ����
	UINT m_NumWaves = 0;										// ���˵���Ŀ
	UINT m_MaxNumWaves = 10;									// �������Ŀ
	std::vector<GerstnerWaveParameter> m_Paramters = {};		// ���˵Ĳ���
};

class CpuGerstnerWavesRender:public GerstnerWavesRender
{
public:
	CpuGerstnerWavesRender() = default;
	~CpuGerstnerWavesRender() = default;
	//��������,�����ƶ�
	CpuGerstnerWavesRender(const CpuGerstnerWavesRender&) = delete;
	CpuGerstnerWavesRender& operator=(const CpuGerstnerWavesRender&) = delete;
	CpuGerstnerWavesRender(CpuGerstnerWavesRender&&) = default;
	CpuGerstnerWavesRender& operator=(CpuGerstnerWavesRender&&) = default;

	HRESULT InitResource(ID3D11Device* device,
		const std::wstring& texFileName,				// �����ļ���
		UINT rows,										// ��������
		UINT cols,										// ��������
		float texU,										// ��������U�������ֵ
		float texV,										// ��������V�������ֵ
		float spatialstep,								// �ռ䲽��
		UINT numwaves,									// ������Ŀ
		float gradient,									// ����
		std::vector<GerstnerWaveParameter> parameters	// ��Ӧ���˲���
	);
	//�޸Ķ�Ӧ���˵Ĳ���
	void SetGerstnerWavesParameter(size_t wavesIndex, GerstnerWaveParameter parameter);

	void Update(float gametime);

	void Draw(ID3D11DeviceContext* deviceContext, GerstnerWavesEffect* gerstnerwaveseffect);



	// ���õ��Զ�����
	void SetDebugObjectName(const std::string& name);

private:
	std::vector<float> m_AngleFrequency = {};					// ��Ƶ��wi
	std::vector<float> m_PrimaryPhase = {};						// ����i
	std::vector<float> m_Gradient = {};							// ����i 

	std::vector<DirectX::XMFLOAT3> m_OriginalPosition;		// ��ʼ����λ������
	std::vector<VertexPosNormalTex> m_Vertices;				// ���浱ǰģ�����Ķ����ά�����һάչ��
	ComPtr<ID3D11Buffer> m_pVertexBuffer;					// ��ǰģ��Ķ��㻺����
	ComPtr<ID3D11Buffer> m_pIndexBuffer;					// ��ǰģ�������������

	ComPtr<ID3D11ShaderResourceView> m_pTextureDiffuse;		// ˮ������


};


class GpuGerstnerWavesRender:public GerstnerWavesRender
{
public:
	GpuGerstnerWavesRender()=default;
	~GpuGerstnerWavesRender()=default;
	//��������,�����ƶ�
	GpuGerstnerWavesRender(const GpuGerstnerWavesRender&) = delete;
	GpuGerstnerWavesRender& operator=(const GpuGerstnerWavesRender&) = delete;
	GpuGerstnerWavesRender(GpuGerstnerWavesRender&&) = default;
	GpuGerstnerWavesRender& operator=(GpuGerstnerWavesRender&&) = default;

	HRESULT InitResource(ID3D11Device* device,
		const std::wstring& texFileName,				// �����ļ���
		UINT rows,										// ��������
		UINT cols,										// ��������
		float texU,										// ��������U�������ֵ
		float texV,										// ��������V�������ֵ
		float spatialstep,								// �ռ䲽��
		UINT numwaves,									// ������Ŀ
		float gradient,									// ����
		std::vector<GerstnerWaveParameter> parameters	// ��Ӧ���˲���
	);

	void Update(ID3D11DeviceContext* deviceContext, GerstnerWavesEffect* gerstnerWavesEffect, float gametime);

	void Draw(ID3D11DeviceContext* deviceContext, GerstnerWavesEffect* gerstnerwaveseffect);

	// ���õ��Զ�����
	void SetDebugObjectName(const std::string& name);

private:
	std::vector<DirectX::XMFLOAT4> m_pCurrVertex;			// ��ǰģ��Ķ�������
	std::vector<DirectX::XMFLOAT4> m_pCurrNormal;			// ��ǰģ��ķ�������

	std::vector<VertexPosNormalTex> m_pVertevices;			// ��������

	ComPtr<ID3D11Texture2D> m_pOriSolution;					// ����ԭʼ�������ݵĶ�ά����
	ComPtr<ID3D11Texture2D> m_pCurrSolution;				// ���浱ǰģ�ⶥ�����Ķ�ά����
	ComPtr<ID3D11Texture2D> m_pNormalSolution;				// ���浱ǰģ�ⷨ�߽���Ķ�ά����

	ComPtr<ID3D11Texture2D> m_pCurrOut;						// �����ǰģ�ⶥ�����Ķ�ά����
	ComPtr<ID3D11Texture2D> m_pNormalOut;					// �����ǰģ�ⷨ�߽���Ķ�ά����

	ComPtr<ID3D11ShaderResourceView> m_pOriSolutionSRV;		// ����ԭʼ�����3d������ɫ����Դ��ͼ

	ComPtr<ID3D11UnorderedAccessView> m_pCurrSolutionUAV;	// ���浱ǰģ������3d�������������ͼ
	ComPtr<ID3D11UnorderedAccessView> m_pNormalSolutionUAV;	// ���浱ǰģ�ⷨ�߽����3d�������������ͼ

	ComPtr<ID3D11Buffer> m_pVertexBuffer;					// ��ǰģ��Ķ��㻺����
	ComPtr<ID3D11Buffer> m_pIndexBuffer;					// ��ǰģ�������������

	ComPtr<ID3D11ShaderResourceView> m_pTextureDiffuse;		// ˮ������
};

#endif // !GERSTNERWAVESRENDER_H
