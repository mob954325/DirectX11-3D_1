#pragma once
#include "../../Common/pch.h"
#include "RectTransform.h"
#include "Canvas.h"

class UIBase
{
public:
	/// <summary>
	/// 참조 받을 canvas 객체
	/// </summary>
	Canvas* canvas;

	/// <summary>
	/// 위치 정보
	/// </summary>
	RectTransform rect;

	/// <summary>
	/// Canvas 등록 시 호출되는 함수
	/// </summary>
	virtual void Init(ComPtr<ID3D11Device>& dev) {};

	/// <summary>
	/// 매 프레임 호출되는 draw
	/// </summary>
	virtual void Render(ComPtr<ID3D11DeviceContext>& context) = 0;
};