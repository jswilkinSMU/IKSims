#include "Game/Game2D.hpp"
#include "Game/App.h"
#include "Engine/Core/EngineCommon.h"
#include "Engine/Input/InputSystem.h"

Game2D::Game2D(App* owner)
	:Game(owner)
{
}

Game2D::~Game2D()
{
}

void Game2D::StartUp()
{
	m_font = g_theRenderer->CreateOrGetBitmapFont("Data/Fonts/SquirrelFixedFont");

	// Initialize skeleton
	m_skeleton = CreateTestSkeleton2D();
	m_skeletonStyle.m_boneRadius = 15.f;
	m_skeletonStyle.m_jointRadius = 20.f;
	m_skeleton.AddVertsForSkeleton2D(m_skeletonVerts, m_skeletonStyle);
}

void Game2D::Update()
{
	double deltaSeconds = g_theApp->m_gameClock->GetDeltaSeconds();
	AnimateSkeleton(static_cast<float>(deltaSeconds));
	ConstraintKeyPresses();
	AdjustForPauseAndTimeDistortion(static_cast<float>(deltaSeconds));
	KeyInputPresses();
}

void Game2D::ConstraintKeyPresses()
{
	if (g_theInput->WasKeyJustPressed('4'))
	{
		m_constraintMode = ConstraintMode::FORTY_FIVE;
	}
	if (g_theInput->WasKeyJustPressed('9'))
	{
		m_constraintMode = ConstraintMode::NINETY;
	}
	if (g_theInput->WasKeyJustPressed('F'))
	{
		m_constraintMode = ConstraintMode::FREE;
	}
}

void Game2D::Render() const
{
	g_theRenderer->ClearScreen(Rgba8::BLACK);
	if (IsAttractMode() == false)
	{
		g_theRenderer->BeginCamera(g_theApp->m_screenCamera);
		RenderSkeleton();
		GameModeAndControlsText();
		g_theRenderer->EndCamera(g_theApp->m_screenCamera);
	}
}

void Game2D::Shutdown()
{
}

Skeleton Game2D::CreateTestSkeleton2D()
{
	Skeleton skeleton;
	skeleton.m_bones.clear();

	Bone root;
	root.m_boneName = "Root";
	root.IsRootBone();
	skeleton.m_bones.push_back(root);

	Bone childBone1;
	childBone1.m_boneName = "Upper";
	childBone1.m_parentBoneIndex = 0;
	childBone1.m_localPosition = Vec3(200.f, 100.f, 0.f);
	skeleton.m_bones.push_back(childBone1);

	Bone childBone2;
	childBone2.m_boneName = "Lower";
	childBone2.m_parentBoneIndex = 1;
	childBone2.m_localPosition = Vec3(400.f, 100.f, 0.f);
	skeleton.m_bones.push_back(childBone2);

	Bone childBone3;
	childBone3.m_boneName = "End";
	childBone3.m_parentBoneIndex = 2;
	childBone3.m_localPosition = Vec3(600.f, -50.f, 0.f);
	skeleton.m_bones.push_back(childBone3);

	skeleton.UpdateSkeletonPose();
	return skeleton;
}

void Game2D::AnimateSkeleton(float deltaSeconds)
{
	static float elapsedTime = 0.f;
	elapsedTime += deltaSeconds;

	Bone* lowerControl = m_skeleton.GetBoneByName("Upper");
	Bone* lowerArm = m_skeleton.GetBoneByName("Lower");

	float swingAngleDegreesUpper = SmoothStep3(0.5f) * elapsedTime;
	float swingAngleDegreesLower = SmoothStop5(0.5f) * elapsedTime;

	Quat upperArmRotation = Quat::MakeFromAxisAngle(Vec3::ZAXE, swingAngleDegreesUpper);
	Quat lowerArmRotation = Quat::MakeFromAxisAngle(Vec3::ZAXE, -swingAngleDegreesLower);

	lowerArmRotation = ApplyConstraints(lowerArm, lowerArmRotation);

	lowerControl->SetLocalBoneRotation(upperArmRotation);
	lowerArm->SetLocalBoneRotation(lowerArmRotation);

	m_skeleton.UpdateSkeletonPose();

	m_skeletonVerts.clear();
	m_skeleton.AddVertsForSkeleton2D(m_skeletonVerts, m_skeletonStyle);
}

Quat Game2D::ApplyConstraints(Bone* lowerArm, Quat lowerArmRotation)
{
	if (m_constraintMode == ConstraintMode::FORTY_FIVE)
	{
		// Lock Yaw and limit Pitch
		lowerArm->m_boneConstraint.m_rotationConstraints[1] = CONSTRAINT_TYPE::LIMITED;

		lowerArm->m_boneConstraint.m_minRotationDegrees.m_pitchDegrees = -45.f;
		lowerArm->m_boneConstraint.m_maxRotationDegrees.m_pitchDegrees = 45.f;

		lowerArmRotation = lowerArm->m_boneConstraint.ApplyRotationConstraint(lowerArmRotation);
	}
	else if (m_constraintMode == ConstraintMode::NINETY)
	{
		lowerArm->m_boneConstraint.m_rotationConstraints[1] = CONSTRAINT_TYPE::LIMITED;

		lowerArm->m_boneConstraint.m_minRotationDegrees.m_pitchDegrees = -90.f;
		lowerArm->m_boneConstraint.m_maxRotationDegrees.m_pitchDegrees = 90.f;

		lowerArmRotation = lowerArm->m_boneConstraint.ApplyRotationConstraint(lowerArmRotation);
	}
	else
	{
		lowerArm->m_boneConstraint.m_rotationConstraints[1] = CONSTRAINT_TYPE::FREE;
	}	return lowerArmRotation;
}

void Game2D::RenderSkeleton() const
{
	g_theRenderer->SetModelConstants();
	g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_NONE);
	g_theRenderer->SetDepthMode(DepthMode::DISABLED);
	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->DrawVertexArray(m_skeletonVerts);
}

void Game2D::GameModeAndControlsText() const
{
	std::vector<Vertex_PCU> textVerts;
	m_font->AddVertsForTextInBox2D(textVerts, "Mode (F6/F7 for Prev/Next): Bone chain (2D)", m_gameSceneBounds, 20.f, Rgba8::GOLD, 0.8f, Vec2(0.f, 0.965f));
	m_font->AddVertsForTextInBox2D(textVerts, "F: No constraints applied  4: 45 degree limitation constraint  9: 90 degree limitation constraint", m_gameSceneBounds, 15.f, Rgba8::GOLD, 0.8f, Vec2(0.0f, 0.925f));
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_NONE);
	g_theRenderer->SetDepthMode(DepthMode::DISABLED);
	g_theRenderer->BindTexture(&m_font->GetTexture());
	g_theRenderer->DrawVertexArray(textVerts);
}
