#include "Game/Octopus.hpp"
#include "Game/GameCommon.h"
#include "Game/Terrain.hpp"
#include "Game/AnimalMode.hpp"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Math/MathUtils.h"

Octopus::Octopus(AnimalMode* mode, Vec3 position)
	:Entity(mode, position)
{
	Initialize();
}

Octopus::~Octopus()
{
}

void Octopus::Initialize()
{
	m_octopus = CreateOctopusSkeleton();

	m_octopusInitialPositions.clear();
	for (int octopusBoneIndex = 0; octopusBoneIndex < static_cast<int>(m_octopus.m_bones.size()); ++octopusBoneIndex)
	{
		Bone& octopusBone = m_octopus.m_bones[octopusBoneIndex];
		m_octopusInitialPositions.push_back(octopusBone.m_localPosition);
	}

	m_speed = 1.f;
}

void Octopus::Update(float deltaSeconds)
{
	UpdateOctopusPose(deltaSeconds);
	UpdateOctopusVerts();
}

void Octopus::UpdateOctopusPose(float deltaSeconds)
{
	static float elapsedTime = 0.f;
	elapsedTime += deltaSeconds;
	m_directionChangeTimer += deltaSeconds;

	if (!m_isStationary)
	{
		// Direction change
		OctopusRoam(deltaSeconds);

		// Move octopus forward
		Vec3 octopusMove = m_moveDirection * m_speed * deltaSeconds;
		m_worldPosition -= octopusMove;

		// Project onto terrain
		float terrainZ = m_animalMode->m_terrain->GetHeightAtXY(m_worldPosition.x, m_worldPosition.y);
		m_worldPosition.z = terrainZ;

		// Check if octopus is out of bounds, if so reset
		if (!m_animalMode->m_terrain->IsInBounds(static_cast<int>(m_worldPosition.x), static_cast<int>(m_worldPosition.y)))
		{
			m_worldPosition = Vec3(85.f, 35.f, m_animalMode->m_terrain->GetHeightAtXY(85.f, 35.f));
			elapsedTime = 0.f;
		}
	}

	float bobbingHeight = sinf(elapsedTime * 2.f) * 0.5f;

	Bone& headBone = m_octopus.m_bones[0];
	Vec3  baseHeadPos = m_octopusInitialPositions[0];
	headBone.SetLocalBonePosition(baseHeadPos + Vec3(0.f, 0.f, bobbingHeight));

	float normalizedBobbing = bobbingHeight + 0.5f;

	// Arm curling
	for (int boneIndex = 1; boneIndex < static_cast<int>(m_octopus.m_bones.size()); ++boneIndex)
	{
		Bone& bone = m_octopus.m_bones[boneIndex];

		// Positional
		//Vec3 basePos = m_octopusInitialPositions[boneIndex];
		//float curlAmount = GetClamped(bobbingHeight * 0.75f, -1.f, 1.f);
		//Vec3  curledPosition = basePos;
		//curledPosition.z += curlAmount;
		//bone.SetLocalBonePosition(curledPosition);

		// Rotational
		float wave = sinf(elapsedTime * 3.f + boneIndex * 0.4f);
		float curlStrength = 0.4f;
		float curlAngle = wave * curlStrength * normalizedBobbing;
		Quat curlRotation = Quat::MakeFromAxisAngle(Vec3::ZAXE, curlAngle);
		bone.SetLocalBoneRotation(curlRotation);
	}


	float offset = 1.f;
	Terrain* terrain = m_animalMode->m_terrain;

	// Get current and neighbor positions
	float hCenter = terrain->GetHeightAtXY(m_worldPosition.x, m_worldPosition.y);
	float hForward = terrain->GetHeightAtXY(m_worldPosition.x + m_moveDirection.x * offset, m_worldPosition.y + m_moveDirection.y * offset);
	float hLeft = terrain->GetHeightAtXY(m_worldPosition.x + m_moveDirection.y * offset, m_worldPosition.y - m_moveDirection.x * offset);

	// Build local tangent vectors
	Vec3 forward = (Vec3(m_moveDirection.x, m_moveDirection.y, hForward - hCenter)).GetNormalized();
	Vec3 left = (Vec3(m_moveDirection.y, -m_moveDirection.x, hLeft - hCenter)).GetNormalized();

	// Calculating the terrain normal
	Vec3 up = CrossProduct3D(left, forward).GetNormalized();

	// Recalculating the orthogonal left vector
	left = CrossProduct3D(forward, up).GetNormalized();

	// Build orientation matrix
	Mat44 orientation;
	orientation.SetIJK3D(forward, left, up);
	orientation.SetTranslation3D(m_worldPosition);

	// Apply it to the skeleton
	m_octopus.m_skeletonModelTransform = orientation;
	m_octopus.UpdateSkeletonPose();
}

void Octopus::UpdateOctopusVerts()
{
	m_octoSkeletonVerts.clear();
	m_octopus.AddVertsForSkeleton3D(m_octoSkeletonVerts);
}

void Octopus::Render() const
{
	DrawOctopus();

	if (m_animalMode->m_isSkeletonBeingDrawn)
	{
		g_theRenderer->SetModelConstants();
		g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
		g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
		g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
		g_theRenderer->SetDepthMode(DepthMode::DISABLED);
		g_theRenderer->BindShader(nullptr);
		g_theRenderer->BindTexture(nullptr);
		g_theRenderer->DrawVertexArray(m_octoSkeletonVerts);
	}
}

