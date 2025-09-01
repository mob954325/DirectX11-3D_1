#pragma once
#include <d3d11.h>

#include "../Common/GameApp.h"

class TutorialApp : public GameApp
{
public:
	TutorialApp(HINSTANCE hInstance);
	~TutorialApp();

	ID3D11Device* m_pDevice = nullptr;					// 디바이스
	ID3D11DeviceContext* m_pDeviceContext = nullptr;	// 디바이스 컨텍스트
	// IDXGISwapChain1* m_pSwapChain = nullptr;					// 스왑체인 // ???
};