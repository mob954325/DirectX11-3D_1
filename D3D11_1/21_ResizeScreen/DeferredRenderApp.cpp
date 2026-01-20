#include "DeferredRenderApp.h"
#include "../Common/Helper.h"

#include <directxtk/SimpleMath.h>
#include <dxgidebug.h>
#include <dxgi1_4.h>
#include <dxgi1_6.h>
#include <algorithm>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "d3dcompiler.lib")

#include <string>
#include <directxtk/DDSTextureLoader.h>

using namespace DirectX::SimpleMath;
using namespace DirectX;
using Microsoft::WRL::ComPtr;

#define USE_FLIPMODE 1 // 경고 메세지를 없애려면 Flip 모드를 사용한다.

// 상수 버퍼
struct ConstantBuffer
{
	// camera
	Matrix cameraView;
	Matrix cameraProjection;

	// 
	Matrix shadowView;
	Matrix shadowProjection;

	Vector4 ambient;	// 환경광
	Vector4 diffuse;	// 난반사
	Vector4 specular;	// 정반사
	FLOAT shininess;	// 광택지수
	Vector3 CameraPos;	// 카메라 위치

	// PBR
	FLOAT metalness;	//  
	FLOAT roughness;	//
	FLOAT lightIntensity;
	FLOAT pad1;

	Color baseColor;
};

struct LightDirectionCB
{
	Vector4 lightDirection;
	Color lightColor;
};

struct CubeVertex
{
	Vector3 position;
};

struct QuadVertex
{
	Vector3 position;
	Vector2 uv;

	QuadVertex(float x, float y, float z, float u, float v) : position(x, y, z), uv(u, v) {}
	QuadVertex(Vector3 p, Vector2 u) : position(p), uv(u) {}
};

std::string WStringToUTF8(const std::wstring& wstr)
{
	int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(),
		(int)wstr.size(),
		nullptr, 0, nullptr, nullptr);

	std::string result(sizeNeeded, 0);

	WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(),
		(int)wstr.size(),
		&result[0], sizeNeeded,
		nullptr, nullptr);

	return result;
}

DeferredRenderApp::DeferredRenderApp(HINSTANCE hInstance)
	: GameApp(hInstance)
{
	gbufferCount = static_cast<int>(EGbuffer::Count);
}

DeferredRenderApp::~DeferredRenderApp()
{
	UninitImGUI();
}

void DeferredRenderApp::InitDebugDraw()
{
	m_states = std::make_unique<CommonStates>(m_device.Get());
	m_batch = std::make_unique<PrimitiveBatch<VertexPositionColor>>(m_deviceContext.Get());

	m_effect = std::make_unique<BasicEffect>(m_device.Get());
	m_effect->SetVertexColorEnabled(true);
	m_effect->SetView(m_shadowView);
	m_effect->SetProjection(m_shadowProj);

	{
		void const* shaderByteCode;
		size_t byteCodeLength;

		m_effect->GetVertexShaderBytecode(&shaderByteCode, &byteCodeLength); // ??

		// https://github.com/Microsoft/DirectXTK/wiki/throwIfFailed -> 이거 문서에 있던건데 이거 결국 HRESULT 값을 반환한다는 소리임
		HR_T(m_device->CreateInputLayout(
			VertexPositionColor::InputElements, VertexPositionColor::InputElementCount,
			shaderByteCode, byteCodeLength,
			m_debugDrawInputLayout.ReleaseAndGetAddressOf()));
	}
}

void DeferredRenderApp::InitShdowMap()
{
	// create shadow map texure desc
	D3D11_TEXTURE2D_DESC texDesc = {}; // https://learn.microsoft.com/ko-kr/windows/win32/api/d3d11/ns-d3d11-d3d11_texture2d_desc
	texDesc.Width = (UINT)m_shadowViewport.width;
	texDesc.Height = (UINT)m_shadowViewport.height;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	texDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	HR_T(m_device->CreateTexture2D(&texDesc, nullptr, m_shadowMap.GetAddressOf()));

	// DSV
	D3D11_DEPTH_STENCIL_VIEW_DESC descDSV = {};
	descDSV.Format = DXGI_FORMAT_D32_FLOAT;
	descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	HR_T(m_device->CreateDepthStencilView(m_shadowMap.Get(), &descDSV, m_shadowMapDSV.GetAddressOf()));

	// SRV
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	HR_T(m_device->CreateShaderResourceView(m_shadowMap.Get(), &srvDesc, m_shadowMapSRV.GetAddressOf()));

	// 빛 계산 ( pov )
	m_shadowProj = XMMatrixPerspectiveFovLH(m_shadowFrustumAngle, m_shadowViewport.width / (FLOAT)m_shadowViewport.height, m_shadowNear, m_shadowFar); // 그림자 절두체
	m_shadowLookAt = m_Camera.m_Position + m_Camera.GetForward() * m_shadowForwardDistFromCamera;	// 바라보는 방향 = 카메라 위치 + 카메라 바라보는 방향으로부터 떨어진 태양의 위치
	m_shadowPos = m_Camera.m_Position + ((Vector3)-m_LightDirection * m_shadowUpDistFromLookAt);	// 위치
	m_shadowView = XMMatrixLookAtLH(m_shadowPos, m_shadowLookAt, Vector3(0.0f, 1.0f, 0.0f));
}

