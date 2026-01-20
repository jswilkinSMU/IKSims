#include "Game/Terrain.hpp"
#include "Game/GameCommon.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Math/SmoothNoise.hpp"
#include "Engine/Math/MathUtils.h"

Terrain::Terrain(Vec3 const& position)
	:m_terrainWorldPosition(position)
{
	m_shader = g_theRenderer->CreateOrGetShader("Data/Shaders/BlinnPhong", VertexType::VERTEX_PCUTBN);
	m_texture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/dirt.jpg");
}

Terrain::~Terrain()
{
	DeleteBuffers();
}

void Terrain::InitializeTerrainHills()
{
	// Initialize terrain
	GenerateHeightMap();
	GenerateMesh();
	CreateBuffers();
}

void Terrain::Render() const
{
	if (m_vertexBuffer == nullptr || m_indexBuffer == nullptr)
	{
		return;
	}

	if (m_indices.empty())
	{
		return;
	}

	g_theRenderer->SetLightingConstants(m_sunDirection, m_sunIntensity, m_ambientIntensity);
	g_theRenderer->SetModelConstants();
	g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
	g_theRenderer->SetSamplerMode(SamplerMode::BILINEAR_WRAP);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_NONE);
	g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
	g_theRenderer->BindSampler(SamplerMode::BILINEAR_WRAP, 0);
	g_theRenderer->BindSampler(SamplerMode::BILINEAR_WRAP, 1);
	g_theRenderer->BindSampler(SamplerMode::BILINEAR_WRAP, 2);
	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->BindShader(m_shader);
	g_theRenderer->DrawIndexedVertexBuffer(m_vertexBuffer, m_indexBuffer, static_cast<int>(m_indices.size()));
}

float Terrain::GetHeightAtXY(float x, float y) const
{
	float terrainX = x / TERRAIN_SCALE;
	float terrainY = y / TERRAIN_SCALE;

	int indexX = RoundDownToInt(terrainX);
	int indexY = RoundDownToInt(terrainY);

	if (!IsInBounds(indexX, indexY))
	{
		return 0.f;
	}

	float terrainIndexX = terrainX - indexX;
	float terrainIndexY = terrainY - indexY;

	int indexBottomLeft = indexY * TERRAIN_SIZE + indexX;
	int indexBottomRight = indexY * TERRAIN_SIZE + (indexX + 1);
	int indexTopLeft = (indexY + 1) * TERRAIN_SIZE + indexX;
	int indexTopRight = (indexY + 1) * TERRAIN_SIZE + (indexX + 1);

	float heightBottomLeft = m_terrainHeights[indexBottomLeft];
	float heightBottomRight = m_terrainHeights[indexBottomRight];
	float heightTopLeft = m_terrainHeights[indexTopLeft];
	float heightTopRight = m_terrainHeights[indexTopRight];

	// Bilinear interpolation
	float heightBottom = Interpolate(heightBottomLeft, heightBottomRight, terrainIndexX);
	float heightTop = Interpolate(heightTopLeft, heightTopRight, terrainIndexX);
	float height = Interpolate(heightBottom, heightTop, terrainIndexY);

	return height;
}

bool Terrain::IsInBounds(int x, int y) const
{
	if (x < 0 || y < 0 || x >= TERRAIN_SIZE - 1 || y >= TERRAIN_SIZE - 1)
	{
		return false;
	}
	return true;
}

void Terrain::GenerateHeightMap()
{
	m_terrainHeights.resize(TERRAIN_SIZE * TERRAIN_SIZE);
	m_minTerrainHeight = FLT_MAX;
	m_maxTerrainHeight = FLT_MIN;

	for (int terrainIndexY = 0; terrainIndexY < TERRAIN_SIZE; ++terrainIndexY)
	{
		for (int terrainIndexX = 0; terrainIndexX < TERRAIN_SIZE; ++terrainIndexX)
		{
			float terrainWorldX = static_cast<float>(terrainIndexX);
			float terrainWorldY = static_cast<float>(terrainIndexY);

			float total = 0.f;
			int numHills = 3;
			Vec2 hillCenters[] =
			{
				Vec2(TERRAIN_SIZE * 0.3f, TERRAIN_SIZE * 0.3f),
				Vec2(TERRAIN_SIZE * 0.7f, TERRAIN_SIZE * 0.4f),
				Vec2(TERRAIN_SIZE * 0.5f, TERRAIN_SIZE * 0.8f)
			};

			for (int hillIndex = 0; hillIndex < numHills; ++hillIndex)
			{
				float dx = (terrainWorldX - hillCenters[hillIndex].x) / (TERRAIN_SIZE * 0.3f);
				float dy = (terrainWorldY - hillCenters[hillIndex].y) / (TERRAIN_SIZE * 0.3f);
				float dist = sqrtf(dx * dx + dy * dy);
				float falloff = GetClamped(1.f - dist, 0.f, 1.f);
				if (!m_areHillsInverted)
				{
					total += falloff * falloff;
				}
				else
				{
					total -= falloff * falloff;
				}
			}
			float terrainHeight = total * 0.5f * HEIGHT_SCALE;

			m_minTerrainHeight = GetMin(m_minTerrainHeight, terrainHeight);
			m_maxTerrainHeight = GetMax(m_maxTerrainHeight, terrainHeight);

			m_terrainHeights[terrainIndexY * TERRAIN_SIZE + terrainIndexX] = terrainHeight;
		}
	}
}

