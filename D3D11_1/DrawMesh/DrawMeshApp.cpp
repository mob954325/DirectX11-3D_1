#include "DrawMeshApp.h"
#include "../Common/Helper.h"

#include <directxtk/SimpleMath.h>
#include <dxgidebug.h>  // ?
#include <dxgi1_3.h>
#include <wrl/client.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib") // ?
#pragma comment(lib, "d3dcompiler.lib")

#include <string>

using namespace DirectX::SimpleMath;
using namespace DirectX;
using Microsoft::WRL::ComPtr;

#define USE_FLIPMODE 1 // 경고 메세지를 없애려면 Flip 모드를 사용한다.

// TODO 
// 1. 정육면체 랜더링하기 [v]
// 2. 정육면체 회전하기 [v]
// 3. 두 개의 서로 다른 정육면체가 계층 구조를 가지고 랜더링 [v]
// 4. 카메라 적용시키기 [v]
// 5. ImGUI를 아래와 같이 연결하기 [v] 
// 	a. 최상위 mesh의 월드 위치 변경 x,y,z
//  b.두번째 mesh 의 상대 위치 변경 x, y, z
//  c.세번째 mesh 의 상대 위치 변경 x, y, z
//  d.카메라의 월드 위치 변경 x, y, z
//  e.카메라의 FOV  각도(degree) 변경
//  f.카메라의 Near, Far 변경
// 6. depthStencil 사용해서 큐브가 뒤에 가리는지 확인하기 [v]

// 정점 
struct Vertex
{
	Vector3 localPosition;
	Vector4 color;

	Vertex(Vector3 pos = Vector3::Zero, Vector4 color = Vector4::Zero) : localPosition(pos), color(color) {}
};

// 상수 버퍼
struct ConstantBuffer
{
	Matrix mWorld;
	Matrix mView;
	Matrix mProjection;
};

DrawMeshApp::DrawMeshApp(HINSTANCE hInstance)
	: GameApp(hInstance)
{

}

DrawMeshApp::~DrawMeshApp()
{
	UninitImGUI();
}

bool DrawMeshApp::OnInitialize()
{
	if (!InitD3D())
		return false;

	if (!InitScene())
		return false;

	if (!InitImGUI())
		return false;

	ResetValues();

	return true;
}

