#include "ResourceManagerApp.h"
#include "../Common/Helper.h"

#include <directxtk/SimpleMath.h>
#include <dxgidebug.h>
#include <dxgi1_3.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "d3dcompiler.lib")

#include <string>

using namespace DirectX::SimpleMath;
using namespace DirectX;
using Microsoft::WRL::ComPtr;

#define USE_FLIPMODE 1 // 경고 메세지를 없애려면 Flip 모드를 사용한다.

// 상수 버퍼
struct ConstantBuffer
{
	Matrix cameraView;
	Matrix cameraProjection;

	Vector4 lightDirection;
	Matrix shadowView;
	Matrix shadowProjection;

	Color lightColor;

	Vector4 ambient;	// 환경광
	Vector4 diffuse;	// 난반사
	Vector4 specular;	// 정반사
	FLOAT shininess;	// 광택지수
	Vector3 CameraPos;
};

std::string WStringToUTF8(const std::wstring& wstr)
{
	int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(),
		(int)wstr.size(),
		nullptr, 0, nullptr, nullptr);

	std::string result(sizeNeeded, 0);

	WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(),
		(int)wstr.size(),
		&result[0], sizeNeeded,
		nullptr, nullptr);

	return result;
}

ResourceManagerApp::ResourceManagerApp(HINSTANCE hInstance)
	: GameApp(hInstance)
{

}

ResourceManagerApp::~ResourceManagerApp()
{
	UninitImGUI();
}

void ResourceManagerApp::InitDebugDraw()
{
	m_states = std::make_unique<CommonStates>(m_pDevice.Get());
	m_batch = std::make_unique<PrimitiveBatch<VertexPositionColor>>(m_pDeviceContext.Get());

	m_effect = std::make_unique<BasicEffect>(m_pDevice.Get());
	m_effect->SetVertexColorEnabled(true);
	m_effect->SetView(m_shadowView);
	m_effect->SetProjection(m_shadowProj);

	{
		void const* shaderByteCode;
		size_t byteCodeLength;

		m_effect->GetVertexShaderBytecode(&shaderByteCode, &byteCodeLength); // ??

		// https://github.com/Microsoft/DirectXTK/wiki/throwIfFailed -> 이거 문서에 있던건데 이거 결국 HRESULT 값을 반환한다는 소리임
		HR_T(m_pDevice->CreateInputLayout(
			VertexPositionColor::InputElements, VertexPositionColor::InputElementCount,
			shaderByteCode, byteCodeLength,
			m_pDebugDrawInputLayout.ReleaseAndGetAddressOf()));
	}
}

void ResourceManagerApp::InitShdowMap()
{
	// create shadow map texure desc
	D3D11_TEXTURE2D_DESC texDesc = {}; // https://learn.microsoft.com/ko-kr/windows/win32/api/d3d11/ns-d3d11-d3d11_texture2d_desc
	texDesc.Width = (UINT)m_shadowViewport.width;
	texDesc.Height = (UINT)m_shadowViewport.height;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	texDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	HR_T(m_pDevice->CreateTexture2D(&texDesc, nullptr, m_pShadowMap.GetAddressOf()));

	// DSV
	D3D11_DEPTH_STENCIL_VIEW_DESC descDSV = {};
	descDSV.Format = DXGI_FORMAT_D32_FLOAT;
	descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	HR_T(m_pDevice->CreateDepthStencilView(m_pShadowMap.Get(), &descDSV, m_pShadowMapDSV.GetAddressOf()));

	// SRV
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	HR_T(m_pDevice->CreateShaderResourceView(m_pShadowMap.Get(), &srvDesc, m_pShadowMapSRV.GetAddressOf()));

	// 빛 계산 ( pov )
	m_shadowProj = XMMatrixPerspectiveFovLH(m_shadowFrustumAngle, m_shadowViewport.width / (FLOAT)m_shadowViewport.height, m_shadowNear, m_shadowFar); // 그림자 절두체
	m_shadowLookAt = m_Camera.m_Position + m_Camera.GetForward() * m_shadowForwardDistFromCamera;	// 바라보는 방향 = 카메라 위치 + 카메라 바라보는 방향으로부터 떨어진 태양의 위치
	m_shadowPos = m_Camera.m_Position + ((Vector3)-m_LightDirection * m_shadowUpDistFromLookAt);	// 위치
	m_shadowView = XMMatrixLookAtLH(m_shadowPos, m_shadowLookAt, Vector3(0.0f, 1.0f, 0.0f));
}

void ResourceManagerApp::DebugDrawFrustum(Matrix worldMat, Matrix viewMat, Matrix proejctionMat,
	float angle, float AspectRatio, float nearZ, float farZ, XMVECTORF32 color)
{
	Vector3 scale = { 1,1,1 };

	// 절투체 만들기
	BoundingFrustum frustum{};
	BoundingFrustum::CreateFromMatrix(frustum, proejctionMat);

	Matrix frustumWorld = viewMat.Transpose();

	frustum.Transform(frustum, frustumWorld); // 위치 옮기기
	frustum.Transform(frustum, worldMat);

	m_effect->SetWorld(Matrix::Identity);	// 해당 그림 위치 설정
	m_effect->SetView(m_View);				// 해당 그림을 어디 기준으로 그릴지 설정
	m_effect->SetProjection(m_Projection);	// 해당 그림이 어디에 투영 될지 설정
	m_effect->Apply(m_pDeviceContext.Get());

	// 문서에 따른 세팅 -> https://github.com/microsoft/DirectXTK/wiki/DebugDraw
	m_pDeviceContext->OMSetBlendState(m_states->Opaque(), nullptr, 0xFFFFFFFF);
	m_pDeviceContext->OMSetDepthStencilState(m_states->DepthNone(), 0);
	m_pDeviceContext->RSSetState(m_states->CullNone());

	m_pDeviceContext->IASetInputLayout(m_pDebugDrawInputLayout.Get()); // 디버그용 InputLayout 적용

	m_batch->Begin();
	DX::Draw(m_batch.get(), frustum, color);

	m_batch->End();
}

