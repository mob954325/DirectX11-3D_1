#include "TextMesh.h"
#include "../../Common/Helper.h"

// === Util ===
// UTF16 CodePoint 찾기
static void DecodeUTF16ToCodepoints(const std::wstring& s, std::vector<uint32_t>& out)
{
	out.clear();
	out.reserve(s.size());

	// Windows wchar_t는 UTF-16. 한글은 BMP라 대부분 1개 wchar_t지만
	// 이모지 등 surrogate pair도 안전하게 처리.
	for (size_t i = 0; i < s.size(); ++i)
	{
		uint32_t wc = (uint32_t)s[i]; // 한글자 값 코드

		if (wc >= 0xD800 && wc <= 0xDBFF && (i + 1) < s.size()) // 한글이면 좌표 찾기 ( codePoint )
		{
			uint32_t wc2 = (uint32_t)s[i + 1];
			if (wc2 >= 0xDC00 && wc2 <= 0xDFFF)
			{
				uint32_t high = wc - 0xD800;
				uint32_t low = wc2 - 0xDC00;
				uint32_t cp = (high << 10) + low + 0x10000;
				out.push_back(cp);
				++i;
				continue;
			}
		}

		out.push_back(wc);
	}
}

// 라인 짜르기
static void SplitLines(const std::vector<uint32_t>& cps,
	std::vector<std::pair<size_t, size_t>>& lines)
{
	lines.clear();
	size_t start = 0;
	for (size_t i = 0; i < cps.size(); ++i)
	{
		if (cps[i] == (uint32_t)L'\n') // 글자에 줄 바꿈 문자가 있으면 쪼개기 ( lines 컨테이너에 push )
		{
			lines.push_back({ start, i }); // [start, i)
			start = i + 1;
		}
	}
	lines.push_back({ start, cps.size() });
}

// === TextMesh ===
void TextMesh::Init(ComPtr<ID3D11Device>& dev)
{
	device = dev; // text rebuild할 때 사용

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

	D3D11_SAMPLER_DESC sampDesc = {};
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = 0; // mip이 1개라고 가정함.
	HR_T(dev->CreateSamplerState(&sampDesc, textSS.GetAddressOf()));
}

void TextMesh::Render(ComPtr<ID3D11DeviceContext>& context)
{
	// Dirty면 Rebuild + Upload만하고 그린다. 

	if (!canvas) return;
	if (!atlas.srv) return;

	// 지오메트리 재생성(필요시)
	if (geometryDirty)
	{
		// glyphCount는 cpuVerts.size()/4로 계산 가능
		RebuildGeometry(device);
		geometryDirty = false;
	}

	if (indexCount == 0) return;

	// VB/IB 바인딩
	UINT stride = sizeof(QuadVertex);
	UINT offset = 0;
	context->IASetVertexBuffers(0, 1, textVB.GetAddressOf(), &stride, &offset);
	context->IASetIndexBuffer(textIB.Get(), DXGI_FORMAT_R16_UINT, 0);

	// VB 업로드
	UploadVB(context);

	// 상수버퍼 업데이트
	Matrix world = MakeWorldFromRect();
	Matrix mvp = world * canvas->GetProjection();
	imageCBData.WVP = mvp.Transpose();
	imageCBData.color = color;
	context->UpdateSubresource(textCbBuffer.Get(), 0, nullptr, &imageCBData, 0, 0);

	// CB/PS/SRV 바인딩
	context->VSSetConstantBuffers(1, 1, textCbBuffer.GetAddressOf());
	context->PSSetConstantBuffers(1, 1, textCbBuffer.GetAddressOf());
	context->PSSetShader(textPS.Get(), nullptr, 0);
	context->PSSetShaderResources(1, 1, atlas.srv.GetAddressOf());

	context->DrawIndexed(indexCount, 0, 0);
}

Matrix TextMesh::MakeWorldFromRect() const
{
	// pivot 기준 이동 -> 스케일/회전 -> 위치
	Vector3 s = rect.GetScale();
	Vector3 r = rect.GetEuler();
	Vector3 p = rect.pos;

	Matrix T0 = Matrix::CreateTranslation(-rect.pivot.x * rect.width,
		-rect.pivot.y * rect.height, 0.0f);
	Matrix S = Matrix::CreateScale(s);
	Matrix R = Matrix::CreateFromYawPitchRoll(r.y, r.x, r.z);
	Matrix T1 = Matrix::CreateTranslation(p);

	return T0 * S * R * T1;
}

void TextMesh::LoadFontAtlas(ComPtr<ID3D11Device>& dev, const std::wstring fontFilePath, float fontPx, int atlasW, int atlasH, int paddingPx)
{
	// 메모리에 올라옴 
	this->fontPath = fontFilePath;
	this->fontPx = fontPx;
	this->atlasW = atlasW;
	this->atlasH = atlasH;
	this->paddingPx = paddingPx;

	atlas = builder.BuildASCII(dev.Get(), fontFilePath, fontPx, atlasW, atlasH, paddingPx);
	geometryDirty = true;
}