bool DeferredRenderApp::InitSkyBox()
{
	const float width = 1.0f;
	const float height = 1.0f;
	const float depth = 1.0f;
	CubeVertex skyboxVertices[] =
	{
		{ Vector3(-width, -height, -depth) },
		{ Vector3(-width, +height, -depth) },
		{ Vector3(+width, +height, -depth) },
		{ Vector3(+width, -height, -depth) },

		{ Vector3(-width, -height, +depth) },
		{ Vector3(+width, -height, +depth) },
		{ Vector3(+width, +height, +depth) },
		{ Vector3(-width, +height, +depth) },

		{ Vector3(-width, +height, -depth) },
		{ Vector3(-width, +height, +depth) },
		{ Vector3(+width, +height, +depth) },
		{ Vector3(+width, +height, -depth) },

		{ Vector3(-width, -height, -depth) },
		{ Vector3(+width, -height, -depth) },
		{ Vector3(+width, -height, +depth) },
		{ Vector3(-width, -height, +depth) },

		{ Vector3(-width, -height, +depth) },
		{ Vector3(-width, +height, +depth) },
		{ Vector3(-width, +height, -depth) },
		{ Vector3(-width, -height, -depth) },

		{ Vector3(+width, -height, -depth) },
		{ Vector3(+width, +height, -depth) },
		{ Vector3(+width, +height, +depth) },
		{ Vector3(+width, -height, +depth) }
	};

	// 버텍스 버퍼
	D3D11_BUFFER_DESC bufferDesc = {};
	bufferDesc.ByteWidth = sizeof(CubeVertex) * ARRAYSIZE(skyboxVertices);
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA vbData = {};
	vbData.pSysMem = skyboxVertices;
	HR_T(m_device->CreateBuffer(&bufferDesc, &vbData, m_skyboxVertexBuffer.GetAddressOf()));

	m_SkyboxVertexBufferStride = sizeof(CubeVertex); 	// 버텍스 버퍼의 정보
	m_SkyboxVertexBufferOffset = 0;

	// 파이프라인에 바인딩할 InputLayout 생성
	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	ComPtr<ID3DBlob> vertexShaderBuffer = nullptr;
	HR_T(CompileShaderFromFile(L"Shaders\\VS_Skybox.hlsl", "main", "vs_5_0", &vertexShaderBuffer));
	HR_T(m_device->CreateInputLayout(layout, ARRAYSIZE(layout), vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), m_skyboxInputLayout.GetAddressOf()));

	// 파이프 라인에 바인딩할 정점 셰이더 생성
	HR_T(m_device->CreateVertexShader(vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), NULL, m_skyboxVS.GetAddressOf()));

	// 인덱스 버퍼
	WORD skyboxIndices[] =
	{
		0, 1, 2,
		0, 2, 3,
		4, 5, 6,
		4, 6, 7,
		8, 9, 10,
		8, 10, 11,
		12, 13, 14,
		12, 14, 15,
		16, 17, 18,
		16, 18, 19,
		20, 21, 22,
		20, 22, 23,
	};

	bufferDesc.ByteWidth = sizeof(WORD) * ARRAYSIZE(skyboxIndices);
	bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.CPUAccessFlags = 0;

	m_SkyboxVertexBufferStride = sizeof(CubeVertex); 	// 버텍스 버퍼의 정보
	m_SkyboxVertexBufferOffset = 0;

	m_nSkyboxIndices = ARRAYSIZE(skyboxIndices); // 인덱스 개수 저장

	D3D11_SUBRESOURCE_DATA ibData = {};
	ibData.pSysMem = skyboxIndices;
	HR_T(m_device->CreateBuffer(&bufferDesc, &ibData, m_skyboxIndexBuffer.GetAddressOf()));

	// 픽셀 셰이더
	ComPtr<ID3DBlob> sbPixelShaderBuffer = nullptr;
	HR_T(CompileShaderFromFile(L"Shaders\\PS_Skybox.hlsl", "main", "ps_5_0", &sbPixelShaderBuffer));
	HR_T(m_device->CreatePixelShader(sbPixelShaderBuffer->GetBufferPointer(), sbPixelShaderBuffer->GetBufferSize(), NULL, m_skyboxPS.GetAddressOf()));

	// 래스터라이저
	D3D11_RASTERIZER_DESC rasterizerState = {};
	rasterizerState.CullMode = D3D11_CULL_BACK;
	rasterizerState.FillMode = D3D11_FILL_SOLID;
	rasterizerState.DepthClipEnable = true;
	rasterizerState.FrontCounterClockwise = true;

	HR_T(m_device->CreateRasterizerState(&rasterizerState, m_skyRasterizerState.ReleaseAndGetAddressOf()));

	// 스카이 박스 뎊스 스텐실 상태 개체 추가
	D3D11_DEPTH_STENCIL_DESC skyboxDsDesc{};
	skyboxDsDesc.DepthEnable = true;
	skyboxDsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;	// 깊이 버퍼 사용 X
	skyboxDsDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;		
	skyboxDsDesc.StencilEnable = false;

	HR_T(m_device->CreateDepthStencilState(&skyboxDsDesc, m_skyDepthStencilState.GetAddressOf()));

	return true;
}

void DeferredRenderApp::CreateSwapchain()
{
	// 2. 스왑체인 생성을 위한 DXGI Factory 생성
	UINT dxgiFactoryFlags = 0;

#ifdef _DEBUG
	dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif // _DEBUG

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
#if USE_FLIPMODE == 1
	swapChainDesc.BufferCount = 2;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // ??
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

	HR_T(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&m_factory)));
	HR_T(m_factory->CreateSwapChainForHwnd
	(
		m_device.Get(),
		m_hWnd,
		&swapChainDesc,
		nullptr,
		nullptr,
		&m_swapChain
	));

	// 3. 랜더타겟 뷰 생성. 랜더타겟 뷰는 "여기에 그림을 그려라"라고 GPU에게 알려주는 역할을 하는 객체
	// 텍스쳐와 영구적으로 연결되는 객체
	ComPtr<ID3D11Texture2D> pBackBufferTexture;
	HR_T(m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBufferTexture));
	HR_T(m_device->CreateRenderTargetView(pBackBufferTexture.Get(), nullptr, m_backbufferRTV.GetAddressOf()));

	// ??
	Microsoft::WRL::ComPtr<IDXGISwapChain3> swapChain3;
	HR_T(m_swapChain->QueryInterface(__uuidof(IDXGISwapChain3), (void**)&swapChain3));
}

void DeferredRenderApp::DrawFrustum(Matrix worldMat, Matrix viewMat, Matrix proejctionMat,
	float angle, float AspectRatio, float nearZ, float farZ, XMVECTORF32 color)
{
	Vector3 scale = { 1,1,1 };

	// 절투체 만들기
	BoundingFrustum frustum{};
	BoundingFrustum::CreateFromMatrix(frustum, proejctionMat);

	Matrix frustumWorld = viewMat.Transpose();

	frustum.Transform(frustum, frustumWorld); // 위치 옮기기
	frustum.Transform(frustum, worldMat);

	m_effect->SetWorld(Matrix::Identity);	// 해당 그림 위치 설정
	m_effect->SetView(m_View);				// 해당 그림을 어디 기준으로 그릴지 설정
	m_effect->SetProjection(m_Projection);	// 해당 그림이 어디에 투영 될지 설정
	m_effect->Apply(m_deviceContext.Get());

	// 문서에 따른 세팅 -> https://github.com/microsoft/DirectXTK/wiki/DebugDraw
	m_deviceContext->OMSetBlendState(m_states->Opaque(), nullptr, 0xFFFFFFFF);
	m_deviceContext->OMSetDepthStencilState(m_states->DepthNone(), 0);
	m_deviceContext->RSSetState(m_states->CullNone());

	m_deviceContext->IASetInputLayout(m_debugDrawInputLayout.Get()); // 디버그용 InputLayout 적용

	m_batch->Begin();
	DX::Draw(m_batch.get(), frustum, color);

	m_batch->End();
}

bool DeferredRenderApp::InitDxgi()
{
	// ============================================
	// 1. IDXGIDevice3 생성
	// ============================================
	HR_T(m_device->QueryInterface(__uuidof(IDXGIDevice3), reinterpret_cast<void**>(dxgiDevice3.GetAddressOf())));


	// ============================================
	// 1. DXGI 어댑터 생성
	// ============================================
	HR_T((CreateDXGIFactory1(__uuidof(IDXGIFactory6), reinterpret_cast<void**>(dxgiFactory6.GetAddressOf()))));

	HR_T(dxgiFactory6->EnumAdapters1(0, dxgiAdapter1.GetAddressOf()));

	// ============================================
	// 3. VRAM 사용량 (DXGI 1.4 - Windows 10+)
	// ============================================
	HR_T(dxgiAdapter1->QueryInterface(__uuidof(IDXGIAdapter3), reinterpret_cast<void**>(dxgiAdapter3.GetAddressOf())));

	return true;
}

