#pragma once
#include "Transform.h"

class RectTransform : public Transform
{
public:


	/// <summary>
	/// 앵커를 기준으로 하는 사각형 피벗 포지션
	/// </summary>
	Vector3 pos{};

	/// <summary>
	/// 직사각형 넓이
	/// </summary>
	float width{};

	/// <summary>
	/// 직사각형 높이
	/// </summary>
	float height{};

	/// <summary>
	/// 회전 중심 점 위치, ((0,0)은 왼쪽 하단, (1,1)은 오른쪽 상단)
	/// </summary>
	Vector2 pivot{};
};