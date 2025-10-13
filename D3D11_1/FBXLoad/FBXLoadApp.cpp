#include "FBXLoadApp.h"
#include "../Common/Helper.h"

#include <directxtk/SimpleMath.h>
#include <dxgidebug.h>
#include <dxgi1_3.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "d3dcompiler.lib")

#include <string>

using namespace DirectX::SimpleMath;
using namespace DirectX;
using Microsoft::WRL::ComPtr;

#define USE_FLIPMODE 1 // 경고 메세지를 없애려면 Flip 모드를 사용한다.

// 상수 버퍼
struct ConstantBuffer
{
	Matrix world;
	Matrix view;
	Matrix projection;

	Vector4 lightDirection;
	Color lightColor;

	Color outputColor;

	Vector4 ambient;	// 환경광
	Vector4 diffuse;	// 난반사
	Vector4 specular;	// 정반사
	FLOAT shininess;	// 광택지수
	Vector3 CameraPos;
};

struct Material
{
	Vector4 ambient;
	Vector4 diffuse;
	Vector4 specular;
};

FBXLoadApp::FBXLoadApp(HINSTANCE hInstance)
	: GameApp(hInstance)
{

}

FBXLoadApp::~FBXLoadApp()
{
	UninitImGUI();
}

bool FBXLoadApp::OnInitialize()
{
	if (!InitD3D())
		return false;

	if (!InitScene())
		return false;

	if (!InitImGUI())
		return false;

	if (!InitEffect())
		return false;

	ResetValues();

	return true;
}

void FBXLoadApp::OnUpdate()
{
	float delta = GameTimer::m_Instance->DeltaTime();

	Matrix scale = Matrix::Identity;
	Matrix rotate = Matrix::Identity;
	Matrix position = Matrix::Identity;

	// Cube Position
	position = m_World.CreateTranslation(m_ZeldaPosition);
	rotate = m_World.CreateFromYawPitchRoll(m_ZeldaRotation);
	scale = m_World.CreateScale(m_ZeldaScale);

	m_World = scale * rotate * position;

	// Directional Light Position

	// Camera
	m_Camera.GetCameraViewMatrix(m_View);
}

