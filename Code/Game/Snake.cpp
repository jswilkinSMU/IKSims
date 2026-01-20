#include "Game/Snake.hpp"
#include "Game/GameCommon.h"
#include "Game/AnimalMode.hpp"
#include "Game/Terrain.hpp"
#include "Engine/Input/InputSystem.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Animation/Animation.hpp"
#include "Engine/AI/BehaviorTree.hpp"

Snake::Snake(AnimalMode* animalMode, Vec3 position)
	:Entity(animalMode, position)
{
	m_snakeTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/snakeskin.jpg");
	Initialize();
}

Snake::~Snake()
{
	DeleteAnimations();

	delete m_snakeBT;
	m_snakeBT = nullptr;
}

void Snake::Initialize()
{
	// Initialize the skeleton
	m_snakeSkeleton = CreateSkeleton();

	m_snakeInitialPositions.clear();
	for (int snakeBoneIndex = 0; snakeBoneIndex < static_cast<int>(m_snakeSkeleton.m_bones.size()); ++snakeBoneIndex)
	{
		Bone& snakeBone = m_snakeSkeleton.m_bones[snakeBoneIndex];
		m_snakeInitialPositions.push_back(snakeBone.m_localPosition);

		Vec3 direction = Vec3::XAXE;
		if (snakeBone.m_parentBoneIndex != -1)
		{
			Vec3 parentPos = m_snakeSkeleton.m_bones[snakeBone.m_parentBoneIndex].m_localPosition;
			direction = snakeBone.m_localPosition - parentPos;
			if (!direction.IsNearlyZero())
			{
				direction.Normalize();
			}
		}
		m_snakeAnimationDirs.push_back(direction);
	}

	SetupAnimations();
	m_snakeBT = CreateSnakeBehaviorTree(this);
}

void Snake::Update(float deltaSeconds)
{
	if (m_snakeBT)
	{
		m_snakeBT->Update(deltaSeconds);
	}

	CheckTransitions();
	m_snakeStateMachine.Update(m_snakeSkeleton, static_cast<float>(deltaSeconds));

	if (!IsMoving())
	{
		UpdateVerts();
	}
	else
	{
		UpdateSnakePose(static_cast<float>(deltaSeconds));
		UpdateVerts();
	}
}

void Snake::CheckTransitions()
{
	if (IsMoving())
	{
		if (m_snakeStateMachine.GetCurrentAnimStateName() != "Slither")
		{
			m_snakeStateMachine.TriggerTransition("StartSlither");
		}
	}
	else
	{
		if (m_snakeStateMachine.GetCurrentAnimStateName() != "Idle")
		{
			m_snakeStateMachine.TriggerTransition("StopSlither");
		}
	}
}

void Snake::UpdateSnakePose(float deltaSeconds)
{
	static float elapsedTime = 0.f;
	elapsedTime += deltaSeconds;

	// Smoothly interpolate move direction
	if (m_moveDirection != m_targetMoveDirection)
	{
		m_directionInterpTime += deltaSeconds;
		float fractionTowardEnd = GetClamped(m_directionInterpTime / m_directionInterpDuration, 0.f, 1.f);

		float easedFraction = SmoothStep3(fractionTowardEnd);
		m_moveDirection = SLerp(m_moveDirection, m_targetMoveDirection, easedFraction).GetNormalized();
	}

	if (!m_isStationary)
	{
		// Move snake forward
		Vec3 snakeMove = -m_moveDirection * m_speed * deltaSeconds;
		m_worldPosition += snakeMove;

		// Project onto terrain
		float terrainZ = m_animalMode->m_terrain->GetHeightAtXY(m_worldPosition.x, m_worldPosition.y);
		m_worldPosition.z = terrainZ;

		// Reset if out of bounds
		if (!m_animalMode->m_terrain->IsInBounds(static_cast<int>(m_worldPosition.x), static_cast<int>(m_worldPosition.y)))
		{
			m_worldPosition = Vec3(85.f, 45.f, m_animalMode->m_terrain->GetHeightAtXY(85.f, 15.f));
			elapsedTime = 0.f;
		}
	}

	float offset = 1.f;
	Terrain* terrain = m_animalMode->m_terrain;

	// Get current and neighbor positions
	float hCenter = terrain->GetHeightAtXY(m_worldPosition.x, m_worldPosition.y);
	float hForward = terrain->GetHeightAtXY(m_worldPosition.x + m_moveDirection.x * offset, m_worldPosition.y + m_moveDirection.y * offset);
	float hLeft = terrain->GetHeightAtXY(m_worldPosition.x + m_moveDirection.y * offset, m_worldPosition.y - m_moveDirection.x * offset);

	// Move entire skeleton to world position
	Vec3 forward = (Vec3(m_moveDirection.x, m_moveDirection.y, hForward - hCenter)).GetNormalized();
	Vec3 left = (Vec3(m_moveDirection.y, -m_moveDirection.x, hLeft - hCenter)).GetNormalized();
	Vec3 up = CrossProduct3D(left, forward).GetNormalized();
	left = CrossProduct3D(forward, up).GetNormalized();

	Mat44 rotation;
	rotation.SetIJK3D(forward, left, up);

	Vec3 headLocalPos = m_snakeSkeleton.m_bones[0].m_localPosition;
	Vec3 headWorldOffset = rotation.TransformPosition3D(headLocalPos);

	Vec3 skeletonHeadOrigin = m_worldPosition - headWorldOffset;
	rotation.SetTranslation3D(skeletonHeadOrigin);

	m_snakeSkeleton.m_skeletonModelTransform = rotation;
	m_snakeSkeleton.UpdateSkeletonPose();
}

