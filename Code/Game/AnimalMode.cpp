#include "Game/AnimalMode.hpp"
#include "Game/App.h"
#include "Game/Terrain.hpp"
#include "Game/Snake.hpp"
#include "Game/Spider.hpp"
#include "Game/Octopus.hpp"
#include "Engine/Core/DebugRender.hpp"
#include "Engine/Core/VertexUtils.h"
#include "Engine/Animation/Animation.hpp"
#include "Engine/Math/SmoothNoise.hpp"
#include "Engine/Input/InputSystem.h"

AnimalMode::AnimalMode(App* owner)
	:Game(owner)
{
}

void AnimalMode::StartUp()
{
	// Setting initial camera position
	m_cameraPos = Vec3(-16.5f, -0.5f, 7.f);

	// Initialize terrain
	m_terrain = new Terrain(Vec3::ZERO);
	m_terrain->InitializeTerrainHills();

	// Get skybox textures
	m_skyBoxFrontTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/stormydays_ft.png");
	m_skyBoxBackTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/stormydays_bk.png");
	m_skyBoxLeftTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/stormydays_lf.png");
	m_skyBoxRightTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/stormydays_rt.png");
	m_skyBoxTopTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/stormydays_up.png");
	m_skyBoxBottomTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/stormydays_dn.png");

	InitializeAnimals();
}

void AnimalMode::InitializeAnimals()
{
	// Initialize the skeletons
	m_snake = new Snake(this, Vec3(25.f, 25.f, 0.f));
	m_snakeStationary = new Snake(this, Vec3(0.f, -10.f, 0.f));
	m_spider = new Spider(this, Vec3(85.f, 30.f, 0.f));
	m_spider2 = new Spider(this, Vec3(50.f, 50.f, 0.f));
	m_spiderStationary = new Spider(this, Vec3(0.f, -15.f, 0.f));
	m_octopus = new Octopus(this, Vec3(40.f, 45.f, 0.f));
	m_octopusStationary = new Octopus(this, Vec3(0.f, -5.f, 1.f));

	m_spider2->SetSpeed(3.5f);
	m_spider2->SetIsRoaming(true);
	m_spider2->SetIsCurlingLegs(true);

	// Set stationary
	m_octopusStationary->SetIsStationary(true);
	m_snakeStationary->SetIsStationary(true);
	m_spiderStationary->SetIsStationary(true);

	m_allEntities.push_back(m_snake);
	m_allEntities.push_back(m_snakeStationary);
	m_allEntities.push_back(m_spider);
	m_allEntities.push_back(m_spider2);
	m_allEntities.push_back(m_spiderStationary);
	m_allEntities.push_back(m_octopus);
	m_allEntities.push_back(m_octopusStationary);
}

void AnimalMode::Update()
{
	double deltaSeconds = g_theApp->m_gameClock->GetDeltaSeconds();
	double totalTime = g_theApp->m_gameClock->GetTotalSeconds();
	double frameRate = Clock::GetSystemClock().GetFrameRate();

	UpdateEntities(static_cast<float>(deltaSeconds));

	std::string timeScaleText = Stringf("Time: %0.2fs FPS: %0.2f", totalTime, frameRate);
	DebugAddScreenText(timeScaleText, m_gameSceneBounds, 15.f, Vec2(0.98f, 0.97f), 0.f);
	DebugAddScreenText("Animal Mode", m_gameSceneBounds, 20.f, Vec2(0.f, 0.97f), 0.f);
	DebugAddScreenText("[I] Invert terrain", m_gameSceneBounds, 15.f, Vec2(0.f, 0.945f), 0.f);
	DebugAddScreenText("[V] Toggle animal verts", m_gameSceneBounds, 15.f, Vec2(0.f, 0.925f), 0.f);
	DebugAddScreenText("[G] Toggle animal skeletons", m_gameSceneBounds, 15.f, Vec2(0.f, 0.905f), 0.f);

	if (g_theInput->WasKeyJustPressed('I'))
	{
		m_terrain->m_areHillsInverted = !m_terrain->m_areHillsInverted;
		m_terrain->InitializeTerrainHills();
	}
	if (g_theInput->WasKeyJustPressed('G'))
	{
		m_isSkeletonBeingDrawn = !m_isSkeletonBeingDrawn;
	}
	if (g_theInput->WasKeyJustPressed('V'))
	{
		m_isAnimalVertsBeingDrawn = !m_isAnimalVertsBeingDrawn;
	}

	AdjustForPauseAndTimeDistortion(static_cast<float>(deltaSeconds));
	KeyInputPresses();
	UpdateCameras(static_cast<float>(deltaSeconds));
}

void AnimalMode::Render() const
{
	if (m_isAttractMode == false)
	{
		g_theRenderer->BeginCamera(m_gameWorldCamera);
		g_theRenderer->ClearScreen(Rgba8::SAPPHIRE);
		RenderSkyBox();
		m_terrain->Render();
		RenderEntities();
		g_theRenderer->EndCamera(m_gameWorldCamera);

		g_theRenderer->BeginCamera(g_theApp->m_screenCamera);
		g_theRenderer->EndCamera(g_theApp->m_screenCamera);

		DebugRenderWorld(m_gameWorldCamera);
		DebugRenderScreen(g_theApp->m_screenCamera);
	}
}

void AnimalMode::Shutdown()
{
	DeleteTerrain();
	DeleteEntities();
}