void Terrain::GenerateMesh()
{
	m_vertices.clear();
	m_indices.clear();

	float texelsPerWorldUnit = 0.01f;

	for (int terrainIndexY = 0; terrainIndexY < TERRAIN_SIZE - 1; ++terrainIndexY)
	{
		for (int terrainIndexX = 0; terrainIndexX < TERRAIN_SIZE - 1; ++terrainIndexX)
		{
			int bL = terrainIndexY * TERRAIN_SIZE + terrainIndexX;
			int bR = terrainIndexY * TERRAIN_SIZE + (terrainIndexX + 1);
			int tR = (terrainIndexY + 1) * TERRAIN_SIZE + (terrainIndexX + 1);
			int tL = (terrainIndexY + 1) * TERRAIN_SIZE + terrainIndexX;

			Vec3 point0 = Vec3(static_cast<float>(terrainIndexX) * TERRAIN_SCALE,
				static_cast<float>(terrainIndexY) * TERRAIN_SCALE,
				m_terrainHeights[bL]);

			Vec3 point1 = Vec3(static_cast<float>(terrainIndexX + 1) * TERRAIN_SCALE,
				static_cast<float>(terrainIndexY) * TERRAIN_SCALE,
				m_terrainHeights[bR]);

			Vec3 point2 = Vec3(static_cast<float>(terrainIndexX + 1) * TERRAIN_SCALE,
				static_cast<float>(terrainIndexY + 1) * TERRAIN_SCALE,
				m_terrainHeights[tR]);

			Vec3 point3 = Vec3(static_cast<float>(terrainIndexX) * TERRAIN_SCALE,
				static_cast<float>(terrainIndexY + 1) * TERRAIN_SCALE,
				m_terrainHeights[tL]);

			float avgHeight = (m_terrainHeights[bL] + m_terrainHeights[bR] + m_terrainHeights[tR] + m_terrainHeights[tL]) * 0.25f;
			Rgba8 color = GetColorForHeight(avgHeight);
			
			// UVs based on world position
			float x0 = static_cast<float>(terrainIndexX) * TERRAIN_SCALE;
			float x1 = static_cast<float>(terrainIndexX + 1) * TERRAIN_SCALE;
			float y0 = static_cast<float>(terrainIndexY) * TERRAIN_SCALE;
			float y1 = static_cast<float>(terrainIndexY + 1) * TERRAIN_SCALE;
			Vec2  uvMins = Vec2(x0, y0) * texelsPerWorldUnit;
			Vec2  uvMaxs = Vec2(x1, y1) * texelsPerWorldUnit;
			AABB2 uvs = AABB2(uvMins, uvMaxs);

			AddVertsForQuad3D(m_vertices, m_indices, point0, point1, point2, point3, color, uvs);
		}
	}
}

void Terrain::CreateBuffers()
{
	if (m_vertices.empty())
	{
		return;
	}

	if (m_indices.empty())
	{
		return;
	}

	DeleteBuffers();

	m_vertexBuffer = g_theRenderer->CreateVertexBuffer(static_cast<unsigned int>(m_vertices.size()) * sizeof(Vertex_PCUTBN), sizeof(Vertex_PCUTBN));
	m_indexBuffer = g_theRenderer->CreateIndexBuffer(static_cast<unsigned int>(m_indices.size()) * sizeof(unsigned int), sizeof(unsigned int));
	g_theRenderer->CopyCPUToGPU(m_vertices.data(), m_vertexBuffer->GetSize(), m_vertexBuffer);
	g_theRenderer->CopyCPUToGPU(m_indices.data(), m_indexBuffer->GetSize(), m_indexBuffer);
}

void Terrain::DeleteBuffers()
{
	delete m_vertexBuffer;
	m_vertexBuffer = nullptr;

	delete m_indexBuffer;
	m_indexBuffer = nullptr;
}

Rgba8 Terrain::GetColorForHeight(float height)
{
	float t = RangeMapClamped(height, m_minTerrainHeight, m_maxTerrainHeight, 0.f, 1.f);

	if (t < 0.25f)
	{
		return Rgba8(0, 0, 180);
	}
	else if (t < 0.4f)
	{
		return Rgba8(194, 178, 128);
	}
	else if (t < 0.75f)
	{
		return Rgba8(34, 139, 34);
	}
	else if (t < 0.9f)
	{
		return Rgba8(120, 120, 120);
	}
	else
	{
		return Rgba8::WHITE;
	}
}
