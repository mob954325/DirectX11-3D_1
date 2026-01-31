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
	/// 가로 새로 크기
	/// </summary>
	Vector2 size{};

	/// <summary>
	/// 회전 중심 점 위치, ((0,0)은 왼쪽 하단, (1,1)은 오른쪽 상단)
	/// </summary>
	Vector2 pivot{};

	Matrix local;	// NOTE : 나중에 변경
	Matrix world;	// NOTE : 나중에 변경

	void OnUpdate(float delta) override;

	Vector3 GetPos() const;
	void SetPos(const Vector3& vec);
	Vector2 GetSize() const;
	void SetSize(const Vector2& vec);
	Vector2 GetPivot() const;
	void SetPivot(const Vector2& vec);

	Matrix GetWorld();

	bool isDirty = false;
};