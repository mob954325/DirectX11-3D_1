#pragma once
#include "UIBase.h"
#include "../../Common/pch.h"
#include "RectTransform.h"

struct ImageCB
{
	Matrix WVP; // View Projection
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
	
private:
	ComPtr<ID3D11Texture2D>				imgTex{};			// 렌더링할 텍스처 데이터?
	ComPtr<ID3D11ShaderResourceView>	imgSRV{};			// 출력할 텍스처 데이터
	ComPtr<ID3D11Buffer>				imageCbBuffer{};	// image용 상수 버퍼->나중에 매니저로 모아두기
	ComPtr<ID3D11PixelShader>			imagePS{};			// 이미지에 사용할 ps

	ImageCB imageCBData{};	//	
	Matrix mvp{};			// model view projection
	std::string path{};
};

