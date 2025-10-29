#pragma once
#include "SkeletalModel.h"

// DirectX11 
#include <d3d11.h>
#include <dxgi1_2.h>
#include <wrl/client.h>
#include <directxtk/SimpleMath.h>

// imgui
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"
#include "imgui.h"

// other files
#include "../Common/GameApp.h"

using namespace DirectX::SimpleMath;
using namespace Microsoft::WRL;

class SkinnedAnimationApp : public GameApp
{
public:
	SkinnedAnimationApp(HINSTANCE hInstance);
	~SkinnedAnimationApp();

	// Model
	unique_ptr<SkeletalModel> m_pSillyDance = nullptr;		// 
	unique_ptr<SkeletalModel> m_pSkinnedTest = nullptr;		// 
	unique_ptr<SkeletalModel> m_pRunCharactor = nullptr;		// 

	// 렌더링 파이프라인을 구성하는 필수 객체 인터페이스
	// ComPtr<ID3D11VertexShader>		m_pVertexShader = nullptr;			// 사용할 정점 셰이더
	ComPtr<ID3D11PixelShader>		m_pPixelShader = nullptr;				// 사용할 픽셀 셰이더
	ComPtr<ID3D11Device>			m_pDevice = nullptr;						// 디바이스
	ComPtr<ID3D11DeviceContext>		m_pDeviceContext = nullptr;			// 디바이스 컨텍스트
	ComPtr<IDXGISwapChain1>			m_pSwapChain = nullptr;					// 스왑체인 
	ComPtr<ID3D11RenderTargetView>	m_pRenderTargetView = nullptr;	// 랜더 타겟
	ComPtr<ID3D11DepthStencilView>	m_pDepthStencilView = nullptr;	// 깊이 값 처리를 위한 뎊스스텐실 뷰
	ComPtr<ID3D11BlendState>		m_pBlendState = nullptr;				// 혼합 상태 객체 
	ComPtr<ID3D11DepthStencilState> m_pDepthStencilStateAllMask = nullptr;
	ComPtr<ID3D11DepthStencilState> m_pDepthStencilStateZeroMask = nullptr;
	ComPtr<ID3D11RasterizerState>	m_pRasterizerState = nullptr;
	ComPtr<ID3D11RasterizerState>	m_pTransparentRasterizerState = nullptr;

	// Phong Shader
	ComPtr<ID3D11PixelShader> m_pPhongShader = nullptr;		
	ComPtr<ID3D11PixelShader> m_pBlinnPhongShader = nullptr;
	ComPtr<ID3D11PixelShader> m_pToonShader = nullptr;

	// vertex shaderes
	ComPtr<ID3D11VertexShader> m_pRigidMeshVertexShader = nullptr;	
	ComPtr<ID3D11VertexShader> m_pSkinnedMeshVertexShader = nullptr;	


	// 렌더링 파이프라인에 적용하는 객체와 정보
	ComPtr<ID3D11InputLayout> m_pInputLayout = nullptr;			// 입력 레이아웃
	ComPtr<ID3D11Buffer> m_pConstantBuffer = nullptr;			// 상수 버퍼
	ComPtr<ID3D11Buffer> m_pMaterialBuffer{};					// 재질 버퍼

	// 리소스 객체
	ComPtr<ID3D11SamplerState> m_pSamplerLinear;	// 샘플링 객체

	// 좌표계 변환을 위한 행렬 모음
	Matrix m_World;				// 월드 좌표계 공간으로 변환을 위한 행렬, origin 위치에 있는 큐브 행렬
	Matrix m_View;				// 뷰 좌표계 공간으로 변환을 위한 행렬.
	Matrix m_Projection;		// 단위 장치 좌표계 ( Normalized Device Coordinate) 공간으로 변환을 위한 행렬.

	Vector4 m_LightDirection;				// Directional Light의 방향
	Color m_LightColor{ 1,1,1,1 };			// Directional Light의 색

	// imgui 컨트롤 변수
	Vector3 m_sillyDancePosition{};
	Vector3 m_sillyDanceRotation{};
	Vector3 m_sillyDanceScale{ 1.0f,1.0f,1.0f };

	// 카메라
	Vector3 m_CameraPositionInitial{ 0.0f, 100.0f, -200.0f };
	Vector3 m_CameraRotation{};

	// 빛
	Vector4 m_LightDirectionInitial{ 0.0f, 0.0f, -1.0f, 1.0f };
	Vector4 m_LightAmbient{ 0.1f, 0.1f, 0.1f, 0.1f }; // 환경광 반사 계수
	Vector4 m_LightDiffuse{ 0.9f, 0.9f, 0.9f, 1.0f }; // 난반사 계수
	Vector4 m_LightSpecular{ 0.9f, 0.9f, 0.9f, 1.0f }; // 정반사 계수
	FLOAT m_Shininess = 40.0f; // 광택 지수

	float m_Near = 0.01f;
	float m_Far = 1000.0f;
	float m_PovAngle = XM_PIDIV2;

	int psIndex = 0; // 픽셀 셰이더 구별용 // 0. blinn-phong, 1. phong, 2. toon 

	// =============================================================
	virtual bool OnInitialize();
	virtual void OnUpdate();
	virtual void OnRender();

	bool InitImGUI();
	void RenderImGUI();
	void UninitImGUI();

	bool InitD3D();
	bool InitScene();
	bool InitEffect();

	void ResetValues();

	virtual LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
};