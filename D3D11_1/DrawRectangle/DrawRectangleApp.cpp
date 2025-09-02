#include "DrawRectangleApp.h"
#include "../Common/Helper.h"

#include "directxtk/SimpleMath.h"
#include <dxgi1_3.h>
#include <d3dcompiler.h>
#include <wrl/client.h>

#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "dxgi.lib")
#pragma comment (lib, "d3dCompiler.lib")

struct Vertex
{
	Vector3 position;	// NDC 좌표계 좌표
	Color color;		// 컬러값
};

DrawRectangleApp::DrawRectangleApp(HINSTANCE instance)
	: GameApp(instance)
{
}

DrawRectangleApp::~DrawRectangleApp()
{
	UninitScene();
	UninitD3D();
}

bool DrawRectangleApp::Initialize(UINT Width, UINT Height)
{
	__super::Initialize(Width, Height);

	if (!InitD3D())
		return false;

	if (!InitScene())
		return false;

	return true;
}

void DrawRectangleApp::Update()
{
}

void DrawRectangleApp::Render()
{
	Color color(0.1f, 0.2f, 0.3f, 1.0f);

	// 화면
	m_pDeviceContext->ClearRenderTargetView(m_pRenderTargetView, color);

	// 랜더 파이프라인 스테이지 설정
	m_pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);	// 정점을 이어서 그릴 방식 설정 -> TriangleList
	m_pDeviceContext->IASetVertexBuffers(0, 1, &m_pVertexBuffer, &m_VertexBufferStride, &m_VertexBufferOffset);
	m_pDeviceContext->IASetInputLayout(m_pInputLayout);
	m_pDeviceContext->VSSetShader(m_pVertexShader, nullptr, 0);
	m_pDeviceContext->PSSetShader(m_pPixelShader, nullptr, 0);

	m_pDeviceContext->IASetIndexBuffer(m_pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);

	// 그리기
	m_pDeviceContext->DrawIndexed(m_IndexCount, 0, 0);

	// 스왑체인 교체
	m_pSwapChain->Present(0, 0);
}

bool DrawRectangleApp::InitD3D()
{
	HRESULT hr = S_OK;

	// 스왑체인 속성
	DXGI_SWAP_CHAIN_DESC swapDesc = {};
	swapDesc.BufferCount = 1;
	swapDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapDesc.OutputWindow = m_hWnd;
	swapDesc.Windowed = true;
	swapDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	// 백버퍼(텍스처)의 가로/세로 크기 설정.
	swapDesc.BufferDesc.Width = m_ClientWidth;
	swapDesc.BufferDesc.Height = m_ClientHeight;
	// 화면 주사율 설정.
	swapDesc.BufferDesc.RefreshRate.Numerator = 60;
	swapDesc.BufferDesc.RefreshRate.Denominator = 1;
	// 샘플링 관련 설정.
	swapDesc.SampleDesc.Count = 1;
	swapDesc.SampleDesc.Quality = 0;

	UINT creationFlags = 0;
#ifdef _DEBUG
	creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif // _DEBUG

	// 1. 장치, 스왑체인, 디바이스 컨텍스트 생성.
	HR_T(D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, creationFlags, NULL, NULL,
		D3D11_SDK_VERSION, &swapDesc, &m_pSwapChain, &m_pDevice, NULL, &m_pDeviceContext));

	// 2. 렌더 타겟 뷰 생성
	ID3D11Texture2D* pBackBufferTexture = nullptr;
	HR_T(m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBufferTexture));
	HR_T(m_pDevice->CreateRenderTargetView(pBackBufferTexture, NULL, &m_pRenderTargetView));
	SAFE_RELEASE(pBackBufferTexture);

	// 랜더 타겟 바인딩
	m_pDeviceContext->OMSetRenderTargets(1, &m_pRenderTargetView, NULL);

	// 뷰포트 설정
	D3D11_VIEWPORT viewport = {};
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = (float)m_ClientWidth;
	viewport.Height = (float)m_ClientHeight;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	m_pDeviceContext->RSSetViewports(1, &viewport);

	return true;
}

void DrawRectangleApp::UninitD3D()
{
	SAFE_RELEASE(m_pRenderTargetView);
	SAFE_RELEASE(m_pDeviceContext);
	SAFE_RELEASE(m_pSwapChain);
	SAFE_RELEASE(m_pDevice);
}

