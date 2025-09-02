#pragma once
#include <d3d11.h>
#include <dxgi1_2.h>

#include "../Common/GameApp.h"

class DrawTriangleApp : public GameApp
{
public:
	DrawTriangleApp(HINSTANCE hInstance);
	~DrawTriangleApp();

	// 렌더링 파이프라인을 구성하는 필수 객체 인터페이스 ( 뎁스 스텐실 뷰는 사용 안함 )
	ID3D11Device* m_pDevice = nullptr;						// 디바이스
	ID3D11DeviceContext* m_pDeviceContext = nullptr;		// 디바이스 컨텍스트
	IDXGISwapChain* m_pSwapChain = nullptr;					// 스왑체인 
	ID3D11RenderTargetView* m_pRenderTargetView = nullptr;	// 랜더 타겟

	// 렌더링 파이프라인에 적용하는 객체 정보
	ID3D11VertexShader* m_pVertexShader = nullptr;		// 정점 셰이더
	ID3D11PixelShader* m_pPixelShader = nullptr;		// 픽셀 셰이더
	ID3D11InputLayout* m_pInputLayout = nullptr;		// 입력 레이아웃
	ID3D11Buffer* m_pVertexBuffer = nullptr;			// 버텍스 버퍼
	UINT m_VertexBufferStride = 0;						// 버텍스 하나의 크기
	UINT m_VertexBufferOffset = 0;						// 버텍스 버퍼의 오프셋
	UINT m_VertexCount = 0;								// 버텍스 개수

	virtual bool Initialize(UINT Width, UINT Height);	// 윈도우 정보는 게임 마다 다를수 있으므로 등록,생성,보이기만 한다.
	virtual void Update();
	virtual void Render();

	bool InitD3D();
	void UninitD3D();

	bool InitScene();
	void UninitScene();
};