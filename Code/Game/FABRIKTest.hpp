#pragma once
#include "Game/Game.h"
// -----------------------------------------------------------------------------
class App;
// -----------------------------------------------------------------------------
class FABRIKTest : public Game
{
public:
	FABRIKTest(App* owner);

	void StartUp() override;
	void Update() override;
	void Render() const override;
	void Shutdown() override;

	// Initialization
	Skeleton CreateTestChain();

	// Updating
	void UpdateCameras(float deltaSeconds);
	void AddJoint();
	void RemoveJoint();

	// Rendering
	void RenderSkeleton() const;

	// Destruction

private:
	std::vector<Vertex_PCU> m_skeletonVerts;
	std::vector<Vertex_PCU> m_textVerts;
	Skeleton m_skeleton;
};