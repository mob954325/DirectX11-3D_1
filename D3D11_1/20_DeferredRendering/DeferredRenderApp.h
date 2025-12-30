#pragma once
#include "SkeletalModel.h"

// DirectX11 
#include <d3d11.h>
#include <dxgi1_2.h>
#include <wrl/client.h>

// DirectXTK
#include <directxtk/SimpleMath.h>
#include <directxtk/CommonStates.h> // https://github.com/microsoft/DirectXTK/wiki/CommonStates
#include <directxtk/Effects.h>		// https://github.com/microsoft/DirectXTK/wiki/BasicEffect

// imgui
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"
#include "imgui.h"

// other files
#include "../Common/GameApp.h"
#include "DebugDraw.h"
#include <deque>

using namespace DirectX::SimpleMath;
using namespace Microsoft::WRL;

class DeferredRenderApp : public GameApp
{
public:
	DeferredRenderApp(HINSTANCE hInstance);
	~DeferredRenderApp();

	// Model
	unique_ptr<SkeletalModel> m_chara = nullptr;	
	unique_ptr<SkeletalModel> m_ground = nullptr;	 
	unique_ptr<SkeletalModel> m_tree = nullptr;	 
	unique_ptr<SkeletalModel> m_sphere = nullptr;	 
	deque<unique_ptr<SkeletalModel>> m_models;

	// 렌더링 파이프라인을 구성하는 필수 객체 인터페이스
	ComPtr<ID3D11PixelShader>		m_pixelShader = nullptr;				// 사용할 픽셀 셰이더
	ComPtr<ID3D11Device>			m_device = nullptr;						// 디바이스
	ComPtr<ID3D11DeviceContext>		m_deviceContext = nullptr;			// 디바이스 컨텍스트
	ComPtr<IDXGISwapChain1>			m_swapChain = nullptr;					// 스왑체인 
	ComPtr<ID3D11RenderTargetView>	m_backbufferRTV = nullptr;	// 랜더 타겟
	ComPtr<ID3D11DepthStencilView>	m_depthStencilView = nullptr;	// 깊이 값 처리를 위한 뎊스스텐실 뷰
	ComPtr<ID3D11DepthStencilState> m_depthStencilStateWriteOn = nullptr;
	ComPtr<ID3D11DepthStencilState> m_depthStencilStateWriteOff = nullptr;
	ComPtr<ID3D11RasterizerState>	m_rasterizerState = nullptr;
	ComPtr<ID3D11RasterizerState>	m_transparentRasterizerState = nullptr;

	// vertex shaderes
	ComPtr<ID3D11VertexShader> m_rigidMeshVertexShader = nullptr;
	ComPtr<ID3D11VertexShader> m_skinnedMeshVertexShader = nullptr;


	// 렌더링 파이프라인에 적용하는 객체와 정보
	ComPtr<ID3D11InputLayout> m_inputLayout = nullptr;			// 입력 레이아웃
	ComPtr<ID3D11Buffer> m_constantBuffer = nullptr;			// 상수 버퍼
	ComPtr<ID3D11Buffer> m_materialBuffer{};					// 재질 버퍼

	// 리소스 객체
	ComPtr<ID3D11SamplerState> m_samplerLinear;	// 샘플링 객체

	// 좌표계 변환을 위한 행렬 모음
	Matrix m_World;				// 월드 좌표계 공간으로 변환을 위한 행렬, origin 위치에 있는 큐브 행렬
	Matrix m_View;				// 뷰 좌표계 공간으로 변환을 위한 행렬.
	Matrix m_Projection;		// 단위 장치 좌표계 ( Normalized Device Coordinate) 공간으로 변환을 위한 행렬.

	Vector4 m_LightDirection{};				// Directional Light의 방향
	Color m_LightColor{ 1,1,1,1 };			// Directional Light의 색

	// imgui 컨트롤 변수
	Vector3 m_charaPosition{};
	Vector3 m_charaRotation{};
	Vector3 m_charaScale{ 1.0f,1.0f,1.0f };

	// 카메라
	Vector3 m_CameraPositionInitial{ 0.0f, 100.0f, -200.0f };
	Vector3 m_CameraRotation{};

