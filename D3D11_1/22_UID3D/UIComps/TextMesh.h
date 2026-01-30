#pragma once
#include "UIBase.h"
#include "../../Common/pch.h"
#include "RectTransform.h"
#include "UIData.h"
#include "Font/FontAtlasBuilder.h"

class TextMesh : public UIBase
{
public:
	void Init(ComPtr<ID3D11Device>& dev) override;
	void Render(ComPtr<ID3D11DeviceContext>& context) override;
	// void GetCB(ComPtr<ID3D11Buffer>& cb);

	// 임시 로컬 로드 함수
	void LoadFontAtlas(ComPtr<ID3D11Device>& dev, const std::wstring fontFilePath, float fontPx, int atlasW, int atlasH, int paddingPx);

	Color GetColor();
	void SetColor(Color color);

private:
	ComPtr<ID3D11Buffer>				imageCbBuffer{};	// image용 상수 버퍼->나중에 매니저로 모아두기
	ComPtr<ID3D11PixelShader>			imagePS{};			// 이미지에 사용할 ps

	ImageCBData imageCBData{};	//	상수버퍼
	Matrix mvp{};					// model view projection
	
	FontAtlasBuilder builder{}; // TODO 매니징 하는 클래스에서 뿌리게 변경해야함
	FontAtlas atlas{};

	std::string text{}; // 출력할 텍스트
	Color color{};
};

