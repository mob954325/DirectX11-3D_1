#pragma once
#include "UIBase.h"
#include "../../Common/pch.h"
#include "RectTransform.h"
#include "UIData.h"

enum class ImageType
{
	Simple,		// 기본 이미지 출력
	Sliced,		// 이미지 중간 부분만 늘려지는거 ( 9-Sliced는 아님 )
	Fill		// 이미지 채우기
};

class Image : public UIBase
{
public:
	/// <summary>
	/// 텍스처를 path를 통해 설정하기
	/// </summary>
	/// 나중에 device 매개변수 받는거 정리하기
	void GetTexureByPath(ComPtr<ID3D11Device>& device, ComPtr<ID3D11DeviceContext>& context, std::string path);

	void Init(ComPtr<ID3D11Device>& dev) override;

	void Render(ComPtr<ID3D11DeviceContext>& context) override;

	// void GetCB(ComPtr<ID3D11Buffer>& cb);

	Color GetColor();
	void SetColor(Color color);

	ImageType GetImageType();
	void SetImageType(ImageType t);

	float GetFillAmount();
	void SetFillAmount(float v);
	
	Vector4 GetSliceBorderPx();
	void SetSliceBorderPx(float l, float r, float t, float b);

	// === 테스트 변수 === ( 옮길 때 제거 )
	Vector3 local{};

private:
	ComPtr<ID3D11Texture2D>				imgTex{};			// 렌더링할 텍스처 데이터?
	ComPtr<ID3D11ShaderResourceView>	imgSRV{};			// 출력할 텍스처 데이터
	ComPtr<ID3D11Buffer>				imageCbBuffer{};	// image용 상수 버퍼->나중에 매니저로 모아두기
	ComPtr<ID3D11PixelShader>			imagePS{};			// 이미지에 사용할 ps

	ImageCBData imageCBData{};	//	
	Matrix mvp{};				// model view projection
	std::string path{};
	Color color{};

	// 마우스 체크
	void CheckMouseHover();
	bool isMouseHover = false;

	// Image type
	ImageType type = ImageType::Simple;
	float fillAmount = 1.0f; // 이미지 채우기 정도 ( ImageType::Fill에서만 작동 )

	// 9-slice
	Vector2 texSizePx{ 1.0f, 1.0f };	// (texW, texH)
	Vector4 sliceBorderPx{ 0,0,0,0 };	// (L,R,T,B) in pixels
};