	// 빛
	Vector4 m_LightDirectionInitial{ 0.0f, -1.0f, 0.0f, 1.0f };
	Vector4 m_LightAmbient{ 0.1f, 0.1f, 0.1f, 0.1f }; // 환경광 반사 계수
	Vector4 m_LightDiffuse{ 0.9f, 0.9f, 0.9f, 1.0f }; // 난반사 계수
	Vector4 m_LightSpecular{ 0.9f, 0.9f, 0.9f, 1.0f }; // 정반사 계수
	FLOAT m_Shininess = 40.0f; // 광택 지수

	float m_Near = 0.01f;
	float m_Far = 3000.0f;
	float m_PovAngle = XM_PIDIV2;

	// 그림자
	ComPtr<ID3D11VertexShader> m_shadowMapVS;			//  shadowMap에 사용할 vs
	ComPtr<ID3D11PixelShader> m_shadowMapPS = nullptr;

	ComPtr<ID3D11Texture2D> m_shadowMap;				// 깊이 값을 기록할 텍스처
	ComPtr<ID3D11DepthStencilView> m_shadowMapDSV;		// 깊이 값 기록을 설정할 객체
	ComPtr<ID3D11ShaderResourceView> m_shadowMapSRV;	// 깊이 버퍼를 슬롯에서 설정하고 사용하기 위한 객체

	Matrix m_shadowView{};
	Matrix m_shadowProj{};
	Vector3 m_shadowLookAt{};
	Vector3 m_shadowPos{};
	float m_shadowFrustumAngle = XM_PIDIV4;

	Viewport m_shadowViewport = { 0, 0, 8192, 8192, 0.0f, 1.0f }; // x, y, width, height, min, max
	D3D11_VIEWPORT m_RenderViewport = {};
	float m_shadowForwardDistFromCamera = 1.0f;
	float m_shadowNear = 400.0f;
	float m_shadowFar = 3000.0f;
	const float m_shadowMinNear = 400.0f;
	const float m_shadowMinFar = 1001.0f;

	// 그림자 디버그
	ComPtr<ID3D11InputLayout> m_debugDrawInputLayout = nullptr; // debug inputlayout
	unique_ptr<DirectX::CommonStates> m_states;	// CommonState : provide the stock? state objects
	unique_ptr<PrimitiveBatch<VertexPositionColor>> m_batch; // vertexPositionColor : proivde input layout
	unique_ptr<DirectX::BasicEffect> m_effect;	// BasicEffect : provide the vertex and pixel shader progrmas

	Vector3 m_shadowUpDistFromLookAt{ 1000, 1000, 1000 };
	Vector3 m_GroundScale{ 20,1,20 };

	// model buffer
	ComPtr<ID3D11Buffer> m_transformBuffer{};
	ComPtr<ID3D11Buffer> m_bonePoseBuffer{};
	ComPtr<ID3D11Buffer> m_boneOffsetBuffer{};

	// dxgi들
	ComPtr<IDXGIDevice3> dxgiDevice3{};
	ComPtr<IDXGIFactory6> dxgiFactory6{};
	ComPtr<IDXGIAdapter1> dxgiAdapter1{};
	ComPtr<IDXGIAdapter3> dxgiAdapter3{};

	// 스카이 박스
	ComPtr<ID3D11ShaderResourceView> m_skyboxTexture;

	ComPtr<ID3D11VertexShader> m_skyboxVS = nullptr;				// 스카이 박스용 정점 셰이더
	ComPtr<ID3D11PixelShader> m_skyboxPS = nullptr;					// 스카이 박스용 픽셀 셰이더
	ComPtr<ID3D11InputLayout> m_skyboxInputLayout = nullptr;		// 입력 레이아웃
	ComPtr<ID3D11Buffer> m_skyboxVertexBuffer = nullptr;			// 스카이 박스 정점 버퍼
	UINT m_SkyboxVertexBufferStride = 0;							// 스카이 박스 정점 하나의 버퍼 크기
	UINT m_SkyboxVertexBufferOffset = 0;							// 스카이 박스 정점 버퍼의 오프셋
	ComPtr<ID3D11Buffer> m_skyboxIndexBuffer;						// 스카이 박스가 사용할 인덱스 버퍼
	int m_nSkyboxIndices = 0;										// 스카이박스 인덱스 버퍼 개수