void DeferredRenderApp::CreateQuad()
{
	QuadVertex QuadVertices[] =
	{
		QuadVertex(Vector3(-1.0f,  1.0f, 1.0f), Vector2(0.0f,0.0f)),	// Left Top 
		QuadVertex(Vector3(1.0f,  1.0f, 1.0f), Vector2(1.0f, 0.0f)),	// Right Top
		QuadVertex(Vector3(-1.0f, -1.0f, 1.0f), Vector2(0.0f, 1.0f)),	// Left Bottom
		QuadVertex(Vector3(1.0f, -1.0f, 1.0f), Vector2(1.0f, 1.0f))		// Right Bottom
	};

	D3D11_BUFFER_DESC vbDesc = {};
	vbDesc.ByteWidth = sizeof(QuadVertex) * ARRAYSIZE(QuadVertices);
	vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbDesc.Usage = D3D11_USAGE_DEFAULT;
	D3D11_SUBRESOURCE_DATA vbData = {};
	vbData.pSysMem = QuadVertices;	// 배열 데이터 할당.
	HR_T(m_device->CreateBuffer(&vbDesc, &vbData, &m_quadVertexBuffer));
	m_quadVertexBufferStride = sizeof(QuadVertex);		// 버텍스 버퍼 정보
	m_quadVertexBufferOffset = 0;

	// InputLayout 생성 	
	D3D11_INPUT_ELEMENT_DESC layout[] = // 입력 레이아웃.
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	ComPtr<ID3DBlob> vertexShaderBuffer{};
	// Light Pass
	vertexShaderBuffer.Reset();
	HR_T(CompileShaderFromFile(L"Shaders\\VS_DirectionalLight.hlsl", "main", "vs_5_0", vertexShaderBuffer.GetAddressOf()));
	HR_T(m_device->CreateInputLayout(layout, ARRAYSIZE(layout),vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), m_quadInputLayout.GetAddressOf()));
	HR_T(m_device->CreateVertexShader(vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), NULL, m_directionalLightVS.GetAddressOf()));

	// 인덱스 버퍼 생성
	WORD indices[] =
	{
		0,1,2,
		1,3,2
	};
	m_quadIndicesCount = ARRAYSIZE(indices);	// 인덱스 개수 저장.
	D3D11_BUFFER_DESC ibDesc = {};
	ibDesc.ByteWidth = sizeof(WORD) * ARRAYSIZE(indices);
	ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibDesc.Usage = D3D11_USAGE_DEFAULT;
	D3D11_SUBRESOURCE_DATA ibData = {};
	ibData.pSysMem = indices;
	HR_T(m_device->CreateBuffer(&ibDesc, &ibData, m_quadIndexBuffer.GetAddressOf()));
}

void DeferredRenderApp::CreateGbuffers()
{
	// gbuffer
	m_gBufferRTV.assign(gbufferCount, {});
	m_gBufferSRV.assign(gbufferCount, {});
	m_gBufferTextures.assign(gbufferCount, {});
	struct RTDesc
	{
		DXGI_FORMAT format;
	};

	vector<RTDesc> formats
	{
		{ DXGI_FORMAT_R8G8B8A8_UNORM_SRGB },	// BaseColor
		{ DXGI_FORMAT_R8G8B8A8_UNORM },			// Normal
		{ DXGI_FORMAT_R16G16B16A16_FLOAT },		// PositionWS 
		{ DXGI_FORMAT_R8_UNORM },			// Metal
		{ DXGI_FORMAT_R8_UNORM },			// rough
		{ DXGI_FORMAT_R8_UNORM },			// specular
		{ DXGI_FORMAT_R8G8B8A8_UNORM },			// emission
	};


	for (int i = 0; i < gbufferCount; i++)
	{
		D3D11_TEXTURE2D_DESC ds{};
		ds.Width = m_ClientWidth;
		ds.Height = m_ClientHeight;
		ds.MipLevels = 1;
		ds.ArraySize = 1;
		ds.Format = formats[i].format;
		ds.SampleDesc.Count = 1;
		ds.Usage = D3D11_USAGE_DEFAULT;
		ds.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

		HR_T(m_device->CreateTexture2D(&ds, nullptr, m_gBufferTextures[i].GetAddressOf()));
		HR_T(m_device->CreateShaderResourceView(m_gBufferTextures[i].Get(), nullptr, m_gBufferSRV[i].GetAddressOf()));
		HR_T(m_device->CreateRenderTargetView(m_gBufferTextures[i].Get(), nullptr, m_gBufferRTV[i].GetAddressOf()));
	}
}

void DeferredRenderApp::ResizeScreen(int width, int height)
{
	if (m_hWnd) return;

	m_ClientWidth = max(width, 1);
	m_ClientHeight = max(height, 1);

	ResizeResource();
}

void DeferredRenderApp::ResizeResource()
{
	// Clear
	m_deviceContext->OMSetRenderTargets(0, nullptr, nullptr);
	m_backbufferRTV.Reset();
	m_depthStencilView.Reset();
	m_deviceContext->Flush();		 // ??

	const UINT backBufferWidth = static_cast<UINT>(m_ClientWidth);
	const UINT backBufferHeight = static_cast<UINT>(m_ClientHeight);
	const DXGI_FORMAT backBufferFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
	const DXGI_FORMAT depthBufferFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	constexpr UINT backBufferCount = 2;

	if (m_swapChain) // check swapchain exist
	{
		// 스왑체인 리사이즈
		HR_T(m_swapChain->ResizeBuffers(backBufferCount, backBufferWidth, backBufferHeight, backBufferFormat, 0));
	}
	else
	{
		return; // -> 해당 함수 사용시 스왑체인이 있다고 가정한다.
	}

	// 백버퍼 생성
	ComPtr<ID3D11Texture2D> backBuffer;
	HR_T(m_swapChain->GetBuffer(0, IID_PPV_ARGS(backBuffer.GetAddressOf())));

	// 백버퍼 rtv 생성
	HR_T(m_device->CreateRenderTargetView(backBuffer.Get(), nullptr, m_backbufferRTV.GetAddressOf()));

	// dsv 생성
	CD3D11_TEXTURE2D_DESC dsDesc(depthBufferFormat, backBufferWidth, backBufferHeight, 1, 1, D3D11_BIND_DEPTH_STENCIL);
	ComPtr<ID3D11Texture2D> depthStencil;

	HR_T(m_device->CreateTexture2D(&dsDesc, nullptr, depthStencil.GetAddressOf()));
	HR_T(m_device->CreateDepthStencilView(depthStencil.Get(), nullptr, m_depthStencilView.ReleaseAndGetAddressOf()));
}

