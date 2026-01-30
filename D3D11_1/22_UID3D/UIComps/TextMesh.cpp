#include "TextMesh.h"
#include "../../Common/Helper.h"

void TextMesh::Init(ComPtr<ID3D11Device>& dev)
{
	// vb ( dynamic )
	D3D11_BUFFER_DESC vb{};
	vb.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vb.Usage = D3D11_USAGE_DYNAMIC;
	vb.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	vb.ByteWidth = sizeof(QuadVertex) * 4 * 256; // maxGlyphs;
	HR_T(dev->CreateBuffer(&vb, nullptr, textVB.GetAddressOf()));

	// IB (한 번 만들고 재사용: maxGlyphs까지) -> 임시 256
	std::vector<uint16_t> inds;
	inds.reserve(6 * 256);
	for (int i = 0; i < 256; ++i)
	{
		uint16_t base = (uint16_t)(i * 4);
		inds.push_back(base + 0);
		inds.push_back(base + 1);
		inds.push_back(base + 2);
		inds.push_back(base + 1);
		inds.push_back(base + 3);
		inds.push_back(base + 2);
	}

	D3D11_BUFFER_DESC ib{};
	ib.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ib.Usage = D3D11_USAGE_IMMUTABLE;
	ib.ByteWidth = (UINT)(inds.size() * sizeof(uint16_t));
	D3D11_SUBRESOURCE_DATA init{};
	init.pSysMem = inds.data();
	HR_T(dev->CreateBuffer(&ib, &init, textIB.GetAddressOf()));

	// 상수 버퍼 만들기
	D3D11_BUFFER_DESC bufferDesc{};
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(ImageCBData);
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	HR_T(dev->CreateBuffer(&bufferDesc, nullptr, textCbBuffer.GetAddressOf()));

	// 픽셀 셰이더 만들기
	ComPtr<ID3DBlob> pixelShaderBuffer = nullptr;
	HR_T(CompileShaderFromFile(L"Shaders\\PS_QuadText.hlsl", "main", "ps_5_0", &pixelShaderBuffer));
	HR_T(dev->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, textPS.GetAddressOf()));
}

void TextMesh::Render(ComPtr<ID3D11DeviceContext>& context)
{
	if (canvas == nullptr) return;
	if (!atlas.srv) return;

	// TODO dirty 추가하기
	D3D11_MAPPED_SUBRESOURCE mapped{};
	HR_T(context->Map(textVB.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped));
	memcpy(mapped.pData, cpuVerts.data(), cpuVerts.size() * sizeof(QuadVertex));
	context->Unmap(textVB.Get(), 0);

	UINT stride = sizeof(QuadVertex);
	UINT offset = 0;
	context->IASetVertexBuffers(0, 1, textVB.GetAddressOf(), &stride, &offset);
	context->IASetIndexBuffer(textIB.Get(), DXGI_FORMAT_R16_UINT, 0);

	context->VSSetConstantBuffers(1, 1, textCbBuffer.GetAddressOf());	// vs 상수 버퍼 설정
	context->PSSetConstantBuffers(1, 1, textCbBuffer.GetAddressOf());	// ps 상수 버퍼 설정

	context->PSSetShader(textPS.Get(), nullptr, 0);						// ps 바인딩
	context->PSSetShaderResources(1, 1, atlas.srv.GetAddressOf());		// 텍스처 리소스 바인딩

	float rot = rect.GetEuler().z;

	Matrix mvp = Matrix::Identity * canvas->GetProjection();
	imageCBData.WVP = mvp.Transpose();
	imageCBData.color = color;

	context->UpdateSubresource(textCbBuffer.Get(), 0, nullptr, &imageCBData, 0, 0);
	context->VSSetConstantBuffers(1, 1, textCbBuffer.GetAddressOf());
	context->PSSetConstantBuffers(1, 1, textCbBuffer.GetAddressOf());

	context->DrawIndexed(indexCount, 0, 0);
}

void TextMesh::LoadFontAtlas(ComPtr<ID3D11Device>& dev, const std::wstring fontFilePath, float fontPx, int atlasW, int atlasH, int paddingPx)
{
	// 메모리에 올라옴 
	atlas = builder.BuildASCII(dev.Get(), fontFilePath, fontPx, atlasW, atlasH, paddingPx);
}

void TextMesh::SetText(const std::wstring& s, HAlign align)
{
	text = s;

	cpuVerts.clear();
	cpuInds.clear(); // IB를 고정으로 쓰면 여기선 필요 없음

	float textW = MeasureWidth(text);
	float offsetX = 0.0f;
	if (align == HAlign::Center) offsetX = (rect.width - textW) * 0.5f;
	else if (align == HAlign::Right) offsetX = (rect.width - textW);

	// baseline: rect.pos를 “좌상단”으로 보고 ascent만큼 내려 baseline 잡기
	float penX = rect.pos.x + offsetX;
	float penY = rect.pos.y + (float)atlas.ascentPx;

	int glyphCount = 0;

	for (wchar_t wc : text)
	{
		uint32_t cp = (uint32_t)wc;				// char -> uint
		auto it = atlas.glyphs.find(cp);		// 글자 찾기
		if (it == atlas.glyphs.end()) continue; // 없으면 무시

		const auto& g = it->second;

		if (g.w == 0 || g.h == 0)
		{
			penX += (float)g.advance;
			continue;
		}

		float x0 = penX + (float)g.bearingX;
		float y0 = penY + (float)g.bearingY; // top
		float x1 = x0 + (float)g.w;
		float y1 = y0 + (float)g.h;

		// NOTE : 만약 vetex정보가 달라지면 따로 TextVertex 만들기
		QuadVertex v0{ Vector3(x0, y0, 0.0f), Vector2(g.u0, g.v0) };
		QuadVertex v1{ Vector3(x1, y0, 0.0f), Vector2(g.u1, g.v0) };
		QuadVertex v2{ Vector3(x0, y1, 0.0f), Vector2(g.u0, g.v1) };
		QuadVertex v3{ Vector3(x1, y1, 0.0f), Vector2(g.u1, g.v1) };

		cpuVerts.push_back(v0);
		cpuVerts.push_back(v1);
		cpuVerts.push_back(v2);
		cpuVerts.push_back(v3);

		glyphCount++;
		penX += (float)g.advance; // ?
	}

	indexCount = glyphCount * 6; // ??

	// VB 업로드 
	if (cpuVerts.empty()) return;
}

float  TextMesh::MeasureWidth(const std::wstring& s)
{
	float w = 0;
	for (wchar_t wc : s)
	{
		auto it = atlas.glyphs.find((uint32_t)wc);
		if (it == atlas.glyphs.end()) continue;
		w += (float)it->second.advance;
	}
	return w;
}

Color TextMesh::GetColor()
{
	return color;
}

void TextMesh::SetColor(Color color)
{
	this->color = color;
}