void Snake::UpdateVerts()
{
	m_snakeVerts.clear();
	m_snakeVertsUntextured.clear();
	if (m_animalMode->m_isSkeletonBeingDrawn)
	{
		m_snakeSkeletonVerts.clear();
	}

	if (m_animalMode->m_isSkeletonBeingDrawn)
	{
		m_snakeSkeleton.AddVertsForSkeleton3D(m_snakeSkeletonVerts);
	}

	for (int boneIndex = 0; boneIndex < static_cast<int>(m_snakeSkeleton.m_bones.size()); ++boneIndex)
	{
		Bone const& bone = m_snakeSkeleton.m_bones[boneIndex];
		Vec3 worldPosition = bone.m_worldBoneTransform.GetTranslation3D();
		float radius = 0.25f;

		// HEAD
		if (boneIndex == 0)
		{
			// Head sphere
			AddVertsForSphere3D(m_snakeVerts, worldPosition, 0.3f, Rgba8::BROWN);

			// Eyes offset from head position
			Vec3 forward = bone.m_worldBoneTransform.GetIBasis3D();
			Vec3 left = bone.m_worldBoneTransform.GetJBasis3D();
			Vec3 up = bone.m_worldBoneTransform.GetKBasis3D();

			// Positioning the eyes
			Vec3 leftEyePosition = worldPosition - forward * 0.2f + left * 0.1f + up * 0.25f;
			Vec3 rightEyePosition = worldPosition - forward * 0.2f - left * 0.1f + up * 0.25f;

			// White eye bases
			AddVertsForSphere3D(m_snakeVertsUntextured, leftEyePosition, 0.05f, Rgba8::WHITE);
			AddVertsForSphere3D(m_snakeVertsUntextured, rightEyePosition, 0.05f, Rgba8::WHITE);

			// Black pupils
			Vec3 pupilOffset = forward * 0.03f; 
			AddVertsForSphere3D(m_snakeVertsUntextured, leftEyePosition - pupilOffset, 0.025f, Rgba8::BLACK);
			AddVertsForSphere3D(m_snakeVertsUntextured, rightEyePosition - pupilOffset, 0.025f, Rgba8::BLACK);
		}
		// TAIL
		else if (boneIndex == static_cast<int>(m_snakeSkeleton.m_bones.size()) - 1)
		{
			int parentIndex = bone.m_parentBoneIndex;
			if (parentIndex != -1)
			{
				Vec3 parentPosition = m_snakeSkeleton.m_bones[parentIndex].m_worldBoneTransform.GetTranslation3D();
				Vec3 tailTip = worldPosition;
				Vec3 tailBase = parentPosition;
				AddVertsForCone3D(m_snakeVerts, tailBase, tailTip, radius, Rgba8::BROWN, AABB2::ZERO_TO_ONE, 24);
			}
		}
		// BODY SEGMENTS
		else
		{
			AddVertsForSphere3D(m_snakeVerts, worldPosition, radius, Rgba8::BROWN);

			if (bone.m_parentBoneIndex == -1)
			{
				continue;
			}

			Vec3 start = m_snakeSkeleton.m_bones[bone.m_parentBoneIndex].m_worldBoneTransform.GetTranslation3D();
			Vec3 end = bone.m_worldBoneTransform.GetTranslation3D();

			AddVertsForCylinder3D(m_snakeVerts, start, end, radius, Rgba8::BROWN, AABB2::ZERO_TO_ONE, 8);
		}
	}
}

void Snake::Render() const
{
	if (m_animalMode->m_isAnimalVertsBeingDrawn)
	{
		g_theRenderer->SetModelConstants();
		g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
		g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
		g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
		g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
		g_theRenderer->BindShader(nullptr);

		// Drawing textured
		g_theRenderer->BindTexture(m_snakeTexture);
		g_theRenderer->DrawVertexArray(m_snakeVerts);

		// Drawing untextured
		g_theRenderer->BindTexture(nullptr);
		g_theRenderer->DrawVertexArray(m_snakeVertsUntextured);
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
		g_theRenderer->DrawVertexArray(m_snakeSkeletonVerts);
	}
}

