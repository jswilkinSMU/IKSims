#include "Game/Game.h"
#include "Game/App.h"
#include "Engine/Input/InputSystem.h"

void Game::KeyInputPresses()
{
	// Attract Mode
	if (g_theInput->WasKeyJustPressed(KEYCODE_SPACE))
	{
		m_isAttractMode = false;
	}
	if (g_theInput->WasKeyJustPressed(KEYCODE_ESC))
	{
		m_isAttractMode = true;
	}
}

void Game::AdjustForPauseAndTimeDistortion(float deltaSeconds) {

	UNUSED(deltaSeconds);

	if (g_theInput->IsKeyDown('T'))
	{
		g_theApp->m_gameClock->SetTimeScale(0.1);
	}
	else
	{
		g_theApp->m_gameClock->SetTimeScale(1.0);
	}

	if (g_theInput->WasKeyJustPressed('P'))
	{
		g_theApp->m_gameClock->TogglePause();
	}

	if (g_theInput->WasKeyJustPressed('O'))
	{
		g_theApp->m_gameClock->StepSingleFrame();
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_ESC) && m_isAttractMode)
	{
		g_theEventSystem->FireEvent("Quit");
	}
}

bool Game::IsAttractMode() const
{
	return m_isAttractMode;
}

void Game::FreeFlyControls(float deltaSeconds)
{
	// Yaw and Pitch with mouse
	m_cameraOrientation.m_yawDegrees += 0.08f * g_theInput->GetCursorClientDelta().x;
	m_cameraOrientation.m_pitchDegrees -= 0.08f * g_theInput->GetCursorClientDelta().y;

	float movementSpeed = 2.f;
	// Increase speed by a factor of 10
	if (g_theInput->IsKeyDown(KEYCODE_SHIFT))
	{
		movementSpeed *= 20.f;
	}

	// Move left or right
	if (g_theInput->IsKeyDown('A'))
	{
		m_cameraPos += movementSpeed * m_cameraOrientation.GetAsMatrix_IFwd_JLeft_KUp().GetJBasis3D() * deltaSeconds;
	}
	if (g_theInput->IsKeyDown('D'))
	{
		m_cameraPos += -movementSpeed * m_cameraOrientation.GetAsMatrix_IFwd_JLeft_KUp().GetJBasis3D() * deltaSeconds;
	}

	// Move Forward and Backward
	if (g_theInput->IsKeyDown('W'))
	{
		m_cameraPos += movementSpeed * m_cameraOrientation.GetAsMatrix_IFwd_JLeft_KUp().GetIBasis3D() * deltaSeconds;
	}
	if (g_theInput->IsKeyDown('S'))
	{
		m_cameraPos += -movementSpeed * m_cameraOrientation.GetAsMatrix_IFwd_JLeft_KUp().GetIBasis3D() * deltaSeconds;
	}

	// Move Up and Down
	if (g_theInput->IsKeyDown('Z'))
	{
		m_cameraPos += -movementSpeed * Vec3::ZAXE * deltaSeconds;
	}
	if (g_theInput->IsKeyDown('C'))
	{
		m_cameraPos += movementSpeed * Vec3::ZAXE * deltaSeconds;
	}
}

Vec3 Game::GetCameraFwdNormal() const
{
	return Vec3::MakeFromPolarDegrees(m_cameraOrientation.m_pitchDegrees, m_cameraOrientation.m_yawDegrees);
}