Skeleton Octopus::CreateOctopusSkeleton()
{
	Skeleton octopusSkeleton;
	octopusSkeleton.m_bones.clear();

	// Central head
	Bone head;
	head.m_boneName = "Head";
	head.SetLocalBonePosition(Vec3::ZERO);
	head.SetLocalBoneRotation(Quat::MakeFromAxisAngle(Vec3::ZAXE, ConvertDegreesToRadians(90.f)));
	octopusSkeleton.m_bones.push_back(head);

	// Separating the octopus arms by 45 degrees
	float angleStep = 360.f / NUM_OCTOPUS_ARMS;
	float armLength = 1.f;

	for (int armIndex = 0; armIndex < NUM_OCTOPUS_ARMS; ++armIndex)
	{
		float armAngleDegrees = armIndex * angleStep;
		float armAngleRadians = ConvertDegreesToRadians(armAngleDegrees);

		// Base direction
		float x = cosf(armAngleRadians);
		float y = sinf(armAngleRadians);

		// Offset adjustment
		Vec3 baseOffset = Vec3(x, y, -0.2f);
		Vec3 middleOffset = baseOffset * 0.7f;
		Vec3 tipOffset = baseOffset * 0.7f;

		// Arm Base
		Bone armBase;
		armBase.m_boneName = "Arm" + std::to_string(armIndex + 1) + "_Base";
		armBase.m_parentBoneIndex = 0;
		armBase.SetLocalBonePosition(baseOffset * armLength);
		octopusSkeleton.m_bones.push_back(armBase);
		int baseIndex = static_cast<int>(octopusSkeleton.m_bones.size() - 1);

		// Arm Middle
		Bone armMiddle;
		armMiddle.m_boneName = "Arm" + std::to_string(armIndex + 1) + "_Mid";
		armMiddle.m_parentBoneIndex = baseIndex;
		armMiddle.SetLocalBonePosition(middleOffset * armLength);
		octopusSkeleton.m_bones.push_back(armMiddle);
		int middleIndex = static_cast<int>(octopusSkeleton.m_bones.size() - 1);

		// Arm Tip
		Bone armTip;
		armTip.m_boneName = "Arm" + std::to_string(armIndex + 1) + "_Tip";
		armTip.m_parentBoneIndex = middleIndex;
		armTip.SetLocalBonePosition(tipOffset * armLength);
		octopusSkeleton.m_bones.push_back(armTip);
	}

	octopusSkeleton.UpdateSkeletonPose();
	return octopusSkeleton;
}

void Octopus::DrawOctopus() const
{
	std::vector<Vertex_PCU> octoVerts;

	Bone const& head = m_octopus.m_bones[0];
	Vec3 headPos = head.GetWorldBonePosition3D();

	Vec3 iBasis = head.m_worldBoneTransform.GetIBasis3D(); 
	Vec3 jBasis = head.m_worldBoneTransform.GetJBasis3D();  
	Vec3 kBasis = head.m_worldBoneTransform.GetKBasis3D(); 

	AddVertsForSphere3D(octoVerts, headPos + kBasis * -0.4f, 0.8f, Rgba8(180, 40, 180));
	AddVertsForSphere3D(octoVerts, headPos, 0.25f, Rgba8(200, 60, 200));

	Vec3 eyeOffset = (iBasis * 0.60f + kBasis * 0.025f);
	AddVertsForSphere3D(octoVerts, headPos + eyeOffset, 0.12f, Rgba8::BLACK);
	AddVertsForSphere3D(octoVerts, headPos - eyeOffset, 0.12f, Rgba8::BLACK);

	for (int boneIndex = 1; boneIndex < static_cast<int>(m_octopus.m_bones.size()); ++boneIndex)
	{
		Bone const& bone = m_octopus.m_bones[boneIndex];
		Vec3 start = bone.GetWorldBonePosition3D();

		Vec3 end = start;
		if (bone.m_parentBoneIndex >= 0)
		{
			Bone const& parent = m_octopus.m_bones[bone.m_parentBoneIndex];
			end = parent.GetWorldBonePosition3D();
		}

		bool isTip = (bone.m_boneName.find("_Tip") != std::string::npos);

		if (isTip)
		{
			AddVertsForCone3D(octoVerts, end, start, 0.08f, Rgba8(160, 30, 160));
		}
		else
		{
			AddVertsForCylinder3D(octoVerts, end, start, 0.08f, Rgba8(150, 35, 150));
		}
	}

	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
	g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
	g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
	if (m_animalMode->m_isAnimalVertsBeingDrawn)
	{
		g_theRenderer->DrawVertexArray(octoVerts);
	}
}

void Octopus::OctopusRoam(float deltaSeconds)
{
	if (m_isRoaming)
	{
		if (m_directionChangeTimer >= m_timeToChangeDir)
		{
			m_directionChangeTimer = 0.f;

			// Picking a new random direction for the octopus to go
			float angle = g_rng->RollRandomFloatInRange(0.f, 360.f);
			Vec2 directionXY = Vec2(CosDegrees(angle), SinDegrees(angle));
			Vec3 newDirection = Vec3(directionXY.x, directionXY.y, 0.f).GetNormalized();
			m_targetMoveDirection = newDirection;
			m_directionInterpTime = 0.f;
		}

		// Smoothly interpolate move direction
		if (m_moveDirection != m_targetMoveDirection)
		{
			m_directionInterpTime += deltaSeconds;
			float fractionTowardEnd = GetClamped(m_directionInterpTime / m_directionInterpDuration, 0.f, 1.f);

			float easedFraction = SmoothStep3(fractionTowardEnd);
			m_moveDirection = SLerp(m_moveDirection, m_targetMoveDirection, easedFraction).GetNormalized();
		}
	}
}