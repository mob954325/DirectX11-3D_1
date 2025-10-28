#pragma once
#include <directxtk/SimpleMath.h>
#include "InputSystem.h"

using namespace DirectX::SimpleMath;

class FreeCamera : public InputProcesser
{
public:
	FreeCamera();
	Vector3 m_Position{};
	Vector3 m_PositionInitial = { 0,0,-30.0f };
	Vector3 m_Rotation{};
	Matrix m_World{};

	Vector3 m_InputVec{};
	float m_MoveSpeed = 100.0f;
	float m_RotationSpeed = 0.004f;

	void Reset();
	void Update(float elapsedTime);

	void SetPosition(const Vector3& posVec);
	void SetRotation(const Vector3& rotVec);
	void GetCameraViewMatrix(Matrix& outMat);

	// 카메라 이동 함수
	virtual void OnInputProcess(const Keyboard::State& KeyState, const Keyboard::KeyboardStateTracker& KeyTracker,
		const Mouse::State& MouseState, const Mouse::ButtonStateTracker& MouseTracker);

protected:
	Vector3 GetForward();
	Vector3 GetRight();
	void AddPitch(float value);
	void AddYaw(float value);

	void SetInputVec(const Vector3& inputVec);
};