void DrawMeshApp::OnUpdate()
{
	float t = GameTimer::m_Instance->TotalTime();

	bool useSimpleFunc = true;
	if (!useSimpleFunc)
	{
#pragma region Calc Matrix
	// 1st Cube: Rotate around the origin
	float fSinAngle1 = sin(t);
	float fCosAngle1 = cos(t);

	Matrix m_World1Transform =
	{
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	};

	Matrix m_World1Rotation =
	{
		1.0f * fCosAngle1, 0.0f, -fSinAngle1      , 0.0f,
		0.0f             , 1.0f, 0.0f             , 0.0f,
		fSinAngle1       , 0.0f, 1.0f * fCosAngle1, 0.0f,
		0.0f            , 0.0f, 0.0f              , 1.0f
	};

	Vector3 scale1 = { 1.0f,1.0f,1.0f };
	Matrix m_World1Scale =
	{
		scale1.x, 0.0f    , 0.0f    , 0.0f,
		0.0f    , scale1.y, 0.0f    , 0.0f,
		0.0f    , 0.0f    , scale1.z, 0.0f,
		0.0f    , 0.0f    , 0.0f    , 1.0f
	};

	//s->r->t
	Matrix m_World1FinalMatrix = m_World1Scale * m_World1Rotation * m_World1Transform;
	m_World1 = m_World1FinalMatrix;

	// 2nd Cube: Rotate around 1st cube and rotate self
	float speedScale = -5.0f;
	float fCosAngle2 = cos(t * speedScale);
	float fSinAngle2 = sin(t * speedScale);

	Matrix m_World2Transform =
	{
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
	   -4.0f, 0.0f, 0.0f, 1.0f
	};

	Matrix m_World2Rotation =
	{
		1.0f * fCosAngle2, 0.0f, -fSinAngle2      , 0.0f,
		0.0f             , 1.0f, 0.0f             , 0.0f,
		fSinAngle2       , 0.0f, 1.0f * fCosAngle2, 0.0f,
		0.0f            , 0.0f, 0.0f              , 1.0f
	};

	Vector3 scale2 = { 0.9f,0.9f,0.9f };
	Matrix m_World2Scale =
	{
		scale2.x, 0.0f    , 0.0f    , 0.0f,
		0.0f    , scale2.y, 0.0f    , 0.0f,
		0.0f    , 0.0f    , scale2.z, 0.0f,
		0.0f    , 0.0f    , 0.0f    , 1.0f
	};

	Matrix m_World2FinalMatrix = m_World2Scale * m_World2Rotation * m_World2Transform;
	m_World2 = m_World2FinalMatrix * m_World1;

	// 3nd Cube: Rotate around 2nd cube
	float fCosAngle3 = cos(t);
	float fSinAngle3 = sin(t);

	Matrix m_World3Transform =
	{
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		-10.0f, 2.0f, 0.0f, 1.0f
	};

	Matrix m_World3Rotation =
	{
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	};

	Vector3 scale3 = { 0.7f,0.7f,0.7f };
	Matrix m_World3Scale =
	{
		scale3.x, 0.0f    , 0.0f    , 0.0f,
		0.0f    , scale3.y, 0.0f    , 0.0f,
		0.0f    , 0.0f    , scale3.z, 0.0f,
		0.0f    , 0.0f    , 0.0f    , 1.0f
	};

	Matrix m_World3FinalMatrix = m_World3Scale * m_World3Rotation * m_World3Transform;
	m_World3 = m_World3FinalMatrix * m_World2;
#pragma endregion
	}
	else
	{
		m_World1 = m_World1.CreateRotationY(t) * m_World1.CreateTranslation(m_World1Position);
		m_World2 = m_World2.CreateScale(0.8f) * m_World2.CreateRotationY(t * 2.4f) * m_World2.CreateTranslation(m_World2Position) * m_World1;
		m_World3 = m_World3.CreateScale(1.2f) * m_World3.CreateTranslation(m_World3Position) * m_World2;
	}

	// Camera
	m_Camera.GetCameraViewMatrix(m_View);
}