void TextMesh::EnsureAtlasForText(ComPtr<ID3D11Device>& dev, const std::vector<uint32_t>& cps)
{
	// 글리프 보장함수
	// builder에 재 빌드가 필요한지, 추가를 해야하는지 확인하고 설정해주는 훅 함수임.
	// if (builder.NeedsRebuild(atlas, cps)) {
	//     atlas = builder.BuildFromCodepoints(dev, fontFilePath, fontPx, atlasW, atlasH, padding, cps);
	// }
	std::vector<uint32_t> need;
	need.reserve(cps.size());

	for (uint32_t cp : cps)
	{
		if (cp == (uint32_t)L'\n') continue;
		if (atlas.glyphs.find(cp) == atlas.glyphs.end())
			need.push_back(cp);
	}

	if (need.empty()) return;

	// 현재 텍스트에 필요한 cp 전체를 굽는 방식(간단/안전)
	// + 필요하면 ASCII도 같이
	atlas = builder.BuildFromCodepoints(
		dev.Get(),
		fontPath,
		fontPx,
		atlasW, atlasH,
		cps,
		paddingPx,
		true
	);
}

void TextMesh::SetText(const std::wstring_view ws, HAlign align)
{
	text = ws;
	alignType = align;
	geometryDirty = true;
}

Color TextMesh::GetColor() const
{
	return color;
}

void TextMesh::SetColor(Color color)
{
	this->color = color;
}

float TextMesh::MeasureWidthCP(const std::vector<uint32_t>& cps, size_t b, size_t e)
{
	float w = 0;
	for (size_t i = b; i < e; ++i)
	{
		uint32_t cp = cps[i];
		auto it = atlas.glyphs.find(cp);
		if (it == atlas.glyphs.end()) continue;
		w += (float)it->second.advance;
	}
	return w;
}

void TextMesh::AppendGlyphQuad(float penX, float baselineY, const decltype(atlas.glyphs.begin()-> second)& g)
{
	// y-down 좌표계 기준:
	// top = baseline - bearingY (bearingY가 baseline->top으로 +인 메트릭일 때)
	float x0 = penX + (float)g.bearingX;
	float y0 = baselineY - (float)g.bearingY;
	float x1 = x0 + (float)g.w;
	float y1 = y0 + (float)g.h;

	cpuVerts.push_back({ Vector3(x0,y0,0), Vector2(g.u0,g.v0) });
	cpuVerts.push_back({ Vector3(x1,y0,0), Vector2(g.u1,g.v0) });
	cpuVerts.push_back({ Vector3(x0,y1,0), Vector2(g.u0,g.v1) });
	cpuVerts.push_back({ Vector3(x1,y1,0), Vector2(g.u1,g.v1) });
}

void TextMesh::RebuildGeometry(ComPtr<ID3D11Device>& dev)
{
	cpuVerts.clear();
	indexCount = 0;

	std::vector<uint32_t> cps;
	DecodeUTF16ToCodepoints(text, cps);

	// (한글 지원) 아틀라스에 필요한 글리프 확보
	EnsureAtlasForText(dev, cps);
	if (!atlas.srv) return;

	std::vector<std::pair<size_t, size_t>> lines;
	SplitLines(cps, lines);

	// 폰트 메트릭: ascent/lineHeight는 atlas에 있어야 함
	const float ascent = (float)atlas.ascentPx;
	const float lineH = (float)atlas.lineHeightPx;

	int glyphCount = 0;

	float penY = ascent; // "로컬 top=0" 기준 baseline
	for (auto [lb, le] : lines)
	{
		float lineW = MeasureWidthCP(cps, lb, le);

		float offsetX = 0.0f;
		if (alignType == HAlign::Center) offsetX = (rect.width - lineW) * 0.5f;
		else if (alignType == HAlign::Right) offsetX = (rect.width - lineW);

		float penX = offsetX;
		float baselineY = penY;

		for (size_t i = lb; i < le; ++i)
		{
			uint32_t cp = cps[i];

			auto it = atlas.glyphs.find(cp);
			if (it == atlas.glyphs.end()) continue;

			const auto& g = it->second;

			if (g.w == 0 || g.h == 0)
			{
				penX += (float)g.advance;
				continue;
			}

			AppendGlyphQuad(penX, baselineY, g);
			++glyphCount;
			penX += (float)g.advance;
		}

		penY += lineH;
	}

	indexCount = glyphCount * 6;
}

void TextMesh::EnsureBufferCapacity(ComPtr<ID3D11Device>& dev, uint32_t glyphCount)
{
	if (glyphCount <= maxGlyphs) return;

	// 2배씩 증가
	while (glyphCount > maxGlyphs) maxGlyphs *= 2;

	// VB 재생성
	D3D11_BUFFER_DESC vb{};
	vb.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vb.Usage = D3D11_USAGE_DYNAMIC;
	vb.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	vb.ByteWidth = sizeof(QuadVertex) * 4 * maxGlyphs;
	HR_T(dev->CreateBuffer(&vb, nullptr, textVB.ReleaseAndGetAddressOf()));

	// IB 재생성(immutable)
	std::vector<uint16_t> inds;
	inds.reserve(6 * maxGlyphs);
	for (uint32_t i = 0; i < maxGlyphs; ++i)
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
	HR_T(dev->CreateBuffer(&ib, &init, textIB.ReleaseAndGetAddressOf()));
}

void TextMesh::UploadVB(ComPtr<ID3D11DeviceContext>& context)
{
	if (cpuVerts.empty()) return;

	D3D11_MAPPED_SUBRESOURCE mapped{};
	HR_T(context->Map(textVB.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped));
	memcpy(mapped.pData, cpuVerts.data(), cpuVerts.size() * sizeof(QuadVertex));
	context->Unmap(textVB.Get(), 0);
}