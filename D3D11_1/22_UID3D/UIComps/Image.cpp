#include "Image.h"
#include "directxtk/WICTextureLoader.h"
#include "../../Common/Helper.h"

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
	bufferDesc.ByteWidth = sizeof(ImageCB);
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	HR_T(dev->CreateBuffer(&bufferDesc, nullptr, imageCbBuffer.GetAddressOf()));

	// 픽셀 셰이더 만들기
	ComPtr<ID3DBlob> pixelShaderBuffer = nullptr;
	HR_T(CompileShaderFromFile(L"Shaders\\PS_2D.hlsl", "main", "ps_5_0", &pixelShaderBuffer));
	HR_T(dev->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, imagePS.GetAddressOf()));



}

void Image::Render(ComPtr<ID3D11DeviceContext>& context)
{
	if (canvas == nullptr) return;

	// rect 계산
	float x = rect.pos.x - rect.pivot.x * rect.width;
	float y = rect.pos.y - rect.pivot.y * rect.height;	

	Matrix world =
		Matrix::CreateScale(rect.width, rect.height, 1.0f) *
		Matrix::CreateFromYawPitchRoll(rect.GetEuler()) * 
		Matrix::CreateTranslation(x, y, 0.0f);

	mvp = world * canvas->GetProjection();	// UI에서  view는 보통 identity
	imageCBData.WVP = mvp.Transpose();
	context->UpdateSubresource(imageCbBuffer.Get(), 0, nullptr, &imageCBData, 0, 0); // 상수 버퍼 업데이트

	context->VSSetConstantBuffers(1, 1, imageCbBuffer.GetAddressOf());	// vs 상수 버퍼 설정
	context->PSSetConstantBuffers(1, 1, imageCbBuffer.GetAddressOf());	// ps 상수 버퍼 설정
	context->PSSetShader(imagePS.Get(), nullptr, 0);					// ps 바인딩

	context->PSSetShaderResources(0, 1, imgSRV.GetAddressOf());			// 텍스처 리소스 바인딩
}