bool DrawRectangleApp::InitScene()
{
	HRESULT hr = S_OK;
	ID3D10Blob* errorMessage = nullptr; // 컴파일 에러 메세지가 저장될 버퍼

	//      /---------------------(1,1,1)   z값은 깊이값
	//     /                      / |   
	// (-1,1,0)----------------(1,1,0)        
	//   |        v1---------v2 |   |
	//   |       /           /  |   |       중앙이 (0,0,0)  
	//   |      /  +        /   |   |
	//   |     /           /    |   |
	//   |    v0-----------v3   |   /
	// (-1,-1,0)-------------(1,-1,0)
	Vertex vertices[] =
	{
		{Vector3(-0.5,-0.5,0.5) , Color(1.0, 0.0, 0.0, 1.0)},
		{Vector3(-0.5,0.5,0.5),	  Color(0.0, 1.0, 0.0, 1.0)},
		{Vector3(0.5,0.5,0.5)   , Color(0.0, 0.0, 1.0, 1.0)},
		{Vector3(0.5,-0.5, 0.5) , Color(1.0, 1.0, 0.0, 1.0)}
	};

	// vertex Buffer Desc
	D3D11_BUFFER_DESC vbDesc = {}; 
	m_VertexCount = ARRAYSIZE(vertices);
	vbDesc.ByteWidth = sizeof(Vertex) * m_VertexCount;	// 버텍스 버퍼의 크기 (Byte).
	vbDesc.CPUAccessFlags = 0;							// 0 == CPU 엑세스 필요하지 않음
	vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;	   // 정적 버퍼로 사용.
	vbDesc.MiscFlags = 0;
	vbDesc.Usage = D3D11_USAGE_DEFAULT;					// CPU 접근 불가, GPU에서 읽기/쓰기로 가능한 버퍼로 생성

	// 정점 버퍼 생성.
	D3D11_SUBRESOURCE_DATA vbData = {};
	vbData.pSysMem = vertices;	// 버퍼를 생성할때 복사할 데이터의 주소 설정 
	HR_T(hr = m_pDevice->CreateBuffer(&vbDesc, &vbData, &m_pVertexBuffer));

	// 버텍스 버퍼 정보 
	m_VertexBufferStride = sizeof(Vertex); // 버텍스 하나의 크기
	m_VertexBufferOffset = 0;	// 버텍스 시작 주소에서 더할 오프셋 주소

	// 버텍스 셰이더 생성
	ID3DBlob* vertexShaderBuffer = nullptr;
	HR_T(CompileShaderFromFile(L"BasicVertexShader.hlsl", "main", "vs_4_0", &vertexShaderBuffer));
	HR_T(m_pDevice->CreateVertexShader(vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), NULL, &m_pVertexShader));

	// 인풋 레이아웃 생성
	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
	};

	HR_T(hr = m_pDevice->CreateInputLayout(layout, ARRAYSIZE(layout),
		vertexShaderBuffer->GetBufferPointer(),
		vertexShaderBuffer->GetBufferSize(),
		&m_pInputLayout));

	SAFE_RELEASE(vertexShaderBuffer);

	// 픽셀 세이더 생성
	ID3DBlob* pixelShaderBuffer = nullptr;
	HR_T(CompileShaderFromFile(L"BasicPixelShader.hlsl", "main", "ps_4_0", &pixelShaderBuffer));
	HR_T(m_pDevice->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, &m_pPixelShader));

	SAFE_RELEASE(pixelShaderBuffer);

	// 인덱스 버퍼 생성
	unsigned int indices[] = { 0, 1, 2, 0, 2, 3 };

	// Fill in a buffer description.
	D3D11_BUFFER_DESC psDesc = {};
	m_IndexCount = ARRAYSIZE(indices);
	psDesc.ByteWidth = sizeof(unsigned int) * m_IndexCount;
	psDesc.CPUAccessFlags = 0;
	psDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	psDesc.MiscFlags = 0;
	psDesc.Usage = D3D11_USAGE_DEFAULT;
	
	// Define the resource data.
	D3D11_SUBRESOURCE_DATA psData;
	psData.pSysMem = indices;
	psData.SysMemPitch = 0;
	psData.SysMemSlicePitch = 0;

	// Create the buffer with the device.
	HR_T(hr = m_pDevice->CreateBuffer(&psDesc, &psData, &m_pIndexBuffer));

	return true;
}

void DrawRectangleApp::UninitScene()
{
	SAFE_RELEASE(m_pVertexBuffer);
	SAFE_RELEASE(m_pInputLayout);
	SAFE_RELEASE(m_pVertexShader);
	SAFE_RELEASE(m_pPixelShader);
}