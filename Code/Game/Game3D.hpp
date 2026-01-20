#pragma once
#include "Game/Game.h"
// -----------------------------------------------------------------------------
class App;
// -----------------------------------------------------------------------------
class Game3D : public Game
{
public:
	Game3D(App* owner);

	void StartUp() override;
	void Update() override;

	void ToggleArms();

	void Render() const override;
	void Shutdown() override;

	// Initialization
	Skeleton CreateTestSkeleton();

	// Updating
	void AnimateSkeleton(float deltaSeconds);
	void UpdateCameras(float deltaSeconds);
	void TargetPosKeyPresses(double deltaSeconds);

	// Rendering
	void RenderSkeleton() const;
	void PrintBoneHierarchy() const;

	// Destruction

private:
	std::vector<Vertex_PCU> m_skeletonVerts;
	std::vector<Vertex_PCU> m_textVerts;
	Skeleton m_skeleton;

	Vec3 m_targetPos = Vec3(-1.5f, -2.f, 3.f);
	bool m_rightHandSelected = true;
	bool m_isAnimatingFreely = false;
};