void FBXLoadApp::OnRender()
{
#if USE_FLIPMODE == 1
	// Flip 모드에서는 매프레임 설정해야한다.
	m_pDeviceContext->OMSetRenderTargets(1, m_pRenderTargetView.GetAddressOf(), m_pDepthStencilView.Get()); // depthStencilView 사용
#endif	

	// 블랜드 상태 바인딩
	float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	UINT sampleMask = 0xffffffff;

	m_pDeviceContext->OMSetBlendState(m_pBlendState.Get(), blendFactor, sampleMask);

	// 화면 칠하기.
	Color color(0.1f, 0.2f, 0.3f, 1.0f);
	m_pDeviceContext->ClearRenderTargetView(m_pRenderTargetView.Get(), color);
	m_pDeviceContext->ClearDepthStencilView(m_pDepthStencilView.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0); // 뎁스버퍼 1.0f로 초기화.

	// 오브젝트 렌더링 ==================================================================================================================================

	// Update Constant Values
	ConstantBuffer cb;
	cb.world = XMMatrixTranspose(m_World);
	cb.view = XMMatrixTranspose(m_View);
	cb.projection = XMMatrixTranspose(m_Projection);
	cb.lightDirection = m_LightDirection;
	cb.lightDirection.Normalize();
	cb.lightColor = m_LightColor;
	cb.outputColor = Vector4::Zero;

	cb.ambient = m_LightAmbient;
	cb.diffuse = m_LightDiffuse;
	cb.specular = m_LightSpecular;

	cb.shininess = m_Shininess;
	cb.CameraPos = m_Camera.m_Position;
	m_pDeviceContext->UpdateSubresource(m_pConstantBuffer.Get(), 0, nullptr, &cb, 0, 0);

	Material mat;
	mat.ambient = m_ModelAmbient;
	mat.diffuse = m_ModelDiffuse;
	mat.specular = m_ModelSpecular;
	m_pDeviceContext->UpdateSubresource(m_pMaterialBuffer.Get(), 0, nullptr, &mat, 0, 0);

	// 텍스처 및 샘플링 설정 
	m_pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_pDeviceContext->IASetInputLayout(m_pInputLayout.Get());

	m_pDeviceContext->VSSetShader(m_pVertexShader.Get(), 0, 0);
	m_pDeviceContext->VSSetConstantBuffers(0, 1, m_pConstantBuffer.GetAddressOf());

	m_pDeviceContext->PSSetShader(isBlinnPhong ? m_pBlinnPhongShader.Get() : m_pPhongShader.Get(), 0, 0);
	m_pDeviceContext->PSSetConstantBuffers(0, 1, m_pConstantBuffer.GetAddressOf());
	m_pDeviceContext->PSSetConstantBuffers(1, 1, m_pMaterialBuffer.GetAddressOf());

	m_pDeviceContext->PSSetSamplers(0, 1, m_pSamplerLinear.GetAddressOf());

	// zelda1 rendering
	Matrix position = m_World.CreateTranslation(m_ZeldaPosition);
	Matrix rotate = m_World.CreateFromYawPitchRoll(m_ZeldaRotation);
	Matrix scale = m_World.CreateScale(m_ZeldaScale);

	m_World = scale * rotate * position;
	cb.world = XMMatrixTranspose(m_World);
	m_pDeviceContext->UpdateSubresource(m_pConstantBuffer.Get(), 0, nullptr, &cb, 0, 0);

	m_pZelda1->Draw(m_pDeviceContext);

	// character1 rendering
	position = m_World.CreateTranslation(m_CharaPosition);
	rotate = m_World.CreateFromYawPitchRoll(m_CharaRotation);
	scale = m_World.CreateScale(m_CharaScale);
	
	m_World = scale * rotate * position;
	cb.world = XMMatrixTranspose(m_World);
	m_pDeviceContext->UpdateSubresource(m_pConstantBuffer.Get(), 0, nullptr, &cb, 0, 0);
	
	m_pCharacter1->Draw(m_pDeviceContext);
	
	// tree redering
	position = m_World.CreateTranslation(m_TreePosition);
	rotate = m_World.CreateFromYawPitchRoll(m_TreeRotation);
	scale = m_World.CreateScale(m_TreeScale);
	
	m_World = scale * rotate * position;
	cb.world = XMMatrixTranspose(m_World);	
	m_pDeviceContext->UpdateSubresource(m_pConstantBuffer.Get(), 0, nullptr, &cb, 0, 0);
	
	m_pTree1->Draw(m_pDeviceContext);

	// Render ImGui
	RenderImGUI();

	// 스왑체인 교체
	m_pSwapChain->Present(0, 0);
}

bool FBXLoadApp::InitImGUI()
{
	bool isSetupSuccess = false;

	// Setup Dear ImGui context 
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();

	// Setup Platform/Renderer backends
	isSetupSuccess = ImGui_ImplWin32_Init(m_hWnd);
	if (!isSetupSuccess) return false;

	isSetupSuccess = ImGui_ImplDX11_Init(m_pDevice.Get(), m_pDeviceContext.Get());
	if (!isSetupSuccess) return false;

	return true;
}

