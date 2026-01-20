#pragma once
#include "Game/Entity.hpp"
#include "Engine/Skeleton/Skeleton.hpp"
#include "Engine/AI/BehaviorNode.hpp"
// -----------------------------------------------------------------------------
class AnimalMode;
// -----------------------------------------------------------------------------
struct SpiderLeg
{
	int m_femurIndex      = 0;
	int m_tibiaIndex      = 0;
	int m_metaTarsusIndex = 0;
	int m_tarsusIndex     = 0;
	Vec3 m_defaultFootOffset  = Vec3::ZERO;
	Vec3 m_footTargetWorldPos = Vec3::ZERO;

	std::vector<int> m_boneIndices;
	std::vector<float> m_boneLengths;
};
// -----------------------------------------------------------------------------
struct SpiderHair
{
	Vec3 m_localOffset = Vec3::ZERO;  
	Vec3 m_localDirection = Vec3::ZERO;
	float m_hairLength = 0.25f;
	Rgba8 m_hairColor = Rgba8::GRAY;

	// Verlet simulation
	Vec3 m_tipPos = Vec3::ZERO;
	Vec3 m_prevTipPos = Vec3::ZERO;
};
// -----------------------------------------------------------------------------
class Spider : public Entity
{
public:
	Spider(AnimalMode* mode, Vec3 position);
	~Spider();

	virtual void Initialize() override;
	virtual void Update(float deltaSeconds) override;
	virtual void Render() const override;

	void SetIsRoaming(bool isRoaming);
	void SetIsCurlingLegs(bool isLegCurling);

private:
	Skeleton CreateSkeleton();
	void PopulateSpiderLegs();
	void ComputeLegBoneLengths();
	void RunFABRIK(SpiderLeg& leg);
	void ApplyFABRIKToSkeletonBones(SpiderLeg const& spiderLeg, std::vector<Vec3> const& joints);
	void UpdateSpiderPose(float deltaSeconds);
	void SpiderRoam(float deltaSeconds);

	void SimulateHair(float deltaSeconds);
	void UpdateSpiderVerts();
	void AddHairGeometry();
	void GenerateHair();

private:
	Skeleton m_spider;
	std::vector<Vertex_PCU> m_spiderVerts;
	std::vector<Vertex_PCU> m_spiderSkeletonVerts;
	std::vector<std::vector<SpiderHair>> m_hairsPerBone;
	std::vector<SpiderLeg>  m_legs;
	bool m_isLegCurling = false;

	// Directional changes
	Vec3  m_targetMoveDirection = Vec3::XAXE;
	float m_directionInterpTime = 0.f;
	float m_directionInterpDuration = 10.f;
	float m_directionChangeTimer = 0.f;
	float m_timeToChangeDir = 7.f;
	bool  m_isRoaming = false;
};
// -----------------------------------------------------------------------------