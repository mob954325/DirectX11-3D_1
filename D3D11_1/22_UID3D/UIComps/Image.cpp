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

	ComPtr<ID3D11Resource> res;
	imgSRV->GetResource(res.GetAddressOf());

	ComPtr<ID3D11Texture2D> tex2D;
	HR_T(res.As(&tex2D));

	D3D11_TEXTURE2D_DESC desc{};
	tex2D->GetDesc(&desc);

	texSizePx = Vector2(( float ) desc.Width, ( float ) desc.Height); // border(px)
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

	mvp = rect.GetWorld() * canvas->GetProjection();	// UI에서  view는 보통 identity
	imageCBData.WVP = mvp.Transpose();

	// mouse hover test
	CheckMouseHover();
	if (isMouseHover) // 마우스가 감지되면 빨강색
	{
		color = { 1,0,0,1 };
	}
	else
	{
		color = { 1,1,1,1 };
	}
	imageCBData.color = color;

	// type / fillAmount 전달하기
	imageCBData.params = Vector4(( float ) type, fillAmount, 0.0f, 0.0f);

	// uvRect : 9-sliced 보더(px)
	imageCBData.uvRect = sliceBorderPx;

	// imageSize : rectW/ rectH/ texW/ texH
	Vector2 rectSize = rect.GetSize();
	imageCBData.imageSize = Vector4(rectSize.x, rectSize.y, texSizePx.x, texSizePx.y);

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

ImageType Image::GetImageType()
{
	return type;
}

void Image::SetImageType(ImageType t)
{
	type = t;
}

float Image::GetFillAmount()
{
	return fillAmount;
}

void Image::SetFillAmount(float v)
{
	fillAmount = std::clamp(v, 0.0f, 1.0f);
}

Vector4 Image::GetSliceBorderPx()
{
	return sliceBorderPx;
}

void Image::SetSliceBorderPx(float l, float r, float t, float b)
{
	sliceBorderPx = Vector4(l, r, t, b);
}

void Image::CheckMouseHover()
{
	// Note : Mouse 충돌 테스트
	// 접근 : 마우스 위치를 로컬 쿼드 좌표로 옮겨서 내부 판정을 시작한다. ( world.invert )
	//		쿼드가 유닛값이므로 (0-1)로 내부판정을 한다.
	int mouseX = DirectX::Mouse::Get().GetState().x;
	int mouseY = DirectX::Mouse::Get().GetState().y;

	Matrix invWorld = rect.GetWorld().Invert();
	Vector3 mouseWorld(mouseX, mouseY, 0.0f);
	Vector3 local = Vector3::Transform(mouseWorld, invWorld);

	// 유닛 쿼드 내부 판정 (0-1)
	isMouseHover = (local.x >= 0.0f && local.x <= 1.0f) &&
		(local.y >= 0.0f && local.y <= 1.0f);
}