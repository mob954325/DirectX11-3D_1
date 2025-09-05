#pragma once
#include <d3d11.h>
#include <dxgi1_2.h>
#include <wrl/client.h>

#include "../Common/GameApp.h"

using namespace Microsoft::WRL;

class TutorialApp : public GameApp
{
public:
	TutorialApp(HINSTANCE hInstance);
	~TutorialApp();

	ComPtr<ID3D11Device> m_pDevice = nullptr;						// 디바이스
	ComPtr<ID3D11DeviceContext> m_pDeviceContext = nullptr;			// 디바이스 컨텍스트
	ComPtr<IDXGISwapChain1> m_pSwapChain = nullptr;					// 스왑체인 
	ComPtr<ID3D11RenderTargetView> m_pRenderTargetView = nullptr;	// 랜더 타겟

	virtual bool Initialize(UINT Width, UINT Height);	// 윈도우 정보는 게임 마다 다를수 있으므로 등록,생성,보이기만 한다.
	virtual void Update();
	virtual void Render();

	bool InitD3D();
};