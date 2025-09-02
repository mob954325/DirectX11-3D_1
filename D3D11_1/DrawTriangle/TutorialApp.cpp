#include "TutorialApp.h"
#include "../Common/Helper.h"

#include <directxtk/SimpleMath.h>
#include <dxgidebug.h> // ?
#include <dxgi1_3.h>
#include <wrl/client.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

#include <d3dcompiler.h>
#pragma comment(lib, "d3dcompiler.lib")


using namespace DirectX::SimpleMath;
using Microsoft::WRL::ComPtr;

// vector3 정점 구조체
struct Vertex
{
	Vector3 position;
};

TutorialApp::TutorialApp(HINSTANCE hInstance)
	: GameApp(hInstance)
{

}

TutorialApp::~TutorialApp()
{
	UninitScene();
	UninitD3D();
}

bool TutorialApp::Initialize(UINT Width, UINT Height)
{
	__super::Initialize(Width, Height);

	if (!InitD3D())
		return false;

	if (!InitScene())
		return false;

	return true;
}

void TutorialApp::Update()
{
}

void TutorialApp::Render()
{
	Color color(0.1f, 0.2f, 0.3f, 1.0f);

	// 화면 칠하기.
	m_pDeviceContext->ClearRenderTargetView(m_pRenderTargetView, color);

	// Draw 계열 함수를 호출 하기 전에 렌더링 파이프라인에 필수 스테이지 설정을 해야한다.
	m_pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);	// 정점을 이어서 그릴 방식 설정
	m_pDeviceContext->IASetVertexBuffers(0, 1, &m_pVertexBuffer, &m_VertexBufferStride, &m_VertexBufferOffset);
	m_pDeviceContext->IASetInputLayout(m_pInputLayout);
	m_pDeviceContext->VSSetShader(m_pVertexShader, nullptr, 0);
	m_pDeviceContext->PSSetShader(m_pPixelShader, nullptr, 0);

	// render a triangle
	m_pDeviceContext->Draw(m_VertexCount, 0);

	// 스왑체인 교체
	m_pSwapChain->Present(0, 0);
}

bool TutorialApp::InitD3D()
{
	// 결과값.
	HRESULT hr = 0;

	// 스왑체인 속성 설정 구조체 생성.
	DXGI_SWAP_CHAIN_DESC swapDesc = {};
	swapDesc.BufferCount = 1;
	swapDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapDesc.OutputWindow = m_hWnd;	// 스왑체인 출력할 창 핸들 값.
	swapDesc.Windowed = true;		// 창 모드 여부 설정.
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
#endif
	// 1. 장치 생성.   2.스왑체인 생성. 3.장치 컨텍스트 생성.
	HR_T(D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, creationFlags, NULL, NULL,
		D3D11_SDK_VERSION, &swapDesc, &m_pSwapChain, &m_pDevice, NULL, &m_pDeviceContext));

	// 4. 렌더타겟뷰 생성.  (백버퍼를 이용하는 렌더타겟뷰)	
	ID3D11Texture2D* pBackBufferTexture = nullptr;
	HR_T(m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBufferTexture));
	HR_T(m_pDevice->CreateRenderTargetView(pBackBufferTexture, NULL, &m_pRenderTargetView));  // 텍스처는 내부 참조 증가
	SAFE_RELEASE(pBackBufferTexture);	//외부 참조 카운트를 감소시킨다.
	// 렌더 타겟을 최종 출력 파이프라인에 바인딩합니다.
	m_pDeviceContext->OMSetRenderTargets(1, &m_pRenderTargetView, NULL);

	// 뷰포트 설정.	
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

void TutorialApp::UninitD3D()
{
	SAFE_RELEASE(m_pRenderTargetView);
	SAFE_RELEASE(m_pDeviceContext);
	SAFE_RELEASE(m_pSwapChain);
	SAFE_RELEASE(m_pDevice);
}