bool Snake::IsMoving() const
{
	return m_isMoving;
}

AnimStateMachine Snake::GetSnakeStateMachine() const
{
	return m_snakeStateMachine;
}

Skeleton Snake::CreateSkeleton()
{
	Skeleton snakeSkeleton;
	snakeSkeleton.m_bones.clear();

	// Ascending to descending
	Bone head;
	head.m_boneName = "Head";
	head.SetLocalBonePosition(Vec3(0.f, 3.f, 0.2f));
	snakeSkeleton.m_bones.push_back(head);

	Bone firstConnector;
	firstConnector.m_boneName = "FirstCoil";
	firstConnector.m_parentBoneIndex = 0;
	firstConnector.SetLocalBonePosition(Vec3(1.f, 0.25f, 0.f));
	snakeSkeleton.m_bones.push_back(firstConnector);

	Bone secondConnector;
	secondConnector.m_boneName = "SecondCoil";
	secondConnector.m_parentBoneIndex = 1;
	secondConnector.SetLocalBonePosition(Vec3(1.f, -0.25f, 0.f));
	snakeSkeleton.m_bones.push_back(secondConnector);

	Bone thirdConnector;
	thirdConnector.m_boneName = "ThirdCoil";
	thirdConnector.m_parentBoneIndex = 2;
	thirdConnector.SetLocalBonePosition(Vec3(1.f, 0.25f, 0.f));
	snakeSkeleton.m_bones.push_back(thirdConnector);

	Bone tail;
	tail.m_boneName = "Tail";
	tail.m_parentBoneIndex = 3;
	tail.SetLocalBonePosition(Vec3(1.f, -0.25f, 0.f));
	snakeSkeleton.m_bones.push_back(tail);

	snakeSkeleton.UpdateSkeletonPose();
	return snakeSkeleton;
}

void Snake::SetupAnimations()
{
	// Snake's stored anim sequences
	m_snakeIdleAnim = new Animation("Idle", 30.f, false, [this](Skeleton& skeleton, float time)
	{
		for (int snakeBoneIndex = 0; snakeBoneIndex < static_cast<int>(skeleton.m_bones.size()); ++snakeBoneIndex)
		{
			Bone& bone = skeleton.m_bones[snakeBoneIndex];
			Vec3 basePosition = m_snakeInitialPositions[snakeBoneIndex];
			float wave = sinf(time * 1.f + snakeBoneIndex * 0.5f) * 0.05f;
			Vec3 offset = m_snakeAnimationDirs[snakeBoneIndex] * wave;
			bone.SetLocalBonePosition(basePosition + offset);
		}
		skeleton.UpdateSkeletonPose();
	});

	m_snakeTailFlickIdleAnim = new Animation("IdleTailFlick", 30.f, false, [this](Skeleton& skeleton, float time)
	{
		float flickDegrees = SinDegrees(time * 720.f) * 20.f;
		Quat tailRotation = Quat::MakeFromEulerAngles(EulerAngles(0.f, 0.f, flickDegrees)); 

		Bone& tailConnector = skeleton.m_bones[3];
		Bone& tailBone = skeleton.m_bones[4];
		tailConnector.SetLocalBoneRotation(tailRotation);
		tailBone.SetLocalBoneRotation(tailRotation);

		skeleton.UpdateSkeletonPose();
	});

	m_snakeHeadRaiseIdleAnim = new Animation("IdleHeadRaise", 30.f, false, [this](Skeleton& skeleton, float time)
	{
		float raiseAmountDegrees = SinDegrees(time * 180.f) * 15.f;
		Quat headRotation = Quat::MakeFromEulerAngles(EulerAngles(0.f, raiseAmountDegrees, 0.f));

		Bone& headBone = skeleton.m_bones[0];
		headBone.SetLocalBoneRotation(headRotation);

		skeleton.UpdateSkeletonPose();
	});

	m_snakeSlitherAnim = new Animation("Slither", 30.f, false, [this](Skeleton& skeleton, float time)
	{
		for (int snakeBoneIndex = 0; snakeBoneIndex < static_cast<int>(skeleton.m_bones.size()); ++snakeBoneIndex)
		{
			Bone& bone = skeleton.m_bones[snakeBoneIndex];
			Vec3 basePosition = m_snakeInitialPositions[snakeBoneIndex];
			float wave = sinf(time * 4.f + snakeBoneIndex * 0.7f) * 0.5f;
			Vec3 offset = m_snakeAnimationDirs[snakeBoneIndex] * wave;
			bone.SetLocalBonePosition(basePosition + offset);
		}
		skeleton.UpdateSkeletonPose();
	});

	// Snake's state machine
	m_snakeStateMachine.AddAnimationState("Idle", m_snakeIdleAnim);
	m_snakeStateMachine.AddAnimationState("IdleTailFlick", m_snakeTailFlickIdleAnim);
	m_snakeStateMachine.AddAnimationState("IdleHeadRaise", m_snakeHeadRaiseIdleAnim);
	m_snakeStateMachine.AddAnimationState("Slither", m_snakeSlitherAnim);
	m_snakeStateMachine.AddTransition("Idle", "StartSlither", "Slither");
	m_snakeStateMachine.AddTransition("IdleTailFlick", "StartSlither", "Slither");
	//m_snakeStateMachine.AddTransition("IdleHeadRaise", "StartSlither", "Slither");
	m_snakeStateMachine.AddTransition("Slither", "StopSlither", "Idle");
	m_snakeStateMachine.SetAnimationState("Idle");
}