bool ResourceManagerApp::InitDxgi()
{
	// ============================================
	// 1. IDXGIDevice3 생성
	// ============================================
	HR_T(m_pDevice->QueryInterface(__uuidof(IDXGIDevice3), reinterpret_cast<void**>(dxgiDevice3.GetAddressOf())));


	// ============================================
	// 1. DXGI 어댑터 생성
	// ============================================
	HR_T((CreateDXGIFactory1(__uuidof(IDXGIFactory6), reinterpret_cast<void**>(dxgiFactory6.GetAddressOf()))));

	HR_T(dxgiFactory6->EnumAdapters1(0, dxgiAdapter1.GetAddressOf()));

	// ============================================
	// 3. VRAM 사용량 (DXGI 1.4 - Windows 10+)
	// ============================================
	HR_T(dxgiAdapter1->QueryInterface(__uuidof(IDXGIAdapter3), reinterpret_cast<void**>(dxgiAdapter3.GetAddressOf())));

	return true;
}

bool ResourceManagerApp::OnInitialize()
{
	if (!InitD3D())
		return false;

	if (!InitScene())
		return false;

	if (!InitImGUI())
		return false;

	if (!InitEffect())
		return false;

	if (!InitDxgi()) 
		return false;

	InitShdowMap();
	InitDebugDraw();
	ResetValues();

	return true;
}

void ResourceManagerApp::OnUpdate()
{
	float delta = GameTimer::m_Instance->DeltaTime();

	Matrix scale = Matrix::Identity;
	Matrix rotate = Matrix::Identity;
	Matrix position = Matrix::Identity;

	// Camera
	m_Camera.GetCameraViewMatrix(m_View);

	m_pSillyDance->Update();
	m_pGround->Update();
	m_pGround->m_Scale = m_GroundScale;
	m_pHuman->Update();

	for (auto& e : m_models)
	{
		if (!e->isRemoved) e->Update();
	}
}

void ResourceManagerApp::OnRender()
{
	DepthOnlyPass();

#if USE_FLIPMODE == 1
	// Flip 모드에서는 매프레임 설정해야한다.
	m_pDeviceContext->OMSetRenderTargets(1, m_pRenderTargetView.GetAddressOf(), m_pDepthStencilView.Get()); // depthStencilView 사용
#endif	
	// 화면 칠하기.
	Color color(0.1f, 0.2f, 0.3f, 1.0f);
	m_pDeviceContext->ClearRenderTargetView(m_pRenderTargetView.Get(), color);
	m_pDeviceContext->ClearDepthStencilView(m_pDepthStencilView.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0); // 뎁스버퍼 1.0f로 초기화.

	RenderPass();

	// Debug Draw Test code ==============

	// 디버그를 위한 회전값 구하기
	Matrix shadowWorldMat = Matrix::CreateTranslation(m_shadowPos);

	DebugDrawFrustum(shadowWorldMat, m_shadowView, m_shadowProj,
		m_shadowFrustumAngle,
		m_shadowViewport.width / (FLOAT)m_shadowViewport.height,
		m_shadowNear, m_shadowFar); // -> 제대로 출력 안됨



	// Debug Draw Test code END ==============

	// Render ImGui
	RenderImGUI();

	// 스왑체인 교체
	m_pSwapChain->Present(0, 0);
}

bool isplayed = false;

void ResourceManagerApp::DepthOnlyPass()
{
	// 바인딩 해제
	ID3D11ShaderResourceView* nullSRV[1] = { nullptr };
	m_pDeviceContext->VSSetShaderResources(4, 1, nullSRV);
	m_pDeviceContext->PSSetShaderResources(4, 1, nullSRV);

	//상수 버퍼 갱신
	ConstantBuffer cb;
	cb.cameraView = XMMatrixTranspose(m_View);
	cb.cameraProjection = XMMatrixTranspose(m_Projection);
	cb.lightDirection = m_LightDirection;
	cb.lightDirection.Normalize();
	cb.shadowView = XMMatrixTranspose(m_shadowView);
	cb.shadowProjection = XMMatrixTranspose(m_shadowProj);
	cb.lightColor = m_LightColor;

	cb.ambient = m_LightAmbient;
	cb.diffuse = m_LightDiffuse;
	cb.specular = m_LightSpecular;

	cb.shininess = m_Shininess;
	cb.CameraPos = m_Camera.m_Position;

	// 뷰포트 설정 + DSV 초기화, RS, OM 설정
	D3D11_VIEWPORT viewport
	{
		m_shadowViewport.x, m_shadowViewport.y,
		m_shadowViewport.width, m_shadowViewport.height,
		m_shadowViewport.minDepth, m_shadowViewport.maxDepth
	};
	m_pDeviceContext->RSSetViewports(1, &viewport);
	m_pDeviceContext->OMSetDepthStencilState(m_pDepthStencilStateAllMask.Get(), 1);
	m_pDeviceContext->OMSetRenderTargets(0, nullptr, m_pShadowMapDSV.Get());
	m_pDeviceContext->ClearDepthStencilView(m_pShadowMapDSV.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);

	// 렌더 파이프라인 설정
	m_pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_pDeviceContext->IASetInputLayout(m_pInputLayout.Get());

	m_pDeviceContext->UpdateSubresource(m_pConstantBuffer.Get(), 0, nullptr, &cb, 0, 0);
	m_pDeviceContext->VSSetConstantBuffers(0, 1, m_pConstantBuffer.GetAddressOf());
	m_pDeviceContext->PSSetSamplers(0, 1, m_pSamplerLinear.GetAddressOf());

	m_pDeviceContext->VSSetShader(m_pShadowMapVS.Get(), 0, 0);
	// m_pDeviceContext->PSSetShader(NULL, NULL, 0); // 렌더 타겟에 기록할 RGBA가 없으므로 실행하지 않는다.
	m_pDeviceContext->PSSetShader(m_pShadowMapPS.Get(), NULL, 0); // 

	// 모델 draw 호출
	m_pGround->Draw(m_pDeviceContext, m_pMaterialBuffer);
	m_pSillyDance->Draw(m_pDeviceContext, m_pMaterialBuffer);
	m_pTree->Draw(m_pDeviceContext, m_pMaterialBuffer);
	m_pHuman->Draw(m_pDeviceContext, m_pMaterialBuffer);

	for (auto& e : m_models)
	{
		if(!e->isRemoved) e->Draw(m_pDeviceContext, m_pMaterialBuffer);
	}
}

