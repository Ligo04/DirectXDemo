#include "GameApp.h"
#include "d3dUtil.h"
#include "DXTrace.h"
using namespace DirectX;

#pragma warning(disable: 26812)

GameApp::GameApp(HINSTANCE hInstance)
	: D3DApp(hInstance),
	m_pBasicEffect(std::make_unique<BasicEffect>()),
	m_pGerstnerWavesEffect(std::make_unique<GerstnerWavesEffect>()),
	m_pCpuGerstnerWavesRender(std::make_unique<CpuGerstnerWavesRender>()),
	m_pGpuGerstnerWavesRender(std::make_unique<GpuGerstnerWavesRender>()),
	m_GerstnerWaveParameters(std::vector<GerstnerWavesEffect::GerstnerWaveParameter>()),
	m_NumWaves(0),
	m_Gradient(0),
	m_WindDirection(0),
	m_WindSpeed(0),
	m_IsGpuEnable(true),
	m_IsWireframe(false)
{
}

GameApp::~GameApp()
{
}

bool GameApp::Init()
{
	if (!D3DApp::Init())
		return false;

	// ����ȳ�ʼ��������Ⱦ״̬���Թ��������Чʹ��
	RenderStates::InitAll(m_pd3dDevice.Get());

	if (!m_pBasicEffect->InitAll(m_pd3dDevice.Get()))
		return false;
	
	if (!m_pGerstnerWavesEffect->InitAll(m_pd3dDevice.Get()))
		return false;

	if (!InitResource())
		return false;

	// ��ʼ����꣬���̲���Ҫ
	m_pMouse->SetWindow(m_hMainWnd);
	m_pMouse->SetMode(DirectX::Mouse::MODE_RELATIVE);

	return true;
}

void GameApp::OnResize()
{
	assert(m_pd2dFactory);
	assert(m_pdwriteFactory);
	// �ͷ�D2D�������Դ
	m_pColorBrush.Reset();
	m_pd2dRenderTarget.Reset();

	D3DApp::OnResize();

	// ΪD2D����DXGI������ȾĿ��
	ComPtr<IDXGISurface> surface;
	HR(m_pSwapChain->GetBuffer(0, __uuidof(IDXGISurface), reinterpret_cast<void**>(surface.GetAddressOf())));
	D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
		D2D1_RENDER_TARGET_TYPE_DEFAULT,
		D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED));
	HRESULT hr = m_pd2dFactory->CreateDxgiSurfaceRenderTarget(surface.Get(), &props, m_pd2dRenderTarget.GetAddressOf());
	surface.Reset();

	if (hr == E_NOINTERFACE)
	{
		OutputDebugStringW(L"\n���棺Direct2D��Direct3D�������Թ������ޣ��㽫�޷������ı���Ϣ�����ṩ������ѡ������\n"
			L"1. ����Win7ϵͳ����Ҫ������Win7 SP1������װKB2670838������֧��Direct2D��ʾ��\n"
			L"2. �������Direct3D 10.1��Direct2D�Ľ�����������ģ�"
			L"https://docs.microsoft.com/zh-cn/windows/desktop/Direct2D/direct2d-and-direct3d-interoperation-overview""\n"
			L"3. ʹ�ñ������⣬����FreeType��\n\n");
	}
	else if (hr == S_OK)
	{
		// �����̶���ɫˢ���ı���ʽ
		HR(m_pd2dRenderTarget->CreateSolidColorBrush(
			D2D1::ColorF(D2D1::ColorF::White),
			m_pColorBrush.GetAddressOf()));
		HR(m_pdwriteFactory->CreateTextFormat(L"����", nullptr, DWRITE_FONT_WEIGHT_NORMAL,
			DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 15, L"zh-cn",
			m_pTextFormat.GetAddressOf()));
	}
	else
	{
		// �����쳣����
		assert(m_pd2dRenderTarget);
	}

	// ����������ʾ
	if (m_pCamera != nullptr)
	{
		m_pCamera->SetFrustum(XM_PI / 3, AspectRatio(), 1.0f, 1000.0f);
		m_pCamera->SetViewPort(0.0f, 0.0f, (float)m_ClientWidth, (float)m_ClientHeight);
		m_pBasicEffect->SetProjMatrix(m_pCamera->GetProjXM());
	}
}