void FBXLoadApp::RenderImGUI()
{
	// Start the Dear ImGui frame
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	// 월드 오브젝트 조종 창 만들기
	ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Once);		// 처음 실행될 때 위치 초기화
	ImGui::SetNextWindowSize(ImVec2(350, 500), ImGuiCond_Once);		// 처음 실행될 때 창 크기 초기화
	ImGui::Begin("World Controller");

	// zelda config
	{
		// zelda1 위치 조절 
		ImGui::DragFloat3("Zelda Position", &m_ZeldaPosition.x);

		// zelda1 회전
		Vector3 zeldaRotation;
		zeldaRotation.x = XMConvertToDegrees(m_ZeldaRotation.x);
		zeldaRotation.y = XMConvertToDegrees(m_ZeldaRotation.y);
		zeldaRotation.z = XMConvertToDegrees(m_ZeldaRotation.z);
		ImGui::DragFloat3("Zelda Rotation", &zeldaRotation.x);
		m_ZeldaRotation.x = XMConvertToRadians(zeldaRotation.x);
		m_ZeldaRotation.y = XMConvertToRadians(zeldaRotation.y);
		m_ZeldaRotation.z = XMConvertToRadians(zeldaRotation.z);


		// zelda1 크기
		ImGui::DragFloat("Zelda Scale", &m_ZeldaScale.x, 0.05f);
		m_ZeldaScale.y = m_ZeldaScale.x;
		m_ZeldaScale.z = m_ZeldaScale.x;
	}

	// character config
	{
		ImGui::DragFloat3("Character Position", &m_CharaPosition.x);

		Vector3 charaRotation;
		charaRotation.x = XMConvertToDegrees(m_CharaRotation.x);
		charaRotation.y = XMConvertToDegrees(m_CharaRotation.y);
		charaRotation.z = XMConvertToDegrees(m_CharaRotation.z);
		ImGui::DragFloat3("Character Rotation", &charaRotation.x);
		m_CharaRotation.x = XMConvertToRadians(charaRotation.x);
		m_CharaRotation.y = XMConvertToRadians(charaRotation.y);
		m_CharaRotation.z = XMConvertToRadians(charaRotation.z);

		ImGui::DragFloat("Character Scale", &m_CharaScale.x, 0.05f);
		m_CharaScale.y = m_CharaScale.x;
		m_CharaScale.z = m_CharaScale.x;
	}

	// tree config
	{
		ImGui::DragFloat3("Tree Position", &m_TreePosition.x);

		Vector3 treeRotation;
		treeRotation.x = XMConvertToDegrees(m_TreeRotation.x);
		treeRotation.y = XMConvertToDegrees(m_TreeRotation.y);
		treeRotation.z = XMConvertToDegrees(m_TreeRotation.z);
		ImGui::DragFloat3("tree Rotation", &treeRotation.x);
		m_TreeRotation.x = XMConvertToRadians(treeRotation.x);
		m_TreeRotation.y = XMConvertToRadians(treeRotation.y);
		m_TreeRotation.z = XMConvertToRadians(treeRotation.z);

		ImGui::DragFloat("tree Scale", &m_TreeScale.x, 0.05f);
		m_TreeScale.y = m_TreeScale.x;
		m_TreeScale.z = m_TreeScale.x;
	}

	ImGui::NewLine();

	// 카메라 위치 및 회전 설정
	ImGui::DragFloat3("Camera Position", &m_Camera.m_Position.x);
	ImGui::DragFloat3("Camera Rotation", &m_CameraRotation.x);

	if (ImGui::Button("Set Rotation"))
	{
		m_Camera.m_Rotation.x = XMConvertToRadians(m_CameraRotation.x);
		m_Camera.m_Rotation.y = XMConvertToRadians(m_CameraRotation.y);
		m_Camera.m_Rotation.z = XMConvertToRadians(m_CameraRotation.z);
	}

	ImGui::NewLine();

	// Near 값 설정
	ImGui::DragFloat("Near", &m_Near, 0.5f);

	// Far 값 설정
	ImGui::DragFloat("Far", &m_Far, 0.5f);

	// Fov 값 설정
	ImGui::DragFloat("Fov Angle", &m_PovAngle, 0.02f);

	if (m_Near <= 0.0f) m_Near = 0.01f;
	if (m_Far <= 0.0f) m_Far = 0.2f;

	ImGui::NewLine();

	// Lighting 설정
	m_Projection = XMMatrixPerspectiveFovLH(m_PovAngle, m_ClientWidth / (FLOAT)m_ClientHeight, m_Near, m_Far);

	ImGui::ColorEdit4("Light Color", &m_LightColor.x);
	ImGui::DragFloat3("Light Direction", &m_LightDirection.x, 0.1f, -1.0f, 1.0f);

	ImGui::ColorEdit4("Light Ambient", &m_LightAmbient.x);
	ImGui::ColorEdit4("Light Diffuse", &m_LightDiffuse.x);
	ImGui::ColorEdit4("Light Specular", &m_LightSpecular.x);
	ImGui::DragFloat("Shininess", &m_Shininess, 10.0f);

	ImGui::Spacing();
	ImGui::ColorEdit4("Cube Ambient", &m_ModelAmbient.x);
	ImGui::ColorEdit4("Cube Diffuse", &m_ModelDiffuse.x);
	ImGui::ColorEdit4("Cube Specular", &m_ModelSpecular.x);

	ImGui::Checkbox("Use Blinn-Phong", &isBlinnPhong);

	ImGui::NewLine();

	// 리셋 버튼
	if (ImGui::Button("Reset", { 50, 20 }))
	{
		ResetValues();
	}

	ImGui::End();

	// rendering
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void FBXLoadApp::UninitImGUI()
{
	// Cleanup
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

bool FBXLoadApp::InitD3D()
{
	HRESULT hr = S_OK;

	// 1. D3D11 Device, DeviceContext 생성
	UINT creationFlag = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

#ifdef _DEBUG
	creationFlag |= D3D11_CREATE_DEVICE_DEBUG;
#endif //  _DEBUG

	// 그래픽 카드 하드웨어의 스펙으로 호환되는 가장 높은 DirectX 기능레벨로 생성하여 드라이버가 작동한다.
	// 인터페이스는 Direct3D11이지만 GPU 드라이버는 D3D12 드라이버가 작동될 수 있다.
	D3D_FEATURE_LEVEL featureLevels[] =
	{	// 0번 index부터 순서대로 시도
		D3D_FEATURE_LEVEL_12_2,D3D_FEATURE_LEVEL_12_1,D3D_FEATURE_LEVEL_12_0,D3D_FEATURE_LEVEL_11_1,D3D_FEATURE_LEVEL_11_0
	};
	D3D_FEATURE_LEVEL actualFeatureLevel;	// 최종 feature level 저장 변수

	HR_T(D3D11CreateDevice
	(
		nullptr,
		D3D_DRIVER_TYPE_HARDWARE,
		0,
		creationFlag,
		featureLevels,
		ARRAYSIZE(featureLevels),
		D3D11_SDK_VERSION,
		&m_pDevice,
		&actualFeatureLevel,
		&m_pDeviceContext
	));


	// 2. 스왑체인 생성을 위한 DXGI Factory 생성
	UINT dxgiFactoryFlags = 0;

#ifdef _DEBUG
	dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif // _DEBUG

	ComPtr<IDXGIFactory2> pFactory;
	HR_T(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&pFactory)));

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
#if USE_FLIPMODE == 1
	swapChainDesc.BufferCount = 2;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
