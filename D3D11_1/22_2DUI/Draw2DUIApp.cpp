#include "Draw2DUIApp.h"
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

#undef min
#undef max

using namespace DirectX::SimpleMath;
using namespace DirectX;
using Microsoft::WRL::ComPtr;

#define USE_FLIPMODE 1 // 경고 메세지를 없애려면 Flip 모드를 사용한다.

// 정점 
struct Vertex
{
	Vector3 position;
	Vector4 color;

	Vertex(Vector3 pos = Vector3::Zero, Vector4 color = Vector4::Zero) : position(pos), color(color) {}
};

// 상수 버퍼
struct ConstantBuffer
{
	Matrix world;
	Matrix view;
	Matrix projection;
};

Draw2DUIApp::Draw2DUIApp(HINSTANCE hInstance)
	: GameApp(hInstance)
{

}

Draw2DUIApp::~Draw2DUIApp()
{
	if(m_D2DDeviceContext)
		m_D2DDeviceContext->SetTarget(nullptr);
	UninitImGUI();
}

bool Draw2DUIApp::OnInitialize()
{
	if (!InitD3D())
		return false;

	if (!InitScene())
		return false;

	if (!InitImGUI())
		return false;

	InitD2D();
	ResetValues();
	CreateStates();
	CreateD2DEffect();

	return true;
}

void Draw2DUIApp::OnUpdate()
{
	float delta = GameTimer::m_Instance->DeltaTime();

	{
		Matrix scale = Matrix::Identity;
		Matrix rotate = Matrix::Identity;
		Matrix position = Matrix::Identity;

		// m_world1
		m_World1Rotation.y += delta;

		position = m_World1.CreateTranslation(m_World1Position);
		rotate = m_World1.CreateFromYawPitchRoll(m_World1Rotation);
		scale = m_World1.CreateScale(m_World1Scale);

		m_World1 = scale * rotate * position;

		// m_world2
		float rotSpeedScale = 1.5f;
		m_World2Rotation.y += delta * rotSpeedScale;

		position = m_World2.CreateTranslation(m_World2Position);
		rotate = m_World2.CreateFromYawPitchRoll(m_World2Rotation);
		scale = m_World2.CreateScale(m_World2Scale);
		m_World2 = scale * rotate * position * m_World1;

		// m_world3	
		position = m_World3.CreateTranslation(m_World3Position);
		rotate = m_World3.CreateFromYawPitchRoll(m_World3Rotation);
		scale = m_World3.CreateScale(m_World3Scale);
		m_World3 = scale * rotate * position * m_World2;
	}

	// Camera
	m_Camera.GetCameraViewMatrix(m_View);
}

void Draw2DUIApp::OnRender()
{
#if USE_FLIPMODE == 1
	// Flip 모드에서는 매프레임 설정해야한다.
	m_pDeviceContext->OMSetRenderTargets(1, m_pRenderTargetView.GetAddressOf(), m_pDepthStencilView.Get()); // depthStencilView 사용

	// m_pDeviceContext->OMSetRenderTargets(1, m_pRenderTargetView.GetAddressOf(), NULL); // depthStencilView X
#endif	

	Color color(0.0f, 0.0f, 0.0f, 0.0f);

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
	cb1.world = XMMatrixTranspose(m_World1);
	cb1.view = XMMatrixTranspose(m_View);
	cb1.projection = XMMatrixTranspose(m_Projection);
	m_pDeviceContext->UpdateSubresource(m_pConstantBuffer.Get(), 0, nullptr, &cb1, 0, 0);

	m_pDeviceContext->DrawIndexed(m_nIndices, 0, 0);

	// Update variables for the second cube	
	cb1.world = XMMatrixTranspose(m_World2);
	cb1.view = XMMatrixTranspose(m_View);
	cb1.projection = XMMatrixTranspose(m_Projection);
	m_pDeviceContext->UpdateSubresource(m_pConstantBuffer.Get(), 0, nullptr, &cb1, 0, 0);

	m_pDeviceContext->DrawIndexed(m_nIndices, 0, 0);

	// Update variables for the third cube	
	cb1.world = XMMatrixTranspose(m_World3);
	cb1.view = XMMatrixTranspose(m_View);
	cb1.projection = XMMatrixTranspose(m_Projection);
	m_pDeviceContext->UpdateSubresource(m_pConstantBuffer.Get(), 0, nullptr, &cb1, 0, 0);

	RenderText(L"Hello World");

	// Render ImGui
	RenderImGUI();

	// 스왑체인 교체
	m_pSwapChain->Present(0, 0);
}

