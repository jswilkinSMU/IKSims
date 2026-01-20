#pragma once
#include "Game/Game.h"
// -----------------------------------------------------------------------------
class App;
// -----------------------------------------------------------------------------
class CCDIKTest : public Game
{
public:
	CCDIKTest(App* owner);

	// Overrides
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
	void RenderChain() const;

	// Destruction

private:
	std::vector<Vertex_PCU> m_skeletonVerts;
	std::vector<Vertex_PCU> m_textVerts;
	Skeleton m_skeleton;
};