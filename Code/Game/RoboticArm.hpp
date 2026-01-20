#pragma once
#include "Game/Game.h"
// -----------------------------------------------------------------------------
class App;
// -----------------------------------------------------------------------------
class RoboticArmMode : public Game
{
public:
	RoboticArmMode(App* owner);

	void StartUp() override;
	void Update() override;
	void Render() const override;
	void Shutdown() override;

	// Initialization
	Skeleton InitializeRoboticArm();
	void	 CreateBuffers();

	// Updating
	void UpdateCameras(float deltaSeconds);
	void UpdateVerts();
	void TargetPositionMovement(float deltaSeconds);
	void DebugVisuals();
	void ToggleConstraints();
	void SolveCCDIK(std::vector<int> const& chainIndices, Vec3 const& targetPosition, int endEffector, int maxIterations = 10, float threshold = 0.01f);
	void SolveCCDIKConstrained(std::vector<int> const& chainIndices, Vec3 const& targetPosition, int endEffector, int maxIterations = 10, float threshold = 0.01f);

	// Rendering
	void RenderRoboticArm() const;
	void GameModeAndControlsText() const;

	// Destroying
	void DeleteBuffers();

private:
	Skeleton m_roboticArm;
	std::vector<Vertex_PCU> m_roboSkeletonDebugVerts;
	std::vector<Vertex_PCU> m_textVerts;
	bool m_isArmConstrained = true;
	bool m_isSkeletonBeingDrawn = false;
	bool m_isDrawingVerts = true;

	Vec3 m_targetPosition = Vec3(0.f, 0.f, 7.f);

	// TBN
	VertexBuffer* m_vertexBuffer = nullptr;
	std::vector<Vertex_PCUTBN> m_roboArmVerts;
	int m_debugInt = 0;

	// Lighting
	Texture* m_testUVTexture = nullptr;
	Shader* m_shader = nullptr;
	Vec3 m_sunDirection = Vec3(3.f, 1.f, -2.f);
	float m_sunIntensity = 0.45f;
	float m_ambientIntensity = 0.35f;
};