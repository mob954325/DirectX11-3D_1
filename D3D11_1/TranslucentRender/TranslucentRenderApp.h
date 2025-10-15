#pragma once
#include "ModelLoader.h"

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

class TranslucentRenderApp : public GameApp
{
public:
	TranslucentRenderApp(HINSTANCE hInstance);
	~TranslucentRenderApp();

	// Model
	unique_ptr<ModelLoader> m_pTree1 = nullptr;		// 
	unique_ptr<ModelLoader> m_pCube1 = nullptr;		// 반투명
	unique_ptr<ModelLoader> m_pCube2 = nullptr;		// 불투명

	// 렌더링 파이프라인을 구성하는 필수 객체 인터페이스
	ComPtr<ID3D11VertexShader>		m_pVertexShader = nullptr;			// 사용할 정점 셰이더
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
	ComPtr<ID3D11PixelShader> m_pPhongShader = nullptr;				// Phong PS
	ComPtr<ID3D11PixelShader> m_pBlinnPhongShader = nullptr;		// Blinn-Phong PS
	ComPtr<ID3D11PixelShader> m_pTranslucentPixelShader = nullptr;	// 반투명으로 만드는 픽셀 세이더


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
	Vector3 m_Cube1Position{};
	Vector3 m_Cube1Rotation{};
	Vector3 m_Cube1Scale{ 10.0f,10.0f,10.0f };
	Vector3 m_Cube1PositionInitial{ 0.0f, 0.0f, 10.0f };

	Vector3 m_Cube2Position{};
	Vector3 m_Cube2Rotation{};
	Vector3 m_Cube2Scale{ 50.0f,50.0f,50.0f };
	Vector3 m_Cube2PositionInitial{ 0.0f, 0.0f, 130.0f };

	Vector3 m_TreePosition{};
	Vector3 m_TreeRotation{};
	Vector3 m_TreeScale{ 100.0f, 100.0f, 100.0f };
	Vector3 m_TreePositionInitial{ 0.0f, -50.0f, 0.0f };

	// 카메라
	Vector3 m_CameraPositionInitial{ 0.0f, 0.0f, -20.0f };
	Vector3 m_CameraRotation{};

	// 빛
	Vector4 m_LightDirectionInitial{ 0.0f, 0.0f, -1.0f, 1.0f };
	Vector4 m_LightAmbient{ 0.1f, 0.1f, 0.1f, 0.1f }; // 환경광 반사 계수
	Vector4 m_LightDiffuse{ 0.9f, 0.9f, 0.9f, 1.0f }; // 난반사 계수
	Vector4 m_LightSpecular{ 0.9f, 0.9f, 0.9f, 1.0f }; // 정반사 계수
	FLOAT m_Shininess = 40.0f; // 광택 지수
	bool isBlinnPhong = true;	

	float m_Near = 0.01f;
	float m_Far = 1000.0f;
	float m_PovAngle = XM_PIDIV2;

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