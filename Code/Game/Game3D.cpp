#include "Game/Game3D.hpp"
#include "Game/App.h"
#include "Engine/Math/AABB3.hpp"
#include "Engine/Core/DebugRender.hpp"
#include "Engine/Input/InputSystem.h"

Game3D::Game3D(App* owner)
	:Game(owner)
{
}

void Game3D::StartUp()
{
	// Setting initial camera position
	m_cameraPos = Vec3(-6.5f, -0.5f, 3.f);

	// Setup text
	m_font = g_theRenderer->CreateOrGetBitmapFont("Data/Fonts/SquirrelFixedFont");
	m_font->AddVertsForTextInBox2D(m_textVerts, "Mode (F6/F7 for Prev/Next): Two-Bone IK Test (3D)", m_gameSceneBounds, 17.5f, Rgba8::ALICEBLUE, 0.8f, Vec2(0.35f, 0.965f));
	m_font->AddVertsForTextInBox2D(m_textVerts, "[1] Switch Arms", m_gameSceneBounds, 17.5f, Rgba8::ALICEBLUE, 0.8f, Vec2(0.258f, 0.925f));
	m_font->AddVertsForTextInBox2D(m_textVerts, "[2] Switch IK on/off, off animates freely", m_gameSceneBounds, 17.5f, Rgba8::ALICEBLUE, 0.8f, Vec2(0.35f, 0.9f));

	// Initialize skeleton
	m_skeleton = CreateTestSkeleton();
	m_skeleton.AddVertsForSkeleton3D(m_skeletonVerts);
	float yPosition = SCREEN_SIZE_Y - 30.f;
	m_skeleton.AddVertsForBoneHierarchy(m_textVerts, *m_font, yPosition);
}

void Game3D::Update()
{
	// Setting clock time variables
	double deltaSeconds = g_theApp->m_gameClock->GetDeltaSeconds();
	double totalTime = g_theApp->m_gameClock->GetTotalSeconds();
	double frameRate = Clock::GetSystemClock().GetFrameRate();

	// Toggle free animtation
	if (g_theInput->WasKeyJustPressed('2'))
	{
		m_isAnimatingFreely = !m_isAnimatingFreely;
	}

	TargetPosKeyPresses(deltaSeconds);

	if (!m_isAnimatingFreely)
	{
		ToggleArms();
	}
	else
	{
		AnimateSkeleton(static_cast<float>(deltaSeconds));
	}

	m_skeletonVerts.clear();
	m_skeleton.AddVertsForSkeleton3D(m_skeletonVerts);
	DebugAddWorldSphere(m_targetPos, 0.25f, 0.f, Rgba8::GREEN, Rgba8::GREEN);

	// Set text for position, time, FPS, and scale
	std::string timeScaleText = Stringf("Time: %0.2fs FPS: %0.2f", totalTime, frameRate);
	DebugAddScreenText(timeScaleText, AABB2(0.f, 0.f, SCREEN_SIZE_X, SCREEN_SIZE_Y), 15.f, Vec2(0.98f, 0.97f), 0.f);

	AdjustForPauseAndTimeDistortion(static_cast<float>(deltaSeconds));
	KeyInputPresses();
	UpdateCameras(static_cast<float>(deltaSeconds));
}

void Game3D::ToggleArms()
{
	if (m_rightHandSelected)
	{
		m_skeleton.SolveTwoBoneIK(8, 10, 11, m_targetPos);

		DebugAddWorldSphere(m_skeleton.GetBoneByIndex(8)->GetWorldBonePosition3D(), 0.25f, 0.f, Rgba8::RED, Rgba8::RED);
		DebugAddWorldSphere(m_skeleton.GetBoneByIndex(10)->GetWorldBonePosition3D(), 0.25f, 0.f, Rgba8::RED, Rgba8::RED);
		DebugAddWorldSphere(m_skeleton.GetBoneByIndex(11)->GetWorldBonePosition3D(), 0.25f, 0.f, Rgba8::RED, Rgba8::RED);
	}
	else
	{
		m_skeleton.SolveTwoBoneIK(7, 9, 12, m_targetPos);

		DebugAddWorldSphere(m_skeleton.GetBoneByIndex(7)->GetWorldBonePosition3D(), 0.25f, 0.f, Rgba8::RED, Rgba8::RED);
		DebugAddWorldSphere(m_skeleton.GetBoneByIndex(9)->GetWorldBonePosition3D(), 0.25f, 0.f, Rgba8::RED, Rgba8::RED);
		DebugAddWorldSphere(m_skeleton.GetBoneByIndex(12)->GetWorldBonePosition3D(), 0.25f, 0.f, Rgba8::RED, Rgba8::RED);
	}

	if (g_theInput->WasKeyJustPressed('1'))
	{
		m_rightHandSelected = !m_rightHandSelected;
	}
}

