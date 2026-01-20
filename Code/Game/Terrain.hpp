#pragma once
#include "Engine/Core/Vertex_PCUTBN.hpp"
#include <vector>
// -----------------------------------------------------------------------------
class VertexBuffer;
class IndexBuffer;
class Shader;
class Texture;
// -----------------------------------------------------------------------------
constexpr int   TERRAIN_SIZE = 100;
constexpr float TERRAIN_SCALE = 1.f;
constexpr float HEIGHT_SCALE = 27.5f;

constexpr float NOISE_SCALE = 0.05f;
constexpr int   NOISE_OCTAVES = 6;
constexpr int   TERRAIN_SEED = 1337;
// -----------------------------------------------------------------------------
class Terrain
{
public:
	Terrain() = default;
	Terrain(Vec3 const& position);
	~Terrain();

	void InitializeTerrainHills();
	void Render() const;

	float GetHeightAtXY(float x, float y) const;
	bool  IsInBounds(int x, int y) const;
	bool  m_areHillsInverted = false;

private:
	void GenerateHeightMap();
	void GenerateMesh();
	void CreateBuffers();
	void DeleteBuffers();
	Rgba8 GetColorForHeight(float height);

private:
	// Terrain
	std::vector<float> m_terrainHeights;
	float m_minTerrainHeight = 0.f;
	float m_maxTerrainHeight = 0.f;
	Vec3  m_terrainWorldPosition = Vec3::ZERO;

	// Lighting
	Texture* m_texture = nullptr;
	Shader* m_shader = nullptr;
	Vec3 m_sunDirection = Vec3(3.f, 1.f, -2.f);
	float m_sunIntensity = 0.85f;
	float m_ambientIntensity = 0.55f;

	// Buffers
	VertexBuffer* m_vertexBuffer = nullptr;
	IndexBuffer* m_indexBuffer = nullptr;
	std::vector<Vertex_PCUTBN> m_vertices;
	std::vector<unsigned int> m_indices;
};