void DrawMeshApp::OnRender()
{
#if USE_FLIPMODE == 1
	// Flip 모드에서는 매프레임 설정해야한다.
	m_pDeviceContext->OMSetRenderTargets(1, m_pRenderTargetView.GetAddressOf(), m_pDepthStencilView.Get()); // depthStencilView 사용

	// m_pDeviceContext->OMSetRenderTargets(1, m_pRenderTargetView.GetAddressOf(), NULL); // depthStencilView X
#endif	

	Color color(0.1f, 0.2f, 0.3f, 1.0f);

	// 화면 칠하기.
	m_pDeviceContext->ClearRenderTargetView(m_pRenderTargetView.Get(), color);
	m_pDeviceContext->ClearDepthStencilView(m_pDepthStencilView.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0); // 뎁스버퍼 1.0f로 초기화.

	m_pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_pDeviceContext->IASetVertexBuffers(0, 1, m_pVertexBuffer.GetAddressOf(), &m_VertexBufferStride, &m_VertexBufferOffset);
	m_pDeviceContext->IASetInputLayout(m_pInputLayout.Get());
	m_pDeviceContext->IASetIndexBuffer(m_pIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
	m_pDeviceContext->VSSetShader(m_pVertexShader.Get(), nullptr, 0);
	m_pDeviceContext->VSSetConstantBuffers(0, 1, m_pConstantBuffer.GetAddressOf());
	m_pDeviceContext->PSSetShader(m_pPixelShader.Get(), nullptr, 0);

	// Update variables for the first cube
	ConstantBuffer cb1;
	cb1.mWorld = XMMatrixTranspose(m_World1);
	cb1.mView = XMMatrixTranspose(m_View);
	cb1.mProjection = XMMatrixTranspose(m_Projection);
	m_pDeviceContext->UpdateSubresource(m_pConstantBuffer.Get(), 0, nullptr, &cb1, 0, 0);

	m_pDeviceContext->DrawIndexed(m_nIndices, 0, 0);

	// Update variables for the second cube	
	ConstantBuffer cb2;
	cb2.mWorld = XMMatrixTranspose(m_World2);
	cb2.mView = XMMatrixTranspose(m_View);
	cb2.mProjection = XMMatrixTranspose(m_Projection);
	m_pDeviceContext->UpdateSubresource(m_pConstantBuffer.Get(), 0, nullptr, &cb2, 0, 0);

	m_pDeviceContext->DrawIndexed(m_nIndices, 0, 0);

	// Update variables for the third cube	
	ConstantBuffer cb3;
	cb3.mWorld = XMMatrixTranspose(m_World3);
	cb3.mView = XMMatrixTranspose(m_View);
	cb3.mProjection = XMMatrixTranspose(m_Projection);
	m_pDeviceContext->UpdateSubresource(m_pConstantBuffer.Get(), 0, nullptr, &cb3, 0, 0);

	m_pDeviceContext->DrawIndexed(m_nIndices, 0, 0);

	// Render ImGui
	RenderImGUI();

	// 스왑체인 교체
	m_pSwapChain->Present(0, 0);
}

bool DrawMeshApp::InitImGUI()
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

void DrawMeshApp::RenderImGUI()
{
	// Start the Dear ImGui frame
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	// 월드 오브젝트 조종 창 만들기
	ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Once);		// 처음 실행될 때 위치 초기화
	ImGui::SetNextWindowSize(ImVec2(300, 500), ImGuiCond_Once);		// 처음 실행될 때 창 크기 초기화
	ImGui::Begin("World Object Controller");

	// 큐브 위치 조절 
	ImGui::DragFloat3("Cube1 Position", &m_World1Position.x);
	ImGui::DragFloat3("Cube2 Position", &m_World2Position.x);
	ImGui::DragFloat3("Cube3 Position", &m_World3Position.x);
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

	// Near 값 설정
	ImGui::DragFloat("Near", &m_Near);
	if (ImGui::Button("Set Near"))
	{
		m_Projection = XMMatrixPerspectiveFovLH(XM_PIDIV2, m_ClientWidth / (FLOAT)m_ClientHeight, m_Near, m_Far);
	}

	// Far 값 설정
	ImGui::DragFloat("Far", &m_Far);
	if (ImGui::Button("Set Far"))
	{
		m_Projection = XMMatrixPerspectiveFovLH(XM_PIDIV2, m_ClientWidth / (FLOAT)m_ClientHeight, m_Near, m_Far);
	}

	// 리셋 버튼
	if (ImGui::Button("Reset", { 50, 20 }))
	{
		ResetValues();
	}
	ImGui::End();

	// 각 큐브 매트릭스 정보 창 만들기
	ImGui::SetNextWindowPos(ImVec2(m_ClientWidth - 300, 0.0f), ImGuiCond_Once);	// 처음 실행될 때 위치 초기화
	ImGui::SetNextWindowSize(ImVec2(300, 500), ImGuiCond_Once);		// 처음 실행될 때 창 크기 초기화
	ImGui::Begin("Info");
	ImGui::Text("Matrix1");

	std::string mat = std::to_string(m_World1._11) + " " + std::to_string(m_World1._12) + " " + std::to_string(m_World1._13) + " " + std::to_string(m_World1._14) + "\n" +
		std::to_string(m_World1._21) + " " + std::to_string(m_World1._22) + " " + std::to_string(m_World1._23) + " " + std::to_string(m_World1._24) + "\n" +
		std::to_string(m_World1._31) + " " + std::to_string(m_World1._32) + " " + std::to_string(m_World1._33) + " " + std::to_string(m_World1._34) + "\n" +
		std::to_string(m_World1._41) + " " + std::to_string(m_World1._42) + " " + std::to_string(m_World1._43) + " " + std::to_string(m_World1._44);

	ImGui::Text(mat.c_str());
	ImGui::NewLine();

	ImGui::Text("Matrix2");
	mat = std::to_string(m_World2._11) + " " + std::to_string(m_World2._12) + " " + std::to_string(m_World2._13) + " " + std::to_string(m_World2._14) + "\n" +
		std::to_string(m_World2._21) + " " + std::to_string(m_World2._22) + " " + std::to_string(m_World2._23) + " " + std::to_string(m_World2._24) + "\n" +
		std::to_string(m_World2._31) + " " + std::to_string(m_World2._32) + " " + std::to_string(m_World2._33) + " " + std::to_string(m_World2._34) + "\n" +
		std::to_string(m_World2._41) + " " + std::to_string(m_World2._42) + " " + std::to_string(m_World2._43) + " " + std::to_string(m_World2._44);

	ImGui::Text(mat.c_str());
	ImGui::NewLine();

	ImGui::Text("Matrix3");
	mat = std::to_string(m_World3._11) + " " + std::to_string(m_World3._12) + " " + std::to_string(m_World3._13) + " " + std::to_string(m_World3._14) + "\n" +
		std::to_string(m_World3._21) + " " + std::to_string(m_World3._22) + " " + std::to_string(m_World3._23) + " " + std::to_string(m_World3._24) + "\n" +
		std::to_string(m_World3._31) + " " + std::to_string(m_World3._32) + " " + std::to_string(m_World3._33) + " " + std::to_string(m_World3._34) + "\n" +
		std::to_string(m_World3._41) + " " + std::to_string(m_World3._42) + " " + std::to_string(m_World3._43) + " " + std::to_string(m_World3._44);

	ImGui::Text(mat.c_str());
	ImGui::NewLine();

	ImGui::End();

	// rendering
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void DrawMeshApp::UninitImGUI()
{
	// Cleanup
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

bool DrawMeshApp::InitD3D()
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

	// create depthStencil texture
	ComPtr<ID3D11Texture2D> pTextureDepthStencil;
	HR_T(m_pDevice->CreateTexture2D(&descDepth, nullptr, pTextureDepthStencil.GetAddressOf()));

	// create the depth stencil view
	D3D11_DEPTH_STENCIL_VIEW_DESC descDSV = {};
	descDSV.Format = descDepth.Format;
	descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D; // 사용되는 리소스 엑세스 방식 설정 : https://learn.microsoft.com/ko-kr/windows/win32/api/d3d11/ne-d3d11-d3d11_dsv_dimension 
	descDSV.Texture2D.MipSlice = 0;
	HR_T(m_pDevice->CreateDepthStencilView(pTextureDepthStencil.Get(), &descDSV, m_pDepthStencilView.GetAddressOf()));

	return true;
}

bool DrawMeshApp::InitScene()
{
	HRESULT hr = S_OK;

	///   0--------------1
	///  / |           / |
	/// /  |          /  |
	/// 3--+---------2   |
	/// |  |4--------|---/ 5
	/// | /          |  /
	/// |/           | /
	/// 7-------------+ 6


	// 1. 파이프라인에서 바인딩할 정점 버퍼 및 버퍼 정보 생성
	Vertex vertices[] = // Local space, color
	{
		{ Vector3(-1.0f, 1.0f, -1.0f),	Vector4(0.0f, 0.0f, 1.0f, 1.0f) },
		{ Vector3(1.0f, 1.0f, -1.0f),	Vector4(0.0f, 1.0f, 0.0f, 1.0f) },
		{ Vector3(1.0f, 1.0f, 1.0f),	Vector4(0.0f, 1.0f, 1.0f, 1.0f) },
		{ Vector3(-1.0f, 1.0f, 1.0f),	Vector4(1.0f, 0.0f, 0.0f, 1.0f) },
		{ Vector3(-1.0f, -1.0f, -1.0f), Vector4(1.0f, 0.0f, 1.0f, 1.0f) },
		{ Vector3(1.0f, -1.0f, -1.0f),	Vector4(1.0f, 1.0f, 0.0f, 1.0f) },
		{ Vector3(1.0f, -1.0f, 1.0f),	Vector4(1.0f, 1.0f, 1.0f, 1.0f) },
		{ Vector3(-1.0f, -1.0f, 1.0f),	Vector4(0.0f, 0.0f, 0.0f, 1.0f) },
	};

	D3D11_BUFFER_DESC bufferDesc = {};
	bufferDesc.ByteWidth = sizeof(Vertex) * ARRAYSIZE(vertices);
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA vbData = {};
	vbData.pSysMem = vertices;
	HR_T(m_pDevice->CreateBuffer(&bufferDesc, &vbData, m_pVertexBuffer.GetAddressOf()));

	m_VertexBufferStride = sizeof(Vertex); 	// 버텍스 버퍼의 정보
	m_VertexBufferOffset = 0;

	// 2. 파이프라인에 바인딩할 InputLayout 생성
	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	ComPtr<ID3DBlob> vertexShaderBuffer = nullptr;
	HR_T(CompileShaderFromFile(L"BasicVertexShader.hlsl", "main", "vs_4_0", &vertexShaderBuffer));
	HR_T(m_pDevice->CreateInputLayout(layout, ARRAYSIZE(layout), vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), m_pInputLayout.GetAddressOf()));

	// 3. 파이프 라인에 바인딩할 정점 셰이더 생성
	HR_T(m_pDevice->CreateVertexShader(vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), NULL, m_pVertexShader.GetAddressOf()));

	// 4. 파이프라인에 바인딩할 인덱스 버퍼
	WORD indices[] = // 사각형 두 개?
	{
		0,3,2, 2,1,0,
		4,5,7, 5,6,7,
		0,4,7, 7,3,0,
		6,5,1, 1,2,6,
		7,6,2, 2,3,7,
		5,4,0, 0,1,5
	};

	m_nIndices = ARRAYSIZE(indices); // 인덱스 개수 저장

	bufferDesc = {};
	bufferDesc.ByteWidth = sizeof(WORD) * ARRAYSIZE(indices);
	bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA ibData = {};
	ibData.pSysMem = indices;
	HR_T(m_pDevice->CreateBuffer(&bufferDesc, &ibData, m_pIndexBuffer.GetAddressOf()));

	// 5. 파이프라인에 바인딩할 픽셀 셰이더 생성
	ComPtr<ID3DBlob> pixelShaderBuffer = nullptr;
	HR_T(CompileShaderFromFile(L"BasicPixelShader.hlsl", "main", "ps_4_0", &pixelShaderBuffer));
	HR_T(m_pDevice->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, m_pPixelShader.GetAddressOf()));

	// 6. Render() 에서 파이프라인에 바인딩할 상수 버퍼 생성
	bufferDesc = {};
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(ConstantBuffer);
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	HR_T(m_pDevice->CreateBuffer(&bufferDesc, nullptr, m_pConstantBuffer.GetAddressOf()));

	// 쉐이더에 상수버퍼에 전달할 시스템 메모리 데이터 초기화
	m_World1 = XMMatrixIdentity();
	m_World2 = XMMatrixIdentity();

	XMVECTOR Eye = XMVectorSet(0.0f, 10.0f, -8.0f, 0.0f);
	XMVECTOR At = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
	XMVECTOR Up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	m_View = XMMatrixLookAtLH(Eye, At, Up);
	m_Projection = XMMatrixPerspectiveFovLH(XM_PIDIV2, m_ClientWidth / (FLOAT)m_ClientHeight, m_Near, m_Far);

	return true;
}

void DrawMeshApp::ResetValues()
{
	m_World1Position = m_World1PositionInitial;
	m_World2Position = m_World2PositionInitial;
	m_World3Position = m_World3PositionInitial;

	m_Near = 0.01f;
	m_Far = 100.0f;

	m_Projection = XMMatrixPerspectiveFovLH(XM_PIDIV2, m_ClientWidth / (FLOAT)m_ClientHeight, m_Near, m_Far);
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT DrawMeshApp::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
		return true;

	return __super::WndProc(hWnd, message, wParam, lParam);
}