void AnimalMode::DeleteTerrain()
{
	delete m_terrain;
	m_terrain = nullptr;
}

void AnimalMode::DeleteEntities()
{
	for (int entityIndex = 0; entityIndex < static_cast<int>(m_allEntities.size()); ++entityIndex)
	{
		Entity*& entity = m_allEntities[entityIndex];
		if (entity != nullptr)
		{
			delete entity;
			entity = nullptr;
		}
	}
}

void AnimalMode::UpdateCameras(float deltaSeconds)
{
	// Game Camera
	Mat44 cameraToRender(Vec3::ZAXE, -Vec3::XAXE, Vec3::YAXE, Vec3::ZERO);
	m_gameWorldCamera.SetCameraToRenderTransform(cameraToRender);

	FreeFlyControls(deltaSeconds);
	m_cameraOrientation.m_pitchDegrees = GetClamped(m_cameraOrientation.m_pitchDegrees, -85.f, 85.f);
	m_cameraOrientation.m_rollDegrees = GetClamped(m_cameraOrientation.m_rollDegrees, -45.f, 45.f);

	m_gameWorldCamera.SetPositionAndOrientation(m_cameraPos, m_cameraOrientation);
	m_gameWorldCamera.SetPerspectiveView(2.f, 60.f, 0.1f, 750.f);
}

void AnimalMode::UpdateEntities(float deltaSeconds)
{
	for (int entityIndex = 0; entityIndex < static_cast<int>(m_allEntities.size()); ++entityIndex)
	{
		Entity*& entity = m_allEntities[entityIndex];
		if (entity)
		{
			entity->Update(deltaSeconds);
		}
	}
}

void AnimalMode::RenderEntities() const
{
	for (int entityIndex = 0; entityIndex < static_cast<int>(m_allEntities.size()); ++entityIndex)
	{
		Entity* entity = m_allEntities[entityIndex];
		if (entity)
		{
			entity->Render();
		}
	}
}

void AnimalMode::RenderSkyBox() const
{
	Vec3 dimensionSize = Vec3(50.f, 50.f, 5.f);
	AABB3 skyBoxBounds = AABB3(Vec3(-5.f, -5.f, -30.f) * dimensionSize, Vec3(5.f, 5.f, 30.f) * dimensionSize);
	Vec3 mins = skyBoxBounds.m_mins;
	Vec3 maxs = skyBoxBounds.m_maxs;

	Vec3 bottomLeftFwd = Vec3(mins.x, maxs.y, mins.z);
	Vec3 bottomRightFwd = Vec3(mins.x, mins.y, mins.z);
	Vec3 topRightFwd = Vec3(mins.x, mins.y, maxs.z);
	Vec3 topLeftFwd = Vec3(mins.x, maxs.y, maxs.z);
	Vec3 bottomLeftBack = Vec3(maxs.x, maxs.y, mins.z);
	Vec3 bottomRightBack = Vec3(maxs.x, mins.y, mins.z);
	Vec3 topRightBack = Vec3(maxs.x, mins.y, maxs.z);
	Vec3 topLeftBack = Vec3(maxs.x, maxs.y, maxs.z);

	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_NONE);
	g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
	g_theRenderer->BindShader(nullptr);

	// FRONT (+X)
	std::vector<Vertex_PCU> frontVerts;
	AddVertsForQuad3D(frontVerts, bottomLeftFwd, bottomRightFwd, topRightFwd, topLeftFwd);
	g_theRenderer->BindTexture(m_skyBoxRightTexture);
	g_theRenderer->DrawVertexArray(frontVerts);

	// BACK (-X)
	std::vector<Vertex_PCU> backVerts;
	AddVertsForQuad3D(backVerts, bottomRightBack, bottomLeftBack, topLeftBack, topRightBack);
	g_theRenderer->BindTexture(m_skyBoxLeftTexture);
	g_theRenderer->DrawVertexArray(backVerts);

	// LEFT (-Y)
	std::vector<Vertex_PCU> leftVerts;
	AddVertsForQuad3D(leftVerts, bottomLeftBack, bottomLeftFwd, topLeftFwd, topLeftBack);
	g_theRenderer->BindTexture(m_skyBoxBackTexture);
	g_theRenderer->DrawVertexArray(leftVerts);

	// RIGHT (+Y)
	std::vector<Vertex_PCU> rightVerts;
	AddVertsForQuad3D(rightVerts, bottomRightFwd, bottomRightBack, topRightBack, topRightFwd);
	g_theRenderer->BindTexture(m_skyBoxFrontTexture);
	g_theRenderer->DrawVertexArray(rightVerts);

	// TOP (+Z)
	std::vector<Vertex_PCU> topVerts;
	AddVertsForQuad3D(topVerts, topLeftFwd, topRightFwd, topRightBack, topLeftBack);
	g_theRenderer->BindTexture(m_skyBoxTopTexture);
	g_theRenderer->DrawVertexArray(topVerts);

	// BOTTOM (-Z)
	std::vector<Vertex_PCU> bottomVerts;
	AddVertsForQuad3D(bottomVerts, bottomLeftBack, bottomRightBack, bottomRightFwd, bottomLeftFwd);
	g_theRenderer->BindTexture(m_skyBoxBottomTexture);
	g_theRenderer->DrawVertexArray(bottomVerts);
}
