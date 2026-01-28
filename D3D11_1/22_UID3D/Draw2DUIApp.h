#pragma once
#include <string>

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

// DirectX11 2D
#include <d2d1_1.h>
#pragma comment(lib, "d2d1.lib")

#include <dwrite.h>		// DirectWrite 사용 : 문자 관련 렌더링 및 설정
#pragma comment(lib, "dwrite.lib")

#include <wincodec.h>	// WIC 디코더 : 이미지 구성요소에 대한 설정 
#pragma comment(lib, "windowscodecs.lib")

#include "UIComps/Image.h"
#include "UIComps/Canvas.h"

using namespace DirectX::SimpleMath;
using namespace Microsoft::WRL;

struct CubeVertex
{
	Vector4 pos;

	Vector2 tex;
	Vector2 pad2{};
	CubeVertex(Vector3 p, Vector2 t) : pos(p.x, p.y, p.z, 1.0f), tex(t) {}
};

struct PerObjectCB
{
	Matrix WVP; // ??View Projection
};

class Draw2DUIApp : public GameApp
{
public:
	Draw2DUIApp(HINSTANCE hInstance);
	~Draw2DUIApp();

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

	// 좌표계 변환을 위한 행렬 모음
	Matrix m_World1;			// 월드 좌표계 공간으로 변환을 위한 행렬, origin 위치에 있는 큐브 행렬
	Matrix m_World2;			// m_World1를 부모로하는 큐브 행렬
	Matrix m_World3;			// m_World2를 부모로하는 큐브 행렬
	Matrix m_View;				// 뷰 좌표계 공간으로 변환을 위한 행렬.
	Matrix m_Projection;		// 단위 장치 좌표계 ( Normalized Device Coordinate) 공간으로 변환을 위한 행렬.

	// imgui 컨트롤 변수 -> position만 다룸
	Vector3 m_World1Position{};
	Vector3 m_World1Rotation{};
	Vector3 m_World1Scale{ 1.0f,1.0f,1.0f };
	Vector3 m_World1PositionInitial{ 0.0f, 0.0f, 0.0f };

	Vector3 m_World2Position{};
	Vector3 m_World2Rotation{};
	Vector3 m_World2Scale{ 0.8f,0.8f,0.8f };
	Vector3 m_World2PositionInitial{ -5.0f, 0.0f, 0.0f };

	Vector3 m_World3Position{};
	Vector3 m_World3Rotation{};
	Vector3 m_World3Scale{ 0.5f,0.5f,0.5f };
	Vector3 m_World3PositionInitial{ -10.0f, 0.0f, 0.0f };

	Vector3 m_CameraPositionInitial{ 0.0f, 0.0f, -30.0f };
	Vector3 m_CameraRotation{};

	float m_Near = 0.01f;
	float m_Far = 100.0f;
	float m_PovAngle = XM_PIDIV2;

	virtual bool OnInitialize();
	virtual void OnUpdate();
	virtual void OnRender();

	bool InitImGUI();
	void RenderImGUI();
	void UninitImGUI();

	bool InitD3D();
	bool InitScene();

	void ResetValues();

	ComPtr<ID3D11ShaderResourceView>	m_D2DTexture{};		// ??

	ComPtr<ID3D11Buffer>				m_D2DVertBuffer{};
	ComPtr<ID3D11Buffer>				m_D2DIndexBuffer{};

	ComPtr<ID3D11Buffer>				m_perObjCB{};

	ComPtr<ID3D11BlendState>			m_TransparencyBS{};
	ComPtr<ID3D11RasterizerState>		m_CWcullModeRS{};
	ComPtr<ID3D11SamplerState>			m_SamplerState{};

	ComPtr<ID3D11VertexShader>			m_d2dVertexShader{};
	ComPtr<ID3D11PixelShader>			m_d2dPixelShader{};

	PerObjectCB m_perObjData{};
	Matrix m_WVP{};

	// UI 컴포넌트 모음
	Canvas canvas;
	std::vector<Image> imgs;

	virtual LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
};