bool DeferredRenderApp::OnInitialize()
{
	if (!InitD3D())
		return false;

	if (!InitScene())
		return false;

	if (!InitImGUI())
		return false;

	if (!InitEffect())
		return false;

	if (!InitSkyBox())
		return false;

	if (!InitDxgi())
		return false;

	CreateSwapchain();
	InitShdowMap();
	InitDebugDraw();
	ResetValues();
	CreateQuad();
	CreateGbuffers();

	return true;
}

void DeferredRenderApp::OnUpdate()
{
	float delta = GameTimer::m_Instance->DeltaTime();

	Matrix scale = Matrix::Identity;
	Matrix rotate = Matrix::Identity;
	Matrix position = Matrix::Identity;

	// Camera
	m_Camera.GetCameraViewMatrix(m_View);

	m_chara->Update();
	m_ground->Update();
	m_ground->m_Scale = m_GroundScale;
	m_sphere->Update();

	for (auto& e : m_models)
	{
		if (!e->isRemoved) e->Update();
	}
}

void DeferredRenderApp::OnRender()
{
	// clear
	Color color(0.0f, 0.0f, 0.0f, 0.0f);
	m_deviceContext->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
	m_deviceContext->ClearRenderTargetView(m_backbufferRTV.Get(), color);

	// GBuffer 초기화
	int gbufferCount = static_cast<int>(EGbuffer::Count);
	float clearValue[4] = { 0,0,0,0 };
	for (int i = 0; i < gbufferCount; i++)
	{
		m_deviceContext->ClearRenderTargetView(m_gBufferRTV[i].Get(), clearValue);
	}

	DepthOnlyPass();
	RenderPassGBuffer();
	RenderPassDirectionalLight();
	SkyboxPass(); 

	RenderImGUI();

	// 스왑체인 교체
	m_swapChain->Present(0, 0);
}

bool isplayed = false;

void DeferredRenderApp::RenderPassGBuffer()
{
	m_deviceContext->OMSetRenderTargets(gbufferCount, m_gBufferRTV.data()->GetAddressOf(), m_depthStencilView.Get());

	// Update Constant Values
	ConstantBuffer cb;
	cb.cameraView = XMMatrixTranspose(m_View);
	cb.cameraProjection = XMMatrixTranspose(m_Projection);
	cb.shadowView = XMMatrixTranspose(m_shadowView);
	cb.shadowProjection = XMMatrixTranspose(m_shadowProj);

	cb.ambient = m_LightAmbient;
	cb.diffuse = m_LightDiffuse;
	cb.specular = m_LightSpecular;

	cb.shininess = m_Shininess;
	cb.CameraPos = m_Camera.m_Position;

	cb.roughness = roughness;
	cb.metalness = metalness;
	cb.lightIntensity = lightIntensity;
	cb.baseColor = BaseColor;

	m_deviceContext->UpdateSubresource(m_constantBuffer.Get(), 0, nullptr, &cb, 0, 0);

	// 텍스처 및 샘플링 설정 -> 초기화
	m_deviceContext->IASetInputLayout(m_inputLayout.Get());
	m_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	m_deviceContext->VSSetShader(m_skinnedMeshVertexShader.Get(), 0, 0);
	m_deviceContext->VSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());

	m_deviceContext->RSSetState(m_rasterizerState.Get());
	m_deviceContext->RSSetViewports(1, &m_RenderViewport); // 뷰포트 되돌리기

	m_deviceContext->PSSetShader(m_PBRPS.Get(), 0, 0);
	m_deviceContext->PSSetShaderResources(4, 1, m_shadowMapSRV.GetAddressOf());
	m_deviceContext->PSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());
	m_deviceContext->PSSetConstantBuffers(1, 1, m_materialBuffer.GetAddressOf());
	m_deviceContext->PSSetSamplers(0, 1, m_samplerLinear.GetAddressOf());
	m_deviceContext->PSSetSamplers(1, 1, m_samplerPoint.GetAddressOf());


	m_deviceContext->OMSetDepthStencilState(m_depthStencilStateWriteOn.Get(), 1);


	// Draw 
	m_chara->Draw(m_deviceContext, m_materialBuffer);
	m_ground->Draw(m_deviceContext, m_materialBuffer);
	m_tree->Draw(m_deviceContext, m_materialBuffer);
	m_sphere->Draw(m_deviceContext, m_materialBuffer);

	for (auto& e : m_models)
	{
		e->Draw(m_deviceContext, m_materialBuffer);
	}

	// RTV Unbind 	
	vector<ID3D11RenderTargetView*> nullRTVs(m_gBufferRTV.size(), {});
	m_deviceContext->OMSetRenderTargets(gbufferCount, nullRTVs.data(), nullptr);
}

void DeferredRenderApp::RenderPassDirectionalLight()
{
	LightDirectionCB lightdirCB;
	//m_LightDirection.Normalize();
	lightdirCB.lightDirection = Vector4(m_LightDirection.x, m_LightDirection.y, m_LightDirection.z, lightIntensity);
	lightdirCB.lightColor = m_LightColor;

	ConstantBuffer cb;
	cb.CameraPos = m_Camera.m_Position;
	cb.shadowView = XMMatrixTranspose(m_shadowView);
	cb.shadowProjection = XMMatrixTranspose(m_shadowProj);
	// 11, 12, 13 -> color, normal, worldpos
	// 4 shadow depth

	// 처음 빛을 추가할 때 백버퍼 초기화
	float blendFactor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	m_deviceContext->OMSetBlendState(m_additiveBlendState.Get(), blendFactor, 0xffffffff);
	m_deviceContext->OMSetRenderTargets(1, m_backbufferRTV.GetAddressOf(), m_depthStencilView.Get());
	m_deviceContext->OMSetDepthStencilState(m_depthStencilStateWriteOff.Get(), 0); // Depth test OFF, write OFF

	vector<ID3D11ShaderResourceView*> SRVs =
	{
		m_gBufferSRV[0].Get(),
		m_gBufferSRV[1].Get(),
		m_gBufferSRV[2].Get(),
		m_gBufferSRV[3].Get(),
		m_gBufferSRV[4].Get()
	};

	m_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_deviceContext->IASetVertexBuffers(0, 1, m_quadVertexBuffer.GetAddressOf(), &m_quadVertexBufferStride, &m_quadVertexBufferOffset);
	m_deviceContext->IASetIndexBuffer(m_quadIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
	m_deviceContext->IASetInputLayout(m_quadInputLayout.Get());

	m_deviceContext->VSSetShader(m_directionalLightVS.Get(), nullptr, 0);

	m_deviceContext->UpdateSubresource(m_lightDirectionBuffer.Get(), 0, nullptr, &lightdirCB, 0, 0);
	m_deviceContext->PSSetConstantBuffers(5, 1, m_lightDirectionBuffer.GetAddressOf());
	m_deviceContext->UpdateSubresource(m_constantBuffer.Get(), 0, nullptr, &cb, 0, 0);
	m_deviceContext->PSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());

	m_deviceContext->PSSetShaderResources(11, SRVs.size(), SRVs.data());			// gbuffer texture 바인드
	m_deviceContext->PSSetShaderResources(4, 1, m_shadowMapSRV.GetAddressOf());	// shadow map 바인드
	m_deviceContext->PSSetShaderResources(8, 1, m_IBLIrradiance.GetAddressOf());		// Irradiance
	m_deviceContext->PSSetShaderResources(9, 1, m_IBLSpecular.GetAddressOf());		// Sepcular
	m_deviceContext->PSSetShaderResources(10, 1, m_IBLLookUpTable.GetAddressOf());	// LUT

	m_deviceContext->PSSetSamplers(0, 1, m_samplerLinear.GetAddressOf());
	m_deviceContext->PSSetSamplers(1, 1, m_samplerLinear.GetAddressOf());
	m_deviceContext->PSSetShader(m_directionalLightPS.Get(), nullptr, 0); 
	m_deviceContext->DrawIndexed(m_quadIndicesCount, 0, 0);

	m_deviceContext->OMSetBlendState(nullptr, blendFactor, 0xffffffff);
	
	// srv unbind
	vector<ID3D11ShaderResourceView*> nullSRVs(SRVs.size(), nullptr);
	m_deviceContext->PSSetShaderResources(11, SRVs.size(), nullSRVs.data());	// gbuffer tex

	vector<ID3D11ShaderResourceView*> nullSRVs2(1, nullptr);
	m_deviceContext->PSSetShaderResources(4, 1, nullSRVs2.data());				// shadow map
}