void ResourceManagerApp::RenderPass()
{
	m_pDeviceContext->RSSetViewports(1, &m_RenderViewport); // 뷰포트 되돌리기

	// Update Constant Values
	ConstantBuffer cb;
	cb.cameraView = XMMatrixTranspose(m_View);
	cb.cameraProjection = XMMatrixTranspose(m_Projection);
	cb.lightDirection = m_LightDirection;
	cb.lightDirection.Normalize();
	cb.lightColor = m_LightColor;
	cb.shadowView = XMMatrixTranspose(m_shadowView);
	cb.shadowProjection = XMMatrixTranspose(m_shadowProj);

	cb.ambient = m_LightAmbient;
	cb.diffuse = m_LightDiffuse;
	cb.specular = m_LightSpecular;

	cb.shininess = m_Shininess;
	cb.CameraPos = m_Camera.m_Position;

	// 텍스처 및 샘플링 설정 
	m_pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_pDeviceContext->IASetInputLayout(m_pInputLayout.Get());

	m_pDeviceContext->VSSetConstantBuffers(0, 1, m_pConstantBuffer.GetAddressOf());

	m_pDeviceContext->VSSetShader(m_pSkinnedMeshVertexShader.Get(), 0, 0);

	switch (psIndex)
	{
	case 0:
		m_pDeviceContext->PSSetShader(m_pBlinnPhongShader.Get(), 0, 0);
		break;
	case 1:
		m_pDeviceContext->PSSetShader(m_pPhongShader.Get(), 0, 0);
		break;
	case 2:
		m_pDeviceContext->PSSetShader(m_pToonShader.Get(), 0, 0);
		break;
	}

	m_pDeviceContext->PSSetConstantBuffers(0, 1, m_pConstantBuffer.GetAddressOf());
	m_pDeviceContext->PSSetConstantBuffers(1, 1, m_pMaterialBuffer.GetAddressOf());

	m_pDeviceContext->PSSetSamplers(0, 1, m_pSamplerLinear.GetAddressOf());

	m_pDeviceContext->RSSetState(m_pRasterizerState.Get());

	m_pDeviceContext->OMSetDepthStencilState(m_pDepthStencilStateAllMask.Get(), 1);
	m_pDeviceContext->UpdateSubresource(m_pConstantBuffer.Get(), 0, nullptr, &cb, 0, 0);
	m_pDeviceContext->PSSetShaderResources(4, 1, m_pShadowMapSRV.GetAddressOf());

	// Draw 
	m_pSillyDance->Draw(m_pDeviceContext, m_pMaterialBuffer);
	m_pGround->Draw(m_pDeviceContext, m_pMaterialBuffer);
	m_pTree->Draw(m_pDeviceContext, m_pMaterialBuffer);
	m_pHuman->Draw(m_pDeviceContext, m_pMaterialBuffer);

	for (auto& e : m_models)
	{
		e->Draw(m_pDeviceContext, m_pMaterialBuffer);
	}
}

bool ResourceManagerApp::InitImGUI()
{
	bool isSetupSuccess = false;

	// Setup Dear ImGui context 
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();

	// Setup Platform/Renderer backends
	isSetupSuccess = ImGui_ImplWin32_Init(m_hWnd);
	if (!isSetupSuccess) return false;

	isSetupSuccess = ImGui_ImplDX11_Init(m_pDevice.Get(), m_pDeviceContext.Get());
	if (!isSetupSuccess) return false;

	return true;
}

