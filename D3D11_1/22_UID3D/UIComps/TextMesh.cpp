#include "TextMesh.h"
#include "../../Common/Helper.h"

void TextMesh::Init(ComPtr<ID3D11Device>& dev)
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
	HR_T(CompileShaderFromFile(L"Shaders\\PS_QuadText.hlsl", "main", "ps_5_0", &pixelShaderBuffer));
	HR_T(dev->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, imagePS.GetAddressOf()));
}

void TextMesh::Render(ComPtr<ID3D11DeviceContext>& context)
{
	if (canvas == nullptr) return;

	// Image.cpp와 동일
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

	context->UpdateSubresource(imageCbBuffer.Get(), 0, nullptr, &imageCBData, 0, 0); // 상수 버퍼 업데이트

	context->VSSetConstantBuffers(1, 1, imageCbBuffer.GetAddressOf());	// vs 상수 버퍼 설정
	context->PSSetConstantBuffers(1, 1, imageCbBuffer.GetAddressOf());	// ps 상수 버퍼 설정
	context->PSSetShader(imagePS.Get(), nullptr, 0);					// ps 바인딩

	context->PSSetShaderResources(1, 1, atlas.srv.GetAddressOf());			// 텍스처 리소스 바인딩
}

void TextMesh::LoadFontAtlas(ComPtr<ID3D11Device>& dev, const std::wstring fontFilePath, float fontPx, int atlasW, int atlasH, int paddingPx)
{
	// 메모리에 올라옴 
	atlas = builder.BuildASCII(dev.Get(), fontFilePath, fontPx, atlasW, atlasH, paddingPx);
}

Color TextMesh::GetColor()
{
	return color;
}

void TextMesh::SetColor(Color color)
{
	this->color = color;
}