void DeferredRenderApp::SkyboxPass()
{
	// 카메라용 뷰 행렬과, 투영행렬
	Matrix m_skyboxProjection = XMMatrixPerspectiveFovLH(m_PovAngle, m_ClientWidth / (FLOAT)m_ClientHeight, 0.1, m_Far);

	// Update Constant Values
	ConstantBuffer cb;
	cb.cameraView = XMMatrixTranspose(m_View); // 쉐이더 코드 내부에서 이동 성분 제거함
	cb.cameraProjection = XMMatrixTranspose(m_skyboxProjection);
	cb.shadowView = XMMatrixTranspose(m_shadowView);
	cb.shadowProjection = XMMatrixTranspose(m_shadowProj);


	m_deviceContext->IASetVertexBuffers(0, 1, m_skyboxVertexBuffer.GetAddressOf(), &m_SkyboxVertexBufferStride, &m_SkyboxVertexBufferOffset);
	m_deviceContext->IASetIndexBuffer(m_skyboxIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
	m_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_deviceContext->IASetInputLayout(m_skyboxInputLayout.Get());

	m_deviceContext->VSSetShader(m_skyboxVS.Get(), nullptr, 0);
	m_deviceContext->VSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());

	m_deviceContext->RSSetState(m_skyRasterizerState.Get());
	m_deviceContext->RSSetViewports(1, &m_RenderViewport); // 뷰포트 되돌리기

	m_deviceContext->PSSetShader(m_skyboxPS.Get(), nullptr, 0);
	m_deviceContext->PSSetShaderResources(5, 1, m_skyboxTexture.GetAddressOf());
	m_deviceContext->PSSetSamplers(0, 1, m_samplerLinear.GetAddressOf());
	m_deviceContext->PSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());

	m_deviceContext->OMSetRenderTargets(1, m_backbufferRTV.GetAddressOf(), m_depthStencilView.Get());
	m_deviceContext->OMSetDepthStencilState(m_skyDepthStencilState.Get(), 1); // 뎊스 스텐실 설정

	m_deviceContext->UpdateSubresource(m_constantBuffer.Get(), 0, nullptr, &cb, 0, 0);

	m_deviceContext->DrawIndexed(m_nSkyboxIndices, 0, 0);
}

void DeferredRenderApp::DepthOnlyPass()
{
	// 바인딩 해제
	ID3D11ShaderResourceView* nullSRV[1] = { nullptr };
	m_deviceContext->VSSetShaderResources(4, 1, nullSRV);
	m_deviceContext->PSSetShaderResources(4, 1, nullSRV);

	//상수 버퍼 갱신
	ConstantBuffer cb;
	cb.cameraView = XMMatrixTranspose(m_View);
	cb.cameraProjection = XMMatrixTranspose(m_Projection);
	cb.shadowView = XMMatrixTranspose(m_shadowView);
	cb.shadowProjection = XMMatrixTranspose(m_shadowProj);

	cb.ambient = m_LightAmbient;
	cb.diffuse = m_LightDiffuse;
	cb.specular = m_LightSpecular;

	cb.shininess = m_Shininess;
	cb.CameraPos = m_Camera.m_Position;

	// 뷰포트 설정 + DSV 초기화, RS, OM 설정
	D3D11_VIEWPORT viewport
	{
		m_shadowViewport.x, m_shadowViewport.y,
		m_shadowViewport.width, m_shadowViewport.height,
		m_shadowViewport.minDepth, m_shadowViewport.maxDepth
	};
	m_deviceContext->RSSetViewports(1, &viewport);
	m_deviceContext->OMSetDepthStencilState(m_depthStencilStateWriteOn.Get(), 1);
	m_deviceContext->OMSetRenderTargets(0, nullptr, m_shadowMapDSV.Get());
	m_deviceContext->ClearDepthStencilView(m_shadowMapDSV.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);

	// 렌더 파이프라인 설정
	m_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_deviceContext->IASetInputLayout(m_inputLayout.Get());

	m_deviceContext->UpdateSubresource(m_constantBuffer.Get(), 0, nullptr, &cb, 0, 0);
	m_deviceContext->VSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());
	m_deviceContext->PSSetSamplers(0, 1, m_samplerLinear.GetAddressOf());

	m_deviceContext->VSSetShader(m_shadowMapVS.Get(), 0, 0);
	// m_pDeviceContext->PSSetShader(NULL, NULL, 0); // 렌더 타겟에 기록할 RGBA가 없으므로 실행하지 않는다.
	m_deviceContext->PSSetShader(m_shadowMapPS.Get(), NULL, 0); // 

	// 모델 draw 호출
	m_ground->Draw(m_deviceContext, m_materialBuffer);
	m_chara->Draw(m_deviceContext, m_materialBuffer);
	m_tree->Draw(m_deviceContext, m_materialBuffer);
	m_sphere->Draw(m_deviceContext, m_materialBuffer);

	for (auto& e : m_models)
	{
		if (!e->isRemoved) e->Draw(m_deviceContext, m_materialBuffer);
	}
}

bool DeferredRenderApp::InitImGUI()
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

	isSetupSuccess = ImGui_ImplDX11_Init(m_device.Get(), m_deviceContext.Get());
	if (!isSetupSuccess) return false;

	return true;
}

