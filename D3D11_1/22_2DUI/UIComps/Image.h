#pragma once
#include "../../Common/pch.h"
#include "RectTransform.h"
#include "Canvas.h"

// DirectX11 2D
#include <d2d1_1.h>
#pragma comment(lib, "d2d1.lib")

#include <dwrite.h>		// DirectWrite 사용 : 문자 관련 렌더링 및 설정
#pragma comment(lib, "dwrite.lib")

#include <wincodec.h>	// WIC 디코더 : 이미지 구성요소에 대한 설정 
#pragma comment(lib, "windowscodecs.lib")

class Image
{
public:
	Canvas* canvas;

	RectTransform rect;

	ComPtr<ID2D1Bitmap1> imageBitmap;
};