void ResourceManagerApp::RenderImGUI()
{
	// Start the Dear ImGui frame
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	ImGui::SetNextWindowPos(ImVec2(800, 10), ImGuiCond_Once);
	ImGui::SetNextWindowSize(ImVec2(200, 200), ImGuiCond_Once);

	if (m_pSillyDance->modelAsset->animations.size() > 0)
	{
		ImGui::Begin("Middle Model Animation info");
		{
			ImGui::Text(std::to_string(m_pSillyDance->m_progressAnimationTime).c_str());
			ImGui::Checkbox("isPlay", &m_pSillyDance->isAnimPlay);
			ImGui::SliderFloat("Animation Duration", &m_pSillyDance->m_progressAnimationTime, 0.0f, m_pSillyDance->modelAsset->animations[m_pSillyDance->m_animationIndex].m_duration);
		}
		ImGui::End();
	}

	ImGui::Begin("ShadowMap");
	{
		ImTextureID img = (ImTextureID)(intptr_t)(m_pShadowMapSRV.Get());
		ImGui::Image(img, ImVec2(256, 256));
		ImGui::DragFloat("m_shadowForwardDistFromCamera", &m_shadowForwardDistFromCamera);
		ImGui::DragFloat3("m_shadowUpDistFromLookAt", &m_shadowUpDistFromLookAt.x);

		ImGui::DragFloat("m_shadowNear", &m_shadowNear);
		ImGui::DragFloat("m_shadowFar", &m_shadowFar);

		if (m_shadowNear <= m_shadowMinNear) m_shadowNear = m_shadowMinNear;
		if (m_shadowFar <= m_shadowMinFar) m_shadowFar = m_shadowMinFar;

		ImGui::DragFloat("m_shadowFrustumAngle", &m_shadowFrustumAngle, 0.02f);

		// m_shadow 관련 갱신
		m_shadowProj = XMMatrixPerspectiveFovLH(m_shadowFrustumAngle, m_shadowViewport.width / (FLOAT)m_shadowViewport.height, m_shadowNear, m_shadowFar);
		m_shadowLookAt = m_Camera.m_Position + m_Camera.GetForward() * m_shadowForwardDistFromCamera;	// 바라보는 방향 = 카메라 위치 + 카메라 바라보는 방향으로부터 떨어진 태양의 위치
		m_shadowPos = m_Camera.m_Position + ((Vector3)-m_LightDirection * m_shadowUpDistFromLookAt);	// 위치
		m_shadowView = XMMatrixLookAtLH(m_shadowPos, m_shadowLookAt, Vector3(0.0f, 1.0f, 0.0f));
	}
	ImGui::End();

	// 리소스 매니터 테스트 코드
	ImGui::Begin("Resource Infos");
	{
		if (ImGui::Button("Generate Silly Dance"))
		{
			auto model = make_unique<SkeletalModel>();
			if (!model->Load(m_hWnd, m_pDevice, m_pDeviceContext, "..\\Resource\\SillyDancing.fbx"))
			{
				MessageBox(m_hWnd, L"FBX file is invaild at path", NULL, MB_ICONERROR | MB_OK);
			}

			model->GetBuffer(m_pTransformBuffer, m_pBonePoseBuffer, m_pBoneOffsetBuffer);

			Vector3 pos = m_Camera.m_Position;
			model->m_Position = pos;
			m_models.push_back(std::move(model));
		}

		if (ImGui::Button("Remove Silly Dance"))
		{
			if (!m_models.empty())
			{
				auto model = std::move(m_models.front());
				m_models.pop_front();
				model->isRemoved = true;

				model.release();
			}
		}

		if (ImGui::Button("Call Trim"))
		{
			dxgiDevice3->Trim();
		}

		ImGui::Spacing();

		// 사용량 출력 하기
		DXGI_ADAPTER_DESC1 desc;
		dxgiAdapter1->GetDesc1(&desc);

		wstring gpuStr = L"GPU: ";
		gpuStr += desc.Description;
		string utf8 = WStringToUTF8(gpuStr);
		ImGui::Text(utf8.c_str());

		// ============================================
		// 2. VRAM 전체 용량
		// ============================================

		string textStr;
		SIZE_T vramTotal = desc.DedicatedVideoMemory;
		std::cout << "VRAM Total: " << (vramTotal / 1024 / 1024) << " MB\n";
		textStr += "VRAM Total: ";
		textStr += to_string(vramTotal / 1024 / 1024);
		textStr += " MB";
		ImGui::Text(textStr.c_str());
		textStr.clear();

		// ============================================
		// 3. VRAM 사용량 (DXGI 1.4 - Windows 10+)
		// ============================================
		DXGI_QUERY_VIDEO_MEMORY_INFO memInfo = {};
		if (SUCCEEDED(dxgiAdapter3->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &memInfo)))
		{
			SIZE_T vramUsed = memInfo.CurrentUsage;
			textStr += "VRAM Used: ";
			textStr += to_string(vramUsed / 1024 / 1024);
			textStr += " MB";
			ImGui::Text(textStr.c_str());
			textStr.clear();
		}

		// ============================================
		// 4. 시스템 메모리 / 페이지 파일
		// ============================================
		MEMORYSTATUSEX mem = {};
		mem.dwLength = sizeof(MEMORYSTATUSEX);

		if (GlobalMemoryStatusEx(&mem))
		{
			textStr += "RAM Total:";
			textStr += to_string(mem.ullTotalPhys / 1024 / 1024);
			textStr += " MB";
			ImGui::Text(textStr.c_str());
			textStr.clear();

			textStr += "RAM Available: ";
			textStr += to_string(mem.ullAvailPhys / 1024 / 1024);
			textStr += " MB";
			ImGui::Text(textStr.c_str());
			textStr.clear();

			textStr += "PageFile Total: ";
			textStr += to_string(mem.ullTotalPageFile / 1024 / 1024);
			textStr += " MB";
			ImGui::Text(textStr.c_str());
			textStr.clear();

			textStr += "PageFile Available: ";
			textStr += to_string(mem.ullAvailPageFile / 1024 / 1024);
			textStr += " MB";
			ImGui::Text(textStr.c_str());
			textStr.clear();
		}

		// ============================================
		// 5. 프로세스 메모리 사용량
		// ============================================
		PROCESS_MEMORY_COUNTERS_EX pmc = {};
		if (GetProcessMemoryInfo(GetCurrentProcess(),
			(PROCESS_MEMORY_COUNTERS*)&pmc,
			sizeof(pmc)))
		{
			textStr += "Process Working Set: ";
			textStr += to_string(pmc.WorkingSetSize / 1024 / 1024);
			textStr += " MB";
			ImGui::Text(textStr.c_str());
			textStr.clear();

			textStr += "Private Bytes: ";
			textStr += to_string(pmc.PrivateUsage / 1024 / 1024);
			textStr += " MB";
			ImGui::Text(textStr.c_str());
			textStr.clear();
		}
	}
	ImGui::End();


	// 월드 오브젝트 조종 창 만들기
	ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Once);		// 처음 실행될 때 위치 초기화
	ImGui::SetNextWindowSize(ImVec2(350, 500), ImGuiCond_Once);		// 처음 실행될 때 창 크기 초기화
	ImGui::Begin("World Controller");

	{
		ImGui::DragFloat3("Middle Object Position", &m_sillyDancePosition.x);

		Vector3 cube1Rotation;
		cube1Rotation.x = XMConvertToDegrees(m_sillyDanceRotation.x);
		cube1Rotation.y = XMConvertToDegrees(m_sillyDanceRotation.y);
		cube1Rotation.z = XMConvertToDegrees(m_sillyDanceRotation.z);
		ImGui::DragFloat3("Middle Object Rotation", &cube1Rotation.x);
		m_sillyDanceRotation.x = XMConvertToRadians(cube1Rotation.x);
		m_sillyDanceRotation.y = XMConvertToRadians(cube1Rotation.y);
		m_sillyDanceRotation.z = XMConvertToRadians(cube1Rotation.z);

		ImGui::DragFloat("Middle Object Scale", &m_sillyDanceScale.x, 0.05f);
		m_sillyDanceScale.y = m_sillyDanceScale.x;
		m_sillyDanceScale.z = m_sillyDanceScale.x;

		m_pSillyDance->m_Position = m_sillyDancePosition;
		m_pSillyDance->m_Rotation = m_sillyDanceRotation;
		m_pSillyDance->m_Scale = m_sillyDanceScale;
	}

	ImGui::NewLine();

	// 카메라 위치 및 회전 설정
	ImGui::DragFloat3("Camera Position", &m_Camera.m_Position.x);
	ImGui::DragFloat3("Camera Rotation", &m_CameraRotation.x);

	if (ImGui::Button("Set Rotation"))
	{
		m_Camera.m_Rotation.x = XMConvertToRadians(m_CameraRotation.x);
		m_Camera.m_Rotation.y = XMConvertToRadians(m_CameraRotation.y);
		m_Camera.m_Rotation.z = XMConvertToRadians(m_CameraRotation.z);
	}

	ImGui::NewLine();

	// Near 값 설정
	ImGui::DragFloat("Near", &m_Near, 0.5f);

	// Far 값 설정
	ImGui::DragFloat("Far", &m_Far, 0.5f);

	// Fov 값 설정
	ImGui::DragFloat("Fov Angle", &m_PovAngle, 0.02f);

	if (m_Near <= 0.0f) m_Near = 0.01f;
	if (m_Far <= m_Near) m_Far = 0.2f;

	ImGui::NewLine();

	ImGui::Text("PixelShader:");
	ImGui::RadioButton("Blinn-Phong", psIndex == 0); if (ImGui::IsItemClicked()) psIndex = 0;
	ImGui::RadioButton("Phong", psIndex == 1); if (ImGui::IsItemClicked()) psIndex = 1;
	ImGui::RadioButton("Toon", psIndex == 2); if (ImGui::IsItemClicked()) psIndex = 2;

	ImGui::NewLine();

	m_Projection = XMMatrixPerspectiveFovLH(m_PovAngle, m_ClientWidth / (FLOAT)m_ClientHeight, m_Near, m_Far);

	ImGui::ColorEdit4("Light Color", &m_LightColor.x);
	ImGui::DragFloat3("Light Direction", &m_LightDirection.x, 0.1f, -1.0f, 1.0f);

	ImGui::ColorEdit4("Light Ambient", &m_LightAmbient.x);
	ImGui::ColorEdit4("Light Diffuse", &m_LightDiffuse.x);
	ImGui::ColorEdit4("Light Specular", &m_LightSpecular.x);
	ImGui::DragFloat("Shininess", &m_Shininess, 10.0f);

	ImGui::Spacing();
	ImGui::DragFloat3("m_pGround->m_Scale", &m_GroundScale.x, 0.1f);
	ImGui::Spacing();

	// 리셋 버튼
	if (ImGui::Button("Reset", { 50, 20 }))
	{
		ResetValues();
	}

	ImGui::End();

	// rendering
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void ResourceManagerApp::UninitImGUI()
{
	// Cleanup
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

bool ResourceManagerApp::InitD3D()
{
	HRESULT hr = S_OK;

	// 1. D3D11 Device, DeviceContext 생성
	UINT creationFlag = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

#ifdef _DEBUG
	creationFlag |= D3D11_CREATE_DEVICE_DEBUG;
#endif //  _DEBUG

	// 그래픽 카드 하드웨어의 스펙으로 호환되는 가장 높은 DirectX 기능레벨로 생성하여 드라이버가 작동한다.
	// 인터페이스는 Direct3D11이지만 GPU 드라이버는 D3D12 드라이버가 작동될 수 있다.
	D3D_FEATURE_LEVEL featureLevels[] =
	{	// 0번 index부터 순서대로 시도
		D3D_FEATURE_LEVEL_12_2,D3D_FEATURE_LEVEL_12_1,D3D_FEATURE_LEVEL_12_0,D3D_FEATURE_LEVEL_11_1,D3D_FEATURE_LEVEL_11_0
	};
	D3D_FEATURE_LEVEL actualFeatureLevel;	// 최종 feature level 저장 변수

	HR_T(D3D11CreateDevice
	(
		nullptr,
		D3D_DRIVER_TYPE_HARDWARE,
		0,
		creationFlag,
		featureLevels,
		ARRAYSIZE(featureLevels),
		D3D11_SDK_VERSION,
		&m_pDevice,
		&actualFeatureLevel,
		&m_pDeviceContext
	));


	// 2. 스왑체인 생성을 위한 DXGI Factory 생성
	UINT dxgiFactoryFlags = 0;

#ifdef _DEBUG
	dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif // _DEBUG

	ComPtr<IDXGIFactory2> pFactory;
	HR_T(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&pFactory)));

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
#if USE_FLIPMODE == 1
	swapChainDesc.BufferCount = 2;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
