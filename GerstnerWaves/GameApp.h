#ifndef GAMEAPP_H
#define GAMEAPP_H

#include <random>
#include "d3dApp.h"
#include "Camera.h"
#include "GameObject.h"
#include "ObjReader.h"
#include "SkyRender.h"
#include "GerstnerWavesRender.h"
#include "Collision.h"

class GameApp : public D3DApp
{
public:
	// 摄像机模式
	enum class CameraMode { FirstPerson, ThirdPerson, Free };
public:
	GameApp(HINSTANCE hInstance);
	~GameApp();

	bool Init();
	void OnResize();
	void UpdateScene(float dt);
	void DrawScene();

private:
	bool InitResource();

private:

	ComPtr<ID2D1SolidColorBrush> m_pColorBrush;											// 单色笔刷
	ComPtr<IDWriteFont> m_pFont;														// 字体
	ComPtr<IDWriteTextFormat> m_pTextFormat;											// 文本格式

	ObjReader m_ObjReader;
	GameObject m_Ground;																// 地面

	std::unique_ptr<BasicEffect> m_pBasicEffect;										// 基础特效
	std::unique_ptr<GerstnerWavesEffect> m_pGerstnerWavesEffect;						// Gerstner波浪特效
	std::unique_ptr<CpuGerstnerWavesRender> m_pCpuGerstnerWavesRender;					// Cpu Gerstner波浪
	std::unique_ptr<GpuGerstnerWavesRender> m_pGpuGerstnerWavesRender;;					// Gpu Gerstner波浪

	std::vector<GerstnerWavesEffect::GerstnerWaveParameter> m_GerstnerWaveParameters;	// 波浪参数
	UINT m_NumWaves;																	// 数目
	float m_Gradient;																	// 波浪陡度	
	float m_WindDirection;																// 风向
	float m_WindSpeed;																	// 风速

	bool m_IsGpuEnable;																	// 是否开启GPU绘制
	bool m_IsWireframe;																	// 是否开启线框
	std::shared_ptr<Camera> m_pCamera;													// 摄像机
};


#endif