bool Draw2DUIApp::InitImGUI()
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

void Draw2DUIApp::RenderImGUI()
{
	// Start the Dear ImGui frame
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	// 월드 오브젝트 조종 창 만들기
	ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Once);		// 처음 실행될 때 위치 초기화
	ImGui::SetNextWindowSize(ImVec2(350, 300), ImGuiCond_Once);		// 처음 실행될 때 창 크기 초기화
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
	ImGui::DragFloat("Near", &m_Near, 0.5f);

	// Far 값 설정
	ImGui::DragFloat("Far", &m_Far, 0.5f);

	// Fov 값 설정
	ImGui::DragFloat("Fov Angle", &m_PovAngle, 0.02f);

	if (m_Near <= 0.0f) m_Near = 0.01f;
	if (m_Far <= 0.0f) m_Far = 0.2f;

	m_Projection = XMMatrixPerspectiveFovLH(m_PovAngle, m_ClientWidth / (FLOAT)m_ClientHeight, m_Near, m_Far);

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

void Draw2DUIApp::UninitImGUI()
{
	// Cleanup
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

bool Draw2DUIApp::InitD3D()
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
	HR_T(m_pSwapChain->GetBuffer(
		0, IID_PPV_ARGS(pBackBufferTexture.ReleaseAndGetAddressOf())));
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

	ComPtr<ID3D11DepthStencilState> depthStencilState;
	m_pDevice->CreateDepthStencilState(&depthStencilDesc, depthStencilState.GetAddressOf());
	m_pDeviceContext->OMSetDepthStencilState(depthStencilState.Get(), 1);

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

bool Draw2DUIApp::InitScene()
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
	HR_T(CompileShaderFromFile(L"Shaders\\BasicVertexShader.hlsl", "main", "vs_4_0", &vertexShaderBuffer));
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
	HR_T(CompileShaderFromFile(L"Shaders\\BasicPixelShader.hlsl", "main", "ps_4_0", &pixelShaderBuffer));
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
	m_Projection = XMMatrixPerspectiveFovLH(m_PovAngle, m_ClientWidth / (FLOAT)m_ClientHeight, m_Near, m_Far);

	return true;
}

void Draw2DUIApp::ResetValues()
{
	m_World1Position = m_World1PositionInitial;
	m_World2Position = m_World2PositionInitial;
	m_World3Position = m_World3PositionInitial;

	m_Near = 0.01f;
	m_Far = 100.0f;
	m_PovAngle = XM_PIDIV2;

	m_CameraRotation = Vector3::Zero;
	m_Camera.SetRotation(m_CameraRotation);
	m_Camera.SetPosition(m_CameraPositionInitial);

	m_Projection = XMMatrixPerspectiveFovLH(m_PovAngle, m_ClientWidth / (FLOAT)m_ClientHeight, m_Near, m_Far);
}