void DeferredRenderApp::RenderImGUI()
{
	// Start the Dear ImGui frame
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	ImGui::SetNextWindowPos(ImVec2(800, 10), ImGuiCond_Once);
	ImGui::SetNextWindowSize(ImVec2(200, 200), ImGuiCond_Once);

	if (m_chara->modelAsset->animations.size() > 0)
	{
		ImGui::Begin("Middle Model Animation info");
		{
			ImGui::Text(std::to_string(m_chara->m_progressAnimationTime).c_str());
			ImGui::Checkbox("isPlay", &m_chara->isAnimPlay);
			ImGui::SliderFloat("Animation Duration", &m_chara->m_progressAnimationTime, 0.0f, m_chara->modelAsset->animations[m_chara->m_animationIndex].m_duration);
		}
		ImGui::End();
	}

	ImGui::Begin("ShadowMap");
	{
		ImTextureID img = (ImTextureID)(intptr_t)(m_shadowMapSRV.Get());
		ImGui::Image(img, ImVec2(256, 256));
		ImGui::DragFloat("m_shadowForwardDistFromCamera", &m_shadowForwardDistFromCamera);
		ImGui::DragFloat3("m_shadowUpDistFromLookAt", &m_shadowUpDistFromLookAt.x);

		ImGui::DragFloat("m_shadowNear", &m_shadowNear);
		ImGui::DragFloat("m_shadowFar", &m_shadowFar);

		if (m_shadowNear <= m_shadowMinNear) m_shadowNear = m_shadowMinNear;
		if (m_shadowFar <= m_shadowMinFar) m_shadowFar = m_shadowMinFar;

		ImGui::DragFloat("m_shadowFrustumAngle", &m_shadowFrustumAngle, 0.02f);

		// m_shadow 관련 갱신
		m_shadowProj = XMMatrixPerspectiveFovLH(m_shadowFrustumAngle, m_shadowViewport.width / (FLOAT)m_shadowViewport.height, m_shadowNear, m_shadowFar);
		m_shadowLookAt = m_Camera.m_Position + m_Camera.GetForward() * m_shadowForwardDistFromCamera;	// 바라보는 방향 = 카메라 위치 + 카메라 바라보는 방향으로부터 떨어진 태양의 위치
		m_shadowPos = m_Camera.m_Position + ((Vector3)-m_LightDirection * m_shadowUpDistFromLookAt);	// 위치
		m_shadowView = XMMatrixLookAtLH(m_shadowPos, m_shadowLookAt, Vector3(0.0f, 1.0f, 0.0f));
	}
	ImGui::End();


	// 월드 오브젝트 조종 창 만들기
	ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Once);		// 처음 실행될 때 위치 초기화
	ImGui::SetNextWindowSize(ImVec2(350, 500), ImGuiCond_Once);		// 처음 실행될 때 창 크기 초기화
	ImGui::Begin("World Controller");

	{
		ImGui::DragFloat3("Middle Object Position", &m_charaPosition.x);

		Vector3 cube1Rotation;
		cube1Rotation.x = XMConvertToDegrees(m_charaRotation.x);
		cube1Rotation.y = XMConvertToDegrees(m_charaRotation.y);
		cube1Rotation.z = XMConvertToDegrees(m_charaRotation.z);
		ImGui::DragFloat3("Middle Object Rotation", &cube1Rotation.x);
		m_charaRotation.x = XMConvertToRadians(cube1Rotation.x);
		m_charaRotation.y = XMConvertToRadians(cube1Rotation.y);
		m_charaRotation.z = XMConvertToRadians(cube1Rotation.z);

		ImGui::DragFloat("Middle Object Scale", &m_charaScale.x, 0.05f);
		m_charaScale.y = m_charaScale.x;
		m_charaScale.z = m_charaScale.x;

		m_chara->m_Position = m_charaPosition;
		m_chara->m_Rotation = m_charaRotation;
		m_chara->m_Scale = m_charaScale;
	}

	ImGui::ColorEdit3("Light Color", &m_LightColor.x);
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
	if (m_Far <= m_Near) m_Far = 0.2f;

	ImGui::NewLine();

	ImGui::Text("Select tex:");
	ImGui::Checkbox("Enable hasDiffuse", &useBaseColor);
	ImGui::Checkbox("Enable hasNormal", &useNormal);
	ImGui::Checkbox("Enable hasMatalness", &useMetalness);
	ImGui::Checkbox("Enable hasRoughness", &useRoughness);

	for (auto& mesh : m_chara->modelAsset->meshes)
	{
		mesh.GetMaterial().hasDiffuse = useBaseColor;
		mesh.GetMaterial().hasNormal = useNormal;
		mesh.GetMaterial().hasMatalness = useMetalness;
		mesh.GetMaterial().hasRoughness = useRoughness;
	}

	ImGui::NewLine();

	m_Projection = XMMatrixPerspectiveFovLH(m_PovAngle, m_ClientWidth / (FLOAT)m_ClientHeight, m_Near, m_Far);

	ImGui::DragFloat3("Light Direction", &m_LightDirection.x, 0.1f, -1.0f, 1.0f);

	ImGui::DragFloat("Roughness", &roughness, 0.01f, 0, 1);
	ImGui::DragFloat("Metalness", &metalness, 0.01f, 0, 1);
	ImGui::DragFloat("Lightintensity", &lightIntensity, 0.01f, 0, 1);
	ImGui::ColorEdit3("BaseColor", &BaseColor.x);

	ImGui::Spacing();

	// 리셋 버튼
	if (ImGui::Button("Reset", { 50, 20 }))
	{
		ResetValues();
	}

	ImGui::End();

	ImGui::Begin("Deferred Info");
	{
		static const char* GBufferNames[] =
		{
			"BaseColor",
			"Normal",
			"WorldPos",
			"Metal",
			"Rough",
			"Specular",
			"Emission"
		};

		int size = static_cast<int>(EGbuffer::Count);

		for (int i = 0; i < size; i++)
		{
			// 카드 하나 시작
			ImGui::BeginGroup();

			// 카드 박스 (Child)
			ImGui::BeginChild(
				GBufferNames[i],
				ImVec2(150, 170),   // 카드 크기 (텍스처 + 여백)
				true,               // 테두리 표시
				ImGuiWindowFlags_NoScrollbar
			);

			// 텍스처 이름
			ImGui::Text("%s", GBufferNames[i]);
			ImGui::Separator();

			// 이미지 출력
			ImTextureID img = (ImTextureID)(intptr_t)(m_gBufferSRV[i].Get());
			ImGui::Image(img, ImVec2(128, 128));

			ImGui::EndChild();
			ImGui::EndGroup();

			// 2개씩 배치
			if (i % 2 == 0)
				ImGui::SameLine();
		}
	}
	ImGui::End();

	// rendering
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void DeferredRenderApp::UninitImGUI()
{
	// Cleanup
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

bool DeferredRenderApp::InitD3D()
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
		&m_device,
		&actualFeatureLevel,
		&m_deviceContext
	));

#if !USE_FLIPMODE
	m_pDeviceContext->OMSetRenderTargets(1, m_pRenderTargetView.GetAddressOf(), nullptr);