#else
	swapChainDesc.BufferCount = 1;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
#endif
	swapChainDesc.Width = m_ClientWidth;
	swapChainDesc.Height = m_ClientHeight;

	// 하나의 픽셀이 채널 RGBA 각 8비트 형식으로 표현
	// Unsigned Normalized Integer 8비트 정수(0~255)단계를 부동소수점으로 정규화한 0.0~1.0으로 매핑하여 표현한다.
	swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // 스왑 체인의 백 버퍼가 렌더링 파이프라인의 최종 출력 대상으로 사용
	swapChainDesc.SampleDesc.Count = 1;	// 멀티 샘플링 사용 안함
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE; // 투명도 조작 무시 | recommand for flip mode ?
	swapChainDesc.Stereo = FALSE;
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH; // 전체 화면 전환을 허용
	swapChainDesc.Scaling = DXGI_SCALING_NONE; // 창의 크기와 백 버퍼의 크기가 다를 때. 백버퍼 크기에 맞게 스케일링 하지 않는다.

	HR_T(pFactory->CreateSwapChainForHwnd
	(
		m_pDevice.Get(),
		m_hWnd,
		&swapChainDesc,
		nullptr,
		nullptr,
		&m_pSwapChain
	));

	// 3. 랜더타겟 뷰 생성. 랜더타겟 뷰는 "여기에 그림을 그려라"라고 GPU에게 알려주는 역할을 하는 객체
	// 텍스쳐와 영구적으로 연결되는 객체
	ComPtr<ID3D11Texture2D> pBackBufferTexture;
	HR_T(m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBufferTexture));
	HR_T(m_pDevice->CreateRenderTargetView(pBackBufferTexture.Get(), nullptr, m_pRenderTargetView.GetAddressOf()));

#if !USE_FLIPMODE
	m_pDeviceContext->OMSetRenderTargets(1, m_pRenderTargetView.GetAddressOf(), nullptr);
