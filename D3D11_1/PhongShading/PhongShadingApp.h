#pragma once

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

class PhongShadingApp : public GameApp
{
public:
	PhongShadingApp(HINSTANCE hInstance);
	~PhongShadingApp();

	// 렌더링 파이프라인을 구성하는 필수 객체 인터페이스
	ComPtr<ID3D11Device> m_pDevice = nullptr;						// 디바이스
	ComPtr<ID3D11DeviceContext> m_pDeviceContext = nullptr;			// 디바이스 컨텍스트
	ComPtr<IDXGISwapChain1> m_pSwapChain = nullptr;					// 스왑체인 
	ComPtr<ID3D11RenderTargetView> m_pRenderTargetView = nullptr;	// 랜더 타겟
	ComPtr<ID3D11DepthStencilView> m_pDepthStencilView = nullptr;	// 깊이 값 처리를 위한 뎊스스텐실 뷰

	// 렌더링 파이프라인에 적용하는 객체와 정보
	ComPtr<ID3D11VertexShader> m_pVertexShader = nullptr;		// 정점 쉐이더
	ComPtr<ID3D11PixelShader> m_pPixelShader = nullptr;			// 픽셀 쉐이더
	ComPtr<ID3D11InputLayout> m_pInputLayout = nullptr;			// 입력 레이아웃
	ComPtr<ID3D11Buffer> m_pVertexBuffer = nullptr;				// 정점 버퍼
	UINT m_VertexBufferStride = 0;								// 정점 하나의 버퍼 크기
	UINT m_VertexBufferOffset = 0;								// 정점 버퍼의 오프셋
	ComPtr<ID3D11Buffer> m_pIndexBuffer = nullptr;				// 인덱스 버퍼
	int m_nIndices = 0;											// 인덱스 버퍼 개수
	ComPtr<ID3D11Buffer> m_pConstantBuffer = nullptr;			// 상수 버퍼

	ComPtr<ID3D11ShaderResourceView> m_pTexture;	// 매핑할 텍스처 객체 -> seafloor.dds
	ComPtr<ID3D11ShaderResourceView> m_pTextureRV2;	// WoodCrate.dds
	ComPtr<ID3D11ShaderResourceView> m_pTextureRV3;	// cubemap.dds
	ComPtr<ID3D11SamplerState> m_pSamplerLinear;	// 샘플링 객체

	// 좌표계 변환을 위한 행렬 모음
	Matrix m_World;				// 월드 좌표계 공간으로 변환을 위한 행렬, origin 위치에 있는 큐브 행렬
	Matrix m_View;				// 뷰 좌표계 공간으로 변환을 위한 행렬.
	Matrix m_Projection;		// 단위 장치 좌표계 ( Normalized Device Coordinate) 공간으로 변환을 위한 행렬.

	Vector4 m_LightDirection;				// Directional Light의 방향
	Color m_LightColor{ 1,1,1,1 };			// Directional Light의 색

	// 빛 관련
	ComPtr<ID3D11PixelShader> m_pSolidPixelShader = nullptr; // 빛 위치 랜더링용
	ComPtr<ID3D11PixelShader> m_pPhongShader = nullptr; // Phong Shader
	ComPtr<ID3D11PixelShader> m_pBlinnPhongShader = nullptr; // Blinn-Phong Shader

	// imgui 컨트롤 변수
	// 큐브
	Vector3 m_CubePosition{};
	Vector3 m_CubeRotation{};
	Vector3 m_CubeScale{ 1.0f,1.0f,1.0f };
	Vector3 m_CubePositionInitial{ 0.0f, 0.0f, 0.0f };

	Vector4 m_CubeAmbient{ 1.0f, 1.0f, 1.0f, 1.0f }; // 환경광 반사 계수
	Vector4 m_CubeDiffuse{ 1.0f, 1.0f, 1.0f, 1.0f }; // 난반사 계수 -> Texture로 대체 가능?
	Vector4 m_CubeSpecular{ 1.0f, 1.0f, 1.0f, 1.0f }; // 정반사 계수

	// 카메라
	Vector3 m_CameraPositionInitial{ 0.0f, 0.0f, -20.0f };
	Vector3 m_CameraRotation{};

	// 빛
	ComPtr<ID3D11Buffer> m_pMaterialBuffer = nullptr;
	Vector4 m_LightDirectionInitial{ 0, 0, 1, 1.0f };
	Vector4 m_LightAmbient{ 0.1f, 0.1f, 0.1f, 0.1f }; // 환경광 반사 계수
	Vector4 m_LightDiffuse{ 0.9f, 0.9f, 0.9f, 1.0f }; // 난반사 계수 -> Texture로 대체 가능?
	Vector4 m_LightSpecular{ 0.9f, 0.9f, 0.9f, 1.0f }; // 정반사 계수
	FLOAT m_Shininess = 1000.0f; // 광택 지수
	bool isBlinnPhong = false;

	float m_Near = 0.01f;
	float m_Far = 100.0f;
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

	void ResetValues();

	virtual LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
};