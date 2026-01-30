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
	if (!atlas.srv) return;

	context->VSSetConstantBuffers(1, 1, imageCbBuffer.GetAddressOf());	// vs 상수 버퍼 설정
	context->PSSetConstantBuffers(1, 1, imageCbBuffer.GetAddressOf());	// ps 상수 버퍼 설정
	context->PSSetShader(imagePS.Get(), nullptr, 0);					// ps 바인딩
	context->PSSetShaderResources(1, 1, atlas.srv.GetAddressOf());			// 텍스처 리소스 바인딩

	float rot = rect.GetEuler().z;

	// 글자 draw
	for (auto& d : draws)
	{
		float pivX = 0.0f, pivY = 0.0f;

		Matrix world =
			Matrix::CreateTranslation(-pivX, -pivY, 0.0f) *
			Matrix::CreateScale(d.w, d.h, 0.0f) *
			Matrix::CreateRotationZ(rot) *
			Matrix::CreateTranslation(d.x, d.y, 0.0f);

		Matrix mvp = world * canvas->GetProjection();
		imageCBData.WVP = mvp.Transpose();
		imageCBData.color = color;
		imageCBData.uvRect = Vector4(d.u0, d.v0, d.u1, d.v1);

		context->UpdateSubresource(imageCbBuffer.Get(), 0, nullptr, &imageCBData, 0, 0);

		context->DrawIndexed(6, 0, 0);
	}
}

void TextMesh::LoadFontAtlas(ComPtr<ID3D11Device>& dev, const std::wstring fontFilePath, float fontPx, int atlasW, int atlasH, int paddingPx)
{
	// 메모리에 올라옴 
	atlas = builder.BuildASCII(dev.Get(), fontFilePath, fontPx, atlasW, atlasH, paddingPx);
}

void TextMesh::SetText(const std::wstring& s)
{
	text = s;
	draws.clear();

	// 텍스트 시작점: rect.pos가 "텍스트 블록의 좌상단"이라고 가정
	float penX = rect.pos.x;
	float penY = rect.pos.y + (float)atlas.ascentPx; // baseline y (y-down)

	for (wchar_t wc : text)
	{
		uint32_t cp = (uint32_t)wc; // char -> uint

		// 문자 위치 찾기
		auto it = atlas.glyphs.find(cp); 
		if (it == atlas.glyphs.end()) continue;

		const auto& g = it->second;

		// 공백은 w/h 0일 수 있음: advance만 적용
		if (g.w == 0 || g.h == 0)
		{
			penX += (float)g.advance;
			continue;
		}

		float x = penX + (float)g.bearingX;
		float y = penY + (float)g.bearingY; // bounds.top이 음수면 위로 올라감

		// 그릴 글자 추가
		draws.push_back(GlyphDraw{
			x, y,
			(float)g.w, (float)g.h,
			g.u0,g.v0,g.u1,g.v1,
			g.advance
			});

		penX += (float)g.advance;
	}
}

Color TextMesh::GetColor()
{
	return color;
}

void TextMesh::SetColor(Color color)
{
	this->color = color;
}