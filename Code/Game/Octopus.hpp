#pragma once
#include "Game/Entity.hpp"
#include "Engine/Skeleton/Skeleton.hpp"
// -----------------------------------------------------------------------------
class AnimalMode;
// -----------------------------------------------------------------------------
constexpr int NUM_OCTOPUS_ARMS = 8;
// -----------------------------------------------------------------------------
class Octopus : public Entity
{
public:
	Octopus(AnimalMode* mode, Vec3 position);
	~Octopus();

	virtual void Initialize() override;
	virtual void Update(float deltaSeconds) override;
	virtual void Render() const override;

private:
	Skeleton CreateOctopusSkeleton();
	void DrawOctopus() const;
	void OctopusRoam(float deltaSeconds);
	void UpdateOctopusPose(float deltaSeconds);
	void UpdateOctopusVerts();

private:
	Skeleton m_octopus;
	std::vector<Vertex_PCU> m_octoSkeletonVerts;
	std::vector<Vec3> m_octopusInitialPositions;

	// Directional changes
	Vec3  m_targetMoveDirection = Vec3::XAXE;
	float m_directionInterpTime = 0.f;
	float m_directionInterpDuration = 10.f;
	float m_directionChangeTimer = 0.f;
	float m_timeToChangeDir = 7.f;
	bool  m_isRoaming = true;
};