void Snake::DeleteAnimations()
{
	delete m_snakeIdleAnim;
	m_snakeIdleAnim = nullptr;

	delete m_snakeSlitherAnim;
	m_snakeSlitherAnim = nullptr;

	delete m_snakeTailFlickIdleAnim;
	m_snakeTailFlickIdleAnim = nullptr;

	delete m_snakeHeadRaiseIdleAnim;
	m_snakeHeadRaiseIdleAnim = nullptr;
}

void Snake::ReflectOffBounds()
{
	bool bounced = false;

	if (!m_animalMode->m_terrain->IsInBounds(static_cast<int>(m_worldPosition.x), static_cast<int>(m_worldPosition.y)))
	{
		m_moveDirection *= -1.f;
		bounced = true;
	}

	if (bounced)
	{
		m_moveDirection.Normalize();
	}
}

BehaviorTree* Snake::CreateSnakeBehaviorTree(Snake* snake)
{
	auto moveNode = std::make_shared<SnakeMoveNode>(snake);
	auto idleNode = std::make_shared<SnakeIdleNode>(snake, moveNode.get());

	auto root = std::make_shared<SelectorNode>(std::vector<BehaviorNodePtr>{moveNode, idleNode});
	return new BehaviorTree(root);
}
// -----------------------------------------------------------------------------
BehaviorResult SnakeMoveNode::Update(float deltaSeconds)
{
	if (m_doneMoving)
	{
		return BehaviorResult::FAILURE;
	}

	m_directionChangeTimer += deltaSeconds;
	m_moveDuration += deltaSeconds;

	if (m_directionChangeTimer >= m_timeToChangeDir)
	{
		m_directionChangeTimer = 0.f;

		// Picking a new random direction for the snake to go
		float angle = g_rng->RollRandomFloatInRange(0.f, 360.f);
		Vec2 directionXY = Vec2(CosDegrees(angle), SinDegrees(angle));
		Vec3 newDirection = Vec3(directionXY.x, directionXY.y, 0.f).GetNormalized();
		m_snake->m_targetMoveDirection = newDirection;
		m_snake->m_directionInterpTime = 0.f;
	}

	if (m_moveDuration >= m_maxMoveDuration)
	{
		m_moveDuration = 0.f;
		m_doneMoving = true;
		return BehaviorResult::FAILURE;
	}

	m_snake->m_isMoving = true;
	return BehaviorResult::SUCCESS;
}

void SnakeMoveNode::Reset()
{
	m_moveDuration = 0.f;
	m_doneMoving   = false;
}
// -----------------------------------------------------------------------------
BehaviorResult SnakeIdleNode::Update(float deltaSeconds)
{
	if (!m_startedIdle)
	{
		PlayRandomIdleAnimation();
		m_startedIdle = true;
	}

	m_idleDuration += deltaSeconds;
	m_snake->m_isMoving = false;

	if (m_idleDuration >= m_maxIdleTime)
	{
		m_idleDuration = 0.f;

		if (m_snakeMoveNode != nullptr)
		{
			m_snakeMoveNode->Reset();
			Reset();
		}

		return BehaviorResult::FAILURE;
	}

	return BehaviorResult::SUCCESS;
}

void SnakeIdleNode::PlayRandomIdleAnimation()
{
	int randomIdle = g_rng->RollRandomIntInRange(0, 2);
	switch (randomIdle)
	{
		case 0: m_snake->m_snakeStateMachine.SetAnimationState("Idle"); break;
		case 1: m_snake->m_snakeStateMachine.SetAnimationState("IdleTailFlick"); break;
		//case 2: m_snake->m_snakeStateMachine.SetAnimationState("IdleHeadRaise"); break;
	}
}

void SnakeIdleNode::Reset()
{
	m_startedIdle = false;
	m_idleDuration = 0.f; 
}
// -----------------------------------------------------------------------------
