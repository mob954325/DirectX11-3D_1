#pragma once

#include <directXTK/Mouse.h>
#include <directXTK/Keyboard.h>

using namespace DirectX;

/// <summary>
/// App에서 사용하는 Input 처리 함수 ( GameApp이 상속받음. )
/// </summary>
class InputProcesser
{
public:
	virtual void OnInputProcess(const Keyboard::State& KeyState, const Keyboard::KeyboardStateTracker& KeyTracker,
		const Mouse::State& MouseState, const Mouse::ButtonStateTracker& MouseTracker) = 0;
};

/// <summary>
/// Input 관련 처리 싱글톤 클래스
/// </summary>
class InputSystem
{
	InputSystem();
	~InputSystem() {};

	static InputSystem* Instance;

	InputProcesser* m_pInputProcessers = nullptr;

	// input
	std::unique_ptr<DirectX::Keyboard>              m_Keyboard;
	std::unique_ptr<DirectX::Mouse>                 m_Mouse;
	DirectX::Keyboard::KeyboardStateTracker         m_KeyboardStateTracker;
	DirectX::Mouse::ButtonStateTracker              m_MouseStateTracker;

	DirectX::Mouse::State                           m_MouseState;
	DirectX::Keyboard::State                        m_KeyboardState;

	void Update(float DeltaTime);
	bool Initialize(HWND hWnd, InputProcesser* processer);
};