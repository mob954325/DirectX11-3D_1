﻿#pragma once
#include <d3d11.h>
#include <dxgi1_2.h>

#include "../Common/GameApp.h"

class TutorialApp : public GameApp
{
public:
	TutorialApp(HINSTANCE hInstance);
	~TutorialApp();

	ID3D11Device* m_pDevice = nullptr;						// 디바이스
	ID3D11DeviceContext* m_pDeviceContext = nullptr;		// 디바이스 컨텍스트
	IDXGISwapChain1* m_pSwapChain = nullptr;				// 스왑체인 
	ID3D11RenderTargetView* m_pRenderTargetView = nullptr;	// 랜더 타겟

	virtual bool Initialize(UINT Width, UINT Height);	// 윈도우 정보는 게임 마다 다를수 있으므로 등록,생성,보이기만 한다.
	virtual void Update();
	virtual void Render();

	bool InitD3D();
	void UninitD3D();
};