#endif

	// 4. viewport 설정
	D3D11_VIEWPORT viewport = {};
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = (float)m_ClientWidth;
	viewport.Height = (float)m_ClientHeight;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	m_pDeviceContext->RSSetViewports(1, &viewport);

	// 5. 뎊스 스텐실 뷰 설정
	D3D11_TEXTURE2D_DESC descDepth = {};
	descDepth.Width = m_ClientWidth;
	descDepth.Height = m_ClientHeight;
	descDepth.MipLevels = 1;
	descDepth.ArraySize = 1;
	descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; // https://learn.microsoft.com/ko-kr/windows/win32/api/dxgiformat/ne-dxgiformat-dxgi_format
	descDepth.SampleDesc.Count = 1;
	descDepth.SampleDesc.Quality = 0;
	descDepth.Usage = D3D11_USAGE_DEFAULT;
	descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	descDepth.CPUAccessFlags = 0;
	descDepth.MiscFlags = 0;

	D3D11_DEPTH_STENCIL_DESC depthStencilDesc = {};
	depthStencilDesc.DepthEnable = TRUE;                // 깊이 테스트 활성화
	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL; // 깊이 버퍼 업데이트 허용
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS; // 작은 Z 값이 앞에 배치되도록 설정
	depthStencilDesc.StencilEnable = FALSE;            // 스텐실 테스트 비활성화

	// m_pDeviceContext->OMSetDepthStencilState(m_pDepthStencilState.Get(), 1);

	// create depthStencil texture
	ComPtr<ID3D11Texture2D> pTextureDepthStencil;
	HR_T(m_pDevice->CreateTexture2D(&descDepth, nullptr, pTextureDepthStencil.GetAddressOf()));

	// create the depth stencil view
	D3D11_DEPTH_STENCIL_VIEW_DESC descDSV = {};
	descDSV.Format = descDepth.Format;
	descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D; // 사용되는 리소스 엑세스 방식 설정 : https://learn.microsoft.com/ko-kr/windows/win32/api/d3d11/ne-d3d11-d3d11_dsv_dimension 
	descDSV.Texture2D.MipSlice = 0;
	HR_T(m_pDevice->CreateDepthStencilView(pTextureDepthStencil.Get(), &descDSV, m_pDepthStencilView.GetAddressOf()));

	// create blending state https://learn.microsoft.com/ko-kr/windows/win32/api/d3d11/ns-d3d11-d3d11_blend_desc
	D3D11_BLEND_DESC descBlend = {};
	ZeroMemory(&descBlend, sizeof(D3D11_BLEND_DESC));
	descBlend.RenderTarget[0].BlendEnable = true;

	descBlend.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;		
	descBlend.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;	

	descBlend.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	descBlend.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;

	descBlend.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE; 
	descBlend.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;

	descBlend.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL; 

	m_pDevice->CreateBlendState(&descBlend, m_pBlendState.GetAddressOf());

	return true;
}

