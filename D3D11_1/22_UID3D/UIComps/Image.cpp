#include "Image.h"
#include "directxtk/WICTextureLoader.h"
#include "../../Common/Helper.h"

// Test
#include <directXTK/Mouse.h>
#include <directXTK/Keyboard.h>

void Image::GetTexureByPath(ComPtr<ID3D11Device>& device, ComPtr<ID3D11DeviceContext>& context, std::string path)
{
	std::wstring wpath = std::wstring(path.begin(), path.end());
	HR_T(CreateWICTextureFromFile(device.Get(), context.Get(), wpath.c_str(), nullptr, imgSRV.GetAddressOf()));
}

void Image::Init(ComPtr<ID3D11Device>& dev)
{
	// 상수 버퍼 만들기
	D3D11_BUFFER_DESC bufferDesc{};
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(ImageCBData);
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	HR_T(dev->CreateBuffer(&bufferDesc, nullptr, imageCbBuffer.GetAddressOf()));

	// 픽셀 셰이더 만들기
	ComPtr<ID3DBlob> pixelShaderBuffer = nullptr;
	HR_T(CompileShaderFromFile(L"Shaders\\PS_QuadImage.hlsl", "main", "ps_5_0", &pixelShaderBuffer));
	HR_T(dev->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, imagePS.GetAddressOf()));
}

void Image::Render(ComPtr<ID3D11DeviceContext>& context)
{
	// TODO : Z축 정렬
	//			마우스 이벤트 클릭, 마우스 클릭은 최상단 UI 1개만 감지한다.

	if (canvas == nullptr) return;

	// rect 계산
	float rot = rect.GetEuler().z;

	// pivot은 0~1 (예: (0.5,0.5) center)
	float pivX = rect.pivot.x;
	float pivY = rect.pivot.y;

	Matrix world =
		Matrix::CreateTranslation(-pivX, -pivY, 0.0f) *				// 피벗만큼 위치 옮기기 [0, 1]
		Matrix::CreateScale(rect.width, rect.height, 1.0f) *		// 픽셀 스케일링
		Matrix::CreateRotationZ(rot) *								// z회전
		Matrix::CreateTranslation(rect.pos.x, rect.pos.y, 0.0f);	// rect position 계산

	mvp = world * canvas->GetProjection();	// UI에서  view는 보통 identity
	imageCBData.WVP = mvp.Transpose();
	imageCBData.color = color;

	// Note : Mouse 충돌 테스트
	// 접근 : 마우스 위치를 로컬 쿼드 좌표로 옮겨서 내부 판정을 시작한다. ( world.invert )
	//		쿼드가 유닛값이므로 (0-1)로 내부판정을 한다.
	int mouseX = DirectX::Mouse::Get().GetState().x;
	int mouseY = DirectX::Mouse::Get().GetState().y;

	Matrix invWorld = world.Invert();
	Vector3 mouseWorld(mouseX, mouseY, 0.0f);
	Vector3 local = Vector3::Transform(mouseWorld, invWorld);

	// 유닛 쿼드 내부 판정 (0-1)
	bool isHover = (local.x >= 0.0f && local.x <= 1.0f) && 
				(local.y >= 0.0f && local.y <= 1.0f);

	if (isHover) // 마우스가 감지되면 빨강색
	{
		color = { 1,0,0,1 };
	}
	else // 그렇지 않으면 하양색
	{
		color = { 1,1,1,1 };
	}

	context->UpdateSubresource(imageCbBuffer.Get(), 0, nullptr, &imageCBData, 0, 0); // 상수 버퍼 업데이트

	context->VSSetConstantBuffers(1, 1, imageCbBuffer.GetAddressOf());	// vs 상수 버퍼 설정
	context->PSSetConstantBuffers(1, 1, imageCbBuffer.GetAddressOf());	// ps 상수 버퍼 설정
	context->PSSetShader(imagePS.Get(), nullptr, 0);					// ps 바인딩

	context->PSSetShaderResources(0, 1, imgSRV.GetAddressOf());			// 텍스처 리소스 바인딩

	context->DrawIndexed(6, 0, 0);	// 쿼드 그리기
}

Color Image::GetColor()
{
	return color;
}

void Image::SetColor(Color color)
{
	this->color = color;
}