void Draw2DUIApp::InitD2D()
{
	D3D11_TEXTURE2D_DESC tex2Ddesc{};
	tex2Ddesc.Width = m_ClientWidth;
	tex2Ddesc.Height = m_ClientHeight;
	tex2Ddesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	tex2Ddesc.MipLevels = 1;
	tex2Ddesc.ArraySize = 1;
	tex2Ddesc.SampleDesc.Count = 1;
	tex2Ddesc.Usage = D3D11_USAGE_DEFAULT;
	tex2Ddesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;

	HR_T(m_pDevice->CreateTexture2D(&tex2Ddesc, nullptr, m_SharedTex11.GetAddressOf()));

	// options
	D2D1_FACTORY_OPTIONS options{};
#ifdef _DEBUG
	options.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
#endif
	HR_T(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED,
		IID_PPV_ARGS(m_D2DFactory.ReleaseAndGetAddressOf())));

	ComPtr<IDXGIDevice> dxgiDevice;
	HR_T(m_pDevice.As(&dxgiDevice));

	HR_T(m_D2DFactory->CreateDevice(dxgiDevice.Get(), m_D2DDevice.GetAddressOf()));

	HR_T(m_D2DDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, m_D2DDeviceContext.GetAddressOf()));

	ComPtr<IDXGISurface> surface;
	HR_T(m_SharedTex11.As(&surface)); // Comptr은 QueryInterface 대신 .As() 사용하는게 정석

	D2D1_BITMAP_PROPERTIES1 bitmapProps{};
	bitmapProps.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;		// 비트맵 만들 dxgi 형식
	bitmapProps.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;	// 알파를 미리 곱해진 값으로 처리하지 않는다? 
	bitmapProps.bitmapOptions = D2D1_BITMAP_OPTIONS_TARGET;

	// 비트맵의 물리적 픽셀을 어떻게 논리적 스크린 좌표에 넣을지? map할지
	// default 값은 96.0f
	bitmapProps.dpiX = 96.0f;	// x 방향 비트맵 dpi (dots-per-inch)
	bitmapProps.dpiY = 96.0f;	// y 방향 비트맵 dpi

	HR_T(m_D2DDeviceContext->CreateBitmapFromDxgiSurface(surface.Get(), 
		&bitmapProps, 
		m_D2DTargetBitmap.GetAddressOf())); // 타겟 비트맵 만들기

	// 이미지를 줄 비트맵 적용?
	m_D2DDeviceContext->SetTarget(m_D2DTargetBitmap.Get());
	HR_T(m_D2DDeviceContext->CreateSolidColorBrush(D2D1::ColorF(1, 1, 0, 1), &m_Brush));
	HR_T(DWriteCreateFactory(
		DWRITE_FACTORY_TYPE_SHARED,
		__uuidof(IDWriteFactory),
		reinterpret_cast<IUnknown**>(m_DWriteFactory.ReleaseAndGetAddressOf())));

	// Text 포맷 설정
	HR_T(m_DWriteFactory->CreateTextFormat(L"Script",
		nullptr,
		DWRITE_FONT_WEIGHT_REGULAR,
		DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL,
		24.0f,
		L"en-us",
		m_TextFormat.ReleaseAndGetAddressOf())
	);

	m_TextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
	m_TextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);

	// 이미지 생성하기
	m_TestBitmap = CreateD2DBitmapFromFile(L"..\\Resource\\neruThumpUp.png");

	// 상수 버퍼
	D3D11_BUFFER_DESC bufferDesc;
	bufferDesc = {};
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(PerObjectCB);
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	HR_T(m_pDevice->CreateBuffer(&bufferDesc, nullptr, m_perObjCB.GetAddressOf()));

	// 정점 버퍼 만들기
	// D2D screen quad + SRV
	// Quad in clip space (WVP = Identity)
	CubeVertex v[] = {
		{ Vector3(-1.0f, -1.0f, 0.0f),	Vector2(0.0f, 1.0f) },
		{ Vector3(-1.0f,  1.0f, 0.0f),	Vector2(0.0f, 0.0f) },
		{ Vector3(1.0f,  1.0f, 0.0f),	Vector2(1.0f, 0.0f) },
		{ Vector3(1.0f, -1.0f, 0.0f),	Vector2(1.0f, 1.0f) },
	};

	D3D11_BUFFER_DESC d2dVertexBufferDesc = {};
	d2dVertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	d2dVertexBufferDesc.ByteWidth = sizeof(CubeVertex) * 4;
	d2dVertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA d2dVinit = {};
	d2dVinit.pSysMem = v;
	HR_T(m_pDevice->CreateBuffer(&d2dVertexBufferDesc, &d2dVinit, &m_D2DVertBuffer));

	DWORD indices[] = { 0, 1, 2, 0, 2, 3 };

	D3D11_BUFFER_DESC d2dIndexBufferDesc = {};
	d2dIndexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	d2dIndexBufferDesc.ByteWidth = sizeof(DWORD) * 6;
	d2dIndexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

	D3D11_SUBRESOURCE_DATA d2dIinit = {};
	d2dIinit.pSysMem = indices;
	HR_T(m_pDevice->CreateBuffer(&d2dIndexBufferDesc, &d2dIinit, &m_D2DIndexBuffer));

	HR_T(m_pDevice->CreateShaderResourceView(m_SharedTex11.Get(), nullptr, m_D2DTexture.ReleaseAndGetAddressOf()));
}

