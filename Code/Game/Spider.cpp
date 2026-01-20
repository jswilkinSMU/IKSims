#include "Game/Spider.hpp"
#include "Game/AnimalMode.hpp"
#include "Game/Terrain.hpp"
#include "Game/GameCommon.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Core/Time.hpp"

Spider::Spider(AnimalMode* mode, Vec3 position)
	:Entity(mode, position)
{
	Initialize();
}

Spider::~Spider()
{
}

void Spider::Initialize()
{
	m_speed = 2.5f;
	m_spider = CreateSkeleton();
	PopulateSpiderLegs();
	ComputeLegBoneLengths();
	GenerateHair();
}

void Spider::Update(float deltaSeconds)
{
	UpdateSpiderPose(deltaSeconds);
	SimulateHair(deltaSeconds);
	UpdateSpiderVerts();
	if (m_animalMode->m_isSkeletonBeingDrawn)
	{
		m_spider.AddVertsForSkeleton3D(m_spiderSkeletonVerts);
	}
}

void Spider::UpdateSpiderPose(float deltaSeconds)
{
	static float elapsedTime = 0.f;
	elapsedTime += deltaSeconds;
	m_directionChangeTimer += deltaSeconds;

	// Direction change
	SpiderRoam(deltaSeconds);

	if (!m_isStationary)
	{
		// Move spider forward
		Vec3 spiderMove = m_moveDirection * m_speed * deltaSeconds;
		m_worldPosition -= spiderMove;

		// Project onto terrain
		float terrainZ = m_animalMode->m_terrain->GetHeightAtXY(m_worldPosition.x, m_worldPosition.y);
		m_worldPosition.z = terrainZ;

		// Check if spider is out of bounds, if so reset
		if (!m_animalMode->m_terrain->IsInBounds(static_cast<int>(m_worldPosition.x), static_cast<int>(m_worldPosition.y)))
		{
			m_worldPosition = Vec3(85.f, 35.f, m_animalMode->m_terrain->GetHeightAtXY(85.f, 35.f));
			elapsedTime = 0.f;
		}
	}

	// Animate spider legs
	for (int spiderBoneIndex = 0; spiderBoneIndex < static_cast<int>(m_spider.m_bones.size()); ++spiderBoneIndex)
	{
		Bone& spiderBone = m_spider.m_bones[spiderBoneIndex];
		if (spiderBone.m_boneName.find("Femur") != std::string::npos)
		{
			float legMovement = sinf(elapsedTime * spiderBoneIndex * 0.4f) * 0.5f;
			spiderBone.SetLocalBoneRotation(Quat::MakeFromAxisAngle(Vec3::XAXE, legMovement));
		}
	}

	// Computing each leg's target foot position based on the terrain
	for (int spiderLegIndex = 0; spiderLegIndex < static_cast<int>(m_legs.size()); ++spiderLegIndex)
	{
		SpiderLeg& spiderLeg = m_legs[spiderLegIndex];
		Vec3 baseLegPosition = m_spider.m_bones[1].GetWorldBonePosition3D();

		// Transform local offset into world space
		Vec3 defaultWorldPos = m_spider.m_skeletonModelTransform.TransformPosition3D(spiderLeg.m_defaultFootOffset);

		// Sampling terrain directly (may change to raycast)
		float footHeight = m_animalMode->m_terrain->GetHeightAtXY(defaultWorldPos.x, defaultWorldPos.y);
		Vec3 targetPosition = defaultWorldPos;
		targetPosition.z = footHeight + 0.05f;

		spiderLeg.m_footTargetWorldPos = targetPosition;
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
	m_spider.m_skeletonModelTransform = orientation;
	m_spider.UpdateSkeletonPose();

	//if (m_isLegCurling)
	//{
	//	for (SpiderLeg& leg : m_legs)
	//	{
	//		RunFABRIK(leg);
	//	}
	//	//m_spider.UpdateSkeletonPose();
	//}
}

void Spider::SpiderRoam(float deltaSeconds)
{
	if (m_isRoaming)
	{
		if (m_directionChangeTimer >= m_timeToChangeDir)
		{
			m_directionChangeTimer = 0.f;

			// Picking a new random direction for the spider to go
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

void Spider::SimulateHair(float deltaSeconds)
{
	Vec3 gravity = Vec3(0.f, 0.f, -9.8f);
	float damping = 0.95f;
	int numConstraintIterations = 2;

	for (int boneIndex = 0; boneIndex < static_cast<int>(m_spider.m_bones.size()); ++boneIndex)
	{
		Bone const& bone = m_spider.m_bones[boneIndex];
		Mat44 const& boneTransform = bone.m_worldBoneTransform;

		std::vector<SpiderHair>& hairs = m_hairsPerBone[boneIndex];
		for (int hairIndex = 0; hairIndex < static_cast<int>(hairs.size()); ++hairIndex)
		{
			SpiderHair& hair = hairs[hairIndex];
			Vec3 rootWorld = boneTransform.TransformPosition3D(hair.m_localOffset);
			Vec3 dirWorld = boneTransform.TransformVectorQuantity3D(hair.m_localDirection).GetNormalized();

			// Verlet integration for the tip
			Vec3 temp = hair.m_tipPos;
			Vec3 velocity = (hair.m_tipPos - hair.m_prevTipPos) * damping;
			Vec3 newPos = hair.m_tipPos + velocity + gravity * (deltaSeconds * deltaSeconds);
			hair.m_prevTipPos = temp;
			hair.m_tipPos = newPos;

			// Apply spring constraint to maintain length
			for (int constraintIndex = 0; constraintIndex < numConstraintIterations; ++constraintIndex)
			{
				Vec3 toTip = hair.m_tipPos - rootWorld;
				float currentLength = toTip.GetLength();
				if (currentLength > 0.f)
				{
					Vec3 direction = toTip / currentLength;
					float error = currentLength - hair.m_hairLength;
					hair.m_tipPos -= direction * error * 0.5f;
				}

				Vec3 wind = Vec3(sinf(static_cast<float>(GetCurrentTimeSeconds()) * 2.f + boneIndex) * 0.2f, cosf(static_cast<float>(GetCurrentTimeSeconds()) * 3.f + boneIndex) * 0.2f, 0.f);
				hair.m_tipPos += wind * deltaSeconds;
			}

			Vec3 restTip = rootWorld + dirWorld * hair.m_hairLength;
			Vec3 toRest = restTip - hair.m_tipPos;
			hair.m_tipPos += toRest * 0.004f;
		}
	}
}

void Spider::UpdateSpiderVerts()
{
	m_spiderVerts.clear();
	if (m_animalMode->m_isSkeletonBeingDrawn)
	{
		m_spiderSkeletonVerts.clear();
	}

	for (int boneIndex = 0; boneIndex < static_cast<int>(m_spider.m_bones.size()); ++boneIndex)
	{
		Bone const& bone = m_spider.m_bones[boneIndex];
		Vec3 boneWorldPos = bone.GetWorldBonePosition3D();
		float radius = 0.25f;

		AddVertsForSphere3D(m_spiderVerts, boneWorldPos, 0.25f, Rgba8::BLACK);

		if (bone.m_parentBoneIndex == -1)
		{
			continue;
		}

		Vec3 start = m_spider.m_bones[bone.m_parentBoneIndex].m_worldBoneTransform.GetTranslation3D();
		Vec3 end = bone.m_worldBoneTransform.GetTranslation3D();

		AddVertsForCylinder3D(m_spiderVerts, start, end, radius, Rgba8::BLACK);
	}

	AddHairGeometry();
}

void Spider::AddHairGeometry()
{
	float baseHairRadius = 0.002f;

	if (m_hairsPerBone.empty())
	{
		GenerateHair();
	}

	for (int boneIndex = 0; boneIndex < static_cast<int>(m_spider.m_bones.size()); ++boneIndex)
	{
		Bone const& bone = m_spider.m_bones[boneIndex];
		Mat44 const& boneTransform = bone.m_worldBoneTransform;

		std::vector<SpiderHair> const& hairs = m_hairsPerBone[boneIndex];
		for (int hairIndex = 0; hairIndex < static_cast<int>(hairs.size()); ++hairIndex)
		{
			SpiderHair const& hair = hairs[hairIndex];
			Vec3 worldRoot = boneTransform.TransformPosition3D(hair.m_localOffset);
			Vec3 worldDir = boneTransform.TransformVectorQuantity3D(hair.m_localDirection).GetNormalized();

			Vec3 worldTip = hair.m_tipPos;
			AddVertsForCylinder3D(m_spiderVerts, worldRoot, worldTip, baseHairRadius, hair.m_hairColor);
		}
	}
}

void Spider::GenerateHair()
{
	m_hairsPerBone.clear();
	m_hairsPerBone.resize(m_spider.m_bones.size());

	for (int boneIndex = 0; boneIndex < static_cast<int>(m_spider.m_bones.size()); ++boneIndex)
	{
		Bone const& bone = m_spider.m_bones[boneIndex];
		std::vector<SpiderHair>& hairs = m_hairsPerBone[boneIndex];
		int numHairs = 0;

		// Here I do a check so that there are more hairs on the body and less hairs on the legs
		if (bone.m_boneName.find("Abdomen") != std::string::npos)
		{
			numHairs = 100;
		}
		else if (bone.m_boneName.find("Head") != std::string::npos)
		{
			numHairs = 50;
		}
		else
		{
			numHairs = 20;
		}

		for (int hairIndex = 0; hairIndex < numHairs; ++hairIndex)
		{
			SpiderHair hair;

			// Random local offset
			hair.m_localOffset = Vec3(g_rng->RollRandomFloatInRange(-1.f, 1.f) * 0.05f, g_rng->RollRandomFloatInRange(-1.f, 1.f) * 0.05f, g_rng->RollRandomFloatInRange(0.f, 1.f) * 0.05f);

			// Local direction around z
			Vec3 randomDir(g_rng->RollRandomFloatInRange(-1.f, 1.f), g_rng->RollRandomFloatInRange(-1.f, 1.f), g_rng->RollRandomFloatInRange(0.f, 1.f));
			randomDir.Normalize();
			float variation = 0.75f;
			Vec3 baseDir = Vec3::ZAXE;
			hair.m_localDirection = (baseDir * (1.f - variation) + randomDir * variation).GetNormalized();

			// Random length
			hair.m_hairLength = 0.25f * g_rng->RollRandomFloatInRange(0.8f, 1.5f);

			// Slight hair color variation
			hair.m_hairColor = Rgba8(static_cast<unsigned char>(g_rng->RollRandomIntInRange(80, 150)), 
				                     static_cast<unsigned char>(g_rng->RollRandomIntInRange(80, 150)), 
				                     static_cast<unsigned char>(g_rng->RollRandomIntInRange(80, 150)));

			hairs.push_back(hair);

			// Update verlet
			Vec3 rootWorld = m_spider.m_bones[boneIndex].m_worldBoneTransform.TransformPosition3D(hair.m_localOffset);
			Vec3 dirWorld = m_spider.m_bones[boneIndex].m_worldBoneTransform.TransformVectorQuantity3D(hair.m_localDirection).GetNormalized();
			hair.m_tipPos = rootWorld + dirWorld * hair.m_hairLength;
			hair.m_prevTipPos = hair.m_tipPos;
		}
	}
}

void Spider::Render() const
{
	g_theRenderer->SetModelConstants();
	g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
	g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
	g_theRenderer->BindShader(nullptr);
	g_theRenderer->BindTexture(nullptr);
	if (m_animalMode->m_isAnimalVertsBeingDrawn)
	{
		g_theRenderer->DrawVertexArray(m_spiderVerts);
	}

	if (m_animalMode->m_isSkeletonBeingDrawn)
	{
		g_theRenderer->SetModelConstants();
		g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
		g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
		g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
		g_theRenderer->SetDepthMode(DepthMode::DISABLED);
		g_theRenderer->BindShader(nullptr);
		g_theRenderer->BindTexture(nullptr);
		g_theRenderer->DrawVertexArray(m_spiderSkeletonVerts);
	}
}

void Spider::SetIsRoaming(bool isRoaming)
{
	m_isRoaming = isRoaming;
}

void Spider::SetIsCurlingLegs(bool isLegCurling)
{
	m_isLegCurling = isLegCurling;
}

Skeleton Spider::CreateSkeleton()
{
	Skeleton spiderSkeleton;
	spiderSkeleton.m_bones.clear();

	Bone abdomen;
	abdomen.m_boneName = "Abdomen";
	abdomen.SetLocalBonePosition(Vec3(0.f, 0.f, 0.75f));
	spiderSkeleton.m_bones.push_back(abdomen);

	/* ----------------Spider Head----------------- */
	Bone head;
	head.m_boneName = "Head";
	head.m_parentBoneIndex = 0;
	head.SetLocalBonePosition(Vec3(-0.4f, 0.f, 0.f));
	spiderSkeleton.m_bones.push_back(head);
	/* -------------------------------------------- */

	/* ------------Spider Left front leg-----------*/
	Bone leftFrontFemur;
	leftFrontFemur.m_boneName = "LeftFrontFemur";
	leftFrontFemur.m_parentBoneIndex = 1;
	leftFrontFemur.SetLocalBonePosition(Vec3(-0.4f, 0.25f, 0.5f));
	spiderSkeleton.m_bones.push_back(leftFrontFemur);

	Bone leftFrontTibia;
	leftFrontTibia.m_boneName = "LeftFrontTibia";
	leftFrontTibia.m_parentBoneIndex = 2;
	leftFrontTibia.SetLocalBonePosition(Vec3(-0.4f, 0.25f, 0.0f));
	spiderSkeleton.m_bones.push_back(leftFrontTibia);

	Bone leftFrontMetaTarsus;
	leftFrontMetaTarsus.m_boneName = "LeftFrontMetaTarsus";
	leftFrontMetaTarsus.m_parentBoneIndex = 3;
	leftFrontMetaTarsus.SetLocalBonePosition(Vec3(-0.2f, 0.25f, -0.5f));
	spiderSkeleton.m_bones.push_back(leftFrontMetaTarsus);

	Bone leftFrontTarsus;
	leftFrontTarsus.m_boneName = "LeftFrontTarsus";
	leftFrontTarsus.m_parentBoneIndex = 4;
	leftFrontTarsus.SetLocalBonePosition(Vec3(-0.2f, 0.25f, -0.5f));
	spiderSkeleton.m_bones.push_back(leftFrontTarsus);
	/* -------------------------------------------- */

	/* ------------Spider Right front leg-----------*/
	Bone rightFrontFemur;
	rightFrontFemur.m_boneName = "RightFrontFemur";
	rightFrontFemur.m_parentBoneIndex = 1;
	rightFrontFemur.SetLocalBonePosition(Vec3(-0.4f, -0.25f, 0.5f));
	spiderSkeleton.m_bones.push_back(rightFrontFemur);

	Bone rightFrontTibia;
	rightFrontTibia.m_boneName = "RightFrontTibia";
	rightFrontTibia.m_parentBoneIndex = 6;
	rightFrontTibia.SetLocalBonePosition(Vec3(-0.4f, -0.25f, 0.0f));
	spiderSkeleton.m_bones.push_back(rightFrontTibia);

	Bone rightFrontMetaTarsus;
	rightFrontMetaTarsus.m_boneName = "RightFrontMetaTarsus";
	rightFrontMetaTarsus.m_parentBoneIndex = 7;
	rightFrontMetaTarsus.SetLocalBonePosition(Vec3(-0.2f, -0.25f, -0.5f));
	spiderSkeleton.m_bones.push_back(rightFrontMetaTarsus);

	Bone rightFrontTarsus;
	rightFrontTarsus.m_boneName = "RightFrontTarsus";
	rightFrontTarsus.m_parentBoneIndex = 8;
	rightFrontTarsus.SetLocalBonePosition(Vec3(-0.2f, -0.25f, -0.5f));
	spiderSkeleton.m_bones.push_back(rightFrontTarsus);
	/* -------------------------------------------- */

	/* ----------Spider Left Front Middle Leg------ */
	Bone leftFrontMiddleFemur;
	leftFrontMiddleFemur.m_boneName = "LeftFrontMidFemur";
	leftFrontMiddleFemur.m_parentBoneIndex = 1;
	leftFrontMiddleFemur.SetLocalBonePosition(Vec3(-0.15f, 0.5f, 0.f));
	spiderSkeleton.m_bones.push_back(leftFrontMiddleFemur);

	Bone leftFrontMiddleTibia;
	leftFrontMiddleTibia.m_boneName = "LeftFrontMiddleTibia";
	leftFrontMiddleTibia.m_parentBoneIndex = 10;
	leftFrontMiddleTibia.SetLocalBonePosition(Vec3(-0.15f, 0.25f, 0.f));
	spiderSkeleton.m_bones.push_back(leftFrontMiddleTibia);

	Bone leftFrontMiddleMetaTarsus;
	leftFrontMiddleMetaTarsus.m_boneName = "LeftFrontMiddleMetaTarsus";
	leftFrontMiddleMetaTarsus.m_parentBoneIndex = 11;
	leftFrontMiddleMetaTarsus.SetLocalBonePosition(Vec3(-0.15f, 0.15f, -0.3f));
	spiderSkeleton.m_bones.push_back(leftFrontMiddleMetaTarsus);

	Bone leftFrontMiddleTarsus;
	leftFrontMiddleTarsus.m_boneName = "LeftFrontMiddleTarsus";
	leftFrontMiddleTarsus.m_parentBoneIndex = 12;
	leftFrontMiddleTarsus.SetLocalBonePosition(Vec3(-0.15f, 0.1f, -0.2f));
	spiderSkeleton.m_bones.push_back(leftFrontMiddleTarsus);
	/* -------------------------------------------- */

	/* ----------Spider Right Front Middle Leg------ */
	Bone rightFrontMiddleFemur;
	rightFrontMiddleFemur.m_boneName = "RightFrontMidFemur";
	rightFrontMiddleFemur.m_parentBoneIndex = 1;
	rightFrontMiddleFemur.SetLocalBonePosition(Vec3(-0.15f, -0.5f, 0.f));
	spiderSkeleton.m_bones.push_back(rightFrontMiddleFemur);

	Bone rightFrontMiddleTibia;
	rightFrontMiddleTibia.m_boneName = "RightFrontMiddleTibia";
	rightFrontMiddleTibia.m_parentBoneIndex = 14;
	rightFrontMiddleTibia.SetLocalBonePosition(Vec3(-0.15f, -0.25f, 0.f));
	spiderSkeleton.m_bones.push_back(rightFrontMiddleTibia);

	Bone rightFrontMiddleMetaTarsus;
	rightFrontMiddleMetaTarsus.m_boneName = "RightFrontMiddleMetaTarsus";
	rightFrontMiddleMetaTarsus.m_parentBoneIndex = 15;
	rightFrontMiddleMetaTarsus.SetLocalBonePosition(Vec3(-0.15f, -0.15f, -0.3f));
	spiderSkeleton.m_bones.push_back(rightFrontMiddleMetaTarsus);

	Bone rightFrontMiddleTarsus;
	rightFrontMiddleTarsus.m_boneName = "RightFrontMiddleTarsus";
	rightFrontMiddleTarsus.m_parentBoneIndex = 16;
	rightFrontMiddleTarsus.SetLocalBonePosition(Vec3(-0.15f, -0.1f, -0.2f));
	spiderSkeleton.m_bones.push_back(rightFrontMiddleTarsus);
	/* -------------------------------------------- */

	/* ----------Spider Left Back Middle Leg------ */
	Bone leftBackMiddleFemur;
	leftBackMiddleFemur.m_boneName = "LeftBackMidFemur";
	leftBackMiddleFemur.m_parentBoneIndex = 1;
	leftBackMiddleFemur.SetLocalBonePosition(Vec3(0.15f, 0.5f, 0.f));
	spiderSkeleton.m_bones.push_back(leftBackMiddleFemur);

	Bone leftBackMiddleTibia;
	leftBackMiddleTibia.m_boneName = "LeftBackMiddleTibia";
	leftBackMiddleTibia.m_parentBoneIndex = 18;
	leftBackMiddleTibia.SetLocalBonePosition(Vec3(0.15f, 0.25f, 0.f));
	spiderSkeleton.m_bones.push_back(leftBackMiddleTibia);

	Bone leftBackMiddleMetaTarsus;
	leftBackMiddleMetaTarsus.m_boneName = "LeftBackMiddleMetaTarsus";
	leftBackMiddleMetaTarsus.m_parentBoneIndex = 19;
	leftBackMiddleMetaTarsus.SetLocalBonePosition(Vec3(0.15f, 0.15f, -0.3f));
	spiderSkeleton.m_bones.push_back(leftBackMiddleMetaTarsus);

	Bone leftBackMiddleTarsus;
	leftBackMiddleTarsus.m_boneName = "LeftBackMiddleTarsus";
	leftBackMiddleTarsus.m_parentBoneIndex = 20;
	leftBackMiddleTarsus.SetLocalBonePosition(Vec3(0.15f, 0.1f, -0.2f));
	spiderSkeleton.m_bones.push_back(leftBackMiddleTarsus);
	/* -------------------------------------------- */

	/* ----------Spider Right Back Middle Leg------ */
	Bone rightBackMiddleFemur;
	rightBackMiddleFemur.m_boneName = "RightBackMidFemur";
	rightBackMiddleFemur.m_parentBoneIndex = 1;
	rightBackMiddleFemur.SetLocalBonePosition(Vec3(0.15f, -0.5f, 0.f));
	spiderSkeleton.m_bones.push_back(rightBackMiddleFemur);

	Bone rightBackMiddleTibia;
	rightBackMiddleTibia.m_boneName = "RightBackMiddleTibia";
	rightBackMiddleTibia.m_parentBoneIndex = 22;
	rightBackMiddleTibia.SetLocalBonePosition(Vec3(0.15f, -0.25f, 0.f));
	spiderSkeleton.m_bones.push_back(rightBackMiddleTibia);

	Bone rightBackMiddleMetaTarsus;
	rightBackMiddleMetaTarsus.m_boneName = "RightBackMiddleMetaTarsus";
	rightBackMiddleMetaTarsus.m_parentBoneIndex = 23;
	rightBackMiddleMetaTarsus.SetLocalBonePosition(Vec3(0.15f, -0.15f, -0.3f));
	spiderSkeleton.m_bones.push_back(rightBackMiddleMetaTarsus);

	Bone rightBackMiddleTarsus;
	rightBackMiddleTarsus.m_boneName = "RightBackMiddleTarsus";
	rightBackMiddleTarsus.m_parentBoneIndex = 24;
	rightBackMiddleTarsus.SetLocalBonePosition(Vec3(0.15f, -0.1f, -0.2f));
	spiderSkeleton.m_bones.push_back(rightBackMiddleTarsus);
	/* -------------------------------------------- */

	/* ------------Spider Left back leg-----------*/
	Bone leftBackFemur;
	leftBackFemur.m_boneName = "LeftBackFemur";
	leftBackFemur.m_parentBoneIndex = 1;
	leftBackFemur.SetLocalBonePosition(Vec3(0.4f, 0.25f, 0.5f));
	spiderSkeleton.m_bones.push_back(leftBackFemur);

	Bone leftBackTibia;
	leftBackTibia.m_boneName = "LeftBackTibia";
	leftBackTibia.m_parentBoneIndex = 26;
	leftBackTibia.SetLocalBonePosition(Vec3(0.4f, 0.25f, 0.0f));
	spiderSkeleton.m_bones.push_back(leftBackTibia);

	Bone leftBackMetaTarsus;
	leftBackMetaTarsus.m_boneName = "LeftBackMetaTarsus";
	leftBackMetaTarsus.m_parentBoneIndex = 27;
	leftBackMetaTarsus.SetLocalBonePosition(Vec3(0.2f, 0.25f, -0.5f));
	spiderSkeleton.m_bones.push_back(leftBackMetaTarsus);

	Bone leftBackTarsus;
	leftBackTarsus.m_boneName = "LeftBackTarsus";
	leftBackTarsus.m_parentBoneIndex = 28;
	leftBackTarsus.SetLocalBonePosition(Vec3(0.2f, 0.25f, -0.5f));
	spiderSkeleton.m_bones.push_back(leftBackTarsus);
	/* -------------------------------------------- */

	/* ------------Spider Right back leg-----------*/
	Bone rightBackFemur;
	rightBackFemur.m_boneName = "RightBackFemur";
	rightBackFemur.m_parentBoneIndex = 1;
	rightBackFemur.SetLocalBonePosition(Vec3(0.4f, -0.25f, 0.5f));
	spiderSkeleton.m_bones.push_back(rightBackFemur);

	Bone rightBackTibia;
	rightBackTibia.m_boneName = "RightBackTibia";
	rightBackTibia.m_parentBoneIndex = 30;
	rightBackTibia.SetLocalBonePosition(Vec3(0.4f, -0.25f, 0.0f));
	spiderSkeleton.m_bones.push_back(rightBackTibia);

	Bone rightBackMetaTarsus;
	rightBackMetaTarsus.m_boneName = "RightBackMetaTarsus";
	rightBackMetaTarsus.m_parentBoneIndex = 31;
	rightBackMetaTarsus.SetLocalBonePosition(Vec3(0.2f, -0.25f, -0.5f));
	spiderSkeleton.m_bones.push_back(rightBackMetaTarsus);

	Bone rightBackTarsus;
	rightBackTarsus.m_boneName = "RightBackTarsus";
	rightBackTarsus.m_parentBoneIndex = 32;
	rightBackTarsus.SetLocalBonePosition(Vec3(0.2f, -0.25f, -0.5f));
	spiderSkeleton.m_bones.push_back(rightBackTarsus);
	/* -------------------------------------------- */

	spiderSkeleton.UpdateSkeletonPose();
	return spiderSkeleton;
}

void Spider::PopulateSpiderLegs()
{
	m_legs =
	{
		{m_spider.FindBoneIndexByName("LeftFrontFemur"), m_spider.FindBoneIndexByName("LeftFrontTibia"),
		 m_spider.FindBoneIndexByName("LeftFrontMetaTarsus"), m_spider.FindBoneIndexByName("LeftFrontTarsus"), Vec3(-0.4f, 0.25f, -0.5f)},
		{m_spider.FindBoneIndexByName("RightFrontFemur"), m_spider.FindBoneIndexByName("RightFrontTibia"), 
		 m_spider.FindBoneIndexByName("RightFrontMetaTarsus"), m_spider.FindBoneIndexByName("RightFrontTarsus"), Vec3(-0.4f, -0.25f, -0.5f)},
		{m_spider.FindBoneIndexByName("LeftFrontMidFemur"), m_spider.FindBoneIndexByName("LeftFrontMiddleTibia"),
		 m_spider.FindBoneIndexByName("LeftFrontMiddleMetaTarsus"), m_spider.FindBoneIndexByName("LeftFrontMiddleTarsus"), Vec3(-0.15f, 0.5f, 0.f)},
		{m_spider.FindBoneIndexByName("RightFrontMidFemur"), m_spider.FindBoneIndexByName("RightFrontMiddleTibia"),
		 m_spider.FindBoneIndexByName("RightFrontMiddleMetaTarsus"), m_spider.FindBoneIndexByName("RightFrontMiddleTarsus"), Vec3(-0.15f, -0.5f, 0.f)},
		{m_spider.FindBoneIndexByName("LeftBackMidFemur"), m_spider.FindBoneIndexByName("LeftBackMiddleTibia"),
		 m_spider.FindBoneIndexByName("LeftBackMiddleMetaTarsus"), m_spider.FindBoneIndexByName("LeftBackMiddleTarsus"), Vec3(0.15f, 0.5f, 0.f)},
		{m_spider.FindBoneIndexByName("RightBackMidFemur"), m_spider.FindBoneIndexByName("RightBackMiddleTibia"),
		 m_spider.FindBoneIndexByName("RightBackMiddleMetaTarsus"), m_spider.FindBoneIndexByName("RightBackMiddleTarsus"), Vec3(0.15f, -0.5f, 0.f)},
		{m_spider.FindBoneIndexByName("LeftBackFemur"), m_spider.FindBoneIndexByName("LeftBackTibia"),
		 m_spider.FindBoneIndexByName("LeftBackMetaTarsus"), m_spider.FindBoneIndexByName("LeftBackTarsus"), Vec3(0.4f, 0.25f, 0.5f)},
		{m_spider.FindBoneIndexByName("RightBackFemur"), m_spider.FindBoneIndexByName("RightBackTibia"),
		 m_spider.FindBoneIndexByName("RightBackMetaTarsus"), m_spider.FindBoneIndexByName("RightBackTarsus"), Vec3(0.4f, -0.25f, 0.5f)}
	};

	for (SpiderLeg& leg : m_legs)
	{
		leg.m_boneIndices = 
		{
			leg.m_femurIndex,
			leg.m_tibiaIndex,
			leg.m_metaTarsusIndex,
			leg.m_tarsusIndex
		};
	}
}

void Spider::ComputeLegBoneLengths()
{
	for (SpiderLeg& leg : m_legs)
	{
		leg.m_boneLengths.clear();
		for (int legBoneIndex = 0; legBoneIndex < 3; ++legBoneIndex)
		{
			Vec3 boneA = m_spider.m_bones[leg.m_boneIndices[legBoneIndex]].GetWorldBonePosition3D();
			Vec3 boneB = m_spider.m_bones[leg.m_boneIndices[legBoneIndex + 1]].GetWorldBonePosition3D();
			leg.m_boneLengths.push_back((boneB - boneA).GetLength());
		}
	}
}

void Spider::RunFABRIK(SpiderLeg& leg)
{
	int legBoneSize = static_cast<int>(leg.m_boneIndices.size());

	// Get current joint world positions
	std::vector<Vec3> joints(legBoneSize);
	for (int legBoneIndex = 0; legBoneIndex < legBoneSize; ++legBoneIndex)
	{
		joints[legBoneIndex] = m_spider.m_bones[leg.m_boneIndices[legBoneIndex]].GetWorldBonePosition3D();
	}

	Vec3 target = leg.m_footTargetWorldPos;
	Vec3 root = joints[0];

	float chainLength = 0.f;
	for (float legLength : leg.m_boneLengths)
	{
		chainLength += legLength;
	}

	// Extend toward if target is too far
	if ((target - root).GetLength() > chainLength)
	{
		for (int legBoneIndex = 1; legBoneIndex < legBoneSize; ++legBoneIndex)
		{
			Vec3 dir = (target - joints[legBoneIndex - 1]).GetNormalized();
			joints[legBoneIndex] = joints[legBoneIndex - 1] + dir * leg.m_boneLengths[legBoneIndex - 1];
		}
	}
	else
	{
		// Backward reaching
		joints[legBoneSize - 1] = target;
		for (int legBoneIndex = legBoneSize - 2; legBoneIndex >= 0; --legBoneIndex)
		{
			Vec3 dir = (joints[legBoneIndex] - joints[legBoneIndex + 1]).GetNormalized();
			joints[legBoneIndex] = joints[legBoneIndex + 1] + dir * leg.m_boneLengths[legBoneIndex];
		}

		// Forward reaching
		joints[0] = root;
		for (int legBoneIndex = 1; legBoneIndex < legBoneSize; ++legBoneIndex)
		{
			Vec3 dir = (joints[legBoneIndex] - joints[legBoneIndex - 1]).GetNormalized();
			joints[legBoneIndex] = joints[legBoneIndex - 1] + dir * leg.m_boneLengths[legBoneIndex - 1];
		}
	}

	// Convert new joint positions into bone rotations
	ApplyFABRIKToSkeletonBones(leg, joints);
}

void Spider::ApplyFABRIKToSkeletonBones(SpiderLeg const& leg, std::vector<Vec3> const& joints)
{
	for (int jointIndex = 0; jointIndex < static_cast<int>(joints.size() - 1); ++jointIndex)
	{
		int boneIndex = leg.m_boneIndices[jointIndex];
		Bone& bone = m_spider.m_bones[boneIndex];

		Vec3 worldA = joints[jointIndex];
		Vec3 worldB = joints[jointIndex + 1];

		Vec3 desiredDir = (worldB - worldA).GetNormalized();
		Vec3 boneWorldDir = (worldB - worldA).GetNormalized();
		Quat worldRot = Quat::MakeRotationFromTwoVectors(boneWorldDir, desiredDir);

		int parent = bone.m_parentBoneIndex;
		Quat parentWorldRot = Quat::DEFAULT;
		if (parent != -1)
		{
			parentWorldRot = Quat::MakeFromMat44(m_spider.m_bones[parent].m_worldBoneTransform);
		}

		// Computing the bones local rotation
		Quat localRot = parentWorldRot.QuatInverse() * worldRot;

		bone.SetLocalBoneRotation(localRot);
	}
}