#endif

	// 4. viewport 설정
	m_RenderViewport = {};
	m_RenderViewport.TopLeftX = 0;
	m_RenderViewport.TopLeftY = 0;
	m_RenderViewport.Width = (float)m_ClientWidth;
	m_RenderViewport.Height = (float)m_ClientHeight;
	m_RenderViewport.MinDepth = 0.0f;
	m_RenderViewport.MaxDepth = 1.0f;
	m_deviceContext->RSSetViewports(1, &m_RenderViewport);

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

	// 뎊스 스탠실 상태 설정
	D3D11_DEPTH_STENCIL_DESC depthStencilDesc = {};
	depthStencilDesc.DepthEnable = FALSE;                // 깊이 테스트 활성화
	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO; // 깊이 버퍼 업데이트 허용
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS; // 작은 Z 값이 앞에 배치되도록 설정
	depthStencilDesc.StencilEnable = FALSE;            // 스텐실 테스트 비활성화

	m_device->CreateDepthStencilState(&depthStencilDesc, &m_depthStencilStateWriteOff);

	depthStencilDesc = {};
	depthStencilDesc.DepthEnable = TRUE;                // 깊이 테스트 활성화
	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL; // 깊이 버퍼 업데이트 허용
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS; // 작은 Z 값이 앞에 배치되도록 설정
	depthStencilDesc.StencilEnable = FALSE;            // 스텐실 테스트 비활성화

	m_device->CreateDepthStencilState(&depthStencilDesc, &m_depthStencilStateWriteOn);

	// create depthStencil texture
	ComPtr<ID3D11Texture2D> pTextureDepthStencil;
	HR_T(m_device->CreateTexture2D(&descDepth, nullptr, pTextureDepthStencil.GetAddressOf()));

	// create the depth stencil view
	D3D11_DEPTH_STENCIL_VIEW_DESC descDSV = {};
	descDSV.Format = descDepth.Format;
	descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D; // 사용되는 리소스 엑세스 방식 설정 : https://learn.microsoft.com/ko-kr/windows/win32/api/d3d11/ne-d3d11-d3d11_dsv_dimension 
	descDSV.Texture2D.MipSlice = 0;
	HR_T(m_device->CreateDepthStencilView(pTextureDepthStencil.Get(), &descDSV, m_depthStencilView.GetAddressOf()));

	// create blending state https://learn.microsoft.com/ko-kr/windows/win32/api/d3d11/ns-d3d11-d3d11_blend_desc
	// Color = SrcAlpha * SrcColor + (1 - SrcAlpha) * DestColor 
	// Alpha = SrcAlpha
	D3D11_BLEND_DESC blendDesc = {};
	blendDesc.RenderTarget[0].BlendEnable			= TRUE;						
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	m_device->CreateBlendState(&blendDesc, m_additiveBlendState.GetAddressOf());

	// material buffer
	D3D11_BUFFER_DESC mbd = {};
	mbd.Usage = D3D11_USAGE_DEFAULT;
	mbd.ByteWidth = sizeof(Material);
	mbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	mbd.CPUAccessFlags = 0;
	HR_T(m_device->CreateBuffer(&mbd, nullptr, m_materialBuffer.GetAddressOf()));

	return true;
}

bool DeferredRenderApp::InitScene()
{
	HRESULT hr = S_OK;

	// 6. Render() 에서 파이프라인에 바인딩할 상수 버퍼 생성
	D3D11_BUFFER_DESC bufferDesc = {};
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(ConstantBuffer);
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	HR_T(m_device->CreateBuffer(&bufferDesc, nullptr, m_constantBuffer.GetAddressOf()));

	bufferDesc = {};
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(LightDirectionCB);
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	HR_T(m_device->CreateBuffer(&bufferDesc, nullptr, m_lightDirectionBuffer.GetAddressOf()));

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
	HR_T(m_device->CreateSamplerState(&sampDesc, m_samplerLinear.GetAddressOf()));

	sampDesc = {};
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.MipLODBias = 0.0f;
	sampDesc.MaxAnisotropy = 1;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	sampDesc.BorderColor[0] = sampDesc.BorderColor[1] = sampDesc.BorderColor[2] = sampDesc.BorderColor[3] = 0.0f;
	sampDesc.MinLOD = 0.0f;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	HR_T(m_device->CreateSamplerState(&sampDesc, m_samplerPoint.GetAddressOf()));

	// 래스터라이저
	D3D11_RASTERIZER_DESC rasterizerDesc = {};
	rasterizerDesc.CullMode = D3D11_CULL_FRONT;
	rasterizerDesc.FillMode = D3D11_FILL_SOLID;
	rasterizerDesc.DepthClipEnable = true;
	rasterizerDesc.FrontCounterClockwise = true;

	m_device->CreateRasterizerState(&rasterizerDesc, &m_rasterizerState);

	rasterizerDesc = {};
	rasterizerDesc.CullMode = D3D11_CULL_FRONT;
	rasterizerDesc.FillMode = D3D11_FILL_SOLID;
	rasterizerDesc.DepthClipEnable = true;
	rasterizerDesc.FrontCounterClockwise = true;

	m_device->CreateRasterizerState(&rasterizerDesc, &m_transparentRasterizerState);

	// 모델들이 사용할 버퍼 만들기
	// 트랜스폼 상수 버퍼 만들기
	bufferDesc = {};
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(TransformBuffer);
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	HR_T(m_device->CreateBuffer(&bufferDesc, nullptr, m_transformBuffer.GetAddressOf()));

	// 본 포즈 버퍼 만들기
	bufferDesc = {};
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(BonePoseBuffer);
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	HR_T(m_device->CreateBuffer(&bufferDesc, nullptr, m_bonePoseBuffer.GetAddressOf()));

	// 본 오프셋 버퍼 만들기
	bufferDesc = {};
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(BoneOffsetBuffer);
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	HR_T(m_device->CreateBuffer(&bufferDesc, nullptr, m_boneOffsetBuffer.GetAddressOf()));

	// 모델 생성
	m_chara = make_unique<SkeletalModel>();
	if (!m_chara->Load(m_hWnd, m_device, m_deviceContext, "..\\Resource\\char.fbx"))
	{
		MessageBox(m_hWnd, L"FBX file is invaild at path", NULL, MB_ICONERROR | MB_OK);
	}
	m_chara->GetBuffer(m_transformBuffer, m_bonePoseBuffer, m_boneOffsetBuffer);
	m_chara->m_Position = { 0, 100, 0 };

	m_ground = make_unique<SkeletalModel>();
	if (!m_ground->Load(m_hWnd, m_device, m_deviceContext, "..\\Resource\\Ground.fbx"))
	{
		MessageBox(m_hWnd, L"FBX file is invaild at path", NULL, MB_ICONERROR | MB_OK);
	}
	m_ground->GetBuffer(m_transformBuffer, m_bonePoseBuffer, m_boneOffsetBuffer);
	m_ground->m_Position = { 0, -100, 0 };

	m_tree = make_unique<SkeletalModel>();
	if (!m_tree->Load(m_hWnd, m_device, m_deviceContext, "..\\Resource\\Tree.fbx"))
	{
		MessageBox(m_hWnd, L"FBX file is invaild at path", NULL, MB_ICONERROR | MB_OK);
	}
	m_tree->GetBuffer(m_transformBuffer, m_bonePoseBuffer, m_boneOffsetBuffer);
	m_tree->m_Scale = { 100, 100, 100 };
	m_tree->m_Position = { 0, 0, 300 };

	m_sphere = make_unique<SkeletalModel>();
	if (!m_sphere->Load(m_hWnd, m_device, m_deviceContext, "..\\Resource\\sphere.fbx"))
	{
		MessageBox(m_hWnd, L"FBX file is invaild at path", NULL, MB_ICONERROR | MB_OK);
	}
	m_sphere->GetBuffer(m_transformBuffer, m_bonePoseBuffer, m_boneOffsetBuffer);
	m_sphere->m_Position = { 200, 100, 100 };

	HR_T(CreateDDSTextureFromFile(m_device.Get(), L"..\\Resource\\skyboxEnvHDR.dds", nullptr, m_skyboxTexture.GetAddressOf()));
	HR_T(CreateDDSTextureFromFile(m_device.Get(), L"..\\Resource\\skyboxDiffuseHDR.dds", nullptr, m_IBLIrradiance.GetAddressOf()));
	HR_T(CreateDDSTextureFromFile(m_device.Get(), L"..\\Resource\\skyboxSpecularHDR.dds", nullptr, m_IBLSpecular.GetAddressOf()));
	HR_T(CreateDDSTextureFromFile(m_device.Get(), L"..\\Resource\\skyboxBrdf.dds", nullptr, m_IBLLookUpTable.GetAddressOf()));

	return true;
}