bool FBXLoadApp::InitScene()
{
	HRESULT hr = S_OK;

	// 6. Render() 에서 파이프라인에 바인딩할 상수 버퍼 생성
	D3D11_BUFFER_DESC bufferDesc = {};
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(ConstantBuffer);
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	HR_T(m_pDevice->CreateBuffer(&bufferDesc, nullptr, m_pConstantBuffer.GetAddressOf()));

	// 쉐이더에 상수버퍼에 전달할 시스템 메모리 데이터 초기화
	m_World = XMMatrixIdentity();

	XMVECTOR Eye = XMVectorSet(0.0f, 10.0f, -8.0f, 0.0f);
	XMVECTOR At = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
	XMVECTOR Up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	m_View = XMMatrixLookAtLH(Eye, At, Up);
	m_Projection = XMMatrixPerspectiveFovLH(m_PovAngle, m_ClientWidth / (FLOAT)m_ClientHeight, m_Near, m_Far);

	// 샘플링 상태 설정
	D3D11_SAMPLER_DESC sampDesc = {};
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;	// 텍스처 샘플링할 때 사용할 필터링 방법
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;		// 범위 밖에 있는 텍스처 좌표 확인 방법
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;	// 샘플링된 데이터를 기존 데이터와 확인하는 방법
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	HR_T(m_pDevice->CreateSamplerState(&sampDesc, m_pSamplerLinear.GetAddressOf()));

	// 래스터라이저
	D3D11_RASTERIZER_DESC rasterizerState = {};
	rasterizerState.CullMode = D3D11_CULL_FRONT;
	rasterizerState.FillMode = D3D11_FILL_SOLID;
	rasterizerState.DepthClipEnable = true;
	rasterizerState.FrontCounterClockwise = true;

	// material buffer
	bufferDesc = {};
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(Material);
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	HR_T(m_pDevice->CreateBuffer(&bufferDesc, nullptr, m_pMaterialBuffer.GetAddressOf()));

	// 모델 생성
	m_pZelda1 = make_unique<ModelLoader>();
	if (!m_pZelda1->Load(m_hWnd, m_pDevice, m_pDeviceContext, "Resource\\zeldaPosed001.fbx"))
	{
		MessageBox(m_hWnd, L"FBX file is invaild at path", NULL, MB_ICONERROR | MB_OK);
	}

	m_pCharacter1 = make_unique<ModelLoader>();
	if (!m_pCharacter1->Load(m_hWnd, m_pDevice, m_pDeviceContext, "Resource\\Character.fbx"))
	{
		MessageBox(m_hWnd, L"FBX file is invaild at path", NULL, MB_ICONERROR | MB_OK);
	}

	m_pTree1 = make_unique<ModelLoader>();
	if (!m_pTree1->Load(m_hWnd, m_pDevice, m_pDeviceContext, "Resource\\Tree.fbx"))
	{
		MessageBox(m_hWnd, L"FBX file is invaild at path", NULL, MB_ICONERROR | MB_OK);
	}

	return true;
}

bool FBXLoadApp::InitEffect()
{
	// 2. 파이프라인에 바인딩할 InputLayout 생성
	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "BINORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	ComPtr<ID3DBlob> vertexShaderBuffer = nullptr;
	HR_T(CompileShaderFromFile(L"Shaders\\BasicVertexShader.hlsl", "main", "vs_4_0", vertexShaderBuffer.GetAddressOf()));
	HR_T(m_pDevice->CreateInputLayout(layout, ARRAYSIZE(layout), vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), m_pInputLayout.GetAddressOf()));

	// 3. 파이프 라인에 바인딩할 정점 셰이더 생성
	HR_T(m_pDevice->CreateVertexShader(vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), NULL, m_pVertexShader.GetAddressOf()));

	// 5. 파이프라인에 바인딩할 픽셀 셰이더 생성
	ComPtr<ID3DBlob> pixelShaderBuffer = nullptr;
	HR_T(CompileShaderFromFile(L"Shaders\\BasicPixelShader.hlsl", "main", "ps_4_0", pixelShaderBuffer.GetAddressOf()));
	HR_T(m_pDevice->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, m_pPixelShader.GetAddressOf()));

	pixelShaderBuffer.Reset();
	HR_T(CompileShaderFromFile(L"Shaders\\PhongShader.hlsl", "main", "ps_4_0", pixelShaderBuffer.GetAddressOf()));
	HR_T(m_pDevice->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, m_pPhongShader.GetAddressOf()));

	pixelShaderBuffer.Reset();
	HR_T(CompileShaderFromFile(L"Shaders\\BlinnPhongShader.hlsl", "main", "ps_4_0", pixelShaderBuffer.GetAddressOf()));
	HR_T(m_pDevice->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, m_pBlinnPhongShader.GetAddressOf()));

	return true;
}

void FBXLoadApp::ResetValues()
{
	m_ZeldaPosition = m_ZeldaPositionInitial;
	m_CharaPosition = m_CharaPositionInitial;
	m_TreePosition = m_TreePositionInitial;

	m_Near = 0.01f;
	m_Far = 1000.0f;
	m_PovAngle = XM_PIDIV2;

	m_CameraRotation = Vector3::Zero;
	m_Camera.SetRotation(m_CameraRotation);
	m_Camera.SetPosition(m_CameraPositionInitial);

	m_Projection = XMMatrixPerspectiveFovLH(m_PovAngle, m_ClientWidth / (FLOAT)m_ClientHeight, m_Near, m_Far);

	m_LightDirection = m_LightDirectionInitial;
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT FBXLoadApp::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
		return true;

	return __super::WndProc(hWnd, message, wParam, lParam);
}