#else
	swapChainDesc.BufferCount = 1;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
#endif
	swapChainDesc.Width = m_ClientWidth;
	swapChainDesc.Height = m_ClientHeight;

	// 하나의 픽셀이 채널 RGBA 각 8비트 형식으로 표현
	// Unsigned Normalized Integer 8비트 정수(0~255)단계를 부동소수점으로 정규화한 0.0~1.0으로 매핑하여 표현한다.
	swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // 스왑 체인의 백 버퍼가 렌더링 파이프라인의 최종 출력 대상으로 사용
	swapChainDesc.SampleDesc.Count = 1;	// 멀티 샘플링 사용 안함
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE; // 투명도 조작 무시 | recommand for flip mode ?
	swapChainDesc.Stereo = FALSE;
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH; // 전체 화면 전환을 허용
	swapChainDesc.Scaling = DXGI_SCALING_NONE; // 창의 크기와 백 버퍼의 크기가 다를 때. 백버퍼 크기에 맞게 스케일링 하지 않는다.

	HR_T(pFactory->CreateSwapChainForHwnd
	(
		m_pDevice.Get(),
		m_hWnd,
		&swapChainDesc,
		nullptr,
		nullptr,
		&m_pSwapChain
	));

	// 3. 랜더타겟 뷰 생성. 랜더타겟 뷰는 "여기에 그림을 그려라"라고 GPU에게 알려주는 역할을 하는 객체
	// 텍스쳐와 영구적으로 연결되는 객체
	ComPtr<ID3D11Texture2D> pBackBufferTexture;
	HR_T(m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBufferTexture));
	HR_T(m_pDevice->CreateRenderTargetView(pBackBufferTexture.Get(), nullptr, m_pRenderTargetView.GetAddressOf()));