void GameApp::UpdateScene(float dt)
{

	// ��������¼�����ȡ���ƫ����
	Mouse::State mouseState = m_pMouse->GetState();
	Mouse::State lastMouseState = m_MouseTracker.GetLastState();
	m_MouseTracker.Update(mouseState);

	Keyboard::State keyState = m_pKeyboard->GetState();
	m_KeyboardTracker.Update(keyState);

	auto cam3rd = std::dynamic_pointer_cast<ThirdPersonCamera>(m_pCamera);

	// ******************
	// �����˳�������Ĳ���
	//

	// ��������ת
	// �����û���봰��ǰ��ΪABSOLUTEģʽ
	if (mouseState.positionMode == Mouse::MODE_RELATIVE)
	{
		cam3rd->RotateX(mouseState.y * dt * 1.25f);
		cam3rd->RotateY(mouseState.x * dt * 1.25f);
		cam3rd->Approach(-mouseState.scrollWheelValue / 120.0f * 1.0f);
	}

	m_pBasicEffect->SetViewMatrix(m_pCamera->GetViewXM());

	m_pBasicEffect->SetEyePos(m_pCamera->GetPosition());


	m_pGerstnerWavesEffect->SetViewMatrix(m_pCamera->GetViewXM());
	m_pGerstnerWavesEffect->SetEyePos(m_pCamera->GetPosition());
	
	// ���ù���ֵ
	m_pMouse->ResetScrollWheelValue();

	// �����߿�ģʽ
	if (m_KeyboardTracker.IsKeyPressed(Keyboard::D2))
	{
		m_IsWireframe = !m_IsWireframe;
		m_pGerstnerWavesEffect->SetRSWireframe(m_IsWireframe);
	}

	// CPU/GPUģʽ�л�
	if (m_KeyboardTracker.IsKeyPressed(Keyboard::D1))
	{
		m_IsGpuEnable = !m_IsGpuEnable;
		if (m_IsGpuEnable)
		{
			HR(m_pGpuGerstnerWavesRender->InitResource(m_pd3dDevice.Get(), L"..\\Texture\\water2.dds",
				256, 256, 5.0f, 5.0f, 0.625f, m_NumWaves, m_Gradient, m_GerstnerWaveParameters));
		}
		else
		{
			HR(m_pCpuGerstnerWavesRender->InitResource(m_pd3dDevice.Get(), L"..\\Texture\\water2.dds",
				256, 256, 5.0f, 5.0f, 0.625f, m_NumWaves, m_Gradient, m_GerstnerWaveParameters));
		}
	}

	// ���²���
	if (m_IsGpuEnable)
		m_pGpuGerstnerWavesRender->Update(m_pd3dImmediateContext.Get(), m_pGerstnerWavesEffect.get(), m_Timer.TotalTime());
	else
		m_pCpuGerstnerWavesRender->Update(m_Timer.TotalTime());


	// �˳���������Ӧ�򴰿ڷ���������Ϣ
	if (m_KeyboardTracker.IsKeyPressed(Keyboard::Escape))
		SendMessage(MainWnd(), WM_DESTROY, 0, 0);
}

