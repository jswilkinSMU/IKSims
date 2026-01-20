#pragma once
#include "Game/Game.h"
// -----------------------------------------------------------------------------
class App;
// -----------------------------------------------------------------------------
enum class ConstraintMode
{
	FREE, 
	FORTY_FIVE,
	NINETY
};
// -----------------------------------------------------------------------------
class Game2D : public Game
{
public:
	Game2D(App* owner);
	~Game2D();

	// Overrides
	void StartUp() override;
	void Update() override;
	void Render() const override;
	void Shutdown() override;

	// Initialization
	Skeleton CreateTestSkeleton2D();

	// Updating
	void ConstraintKeyPresses();
	void AnimateSkeleton(float deltaSeconds);
	Quat ApplyConstraints(Bone* lowerArm, Quat lowerArmRotation);

	// Rendering
	void RenderSkeleton() const;
	void GameModeAndControlsText() const;

private:
	std::vector<Vertex_PCU> m_skeletonVerts;
	Skeleton m_skeleton;
	SkeletonStyle m_skeletonStyle;
	ConstraintMode m_constraintMode = ConstraintMode::FREE;
};