#pragma once
#include "Game/Entity.hpp"
#include "Engine/Skeleton/Skeleton.hpp"
#include "Engine/Animation/AnimStateMachine.hpp"
#include "Engine/AI/BehaviorNode.hpp"
#include <vector>
// -----------------------------------------------------------------------------
class AnimalMode;
class Animation;
class BehaviorTree;
// -----------------------------------------------------------------------------
class Snake : public Entity
{
public:
	Snake(AnimalMode* animalMode, Vec3 position);
	~Snake();

	virtual void Initialize() override;
	virtual void Update(float deltaSeconds) override;
	virtual void Render() const override;

	bool IsMoving() const;
	AnimStateMachine GetSnakeStateMachine() const;

public:
	Vec3  m_targetMoveDirection = Vec3::XAXE;
	float m_directionInterpTime = 0.f;
	float m_directionInterpDuration = 10.f;
	AnimStateMachine m_snakeStateMachine;

private:
	Skeleton CreateSkeleton();
	void UpdateSnakePose(float deltaSeconds);
	void UpdateVerts();

	void SetupAnimations();
	void CheckTransitions();
	void DeleteAnimations();

	void ReflectOffBounds();
	BehaviorTree* CreateSnakeBehaviorTree(Snake* snake);

private:
	Skeleton m_snakeSkeleton;
	std::vector<Vertex_PCU> m_snakeVerts;
	std::vector<Vertex_PCU> m_snakeVertsUntextured;
	std::vector<Vertex_PCU> m_snakeSkeletonVerts;

	Animation* m_snakeIdleAnim = nullptr;
	Animation* m_snakeTailFlickIdleAnim = nullptr;
	Animation* m_snakeHeadRaiseIdleAnim = nullptr;
	Animation* m_snakeSlitherAnim = nullptr;
	BehaviorTree* m_snakeBT = nullptr;
	Texture* m_snakeTexture = nullptr;

	std::vector<Vec3> m_snakeInitialPositions;
	std::vector<Vec3> m_snakeAnimationDirs;
};
// -----------------------------------------------------------------------------
/* Leaf Nodes */
class SnakeMoveNode : public BehaviorNode
{
public:
	SnakeMoveNode(Snake* snake)
		:m_snake(snake) {}

	BehaviorResult Update(float deltaSeconds) override;
	void Reset();

private:
	Snake* m_snake = nullptr;
	float m_directionChangeTimer = 0.f;
	const float m_timeToChangeDir = 7.f;

	float m_moveDuration = 0.f;
	const float m_maxMoveDuration = 10.f;
	bool m_doneMoving = false;
};

class SnakeIdleNode : public BehaviorNode
{
public:
	SnakeIdleNode(Snake* snake, SnakeMoveNode* moveNode)
		:m_snake(snake), m_snakeMoveNode(moveNode) {}

	BehaviorResult Update(float deltaSeconds) override;
	void PlayRandomIdleAnimation();
	void Reset();

private:
	Snake* m_snake = nullptr;
	SnakeMoveNode* m_snakeMoveNode = nullptr;

	float m_idleDuration = 0.f;
	const float m_maxIdleTime = 3.f;

	bool m_startedIdle = false;
};
// -----------------------------------------------------------------------------