void GameApp::DrawScene()
{
	assert(m_pd3dImmediateContext);
	assert(m_pSwapChain);

	m_pd3dImmediateContext->ClearRenderTargetView(m_pRenderTargetView.Get(), reinterpret_cast<const float*>(&Colors::Silver));
	m_pd3dImmediateContext->ClearDepthStencilView(m_pDepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	// ******************
	// �������Ƴ���
	//

	// ���Ƶ���
	m_pBasicEffect->SetRenderDefault(m_pd3dImmediateContext.Get(), BasicEffect::RenderObject);
	m_Ground.Draw(m_pd3dImmediateContext.Get(), m_pBasicEffect.get());

	// ���Ʋ���
	m_pGerstnerWavesEffect->SetRenderDefault(m_pd3dImmediateContext.Get());
	if (m_IsGpuEnable)
		m_pGpuGerstnerWavesRender->Draw(m_pd3dImmediateContext.Get(), m_pGerstnerWavesEffect.get());
	else
		m_pCpuGerstnerWavesRender->Draw(m_pd3dImmediateContext.Get(), m_pGerstnerWavesEffect.get());
	// ******************
	// ����Direct2D����
	//
	if (m_pd2dRenderTarget != nullptr)
	{
		m_pd2dRenderTarget->BeginDraw();
		std::wstring text = L"��ǰ�����ģʽ: �����˳�  Esc�˳�\n"
			L"����ƶ�������Ұ ���ֿ��Ƶ����˳ƹ۲����\n"
			L"��ǰGerstner���˻���ģʽ: ";
		text += m_IsGpuEnable ? L"GPUͨ�ü���ģʽ  " : L"CPU��̬����ģʽ  ";
		text += L"(1-�л�)\n�߿�:";
		text += m_IsWireframe ? L"��  " : L"��  ";
		text += L"(2-�л�)";


		m_pd2dRenderTarget->DrawTextW(text.c_str(), (UINT32)text.length(), m_pTextFormat.Get(),
			D2D1_RECT_F{ 0.0f, 0.0f, 600.0f, 200.0f }, m_pColorBrush.Get());
		HR(m_pd2dRenderTarget->EndDraw());
	}

	HR(m_pSwapChain->Present(0, 0));
}

bool GameApp::InitResource()
{
	// ******************
	// ��ʼ�������
	//

	auto camera = std::shared_ptr<ThirdPersonCamera>(new ThirdPersonCamera);
	m_pCamera = camera;

	camera->SetViewPort(0.0f, 0.0f, (float)m_ClientWidth, (float)m_ClientHeight);
	camera->SetTarget(XMFLOAT3(0.0f, 2.5f, 0.0f));
	camera->SetDistance(20.0f);
	camera->SetDistanceMinMax(10.0f, 90.0f);
	camera->SetFrustum(XM_PI / 3, AspectRatio(), 1.0f, 1000.0f);
	camera->SetRotationX(XM_PIDIV4);

	// ******************
	// ��ʼ����Ч
	//

	m_pBasicEffect->SetTextureUsed(true);
	m_pBasicEffect->SetShadowEnabled(false);
	m_pBasicEffect->SetSSAOEnabled(false);
	m_pBasicEffect->SetViewMatrix(camera->GetViewXM());
	m_pBasicEffect->SetProjMatrix(camera->GetProjXM());

	m_pGerstnerWavesEffect->SetTextureUsed(true);
	m_pGerstnerWavesEffect->SetViewMatrix(camera->GetViewXM());
	m_pGerstnerWavesEffect->SetProjMatrix(camera->GetProjXM());

	// ******************
	// ��ʼ������
	//

	// ��ʼ������
	m_ObjReader.Read(L"..\\Model\\ground_35.mbo", L"..\\Model\\ground_35.obj");
	m_Ground.SetModel(Model(m_pd3dDevice.Get(), m_ObjReader));

	// ******************
	// ��ʼ������
	//
	// �����(Ĭ��)
	DirectionalLight dirLight[4]{};
	dirLight[0].ambient = XMFLOAT4(0.15f, 0.15f, 0.15f, 1.0f);
	dirLight[0].diffuse = XMFLOAT4(0.25f, 0.25f, 0.25f, 1.0f);
	dirLight[0].specular = XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f);
	dirLight[0].direction = XMFLOAT3(-0.577f, -0.577f, 0.577f);
	dirLight[1] = dirLight[0];
	dirLight[1].direction = XMFLOAT3(0.577f, -0.577f, 0.577f);
	dirLight[2] = dirLight[0];
	dirLight[2].direction = XMFLOAT3(0.577f, -0.577f, -0.577f);
	dirLight[3] = dirLight[0];
	dirLight[3].direction = XMFLOAT3(-0.577f, -0.577f, -0.577f);
	for (int i = 0; i < 4; ++i)
	{
		m_pBasicEffect->SetDirLight(i, dirLight[i]);
		m_pGerstnerWavesEffect->SetDirLight(i, dirLight[i]);
	}

	// *****************
	// ��ʼ������
	//

	//���˲���
	m_NumWaves = 2;
	m_GerstnerWaveParameters.resize(m_NumWaves);
	m_GerstnerWaveParameters[0].direction = 45.0f;
	m_GerstnerWaveParameters[0].amplitude = 1.0f;
	m_GerstnerWaveParameters[0].waveLength = 20.0f;
	m_GerstnerWaveParameters[0].wavespeed = 0.01f;

	m_GerstnerWaveParameters[1].direction = 0.0f;
	m_GerstnerWaveParameters[1].amplitude = 1.0f;
	m_GerstnerWaveParameters[1].waveLength = 10.0f;
	m_GerstnerWaveParameters[1].wavespeed = 0.01f;

	m_Gradient = 0.25f;

	HR(m_pCpuGerstnerWavesRender->InitResource(m_pd3dDevice.Get(), L"..\\Texture\\water2.dds",
		256, 256, 5.0f, 5.0f, 0.625f, m_NumWaves, m_Gradient, m_GerstnerWaveParameters));
	Material material{};
	material.ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	material.diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 0.5f);
	material.specular = XMFLOAT4(0.8f, 0.8f, 0.8f, 32.0f);
	material.reflect = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
	m_pCpuGerstnerWavesRender->SetMaterial(material);


	HR(m_pGpuGerstnerWavesRender->InitResource(m_pd3dDevice.Get(), L"..\\Texture\\water2.dds",
		256, 256, 5.0f, 5.0f, 0.625f, m_NumWaves, m_Gradient, m_GerstnerWaveParameters));
	m_pGpuGerstnerWavesRender->SetMaterial(material);

	// ******************
	// ���õ��Զ�����
	//
	m_Ground.SetDebugObjectName("Ground");
	m_pGerstnerWavesEffect->SetDebugObjectName("GerstnerWavesEffect");
	m_pCpuGerstnerWavesRender->SetDebugObjectName("CpuGerstnerWaves");
	m_pGpuGerstnerWavesRender->SetDebugObjectName("GpuGerstnerWaves");
	return true;
}