void Game3D::TargetPosKeyPresses(double deltaSeconds)
{
	if (g_theInput->IsKeyDown('I'))
	{
		m_targetPos.x += 2.f * static_cast<float>(deltaSeconds);
	}
	if (g_theInput->IsKeyDown('K'))
	{
		m_targetPos.x -= 2.f * static_cast<float>(deltaSeconds);
	}
	if (g_theInput->IsKeyDown('J'))
	{
		m_targetPos.y += 2.f * static_cast<float>(deltaSeconds);
	}
	if (g_theInput->IsKeyDown('L'))
	{
		m_targetPos.y -= 2.f * static_cast<float>(deltaSeconds);
	}
	if (g_theInput->IsKeyDown('N'))
	{
		m_targetPos.z += 2.f * static_cast<float>(deltaSeconds);
	}
	if (g_theInput->IsKeyDown('M'))
	{
		m_targetPos.z -= 2.f * static_cast<float>(deltaSeconds);
	}
}

void Game3D::Render() const
{
	if (m_isAttractMode == false)
	{
		g_theRenderer->BeginCamera(m_gameWorldCamera);
		g_theRenderer->ClearScreen(Rgba8(70, 70, 70, 255));
		RenderSkeleton();
		g_theRenderer->EndCamera(m_gameWorldCamera);

		g_theRenderer->BeginCamera(g_theApp->m_screenCamera);
		PrintBoneHierarchy();
		g_theRenderer->EndCamera(g_theApp->m_screenCamera);

		DebugRenderWorld(m_gameWorldCamera);
		DebugRenderScreen(g_theApp->m_screenCamera);
	}
}

void Game3D::Shutdown()
{
}

Skeleton Game3D::CreateTestSkeleton()
{
	Skeleton skeleton;
	skeleton.m_bones.clear();

	Bone root;
	root.m_boneName = "Root/Hip";
	root.SetLocalBonePosition(Vec3::ZAXE);
	skeleton.m_bones.push_back(root);

	Bone childBone1;
	childBone1.m_boneName = "LLeg";
	childBone1.m_parentBoneIndex = 0;
	childBone1.SetLocalBonePosition(Vec3(0.f, 1.f, -0.5f));
	skeleton.m_bones.push_back(childBone1);
	skeleton.m_bones[0].m_childBoneIndices.push_back(1);

	Bone childBone2;
	childBone2.m_boneName = "LFoot";
	childBone2.m_parentBoneIndex = 1;
	childBone2.SetLocalBonePosition(-Vec3::ZAXE);
	skeleton.m_bones.push_back(childBone2);
	skeleton.m_bones[1].m_childBoneIndices.push_back(2);

	Bone childBone3;
	childBone3.m_boneName = "RLeg";
	childBone3.m_parentBoneIndex = 0;
	childBone3.SetLocalBonePosition(Vec3(0.f, -1.f, -0.5f));
	skeleton.m_bones.push_back(childBone3);
	skeleton.m_bones[0].m_childBoneIndices.push_back(3);

	Bone childBone4;
	childBone4.m_boneName = "Body";
	childBone4.m_parentBoneIndex = 0;
	childBone4.SetLocalBonePosition(Vec3(0.f, 0.f, 3.f));
	skeleton.m_bones.push_back(childBone4);
	skeleton.m_bones[0].m_childBoneIndices.push_back(4);

	Bone childBone5;
	childBone5.m_boneName = "RFoot";
	childBone5.m_parentBoneIndex = 3;
	childBone5.SetLocalBonePosition(-Vec3::ZAXE);
	skeleton.m_bones.push_back(childBone5);
	skeleton.m_bones[3].m_childBoneIndices.push_back(5);

	Bone childBone6;
	childBone6.m_boneName = "Head";
	childBone6.m_parentBoneIndex = 4;
	childBone6.SetLocalBonePosition(Vec3::ZAXE);
	skeleton.m_bones.push_back(childBone6);
	skeleton.m_bones[4].m_childBoneIndices.push_back(6);

	Bone childBone7;
	childBone7.m_boneName = "LShoulder";
	childBone7.m_parentBoneIndex = 4;
	childBone7.SetLocalBonePosition(Vec3::YAXE);
	skeleton.m_bones.push_back(childBone7);
	skeleton.m_bones[4].m_childBoneIndices.push_back(7);

	Bone childBone8;
	childBone8.m_boneName = "RShoulder";
	childBone8.m_parentBoneIndex = 4;
	childBone8.SetLocalBonePosition(-Vec3::YAXE);
	skeleton.m_bones.push_back(childBone8);
	skeleton.m_bones[4].m_childBoneIndices.push_back(8);

	Bone childBone9;
	childBone9.m_boneName = "LArm";
	childBone9.m_parentBoneIndex = 7;
	childBone9.SetLocalBonePosition(Vec3(0.f, 0.5f, -1.f));
	skeleton.m_bones.push_back(childBone9);
	skeleton.m_bones[7].m_childBoneIndices.push_back(9);

	Bone childBone10;
	childBone10.m_boneName = "RArm";
	childBone10.m_parentBoneIndex = 8;
	childBone10.SetLocalBonePosition(Vec3(0.f, -0.5f, -1.f));
	skeleton.m_bones.push_back(childBone10);
	skeleton.m_bones[8].m_childBoneIndices.push_back(10);

	Bone childBone11;
	childBone11.m_boneName = "RHand";
	childBone11.m_parentBoneIndex = 10;
	childBone11.SetLocalBonePosition(Vec3(0.f, -0.5f, -0.5f));
	skeleton.m_bones.push_back(childBone11);
	skeleton.m_bones[10].m_childBoneIndices.push_back(11);

	Bone childBone12;
	childBone12.m_boneName = "LHand";
	childBone12.m_parentBoneIndex = 9;
	childBone12.SetLocalBonePosition(Vec3(0.f, 0.5f, -0.5f));
	skeleton.m_bones.push_back(childBone12);
	skeleton.m_bones[9].m_childBoneIndices.push_back(12);

	skeleton.UpdateSkeletonPose();

	return skeleton;
}

