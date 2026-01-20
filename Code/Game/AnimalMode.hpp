#pragma once
#include "Game/Game.h"
#include "Engine/Animation/AnimStateMachine.hpp"
// -----------------------------------------------------------------------------
class App;
class Entity;
class Terrain;
class Snake;
class Spider;
class Octopus;
// -----------------------------------------------------------------------------
class AnimalMode : public Game 
{
public:
	AnimalMode(App* owner);
	void InitializeAnimals();

	// Overrides
	void StartUp() override;
	void Update() override;
	void Render() const override;
	void Shutdown() override;

	// Updating
	void UpdateCameras(float deltaSeconds);
	void UpdateEntities(float deltaSeconds);

	// Rendering
	void RenderEntities() const;
	void RenderSkyBox() const;

	// Destruction
	void DeleteTerrain();
	void DeleteEntities();

public:
	// Animals
	Snake*   m_snake = nullptr;
	Spider*  m_spider = nullptr;
	Spider*  m_spider2 = nullptr;
	Octopus* m_octopus = nullptr;

	// Stationary animals
	Snake* m_snakeStationary = nullptr;
	Spider* m_spiderStationary = nullptr;
	Octopus* m_octopusStationary = nullptr;

	// Entity list
	std::vector<Entity*> m_allEntities;

	// Terrain
	Terrain* m_terrain = nullptr;

	// Skybox
	Texture* m_skyBoxFrontTexture = nullptr;
	Texture* m_skyBoxBackTexture = nullptr;
	Texture* m_skyBoxLeftTexture = nullptr;
	Texture* m_skyBoxRightTexture = nullptr;
	Texture* m_skyBoxTopTexture = nullptr;
	Texture* m_skyBoxBottomTexture = nullptr;

	// Debug Draw
	bool m_isSkeletonBeingDrawn = false;
	bool m_isAnimalVertsBeingDrawn = true;
};