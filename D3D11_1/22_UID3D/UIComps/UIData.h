#pragma once

// 상수버퍼
struct ImageCBData
{
	Matrix  WVP;        // imageWVP
	Color   color;      // ImageBaseColor
	Vector4 uvRect;     // L,R,T,B (px)
	Vector4 params;     // imageParams (x=type, y=fillAmount)
	Vector4 imageSize;  // (rectW, rectH, texW, texH)
};