	ComPtr<ID3D11RasterizerState> m_skyRasterizerState = nullptr;	// 스카이박스 래스터라이저 상태
	ComPtr<ID3D11DepthStencilState> m_skyDepthStencilState = nullptr;	// 스카이 박스를 위한 뎊스스텐실 상태 개체

	// PBR
	ComPtr<ID3D11PixelShader> m_PBRPS = nullptr;
	float roughness = 0.1;
	float metalness = 0.5;
	float lightIntensity = 1.0f;

	Color BaseColor{ 0,0,0,1 };
	bool useBaseColor = true; // NOTE: 텍스처 없는 오브젝트는 강제로 플래그 활성화되서 출력이 안될 수 있음.
	bool useNormal = true;
	bool useMetalness = true;
	bool useRoughness = true;

	// PBR IBL
	ComPtr<ID3D11ShaderResourceView> m_IBLIrradiance;
	ComPtr<ID3D11ShaderResourceView> m_IBLSpecular;
	ComPtr<ID3D11ShaderResourceView> m_IBLLookUpTable;

	// Quad
	DXGI_FORMAT m_HDRFormat = DXGI_FORMAT_R10G10B10A2_UNORM;
	ComPtr<IDXGIFactory2> m_factory;

	ComPtr<ID3D11InputLayout> m_quadInputLayout;
	ComPtr<ID3D11VertexShader> m_quadVertexShader;
	ComPtr<ID3D11Buffer> m_quadVertexBuffer;
	ComPtr<ID3D11Buffer> m_quadIndexBuffer;
	ComPtr<ID3D11Buffer> m_lightDirectionBuffer;

	UINT m_quadVertexBufferStride;
	UINT m_quadVertexBufferOffset;
	UINT m_quadIndicesCount;

	void CreateQuad();

	// Deferred Rendering
	enum class EGbuffer
	{
		BaseColor = 0,
		Normal,
		WorldPos,
		Matal,
		Rough,
		Specular,
		Emission,
		Count
	};

	int gbufferCount = 0;

	ComPtr<ID3D11SamplerState> m_samplerPoint{}; // ??

	std::vector<ComPtr<ID3D11RenderTargetView>> m_gBufferRTV; 
	std::vector<ComPtr<ID3D11ShaderResourceView>> m_gBufferSRV;
	std::vector<ComPtr<ID3D11Texture2D>> m_gBufferTextures;

	ComPtr<ID3D11BlendState> m_additiveBlendState{};

	ComPtr<ID3D11VertexShader> m_directionalLightVS{};
	ComPtr<ID3D11PixelShader> m_directionalLightPS{};

	void CreateGbuffers();
	
	// =============================================================
	virtual bool OnInitialize();
	virtual void OnUpdate();
	virtual void OnRender();

	void DepthOnlyPass();				// 그림자 그리는 패스
	void RenderPassGBuffer();			// 디퍼드 렌더링을 위한 Gbuffer 패스
	void RenderPassDirectionalLight();	// directional light에 대해 추가하는 패스
	void SkyboxPass();					// Skybox 그리는 패스


	bool InitImGUI();
	void RenderImGUI();
	void UninitImGUI();

	bool InitD3D();
	bool InitScene();
	bool InitEffect();
	void InitDebugDraw();	// 디버그 관련 초기화 함수
	void InitShdowMap();	// ShadowMap 관련 초기화 함수
	bool InitDxgi();		// 메모리 사용량 디버그용 dxgi
	bool InitSkyBox();
	void CreateSwapchain();

	void DrawFrustum(Matrix worldMat, Matrix viewMat, Matrix proejctionMat,
		float angle, float AspectRatio, float nearZ, float farZ, XMVECTORF32 color = Colors::Red); // 절두체 그리는 함수

	void ResetValues();
	void ShowMatrix(const DirectX::XMFLOAT4X4& mat, const char* label = "Matrix");

	virtual LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	void OnInputProcess(const Keyboard::State& KeyState, const Keyboard::KeyboardStateTracker& KeyTracker,
		const Mouse::State& MouseState, const Mouse::ButtonStateTracker& MouseTracker) override;
};