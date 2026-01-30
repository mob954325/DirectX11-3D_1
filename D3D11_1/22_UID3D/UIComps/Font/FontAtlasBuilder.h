#pragma once
#include "../../../Common/pch.h"
#include "../../../Common/Helper.h"
#include <unordered_map>
#include "dwrite.h" // glyph 실행헤 필요한 low-level 정보 

#undef max
#undef min

/// <summary>
/// glyph 위치정보
/// </summary>
struct GlyphInfo
{
    uint32_t codepoint;		// ASCII면 char로 대체 가능
    uint16_t glyphIndex;	// DWrite glyph index

    // 아틀라스 좌표 ( 정규화 UV )
    float u0, v0, u1, v1;

    // 비트맵 크기 ( px )
    int w, h;

    // 베어링 ( px ) : pen(베이스라인 기준)에서 glyph bitmap 좌상단까지 -> ???
    int bearingX;
    int bearingY;

    // advance ( px ) -> ??
    int advance;
};

/// <summary>
/// 폰트 텍스처 아틀라스
/// </summary>
struct FontAtlas
{
    int atlasW = 0, atlasH = 0;

    ComPtr<ID3D11Texture2D> texture;
    ComPtr<ID3D11ShaderResourceView> srv;

    std::unordered_map<uint32_t, GlyphInfo> glyphs;
    int ascentPx; // baseline 계산에 필요하면 보간
};

/// <summary>
/// 아틀라스 패킹 : 행 채우기 
/// </summary>
struct ShelfPacker
{
    int W, H;
    int x = 0, y = 0, rowH = 0;

    ShelfPacker(int w, int h) : W(w), H(h), x(0), y(0), rowH(0) {}


    bool TryAlloc(int w, int h, int& outX, int& outY)
    {
        if (x + w > W) { x = 0; y += rowH; rowH = 0; }
        if (x + h > H) return false; // 높이 초과
        outX = x; outY = y;
        x += w;
        rowH = std::max(rowH, h);
        return true;
    }
};


/// <summary>
///
/// </summary>
class FontAtlasBuilder
{
public:
    static FontAtlas BuildASCII(ID3D11Device* dev, const std::wstring fontFilePath, 
                            float fontPx, // 예: 32.0f
                            int atlasW, int atlasH, // 1024x1024, 2048x2048
                            int paddingPx = 1);
};