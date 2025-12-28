#pragma once
#include <directxtk/SimpleMath.h>

using namespace DirectX::SimpleMath;

class AnimationKey
{
public:
	float m_time;			// 키 프레임 시간
	Vector3 m_position;		// 아마 해당 본의 상대적 위치일 것으로 추정
	Quaternion m_rotation;	// 
	Vector3 m_scale;		// 
};