void Draw2DUIApp::CreateD2DEffect()
{
	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	ComPtr<ID3DBlob> vsbuffer{};
	HR_T(CompileShaderFromFile(L"Shaders\\VS_2D.hlsl", "main", "vs_5_0", &vsbuffer));
	HR_T(m_pDevice->CreateInputLayout(layout, ARRAYSIZE(layout), vsbuffer->GetBufferPointer(), vsbuffer->GetBufferSize(), m_D2DInputLayaout.GetAddressOf()));

	HR_T(m_pDevice->CreateVertexShader(vsbuffer->GetBufferPointer(), vsbuffer->GetBufferSize(), NULL, m_d2dVertexShader.GetAddressOf()));

	ComPtr<ID3DBlob> psbuffer{};
	HR_T(CompileShaderFromFile(L"Shaders\\PS_2D.hlsl", "main", "ps_5_0", &psbuffer));
	HR_T(m_pDevice->CreatePixelShader(psbuffer->GetBufferPointer(), psbuffer->GetBufferSize(), NULL, m_d2dPixelShader.GetAddressOf()));
}

ComPtr<ID2D1Bitmap1> Draw2DUIApp::CreateD2DBitmapFromFile(std::wstring path) const
{
	ComPtr<ID2D1Bitmap1> outBitmap{};

	if (!m_D2DDeviceContext) return outBitmap;


	ComPtr<IWICImagingFactory>		wicFactory{};	// 
	ComPtr<IWICBitmapDecoder>		decoder{};
	ComPtr<IWICBitmapFrameDecode>	frame{};
	ComPtr<IWICFormatConverter>		converter{};

	HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory2, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(wicFactory.ReleaseAndGetAddressOf()));
	if (FAILED(hr))
	{
		HR_T(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(wicFactory.ReleaseAndGetAddressOf())));
	}

	// 디코더 생성
	HR_T(wicFactory->CreateDecoderFromFilename(path.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad, decoder.ReleaseAndGetAddressOf()));

	// 첫 프레임 획득
	HR_T(decoder->GetFrame(0, frame.ReleaseAndGetAddressOf()));

	// 포맷 변환기 생성
	HR_T(wicFactory->CreateFormatConverter(converter.ReleaseAndGetAddressOf()));

	// GUID_WICPixelFormat32bppPBGRA로 변환
	HR_T(converter->Initialize(
		frame.Get(),
		GUID_WICPixelFormat32bppPBGRA,
		WICBitmapDitherTypeNone,
		nullptr,
		0.0,
		WICBitmapPaletteTypeMedianCut));

	// 문제 회피용 명시적 props 선언
	D2D1_BITMAP_PROPERTIES1 props = D2D1::BitmapProperties1(
		D2D1_BITMAP_OPTIONS_NONE,
		D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
	);

	// NOTE : ComPtrfh
	HR_T(m_D2DDeviceContext->CreateBitmapFromWicBitmap(converter.Get(), &props, outBitmap.ReleaseAndGetAddressOf()));

	return outBitmap;
}

