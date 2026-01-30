#pragma once
#include "UIBase.h"
#include "../../Common/pch.h"
#include "RectTransform.h"
#include "UIData.h"
#include "Font/FontAtlasBuilder.h"

//struct TextVertexData
//{
//	Vector3 position;
//	Vector2 uv;
//};

struct GlyphDraw
{
	float x, y;   // 글리프 top-left (캔버스 좌표, y-down)
	float w, h;   // 글리프 bitmap size
	float u0, v0, u1, v1;
	int advance;
};

// 정렬 타입
enum class HAlign
{
	Left, Center, Right
}; 

class TextMesh : public UIBase
{
public:
	void Init(ComPtr<ID3D11Device>& dev) override;
	void Render(ComPtr<ID3D11DeviceContext>& context) override;

	Matrix MakeWorldFromRect() const;

	// 임시 로컬 로드 함수
	void LoadFontAtlas(ComPtr<ID3D11Device>& dev, const std::wstring fontFilePath, float fontPx, int atlasW, int atlasH, int paddingPx);

	void EnsureAtlasForText(ComPtr<ID3D11Device>& dev, const std::vector<uint32_t>& cps);

	void SetText(const std::wstring& s, HAlign align = HAlign::Left); // 글자 설정
	void SetText(const std::string& s, HAlign align = HAlign::Left); // 글자 설정

	Color GetColor();
	void SetColor(Color color);

private:
	ComPtr<ID3D11Device>				device{}; // init할 때 참조한 device
	ComPtr<ID3D11Buffer>				textCbBuffer{};	// image용 상수 버퍼->나중에 매니저로 모아두기
	ComPtr<ID3D11PixelShader>			textPS{};			// 이미지에 사용할 ps

	ComPtr<ID3D11Buffer> textVB;
	ComPtr<ID3D11Buffer> textIB;
	uint32_t indexCount = 0;

	std::vector<QuadVertex> cpuVerts;	// 복사할 정점 데이터
	std::vector<uint16_t>	cpuInds;	// 인덱스 데이터 ?

	ImageCBData imageCBData{};	//	상수버퍼
	Matrix mvp{};				// model view projection ( 상수 버퍼에 넘길 위치 매트릭스 )
	
	FontAtlasBuilder builder{}; // TODO 매니징 하는 클래스에서 뿌리게 변경해야함
	FontAtlas atlas{};			// 폰트 아틀라스

	Color color{};					// 글자 전체 생깔
	std::wstring text{};			// 출력할 텍스트 

	HAlign alignType = HAlign::Left; // 폰트 정렬 타입

	std::wstring fontPath;			// 폰트 위치
	float fontPx = 0;				// 폰트 크기
	int atlasW = 0, atlasH = 0, paddingPx = 1; // 아틀라스 크기, 패딩 크기

	bool geometryDirty = true;
	uint32_t maxGlyphs = 256; // 초기값 ( 아마 글자 수 )

	/// <summary>
	/// 출력할 width 계산 함수
	/// </summary>
	float MeasureWidthCP(const std::vector<uint32_t>& cps, size_t b, size_t e);

	/// <summary>
	/// 글자 쿼드 갱신
	/// </summary>
	void AppendGlyphQuad(float penX, float baselineY, const decltype(atlas.glyphs.begin()->second)& g); // third : glyphInfo

	/// <summary>
	/// 글자 크기 확보
	/// </summary>
	void RebuildGeometry(ComPtr<ID3D11Device>& dev);

	/// <summary>
	/// Glyph 범위 초과하는 지 확인하고 넘으면 2배로 증가 시킴
	/// </summary>
	void EnsureBufferCapacity(ComPtr<ID3D11Device>& dev, uint32_t glyphCount); // NOTE : 이거 Glyph개수 고정이여서 추가한건데, 고정도 비효율적이라 처음 설정할 때 glyphCount 설정할 수 있게 수정하면 이거 제거 

	/// <summary>
	/// 버텍스 버퍼 갱신 함수 
	/// </summary>
	void UploadVB(ComPtr<ID3D11DeviceContext>& context);
};