#if !USE_FLIPMODE
	m_pDeviceContext->OMSetRenderTargets(1, m_pRenderTargetView.GetAddressOf(), nullptr);
#endif

	// 4. viewport 설정
	m_RenderViewport = {};
	m_RenderViewport.TopLeftX = 0;
	m_RenderViewport.TopLeftY = 0;
	m_RenderViewport.Width = (float)m_ClientWidth;
	m_RenderViewport.Height = (float)m_ClientHeight;
	m_RenderViewport.MinDepth = 0.0f;
	m_RenderViewport.MaxDepth = 1.0f;
	m_pDeviceContext->RSSetViewports(1, &m_RenderViewport);

	// 5. 뎊스 스텐실 뷰 설정
	D3D11_TEXTURE2D_DESC descDepth = {};
	descDepth.Width = m_ClientWidth;
	descDepth.Height = m_ClientHeight;
	descDepth.MipLevels = 1;
	descDepth.ArraySize = 1;
	descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; // https://learn.microsoft.com/ko-kr/windows/win32/api/dxgiformat/ne-dxgiformat-dxgi_format
	descDepth.SampleDesc.Count = 1;
	descDepth.SampleDesc.Quality = 0;
	descDepth.Usage = D3D11_USAGE_DEFAULT;
	descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	descDepth.CPUAccessFlags = 0;
	descDepth.MiscFlags = 0;

	// 뎊스 스탠실 상태 설정
	D3D11_DEPTH_STENCIL_DESC depthStencilDesc = {};
	depthStencilDesc.DepthEnable = TRUE;                // 깊이 테스트 활성화
	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO; // 깊이 버퍼 업데이트 허용
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS; // 작은 Z 값이 앞에 배치되도록 설정
	depthStencilDesc.StencilEnable = FALSE;            // 스텐실 테스트 비활성화

	m_pDevice->CreateDepthStencilState(&depthStencilDesc, &m_pDepthStencilStateZeroMask);

	depthStencilDesc = {};
	depthStencilDesc.DepthEnable = TRUE;                // 깊이 테스트 활성화
	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL; // 깊이 버퍼 업데이트 허용
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS; // 작은 Z 값이 앞에 배치되도록 설정
	depthStencilDesc.StencilEnable = FALSE;            // 스텐실 테스트 비활성화

	m_pDevice->CreateDepthStencilState(&depthStencilDesc, &m_pDepthStencilStateAllMask);

	// create depthStencil texture
	ComPtr<ID3D11Texture2D> pTextureDepthStencil;
	HR_T(m_pDevice->CreateTexture2D(&descDepth, nullptr, pTextureDepthStencil.GetAddressOf()));

	// create the depth stencil view
	D3D11_DEPTH_STENCIL_VIEW_DESC descDSV = {};
	descDSV.Format = descDepth.Format;
	descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D; // 사용되는 리소스 엑세스 방식 설정 : https://learn.microsoft.com/ko-kr/windows/win32/api/d3d11/ne-d3d11-d3d11_dsv_dimension 
	descDSV.Texture2D.MipSlice = 0;
	HR_T(m_pDevice->CreateDepthStencilView(pTextureDepthStencil.Get(), &descDSV, m_pDepthStencilView.GetAddressOf()));

	// create blending state https://learn.microsoft.com/ko-kr/windows/win32/api/d3d11/ns-d3d11-d3d11_blend_desc
	// Color = SrcAlpha * SrcColor + (1 - SrcAlpha) * DestColor 
	// Alpha = SrcAlpha
	D3D11_BLEND_DESC descBlend = {};
	descBlend.RenderTarget[0].BlendEnable = true;						// blend 사용 여부

	// SrcBlend -> 소스 텍스처의 색상
	// DestBlend -> 이미 해당 자리에 그려져있는 색상
	descBlend.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;			// D3D11_BLEND_SRC_ALPHA -> 픽셀 셰이더 결과 값의 알파 데이터 값
	descBlend.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;	// D3D11_BLEND_INV_SRC_ALPHA -> D3D11_BLEND_SRC_ALPHA의 반전 값 ( 1 - 값 )
	descBlend.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;				// SrcBlend 및 DestBlend 작업을 결합하는 방법을 정의, D3D11_BLEND_OP_ADD -> Add source 1 and source 2

	descBlend.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;			// D3D11_BLEND_ONE -> (1,1,1,1)
	descBlend.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;		// D3D11_BLEND_ZERO -> (0,0,0,0)
	descBlend.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;		// 

	descBlend.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;  // 모든 색 전부 사용

	m_pDevice->CreateBlendState(&descBlend, m_pBlendState.GetAddressOf());

	// material buffer
	D3D11_BUFFER_DESC mbd = {};
	mbd.Usage = D3D11_USAGE_DEFAULT;
	mbd.ByteWidth = sizeof(Material);
	mbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	mbd.CPUAccessFlags = 0;
	HR_T(m_pDevice->CreateBuffer(&mbd, nullptr, m_pMaterialBuffer.GetAddressOf()));

	return true;
}

