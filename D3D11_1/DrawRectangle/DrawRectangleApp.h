#pragma once
#include <d3d11.h>

#include "../Common/GameApp.h"

// 1. NDC좌표계 좌표와 컬러값을 갖는 Vertex를 만들고 IndexBuffer를 사용해서 사각형 그리기
// https://learn.microsoft.com/en-us/windows/win32/direct3d11/overviews-direct3d-11-resources-buffers-intro
class DrawRectangleApp : public GameApp
{
public:
	DrawRectangleApp(HINSTANCE instance);
	~DrawRectangleApp();

	ID3D11Device* m_pDevice = nullptr;
	ID3D11DeviceContext* m_pDeviceContext = nullptr;
	IDXGISwapChain* m_pSwapChain = nullptr;
	ID3D11RenderTargetView* m_pRenderTargetView = nullptr;

	ID3D11VertexShader* m_pVertexShader = nullptr;		// 정점 셰이더
	ID3D11PixelShader* m_pPixelShader = nullptr;		// 픽셀 셰이더
	ID3D11InputLayout* m_pInputLayout = nullptr;		// 입력 레이아웃
	ID3D11Buffer* m_pVertexBuffer = nullptr;			// 버텍스 버퍼
	UINT m_VertexBufferStride = 0;						// 버텍스 하나의 크기
	UINT m_VertexBufferOffset = 0;						// 버텍스 버퍼의 오프셋
	UINT m_VertexCount = 0;								// 버텍스 개수
	ID3D11Buffer* m_pIndexBuffer = nullptr;				// 인덱스 버퍼
	UINT m_IndexCount = 0;								// 인덱스 개수

	virtual bool Initialize(UINT Width, UINT Height);	// 윈도우 정보는 게임 마다 다를수 있으므로 등록,생성,보이기만 한다.
	virtual void OnUpdate();
	virtual void OnRender();

	bool InitD3D();
	void UninitD3D();

	bool InitScene();
	void UninitScene();	
};

