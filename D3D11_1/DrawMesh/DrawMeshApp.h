#pragma once

// DirectX11 
#include <d3d11.h>
#include <dxgi1_2.h>
#include <wrl/client.h>

// imgui
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"
#include "imgui.h"

// other files
#include "../Common/GameApp.h"

using namespace Microsoft::WRL;

class DrawMeshApp : public GameApp
{
public:
	DrawMeshApp(HINSTANCE hInstance);
	~DrawMeshApp();

	ComPtr<ID3D11Device> m_pDevice = nullptr;						// 디바이스
	ComPtr<ID3D11DeviceContext> m_pDeviceContext = nullptr;			// 디바이스 컨텍스트
	ComPtr<IDXGISwapChain1> m_pSwapChain = nullptr;					// 스왑체인 
	ComPtr<ID3D11RenderTargetView> m_pRenderTargetView = nullptr;	// 랜더 타겟

	virtual bool OnInitialize();	// 윈도우 정보는 게임 마다 다를수 있으므로 등록,생성,보이기만 한다.
	virtual void OnUpdate();
	virtual void OnRender();

	bool InitImGUI();
	void RenderImGUI();
	void UninitImGUI();

	bool InitD3D();

	virtual LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
};