bool TutorialApp::InitScene()
{
	HRESULT hr = S_OK;
	ID3D10Blob* errorMessage = nullptr;	// 컴파일 에러 메세지가 저장될 버퍼.

	// 1. Render()에서 파이프 라인에 바인딩할 버텍스 버퍼와 정보 준비
	// 아직 VertexShader의 World, View, Projection 변환을 사용하지 않으므로
	// 직접 Normalized Device Coordinate의 위치로 설정
	//      /---------------------(1,1,1)   z값은 깊이값
	//     /                      / |   
	// (-1,1,0)----------------(1,1,0)        
	//   |        v1           |   |
	//   |       /   \         |   |       중앙이 (0,0,0)  
	//   |      /  +   \       |   |
	//   |     /         \     |   |
	//	 |   v0-----------v2   |  /
	// (-1,-1,0)-------------(1,-1,0)
	Vertex vertices[] =
	{
		Vector3(-0.5,-0.5,0.5), // v0    
		Vector3(0,0.5,0.5),		// v1    
		Vector3(0.5,-0.5,0.5),	// v2		
	};
	
	D3D11_BUFFER_DESC vbDesc = {};
	m_VertexCount = ARRAYSIZE(vertices);				// 정점 개수
	vbDesc.ByteWidth = sizeof(Vertex) * m_VertexCount;	// 버텍스 버퍼의 크기 (Byte).
	vbDesc.CPUAccessFlags = 0;							// 0 == CPU 엑세스 필요하지 않음
	vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;		// 정점 버퍼로 사용.
	vbDesc.MiscFlags = 0;
	vbDesc.Usage = D3D11_USAGE_DEFAULT;					// CPU 접근 불가, GPU에서 읽기/쓰기로 가능한 버퍼로 생성

	// 정점 버퍼 생성
	D3D11_SUBRESOURCE_DATA vbData = {};
	vbData.pSysMem = vertices; // 버퍼를 생성할 때 복사할 데이터의 주소 설정
	HR_T(hr = m_pDevice->CreateBuffer(&vbDesc, &vbData, &m_pVertexBuffer));
	
	// 버텍스 버퍼 정보
	m_VertexBufferStride = sizeof(Vertex);	// 버텍스 하나의 크기 설정
	m_VertexBufferOffset = 0;				// 버텍스 시작 주소에서 더 할 오프셋 주소 

	// 2. Render에서 파이프라인에 바인딩할 버텍스 셰이더 생성
	ID3DBlob* vertexShaderBuffer = nullptr;	// 버텍스 셰이더 HLSL의 컴파일된 결과(바이트 코드)를 담을 수 있는 버퍼 객체
	HR_T(CompileShaderFromFile(L"BasicVertexShader.hlsl", "main", "vs_4_0", &vertexShaderBuffer));
	HR_T(m_pDevice->CreateVertexShader(vertexShaderBuffer->GetBufferPointer(), // 필요한 데이터 복사하며 객체 생성
									   vertexShaderBuffer->GetBufferSize(), 
									   NULL, &m_pVertexShader));

	// 3. Render()에서 파이프라인에 바인딩할 InputLayout 생성
	// 
	// 인풋 레이아웃은 버텍스 셰이더가 입력받을 데이터 형식을 지정한다.
	D3D11_INPUT_ELEMENT_DESC layout[] =
	{	// SemanticName , SemanticIndex , Format , InputSlot , AlignedByteOffset , InputSlotClass , InstanceDataStepRate	
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0}
	};

	// 버텍스 셰이더의 Input에 지정된 내용과 같은지 검증하면서 InputLayout 생성
	HR_T(hr = m_pDevice->CreateInputLayout(layout, 
										   ARRAYSIZE(layout),
										   vertexShaderBuffer->GetBufferPointer(), 
										   vertexShaderBuffer->GetBufferSize(), 
										   &m_pInputLayout));

	SAFE_RELEASE(vertexShaderBuffer); // 복사했으니 버퍼는 해제 가능

	// 4. Render에서 파이프라인에 바인딩할 픽셀 셰이더 생성
	ID3DBlob* pixelShaderBuffer = nullptr;	// 픽셀 셰이더 HLSL의 컴파일된 결과(바이트 코드)를 담을 수 있는 객체
	HR_T(CompileShaderFromFile(L"BasicPixelShader.hlsl", "main", "ps_4_0", &pixelShaderBuffer));
	HR_T(m_pDevice->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(), 
									  pixelShaderBuffer->GetBufferSize(), 
									  NULL, &m_pPixelShader));

	SAFE_RELEASE(pixelShaderBuffer); // 복사했으니 버퍼는 해제 가능

	return true;
}

void TutorialApp::UninitScene()
{
	SAFE_RELEASE(m_pVertexBuffer);
	SAFE_RELEASE(m_pInputLayout);
	SAFE_RELEASE(m_pVertexShader);
	SAFE_RELEASE(m_pPixelShader);
}