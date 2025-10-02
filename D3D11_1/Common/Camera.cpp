#include "pch.h"
#include "Camera.h"

FreeCamera::FreeCamera()
{
	Reset();
}

void FreeCamera::Reset()
{
	m_World = Matrix::Identity;
	m_Position = m_PositionInitial;
	m_Rotation = Vector3::Zero;
}

void FreeCamera::Update(float elapsedTime)
{
	if (m_InputVec.Length() > 0.0f)
	{
		m_Position += m_InputVec * m_MoveSpeed * elapsedTime;
		m_InputVec = Vector3::Zero;
	}

	m_World = Matrix::CreateFromYawPitchRoll(m_Rotation) * 
		Matrix::CreateTranslation(m_Position);	
}

void FreeCamera::SetPosition(const Vector3& posVec)
{
	m_Position = posVec;
}

void FreeCamera::SetRotation(const Vector3& rotVec)
{
	m_Rotation = rotVec;
}

void FreeCamera::GetCameraViewMatrix(Matrix& outMat)
{
	Vector3 eye = m_World.Translation();
	Vector3 target = m_World.Translation() + GetForward();
	Vector3 up = m_World.Up();
	outMat = XMMatrixLookAtLH(eye, target, up);
}

void FreeCamera::OnInputProcess(const Keyboard::State& KeyState, const Keyboard::KeyboardStateTracker& KeyTracker, const Mouse::State& MouseState, const Mouse::ButtonStateTracker& MouseTracker)
{	
	Vector3 forward = GetForward();
	Vector3 right = GetRight();

	if (KeyTracker.IsKeyPressed(Keyboard::Keyboard::R))
	{
		Reset();
	}

	if (KeyState.IsKeyDown(DirectX::Keyboard::Keys::W))
	{
		SetInputVec(forward);
	}
	else if (KeyState.IsKeyDown(DirectX::Keyboard::Keys::S))
	{
		SetInputVec(-forward);
	}

	if (KeyState.IsKeyDown(DirectX::Keyboard::Keys::A))
	{
		SetInputVec(-right);
	}
	else if (KeyState.IsKeyDown(DirectX::Keyboard::Keys::D))
	{
		SetInputVec(right);
	}

	if (KeyState.IsKeyDown(DirectX::Keyboard::Keys::E))
	{
		SetInputVec(-m_World.Up());
	}
	else if (KeyState.IsKeyDown(DirectX::Keyboard::Keys::Q))
	{
		SetInputVec(m_World.Up());
	}

	// 속도 추가
	if (KeyState.IsKeyDown(DirectX::Keyboard::Keys::D1))
	{
		m_MoveSpeed = 20;
	}
	if (KeyState.IsKeyDown(DirectX::Keyboard::Keys::D2))
	{
		m_MoveSpeed = 100;
	}
	if (KeyState.IsKeyDown(DirectX::Keyboard::Keys::D3))
	{
		m_MoveSpeed = 200;
	}

	InputSystem::Instance->m_Mouse->SetMode(MouseState.rightButton ? Mouse::MODE_RELATIVE : Mouse::MODE_ABSOLUTE);
	if (MouseState.positionMode == Mouse::MODE_RELATIVE)
	{
		Vector3 delta = Vector3(float(MouseState.x), float(MouseState.y), 0.f) * m_RotationSpeed;
		AddPitch(delta.y);
		AddYaw(delta.x);
	}
}

Vector3 FreeCamera::GetForward()
{
	return -m_World.Forward();
}

Vector3 FreeCamera::GetRight()
{
	return m_World.Right();
}

void FreeCamera::AddPitch(float value)
{
	m_Rotation.x += value;

	if (m_Rotation.x > XM_PI)
	{
		m_Rotation.x -= XM_2PI;
	}
	else if (m_Rotation.x < -XM_PI)
	{
		m_Rotation.x += XM_2PI;
	}
}

void FreeCamera::AddYaw(float value)
{
	m_Rotation.y += value;

	if (m_Rotation.y > XM_PI)
	{
		m_Rotation.y -= XM_2PI;
	}
	else if (m_Rotation.y < -XM_PI)
	{
		m_Rotation.y += XM_2PI;
	}
}

void FreeCamera::SetInputVec(const Vector3& inputVec)
{
	m_InputVec += inputVec;	
	m_InputVec.Normalize();
}