bool DeferredRenderApp::InitEffect()
{
	// Geometry Pass
	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "BINORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "BLENDINDICES", 0, DXGI_FORMAT_R32G32B32A32_SINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "BLENDWEIGHTS", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	ComPtr<ID3DBlob> vertexShaderBuffer = nullptr;
	HR_T(CompileShaderFromFile(L"Shaders\\VS_GBuffer.hlsl", "main", "vs_5_0", vertexShaderBuffer.GetAddressOf()));
	HR_T(m_device->CreateInputLayout(layout, ARRAYSIZE(layout), vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), m_inputLayout.GetAddressOf()));

	HR_T(m_device->CreateVertexShader(vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), NULL, m_skinnedMeshVertexShader.GetAddressOf()));

	vertexShaderBuffer.Reset();
	HR_T(CompileShaderFromFile(L"Shaders\\VS_DepthOnlyPass.hlsl", "main", "vs_5_0", vertexShaderBuffer.GetAddressOf()));
	HR_T(m_device->CreateVertexShader(vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), NULL, m_shadowMapVS.GetAddressOf()));

	// Light Pass
	// ....

	// Ps
	ComPtr<ID3DBlob> pixelShaderBuffer = nullptr;
	pixelShaderBuffer.Reset();
	HR_T(CompileShaderFromFile(L"Shaders\\PS_DepthOnlyPass.hlsl", "main", "ps_5_0", pixelShaderBuffer.GetAddressOf()));
	HR_T(m_device->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, m_shadowMapPS.GetAddressOf()));

	pixelShaderBuffer.Reset();
	HR_T(CompileShaderFromFile(L"Shaders\\PS_Gbuffer.hlsl", "main", "ps_5_0", pixelShaderBuffer.GetAddressOf()));
	HR_T(m_device->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, m_PBRPS.GetAddressOf()));

	pixelShaderBuffer.Reset();
	HR_T(CompileShaderFromFile(L"Shaders\\PS_DirectionalLight.hlsl", "main", "ps_5_0", pixelShaderBuffer.GetAddressOf()));
	HR_T(m_device->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, m_directionalLightPS.GetAddressOf()));

	return true;
}

void DeferredRenderApp::ResetValues()
{
	m_Near = 0.01f;
	m_Far = 3000.0f;
	m_PovAngle = XM_PIDIV2;

	m_CameraRotation = Vector3::Zero;
	m_Camera.SetRotation(m_CameraRotation);
	m_Camera.SetPosition(m_CameraPositionInitial);

	m_Projection = XMMatrixPerspectiveFovLH(m_PovAngle, m_ClientWidth / (FLOAT)m_ClientHeight, m_Near, m_Far);

	m_LightDirection = m_LightDirectionInitial;
}

void DeferredRenderApp::ShowMatrix(const DirectX::XMFLOAT4X4& mat, const char* label)
{
	// 각 행을 출력
	ImGui::Text("%.3f  %.3f  %.3f  %.3f", mat._11, mat._12, mat._13, mat._14);
	ImGui::Text("%.3f  %.3f  %.3f  %.3f", mat._21, mat._22, mat._23, mat._24);
	ImGui::Text("%.3f  %.3f  %.3f  %.3f", mat._31, mat._32, mat._33, mat._34);
	ImGui::Text("%.3f  %.3f  %.3f  %.3f", mat._41, mat._42, mat._43, mat._44);
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT DeferredRenderApp::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
		return true;

	switch (message)
	{
	case WM_ENTERSIZEMOVE:
		screenIsSizeMove = true;
		break;
	case WM_EXITSIZEMOVE:
		screenIsSizeMove = false;
		if (m_pInstance)
		{
			RECT rc;
			GetClientRect(hWnd, &rc);
			ResizeScreen(rc.right - rc.left, rc.bottom - rc.top);
		}
		break;
	default:
		break;
	}

	return __super::WndProc(hWnd, message, wParam, lParam);
}

void DeferredRenderApp::OnInputProcess(const Keyboard::State& KeyState, const Keyboard::KeyboardStateTracker& KeyTracker, const Mouse::State& MouseState, const Mouse::ButtonStateTracker& MouseTracker)
{
	__super::OnInputProcess(KeyState, KeyTracker, MouseState, MouseTracker);

	if(KeyState.O)
	{
		auto model = make_unique<SkeletalModel>();
		if (!model->Load(m_hWnd, m_device, m_deviceContext, "..\\Resource\\SillyDancing.fbx"))
		{
			MessageBox(m_hWnd, L"FBX file is invaild at path", NULL, MB_ICONERROR | MB_OK);
		}

		model->GetBuffer(m_transformBuffer, m_bonePoseBuffer, m_boneOffsetBuffer);

		Vector3 pos = m_Camera.m_Position;
		model->m_Position = pos;
		m_models.push_back(std::move(model));
	}

	if (KeyState.P)
	{
		if (!m_models.empty())
		{
			m_models.front()->isRemoved = true;
			m_models.pop_front();
		}
	}

	if (KeyState.Delete)
	{
		for (auto& e : m_models)
		{
			e->isRemoved = true;
		}

		m_models.clear();
	}
}