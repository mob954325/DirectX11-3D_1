#include "FontAtlasBuilder.h"

static void DebugPrintfW(const wchar_t* fmt, ...)
{
    wchar_t buf[1024];

    va_list ap;
    va_start(ap, fmt);
    vswprintf(buf, sizeof(buf) / sizeof(buf[0]), fmt, ap);
    va_end(ap);

    OutputDebugStringW(buf);
}

FontAtlas FontAtlasBuilder::BuildASCII(ID3D11Device* dev, const std::wstring fontFilePath, float fontPx, int atlasW, int atlasH, int paddingPx)
{
    {
        FontAtlas atlas;
        atlas.atlasW = atlasW;
        atlas.atlasH = atlasH;

        // 1) DWrite factory
        ComPtr<IDWriteFactory> dwrite;
        HR_T(DWriteCreateFactory(
            DWRITE_FACTORY_TYPE_SHARED,
            __uuidof(IDWriteFactory),
            reinterpret_cast<IUnknown**>(dwrite.GetAddressOf())));

        // 2) font file -> face
        ComPtr<IDWriteFontFile> fontFile;
        HR_T(dwrite->CreateFontFileReference(fontFilePath.c_str(), nullptr, &fontFile));

        BOOL isSupported = FALSE;
        DWRITE_FONT_FILE_TYPE fileType{};
        DWRITE_FONT_FACE_TYPE faceType{};
        UINT32 nFaces = 0;
        HR_T(fontFile->Analyze(&isSupported, &fileType, &faceType, &nFaces));
        if (!isSupported || nFaces == 0) throw std::runtime_error("font not supported");

        ComPtr<IDWriteFontFace> face;
        IDWriteFontFile* files[] = { fontFile.Get() };
        HR_T(dwrite->CreateFontFace(faceType, 1, files, 0, DWRITE_FONT_SIMULATIONS_NONE, &face));

        // DPI (보통 96)
        constexpr float dpi = 96.0f;

        // ascent(px) 보관 (bearingY 계산에 필요하면 사용)
        DWRITE_FONT_METRICS fm{};
        face->GetMetrics(&fm);
        atlas.ascentPx = (int)std::round(fm.ascent * fontPx / fm.designUnitsPerEm);

        // 아틀라스 CPU 버퍼 (R8)
        std::vector<uint8_t> cpu(atlasW * atlasH, 0);

        ShelfPacker pack(atlasW, atlasH); // 아틀라스의 행과 열 개수 저장?

        for (uint32_t cp = 32; cp <= 126; ++cp)
        {
            uint16_t glyphIndex = 0;
            UINT32 code = cp;
            HR_T(face->GetGlyphIndices(&code, 1, &glyphIndex));
            if (glyphIndex == 0) continue;

            // glyph metrics (design units)
            DWRITE_GLYPH_METRICS gm{};
            HR_T(face->GetDesignGlyphMetrics(&glyphIndex, 1, &gm, FALSE));

            // advance(px)
            int advancePx = (int)std::round(gm.advanceWidth * fontPx / fm.designUnitsPerEm);

            // GlyphRun 준비
            DWRITE_GLYPH_RUN run{};
            run.fontFace = face.Get();
            run.fontEmSize = fontPx;
            run.glyphCount = 1;
            run.glyphIndices = &glyphIndex;

            FLOAT advance = 0.0f; // DIP : 디바이스 독립적 픽셀
            DWRITE_GLYPH_OFFSET offset{};
            run.glyphAdvances = nullptr;
            run.glyphOffsets = &offset;

            // analysis
            ComPtr<IDWriteGlyphRunAnalysis> analysis;
            HR_T(dwrite->CreateGlyphRunAnalysis(
                &run,
                1.0f, // pixelsPerDip
                nullptr,
                DWRITE_RENDERING_MODE_ALIASED,
                DWRITE_MEASURING_MODE_GDI_CLASSIC,
                0.0f, 0.0f,
                &analysis));

            RECT bounds{};
            HR_T(analysis->GetAlphaTextureBounds(DWRITE_TEXTURE_ALIASED_1x1, &bounds));

            int bw = bounds.right - bounds.left;
            int bh = bounds.bottom - bounds.top;

            // 공백 같은 경우 bw/bh가 0일 수 있음
            if (bw <= 0 || bh <= 0)
            {
                GlyphInfo gi{};
                gi.codepoint = cp;
                gi.glyphIndex = glyphIndex;
                gi.w = gi.h = 0;
                gi.bearingX = 0;
                gi.bearingY = 0;
                gi.advance = advancePx;
                gi.u0 = gi.v0 = gi.u1 = gi.v1 = 0;
                atlas.glyphs[cp] = gi;
                continue;
            }

            // 글리프 알파 뽑기
            std::vector<uint8_t> alpha(bw * bh);
            HR_T(analysis->CreateAlphaTexture(
                DWRITE_TEXTURE_ALIASED_1x1,
                &bounds,
                alpha.data(),
                (UINT32)alpha.size()));

            // 패킹(패딩 포함)
            int allocX, allocY;
            int pw = bw + paddingPx * 2;
            int ph = bh + paddingPx * 2;
            if (!pack.TryAlloc(pw, ph, allocX, allocY))
                throw std::runtime_error("atlas full");

            int dstX = allocX + paddingPx;
            int dstY = allocY + paddingPx;

            // CPU 아틀라스에 복사
            for (int y = 0; y < bh; ++y)
            {
                uint8_t* dst = cpu.data() + (dstY + y) * atlasW + dstX;
                const uint8_t* src = alpha.data() + y * bw;
                memcpy(dst, src, bw);
            }

            // bearing(px)
            // bounds는 glyph origin(베이스라인 x=0,y=0) 기준의 bounding box.
            // 좌상단 오프셋은 (bounds.left, bounds.top)인데,
            // 일반적인 2D UI 좌표계(y down)에서 bearingY는 "baseline에서 위로 얼마나"가 필요.
            // 여기서는 Text 쿼드 생성 때 y 축을 어떻게 쓰는지에 맞춰 일관되게 처리하면 됨.
            int bearingX = bounds.left;
            int bearingY = -bounds.top; // baseline 위(+) // NOTE : 베이스라인의 기준에 따라 설정하기, 현재 기준 dirctX 좌표계

            GlyphInfo gi{};
            gi.codepoint = cp;
            gi.glyphIndex = glyphIndex;
            gi.w = bw; gi.h = bh;
            gi.bearingX = bearingX;
            gi.bearingY = bearingY;
            gi.advance = advancePx;

            gi.u0 = (float)dstX / (float)atlasW;
            gi.v0 = (float)dstY / (float)atlasH;
            gi.u1 = (float)(dstX + bw) / (float)atlasW;
            gi.v1 = (float)(dstY + bh) / (float)atlasH;

            atlas.glyphs[cp] = gi;
        }

        // 5) D3D11 R8 텍스처 생성
        D3D11_TEXTURE2D_DESC td{};
        td.Width = atlasW;
        td.Height = atlasH;
        td.MipLevels = 1;
        td.ArraySize = 1;
        td.Format = DXGI_FORMAT_R8_UNORM;
        td.SampleDesc.Count = 1;
        td.Usage = D3D11_USAGE_IMMUTABLE;
        td.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        D3D11_SUBRESOURCE_DATA init{};
        init.pSysMem = cpu.data();
        init.SysMemPitch = atlasW;        

        HR_T(dev->CreateTexture2D(&td, &init, &atlas.texture));

        D3D11_SHADER_RESOURCE_VIEW_DESC sd{};
        sd.Format = td.Format;
        sd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        sd.Texture2D.MipLevels = 1;
        HR_T(dev->CreateShaderResourceView(atlas.texture.Get(), &sd, &atlas.srv));

        return atlas;
    }
}