void Draw2DUIApp::CreateStates()
{
	// Blend state for overlay
	D3D11_BLEND_DESC blendDesc = {};
	D3D11_RENDER_TARGET_BLEND_DESC rtbd = {};
	rtbd.BlendEnable = true;
	rtbd.SrcBlend = D3D11_BLEND_ONE;
	rtbd.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	rtbd.BlendOp = D3D11_BLEND_OP_ADD;
	rtbd.SrcBlendAlpha = D3D11_BLEND_ONE;
	rtbd.DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
	rtbd.BlendOpAlpha = D3D11_BLEND_OP_ADD;
	rtbd.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	blendDesc.RenderTarget[0] = rtbd;

	HR_T(m_pDevice->CreateBlendState(&blendDesc, m_TransparencyBS.GetAddressOf()));

	// Rasterizer
	D3D11_RASTERIZER_DESC cmdesc = {};
	cmdesc.FillMode = D3D11_FILL_SOLID;
	cmdesc.CullMode = D3D11_CULL_BACK;
	cmdesc.FrontCounterClockwise = false;
	HR_T(m_pDevice->CreateRasterizerState(&cmdesc, m_CWcullModeRS.GetAddressOf()));

	D3D11_SAMPLER_DESC sampDesc = {};
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	HR_T(m_pDevice->CreateSamplerState(&sampDesc, m_SamplerState.GetAddressOf()));
}

void Draw2DUIApp::RenderText(std::wstring str)
{
	ID3D11ShaderResourceView* nullSRV = nullptr;
	m_pDeviceContext->PSSetShaderResources(0, 1, &nullSRV);

	m_D2DDeviceContext->BeginDraw();
	m_D2DDeviceContext->Clear(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f));

	if (m_TestBitmap)
	{
		D2D1_SIZE_F size = m_TestBitmap->GetSize();
		const float maxDim = 128.0f;
		const float sx = (size.width > 0.0f) ? (maxDim / size.width) : 1.0f;
		const float sy = (size.height > 0.0f) ? (maxDim / size.height) : 1.0f;
		const float scale = std::min(1.0f, std::min(sx, sy));
		const float drawW = std::min(size.width * scale, (float)m_ClientWidth);
		const float drawH = std::min(size.height * scale, (float)m_ClientHeight);
		D2D1_RECT_F dest = D2D1::RectF(10.0f, 10.0f, 10.0f + drawW, 10.0f + drawH);
		m_D2DDeviceContext->DrawBitmap(m_TestBitmap.Get(), dest, 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
	}

	m_Brush->SetColor(D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f));
	D2D1_RECT_F layoutRect = D2D1::RectF(0, 0, (FLOAT)m_ClientWidth, (FLOAT)m_ClientHeight);

	m_D2DDeviceContext->DrawText(
		str.c_str(),
		static_cast<UINT32>(str.length()),
		m_TextFormat.Get(),
		layoutRect,
		m_Brush.Get());

	HR_T(m_D2DDeviceContext->EndDraw()); // 반드시 체크하기

	m_pDeviceContext->OMSetBlendState(m_TransparencyBS.Get(), nullptr, 0xffffffff);	//

	m_pDeviceContext->IASetInputLayout(m_D2DInputLayaout.Get());
	m_pDeviceContext->IASetIndexBuffer(m_D2DIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
	UINT stride = sizeof(CubeVertex);
	UINT offset = 0;
	m_pDeviceContext->IASetVertexBuffers(0, 1, m_D2DVertBuffer.GetAddressOf(), &stride, &offset);

	Matrix mat = XMMatrixIdentity();
	m_perObjData.WVP = XMMatrixTranspose(mat);
	m_pDeviceContext->UpdateSubresource(m_perObjCB.Get(), 0, nullptr, &m_perObjData, 0, 0);
	m_pDeviceContext->VSSetConstantBuffers(1, 1, m_perObjCB.GetAddressOf());
	m_pDeviceContext->PSSetShaderResources(10, 1, m_D2DTexture.GetAddressOf()); // 
	m_pDeviceContext->PSSetSamplers(0, 1, m_SamplerState.GetAddressOf());		// 
	m_pDeviceContext->RSSetState(m_CWcullModeRS.Get());						//

	m_pDeviceContext->VSSetShader(m_d2dVertexShader.Get(), nullptr, 0);
	m_pDeviceContext->PSSetShader(m_d2dPixelShader.Get(), nullptr, 0);

	m_pDeviceContext->DrawIndexed(6, 0, 0);

	// Restore opaque blend state for next frame
	m_pDeviceContext->OMSetBlendState(nullptr, nullptr, 0xffffffff);
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT Draw2DUIApp::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
		return true;

	return __super::WndProc(hWnd, message, wParam, lParam);
}