bool ResourceManagerApp::InitScene()
{
	HRESULT hr = S_OK;

	// 6. Render() 에서 파이프라인에 바인딩할 상수 버퍼 생성
	D3D11_BUFFER_DESC bufferDesc = {};
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(ConstantBuffer);
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	HR_T(m_pDevice->CreateBuffer(&bufferDesc, nullptr, m_pConstantBuffer.GetAddressOf()));

	// 쉐이더에 상수버퍼에 전달할 시스템 메모리 데이터 초기화
	m_World = XMMatrixIdentity();

	XMVECTOR Eye = XMVectorSet(0.0f, 10.0f, -8.0f, 0.0f);
	XMVECTOR At = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
	XMVECTOR Up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	m_View = XMMatrixLookAtLH(Eye, At, Up);
	m_Projection = XMMatrixPerspectiveFovLH(m_PovAngle, m_ClientWidth / (FLOAT)m_ClientHeight, m_Near, m_Far);

	// 샘플링 상태 설정
	D3D11_SAMPLER_DESC sampDesc = {};
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;	// 텍스처 샘플링할 때 사용할 필터링 방법
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;		// 범위 밖에 있는 텍스처 좌표 확인 방법
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;	// 샘플링된 데이터를 기존 데이터와 확인하는 방법
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	HR_T(m_pDevice->CreateSamplerState(&sampDesc, m_pSamplerLinear.GetAddressOf()));

	// 래스터라이저
	D3D11_RASTERIZER_DESC rasterizerDesc = {};
	rasterizerDesc.CullMode = D3D11_CULL_FRONT;
	rasterizerDesc.FillMode = D3D11_FILL_SOLID;
	rasterizerDesc.DepthClipEnable = true;
	rasterizerDesc.FrontCounterClockwise = true;

	m_pDevice->CreateRasterizerState(&rasterizerDesc, &m_pRasterizerState);

	rasterizerDesc = {};
	rasterizerDesc.CullMode = D3D11_CULL_FRONT;
	rasterizerDesc.FillMode = D3D11_FILL_SOLID;
	rasterizerDesc.DepthClipEnable = true;
	rasterizerDesc.FrontCounterClockwise = true;

	m_pDevice->CreateRasterizerState(&rasterizerDesc, &m_pTransparentRasterizerState);

	// 모델들이 사용할 버퍼 만들기
	// 트랜스폼 상수 버퍼 만들기
	bufferDesc = {};
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(TransformBuffer);
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	HR_T(m_pDevice->CreateBuffer(&bufferDesc, nullptr, m_pTransformBuffer.GetAddressOf()));

	// 본 포즈 버퍼 만들기
	bufferDesc = {};
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(BonePoseBuffer);
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	HR_T(m_pDevice->CreateBuffer(&bufferDesc, nullptr, m_pBonePoseBuffer.GetAddressOf()));

	// 본 오프셋 버퍼 만들기
	bufferDesc = {};
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(BoneOffsetBuffer);
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	HR_T(m_pDevice->CreateBuffer(&bufferDesc, nullptr, m_pBoneOffsetBuffer.GetAddressOf()));

	// 모델 생성
	m_pSillyDance = make_unique<SkeletalModel>();
	if (!m_pSillyDance->Load(m_hWnd, m_pDevice, m_pDeviceContext, "..\\Resource\\SillyDancing.fbx"))
	{
		MessageBox(m_hWnd, L"FBX file is invaild at path", NULL, MB_ICONERROR | MB_OK);
	}
	m_pSillyDance->GetBuffer(m_pTransformBuffer, m_pBonePoseBuffer, m_pBoneOffsetBuffer);

	m_pGround = make_unique<SkeletalModel>();
	if (!m_pGround->Load(m_hWnd, m_pDevice, m_pDeviceContext, "..\\Resource\\Ground.fbx"))
	{
		MessageBox(m_hWnd, L"FBX file is invaild at path", NULL, MB_ICONERROR | MB_OK);
	}
	m_pGround->GetBuffer(m_pTransformBuffer, m_pBonePoseBuffer, m_pBoneOffsetBuffer);
	m_pGround->m_Position = { 0, -100, 0 };

	m_pTree = make_unique<SkeletalModel>();
	if (!m_pTree->Load(m_hWnd, m_pDevice, m_pDeviceContext, "..\\Resource\\Tree.fbx"))
	{
		MessageBox(m_hWnd, L"FBX file is invaild at path", NULL, MB_ICONERROR | MB_OK);
	}
	m_pTree->GetBuffer(m_pTransformBuffer, m_pBonePoseBuffer, m_pBoneOffsetBuffer);
	m_pTree->m_Scale = { 100, 100, 100 };

	m_pHuman = make_unique<SkeletalModel>();
	if (!m_pHuman->Load(m_hWnd, m_pDevice, m_pDeviceContext, "..\\Resource\\Zombie_Run.fbx"))
	{
		MessageBox(m_hWnd, L"FBX file is invaild at path", NULL, MB_ICONERROR | MB_OK);
	}
	m_pHuman->GetBuffer(m_pTransformBuffer, m_pBonePoseBuffer, m_pBoneOffsetBuffer);
	m_pHuman->m_Position = { 200, 10, 100 };

	return true;
}