void Game3D::AnimateSkeleton(float deltaSeconds)
{
	static float elapsedTime = 0.f;
	elapsedTime += deltaSeconds;

	Bone* leftShoulder = m_skeleton.GetBoneByName("LShoulder");
	Bone* rightShoulder = m_skeleton.GetBoneByName("RShoulder");

	float swingAngleDegrees = 2.f * sinf(elapsedTime);

	Quat leftArmRotation = Quat::MakeFromAxisAngle(Vec3::XAXE, swingAngleDegrees);
	Quat rightArmRotation = Quat::MakeFromAxisAngle(Vec3::XAXE, -swingAngleDegrees);

	leftShoulder->SetLocalBoneRotation(leftArmRotation);
	rightShoulder->SetLocalBoneRotation(rightArmRotation);

	m_skeleton.UpdateSkeletonPose();

	m_skeletonVerts.clear();
	m_skeleton.AddVertsForSkeleton3D(m_skeletonVerts);
}

void Game3D::UpdateCameras(float deltaSeconds)
{
	// Game Camera
	Mat44 cameraToRender(Vec3::ZAXE, -Vec3::XAXE, Vec3::YAXE, Vec3::ZERO);
	m_gameWorldCamera.SetCameraToRenderTransform(cameraToRender);

	FreeFlyControls(deltaSeconds);
	m_cameraOrientation.m_pitchDegrees = GetClamped(m_cameraOrientation.m_pitchDegrees, -85.f, 85.f);
	m_cameraOrientation.m_rollDegrees = GetClamped(m_cameraOrientation.m_rollDegrees, -45.f, 45.f);

	m_gameWorldCamera.SetPositionAndOrientation(m_cameraPos, m_cameraOrientation);
	m_gameWorldCamera.SetPerspectiveView(2.f, 60.f, 0.1f, 100.f);
}

void Game3D::RenderSkeleton() const
{
	g_theRenderer->SetModelConstants();
	g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->DrawVertexArray(m_skeletonVerts);
}

void Game3D::PrintBoneHierarchy() const
{
	g_theRenderer->SetModelConstants();
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_NONE);
	g_theRenderer->BindTexture(&m_font->GetTexture());
	g_theRenderer->DrawVertexArray(m_textVerts);

	std::vector<Vertex_PCU> quadVerts;
	AddVertsForAABB2D(quadVerts, AABB2(Vec2(0.f, 515.f), Vec2(250.f, 800.f)), Rgba8(0, 0, 0, 80));
	g_theRenderer->SetModelConstants();
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_NONE);
	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->DrawVertexArray(quadVerts);
}