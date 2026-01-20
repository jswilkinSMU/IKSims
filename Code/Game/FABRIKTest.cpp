#include "Game/FABRIKTest.hpp"
#include "Game/App.h"
#include "Engine/Input/InputSystem.h"
#include "Engine/Core/DebugRender.hpp"
#include "Engine/Core/EngineCommon.h"

FABRIKTest::FABRIKTest(App* owner)
	:Game(owner)
{
}

void FABRIKTest::StartUp()
{
	m_font = g_theRenderer->CreateOrGetBitmapFont("Data/Fonts/SquirrelFixedFont");
	m_skeleton = CreateTestChain();
	m_skeleton.AddVertsForSkeleton3D(m_skeletonVerts);
}

void FABRIKTest::Update()
{
	double deltaSeconds = g_theApp->m_gameClock->GetDeltaSeconds();
	double totalTime = g_theApp->m_gameClock->GetTotalSeconds();
	double frameRate = Clock::GetSystemClock().GetFrameRate();

	Vec3 targetPosition = Vec3(0.0f, sinf(static_cast<float>(totalTime) * 2.0f) * 5.f, 0.5f);
	std::vector<int> boneChain;
	for (int i = 0; i < static_cast<int>(m_skeleton.m_bones.size()); ++i) 
	{
		boneChain.push_back(i);
	}
	m_skeleton.SolveFABRIK(boneChain, targetPosition);
	m_skeletonVerts.clear();
	m_skeleton.AddVertsForSkeleton3D(m_skeletonVerts);

	DebugAddWorldSphere(targetPosition, 0.25f, 0.f, Rgba8::GREEN, Rgba8::GREEN);

	// Set text for position, time, FPS, and scale
	std::string timeScaleText = Stringf("Time: %0.2fs FPS: %0.2f", totalTime, frameRate);
	DebugAddScreenText(timeScaleText, AABB2(0.f, 0.f, SCREEN_SIZE_X, SCREEN_SIZE_Y), 15.f, Vec2(0.98f, 0.97f), 0.f);
	DebugAddScreenText("FABRIK Test", m_gameSceneBounds, 25.f, Vec2(0.5f, 0.95f), 0.f);
	DebugAddScreenText("Up Arrow  : Add Joint", m_gameSceneBounds, 17.5f, Vec2(0.f, 0.97f), 0.f);
	DebugAddScreenText("Down Arrow: Remove Joint", m_gameSceneBounds, 17.5f, Vec2(0.f, 0.94f), 0.f);

	if (g_theInput->WasKeyJustPressed(KEYCODE_UPARROW))
	{
		AddJoint();
	}
	if (g_theInput->WasKeyJustPressed(KEYCODE_DOWNARROW))
	{
		RemoveJoint();
	}

	AdjustForPauseAndTimeDistortion(static_cast<float>(deltaSeconds));
	KeyInputPresses();
	UpdateCameras(static_cast<float>(deltaSeconds));
}

void FABRIKTest::Render() const
{
	if (m_isAttractMode == false)
	{
		g_theRenderer->BeginCamera(m_gameWorldCamera);
		g_theRenderer->ClearScreen(Rgba8::BROWN);
		RenderSkeleton();
		g_theRenderer->EndCamera(m_gameWorldCamera);

		g_theRenderer->BeginCamera(g_theApp->m_screenCamera);
		//PrintBoneHierarchy();
		g_theRenderer->EndCamera(g_theApp->m_screenCamera);

		DebugRenderWorld(m_gameWorldCamera);
		DebugRenderScreen(g_theApp->m_screenCamera);
	}
}

void FABRIKTest::Shutdown()
{
}

Skeleton FABRIKTest::CreateTestChain()
{
	Skeleton skeleton;
	skeleton.m_bones.clear();

	Bone first;
	first.m_boneName = "first";
	first.SetLocalBonePosition(Vec3::ZERO);
	skeleton.m_bones.push_back(first);
	skeleton.m_bones[0].m_childBoneIndices.push_back(1);

	Bone second;
	second.m_boneName = "second";
	second.m_parentBoneIndex = 0;
	second.SetLocalBonePosition(Vec3::ZAXE);
	skeleton.m_bones.push_back(second);
	skeleton.m_bones[1].m_childBoneIndices.push_back(2);

	Bone third;
	third.m_boneName = "third";
	third.m_parentBoneIndex = 1;
	third.SetLocalBonePosition(Vec3::ZAXE);
	skeleton.m_bones.push_back(third);
	skeleton.m_bones[2].m_childBoneIndices.push_back(3);

	Bone fourth;
	fourth.m_boneName = "fourth";
	fourth.m_parentBoneIndex = 2;
	fourth.SetLocalBonePosition(Vec3::ZAXE);
	skeleton.m_bones.push_back(fourth);
	skeleton.m_bones[3].m_childBoneIndices.push_back(4);

	Bone fifth;
	fifth.m_boneName = "fifth";
	fifth.m_parentBoneIndex = 3;
	fifth.SetLocalBonePosition(Vec3::ZAXE);
	skeleton.m_bones.push_back(fifth);
	skeleton.m_bones[4].m_childBoneIndices.push_back(5);

	skeleton.UpdateSkeletonPose();
	return skeleton;
}

void FABRIKTest::UpdateCameras(float deltaSeconds)
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

void FABRIKTest::AddJoint()
{
	int parentIndex = static_cast<int>(m_skeleton.m_bones.size()) - 1;
	Bone newBone;
	newBone.m_parentBoneIndex = parentIndex;
	newBone.SetLocalBonePosition(Vec3::ZAXE);
	newBone.m_boneName = Stringf("bone_%d", parentIndex + 1);
	m_skeleton.m_bones.push_back(newBone);
	m_skeleton.m_bones[parentIndex].m_childBoneIndices.push_back(static_cast<int>(m_skeleton.m_bones.size()) - 1);

	m_skeleton.UpdateSkeletonPose();
}

void FABRIKTest::RemoveJoint()
{
	int boneCount = static_cast<int>(m_skeleton.m_bones.size());
	if (boneCount <= 1)
	{
		return;
	}

	int removeIndex = boneCount - 1;
	int parentIndex = m_skeleton.m_bones[removeIndex].m_parentBoneIndex;

	std::vector<unsigned int>& siblings = m_skeleton.m_bones[parentIndex].m_childBoneIndices;
	siblings.erase(std::remove(siblings.begin(), siblings.end(), static_cast<unsigned int>(removeIndex)), siblings.end());

	m_skeleton.m_bones.pop_back();
	m_skeleton.UpdateSkeletonPose();
}

void FABRIKTest::RenderSkeleton() const
{
	g_theRenderer->SetModelConstants();
	g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->DrawVertexArray(m_skeletonVerts);
}