bool ResourceManagerApp::InitEffect()
{
	// 2. 파이프라인에 바인딩할 InputLayout 생성
	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "BINORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "BLENDINDICES", 0, DXGI_FORMAT_R32G32B32A32_SINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "BLENDWEIGHTS", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	ComPtr<ID3DBlob> vertexShaderBuffer = nullptr;
	HR_T(CompileShaderFromFile(L"Shaders\\VS_SkinnedMesh.hlsl", "main", "vs_5_0", vertexShaderBuffer.GetAddressOf()));
	HR_T(m_pDevice->CreateInputLayout(layout, ARRAYSIZE(layout), vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), m_pInputLayout.GetAddressOf()));

	// 3. 파이프 라인에 바인딩할 정점 셰이더 생성
	HR_T(m_pDevice->CreateVertexShader(vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), NULL, m_pSkinnedMeshVertexShader.GetAddressOf()));

	vertexShaderBuffer.Reset();
	HR_T(CompileShaderFromFile(L"Shaders\\VS_RigidMesh.hlsl", "main", "vs_5_0", vertexShaderBuffer.GetAddressOf()));
	HR_T(m_pDevice->CreateVertexShader(vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), NULL, m_pRigidMeshVertexShader.GetAddressOf()));

	vertexShaderBuffer.Reset();
	HR_T(CompileShaderFromFile(L"Shaders\\VS_DepthOnlyPass.hlsl", "main", "vs_5_0", vertexShaderBuffer.GetAddressOf()));
	HR_T(m_pDevice->CreateVertexShader(vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), NULL, m_pShadowMapVS.GetAddressOf()));

	// 5. 파이프라인에 바인딩할 픽셀 셰이더 생성
	ComPtr<ID3DBlob> pixelShaderBuffer = nullptr;
	HR_T(CompileShaderFromFile(L"Shaders\\PS_Basic.hlsl", "main", "ps_5_0", pixelShaderBuffer.GetAddressOf()));
	HR_T(m_pDevice->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, m_pPixelShader.GetAddressOf()));

	pixelShaderBuffer.Reset();
	HR_T(CompileShaderFromFile(L"Shaders\\PS_Phong.hlsl", "main", "ps_5_0", pixelShaderBuffer.GetAddressOf()));
	HR_T(m_pDevice->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, m_pPhongShader.GetAddressOf()));

	pixelShaderBuffer.Reset();
	HR_T(CompileShaderFromFile(L"Shaders\\PS_BlinnPhong.hlsl", "main", "ps_5_0", pixelShaderBuffer.GetAddressOf()));
	HR_T(m_pDevice->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, m_pBlinnPhongShader.GetAddressOf()));

	pixelShaderBuffer.Reset();
	HR_T(CompileShaderFromFile(L"Shaders\\PS_Toon.hlsl", "main", "ps_5_0", pixelShaderBuffer.GetAddressOf()));
	HR_T(m_pDevice->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, m_pToonShader.GetAddressOf()));

	pixelShaderBuffer.Reset();
	HR_T(CompileShaderFromFile(L"Shaders\\PS_DepthOnlyPass.hlsl", "main", "ps_5_0", pixelShaderBuffer.GetAddressOf()));
	HR_T(m_pDevice->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, m_pShadowMapPS.GetAddressOf()));

	return true;
}

void ResourceManagerApp::ResetValues()
{
	m_Near = 0.01f;
	m_Far = 3000.0f;
	m_PovAngle = XM_PIDIV2;

	m_CameraRotation = Vector3::Zero;
	m_Camera.SetRotation(m_CameraRotation);
	m_Camera.SetPosition(m_CameraPositionInitial);

	m_Projection = XMMatrixPerspectiveFovLH(m_PovAngle, m_ClientWidth / (FLOAT)m_ClientHeight, m_Near, m_Far);

	m_LightDirection = m_LightDirectionInitial;
}

void ResourceManagerApp::ShowMatrix(const DirectX::XMFLOAT4X4& mat, const char* label)
{
	// 각 행을 출력
	ImGui::Text("%.3f  %.3f  %.3f  %.3f", mat._11, mat._12, mat._13, mat._14);
	ImGui::Text("%.3f  %.3f  %.3f  %.3f", mat._21, mat._22, mat._23, mat._24);
	ImGui::Text("%.3f  %.3f  %.3f  %.3f", mat._31, mat._32, mat._33, mat._34);
	ImGui::Text("%.3f  %.3f  %.3f  %.3f", mat._41, mat._42, mat._43, mat._44);
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT ResourceManagerApp::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
		return true;

	return __super::WndProc(hWnd, message, wParam, lParam);
}