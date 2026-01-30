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
	// void GetCB(ComPtr<ID3D11Buffer>& cb);

	// 임시 로컬 로드 함수
	void LoadFontAtlas(ComPtr<ID3D11Device>& dev, const std::wstring fontFilePath, float fontPx, int atlasW, int atlasH, int paddingPx);

	void SetText(const std::wstring& s, HAlign align = HAlign::Left);
	float MeasureWidth(const std::wstring& s); // 출력할 width 계산 함수

	Color GetColor();
	void SetColor(Color color);

private:
	ComPtr<ID3D11Buffer>				textCbBuffer{};	// image용 상수 버퍼->나중에 매니저로 모아두기
	ComPtr<ID3D11PixelShader>			textPS{};			// 이미지에 사용할 ps

	ComPtr<ID3D11Buffer> textVB;
	ComPtr<ID3D11Buffer> textIB;
	uint32_t indexCount = 0;

	std::vector<QuadVertex> cpuVerts;	// NOTE : 구조가 같아서 사용하는데 나중에 텍스트 메쉬가 구조가 달라지면 분리하기
	std::vector<uint16_t>	cpuInds;	

	ImageCBData imageCBData{};	//	상수버퍼
	Matrix mvp{};					// model view projection
	
	FontAtlasBuilder builder{}; // TODO 매니징 하는 클래스에서 뿌리게 변경해야함
	FontAtlas atlas{};

	Color color{};
	std::wstring text{}; // 출력할 텍스트 
	std::vector<GlyphDraw> draws; // 글자 하나씩 draw -> 비효율적이긴함

	HAlign alignType = HAlign::Left;
};

