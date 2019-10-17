#include "GameState.h"
#include "imgui.h"
#include "Sail/entities/ECS.h"
#include "Sail/entities/components/Components.h"
#include "Sail/entities/systems/Systems.h"
#include "Sail/ai/states/AttackingState.h"
#include "Sail/graphics/shader/compute/AnimationUpdateComputeShader.h"
#include "Sail/TimeSettings.h"
#include "Sail/utils/GameDataTracker.h"
#include "../SPLASH/src/game/events/NetworkSerializedPackageEvent.h"
#include "Network/NWrapperSingleton.h"
#include <sstream>
#include <iomanip>

// Uncomment to use forward rendering
//#define DISABLE_RT

GameState::GameState(StateStack& stack)
	: State(stack)
	, m_cam(90.f, 1280.f / 720.f, 0.1f, 5000.f)
	, m_profiler(true)
	, m_showcaseProcGen(false) {
	
	initConsole();

	// Get the Application instance
	m_app = Application::getInstance();
	m_isSingleplayer = NWrapperSingleton::getInstance().getPlayers().size() == 1;

	//----Octree creation----
	//Wireframe shader
	auto* wireframeShader = &m_app->getResourceManager().getShaderSet<GBufferWireframe>();

	//Wireframe bounding box model
	Model* boundingBoxModel = &m_app->getResourceManager().getModel("boundingBox.fbx", wireframeShader);
	//boundingBoxModel->getMesh(0)->getMaterial()->setAlbedoTexture("sponza/textures/candleBasicTexture.tga");

	boundingBoxModel->getMesh(0)->getMaterial()->setColor(glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
	boundingBoxModel->getMesh(0)->getMaterial()->setAOScale(0.5);
	boundingBoxModel->getMesh(0)->getMaterial()->setMetalnessScale(0.5);
	boundingBoxModel->getMesh(0)->getMaterial()->setRoughnessScale(0.5);

	//Create octree
	m_octree = SAIL_NEW Octree(boundingBoxModel);
	//-----------------------

	m_renderSettingsWindow.activateMaterialPicking(&m_cam, m_octree);

	// Setting light index
	m_currLightIndex = 0;

	// Get the player id's and names from the lobby
	const unsigned char playerID = NWrapperSingleton::getInstance().getMyPlayerID();

#ifdef _PERFORMANCE_TEST
	// TODO: Should be used but initial yaw and pitch isn't calculated from the cams direction vector in GameInputSystem
	m_cam.setDirection(glm::normalize(glm::vec3(-0.715708f, 0.0819399f, 0.693576f)));
#endif

	// Initialize the component systems
	initSystems(playerID);

	// Textures needs to be loaded before they can be used
	// TODO: automatically load textures when needed so the following can be removed
	m_app->getResourceManager().loadTexture("sponza/textures/spnza_bricks_a_ddn.tga");
	m_app->getResourceManager().loadTexture("sponza/textures/spnza_bricks_a_diff.tga");
	m_app->getResourceManager().loadTexture("sponza/textures/spnza_bricks_a_spec.tga");
	m_app->getResourceManager().loadTexture("sponza/textures/arenaBasicTexture.tga");
	m_app->getResourceManager().loadTexture("sponza/textures/barrierBasicTexture.tga");
	m_app->getResourceManager().loadTexture("sponza/textures/containerBasicTexture.tga");
	m_app->getResourceManager().loadTexture("sponza/textures/rampBasicTexture.tga");
	m_app->getResourceManager().loadTexture("sponza/textures/candleBasicTexture.tga");
	m_app->getResourceManager().loadTexture("sponza/textures/character1texture.tga");

	Application::getInstance()->getResourceManager().loadTexture("pbr/Character/CharacterMRAO.tga");
	Application::getInstance()->getResourceManager().loadTexture("pbr/Character/CharacterNM.tga");
	Application::getInstance()->getResourceManager().loadTexture("pbr/Character/CharacterTex.tga");

	// Add a directional light which is used in forward rendering
	glm::vec3 color(0.0f, 0.0f, 0.0f);
	glm::vec3 direction(0.4f, -0.2f, 1.0f);
	direction = glm::normalize(direction);
	m_lights.setDirectionalLight(DirectionalLight(color, direction));
	m_app->getRenderWrapper()->getCurrentRenderer()->setLightSetup(&m_lights);

	m_lightDebugWindow.setLightSetup(&m_lights);

	// Disable culling for testing purposes
	m_app->getAPI()->setFaceCulling(GraphicsAPI::NO_CULLING);

#ifdef DISABLE_RT
	auto* shader = &m_app->getResourceManager().getShaderSet<MaterialShader>();
	m_app->getRenderWrapper()->changeRenderer(1);
	m_app->getRenderWrapper()->getCurrentRenderer()->setLightSetup(&m_lights);
#else
	auto* shader = &m_app->getResourceManager().getShaderSet<GBufferOutShader>();
#endif
	m_app->getResourceManager().setDefaultShader(shader);

	// Create/load models
	Model* cubeModel = &m_app->getResourceManager().getModel("cubeWidth1.fbx", shader);
	cubeModel->getMesh(0)->getMaterial()->setColor(glm::vec4(0.2f, 0.8f, 0.4f, 1.0f));

	m_componentSystems.animationInitSystem->loadAnimations();

	Model* lightModel = &m_app->getResourceManager().getModel("candleExported.fbx", shader);
	lightModel->getMesh(0)->getMaterial()->setAlbedoTexture("sponza/textures/candleBasicTexture.tga");

	Model* characterModel = &m_app->getResourceManager().getModel("Character.fbx", shader);
	characterModel->getMesh(0)->getMaterial()->setMetalnessRoughnessAOTexture("pbr/Character/CharacterMRAO.tga");
	characterModel->getMesh(0)->getMaterial()->setAlbedoTexture("pbr/Character/CharacterTex.tga");
	characterModel->getMesh(0)->getMaterial()->setNormalTexture("pbr/Character/CharacterNM.tga");

	Model* aiModel = &m_app->getResourceManager().getModel("Character.fbx", shader);
	aiModel->getMesh(0)->getMaterial()->setMetalnessRoughnessAOTexture("pbr/Character/CharacterMRAO.tga");
	aiModel->getMesh(0)->getMaterial()->setAlbedoTexture("pbr/Character/CharacterTex.tga");
	aiModel->getMesh(0)->getMaterial()->setNormalTexture("pbr/Character/CharacterNM.tga");

	// Level Creation

	createLevel(shader, boundingBoxModel);

	// Player creation

	int id = static_cast<int>(playerID);
	glm::vec3 spawnLocation = glm::vec3(0.f);
	for (int i = -1; i < id; i++) {
		spawnLocation = m_componentSystems.levelGeneratorSystem->getSpawnPoint();
	}

	m_player = EntityFactory::CreatePlayer(boundingBoxModel, cubeModel, lightModel, playerID, m_currLightIndex++, spawnLocation).get();
	m_componentSystems.networkReceiverSystem->initPlayer(m_player);

	m_componentSystems.animationInitSystem->initAnimations();

	// Inform CandleSystem of the player
	m_componentSystems.candleSystem->setPlayerEntityID(m_player->getID(), m_player);

	// Bots creation
	createBots(boundingBoxModel, characterModel, cubeModel, lightModel);

#ifdef _PERFORMANCE_TEST
	populateScene(characterModel, lightModel, boundingBoxModel, boundingBoxModel, shader);

	m_player->getComponent<TransformComponent>()->setTranslation(glm::vec3(120.83f, 1.7028f, 114.2561f));
#endif


#ifdef _DEBUG
	// Candle1 holds all lights you can place in debug...
	m_componentSystems.lightListSystem->setDebugLightListEntity("Map_Candle1");
#endif

	auto nodeSystemCube = ModelFactory::CubeModel::Create(glm::vec3(0.1f), shader);
#ifdef _DEBUG_NODESYSTEM
	m_componentSystems.aiSystem->initNodeSystem(nodeSystemCube.get(), m_octree, wireframeShader);
#else
	m_componentSystems.aiSystem->initNodeSystem(nodeSystemCube.get(), m_octree);
#endif

	m_gameMusic = ECS::Instance()->createEntity("LobbyAudio").get();
	m_gameMusic->addComponent<AudioComponent>();
	m_gameMusic->addComponent<TransformComponent>(glm::vec3{ 0.0f, 0.0f, 0.0f });
	m_gameMusic->getComponent<AudioComponent>()->streamSoundRequest_HELPERFUNC("../Audio/LobbyMusic.xwb", true, 1.0f, true);

	m_playerInfoWindow.setPlayerInfo(m_player, &m_cam);
}

GameState::~GameState() {
	shutDownGameState();
	delete m_octree;
}

// Process input for the state
// NOTE: Done every frame
bool GameState::processInput(float dt) {

#ifdef _DEBUG
	// Add point light at camera pos
	if (Input::WasKeyJustPressed(KeyBinds::addLight)) {
		m_componentSystems.lightListSystem->addPointLightToDebugEntity(&m_lights, &m_cam);
	}

#endif

	// Enable bright light and move camera to above procedural generated level
	if (Input::WasKeyJustPressed(KeyBinds::toggleSun)) {
		m_lightDebugWindow.setManualOverride(!m_lightDebugWindow.isManualOverrideOn());
		m_showcaseProcGen = m_lightDebugWindow.isManualOverrideOn();
		if (m_showcaseProcGen) {
			m_lights.getPLs()[0].setPosition(glm::vec3(100.f, 20.f, 100.f));
			m_lights.getPLs()[0].setAttenuation(0.2f, 0.f, 0.f);
		} else {
			m_cam.setPosition(glm::vec3(0.f, 1.f, 0.f));
		}
	}

	// Show boudning boxes
	if (Input::WasKeyJustPressed(KeyBinds::toggleBoundingBoxes)) {
		m_componentSystems.boundingboxSubmitSystem->toggleHitboxes();
	}

	//Test ray intersection
	if (Input::IsKeyPressed(KeyBinds::testRayIntersection)) {
		Octree::RayIntersectionInfo tempInfo;
		m_octree->getRayIntersection(m_cam.getPosition(), m_cam.getDirection(), &tempInfo);
		if (tempInfo.info[tempInfo.closestHitIndex].entity) {
			Logger::Log("Ray intersection with " + tempInfo.info[tempInfo.closestHitIndex].entity->getName() + ", " + std::to_string(tempInfo.closestHit) + " meters away");
		}
	}

	if (Input::WasKeyJustPressed(KeyBinds::spray)) {
		Octree::RayIntersectionInfo tempInfo;
		m_octree->getRayIntersection(m_cam.getPosition(), m_cam.getDirection(), &tempInfo);
		if (tempInfo.closestHitIndex != -1) {
			// size (the size you want) = 0.3
			// halfSize = (1 / 0.3) * 0.5 = 1.667
			m_app->getRenderWrapper()->getCurrentRenderer()->submitDecal(m_cam.getPosition() + m_cam.getDirection() * tempInfo.closestHit, glm::identity<glm::mat4>(), glm::vec3(1.667f));
		}
	}

	//Test frustum culling
	if (Input::IsKeyPressed(KeyBinds::testFrustumCulling)) {
		int nrOfDraws = m_octree->frustumCulledDraw(m_cam);
		Logger::Log("Number of draws " + std::to_string(nrOfDraws));
	}

	// TODO: Move this to a system
	// Toggle ai following the player
	if (Input::WasKeyJustPressed(KeyBinds::toggleAIFollowing)) {
		auto entities = m_componentSystems.aiSystem->getEntities();
		for (int i = 0; i < entities.size(); i++) {
			auto aiComp = entities[i]->getComponent<AiComponent>();
			if (aiComp->entityTarget == nullptr) {

				// Find the candle child entity of player
				Entity* candle = nullptr;
				std::vector<Entity::SPtr> children = m_player->getChildEntities();
				for (auto& child : children) {
					if (child->hasComponent<CandleComponent>()) {
						candle = child.get();
						break;
					}
				}
				aiComp->setTarget(candle);
			} else {
				aiComp->setTarget(nullptr);
			}
		}
	}

	// Set directional light if using forward rendering
	if (Input::IsKeyPressed(KeyBinds::setDirectionalLight)) {
		glm::vec3 color(1.0f, 1.0f, 1.0f);
		m_lights.setDirectionalLight(DirectionalLight(color, m_cam.getDirection()));
	}
	
	// Reload shaders
	if (Input::WasKeyJustPressed(KeyBinds::reloadShader)) {
		m_app->getResourceManager().reloadShader<AnimationUpdateComputeShader>();
		m_app->getResourceManager().reloadShader<GBufferOutShader>();
	}

	// Pause game
	if (Input::WasKeyJustPressed(KeyBinds::showInGameMenu)) {
		if (m_paused) {
			Input::HideCursor(true);
			m_paused = false;
			requestStackPop();
		} else if (!m_paused && m_isSingleplayer) {
			Input::HideCursor(false);
			m_paused = true;
			requestStackPush(States::Pause);

		}

	}

	if (Input::WasKeyJustPressed(KeyBinds::toggleSphere)) {
		static bool attach = false;
		attach = !attach;
		if (attach) {
			CollisionSpheresComponent* csc = m_player->addComponent<CollisionSpheresComponent>();
			csc->spheres[0].radius = 0.4f;
			csc->spheres[1].radius = csc->spheres[0].radius;
			csc->spheres[0].position = m_player->getComponent<TransformComponent>()->getTranslation() + glm::vec3(0, 1, 0) * (-0.9f + csc->spheres[0].radius);
			csc->spheres[1].position = m_player->getComponent<TransformComponent>()->getTranslation() + glm::vec3(0, 1, 0) * (0.9f - csc->spheres[1].radius);
		} else {
			m_player->removeComponent<CollisionSpheresComponent>();
		}
	}

#ifdef _DEBUG
	// Removes first added pointlight in arena
	if (Input::WasKeyJustPressed(KeyBinds::removeOldestLight)) {
		m_componentSystems.lightListSystem->removePointLightFromDebugEntity();
	}
#endif

	return true;
}

void GameState::initSystems(const unsigned char playerID) {
	m_componentSystems.movementSystem = ECS::Instance()->createSystem<MovementSystem>();
	
	m_componentSystems.collisionSystem = ECS::Instance()->createSystem<CollisionSystem>();
	m_componentSystems.collisionSystem->provideOctree(m_octree);

	m_componentSystems.movementPostCollisionSystem = ECS::Instance()->createSystem<MovementPostCollisionSystem>();

	m_componentSystems.speedLimitSystem = ECS::Instance()->createSystem<SpeedLimitSystem>();

	m_componentSystems.animationSystem = ECS::Instance()->createSystem<AnimationSystem>();
	m_componentSystems.animationInitSystem = ECS::Instance()->createSystem<AnimationInitSystem>();

	m_componentSystems.updateBoundingBoxSystem = ECS::Instance()->createSystem<UpdateBoundingBoxSystem>();

	m_componentSystems.octreeAddRemoverSystem = ECS::Instance()->createSystem<OctreeAddRemoverSystem>();
	m_componentSystems.octreeAddRemoverSystem->provideOctree(m_octree);
	m_componentSystems.octreeAddRemoverSystem->setCulling(true, &m_cam); // Enable frustum culling

	m_componentSystems.lifeTimeSystem = ECS::Instance()->createSystem<LifeTimeSystem>();

	m_componentSystems.entityAdderSystem = ECS::Instance()->getEntityAdderSystem();

	m_componentSystems.entityRemovalSystem = ECS::Instance()->getEntityRemovalSystem();

	m_componentSystems.aiSystem = ECS::Instance()->createSystem<AiSystem>();

	m_componentSystems.lightSystem = ECS::Instance()->createSystem<LightSystem>();
	m_componentSystems.lightListSystem = ECS::Instance()->createSystem<LightListSystem>();

	m_componentSystems.candleSystem = ECS::Instance()->createSystem<CandleSystem>();
	m_componentSystems.candleSystem->init(this);

	// Create system which prepares each new update
	m_componentSystems.prepareUpdateSystem = ECS::Instance()->createSystem<PrepareUpdateSystem>();

	// Create system which handles creation of projectiles
	m_componentSystems.gunSystem = ECS::Instance()->createSystem<GunSystem>();

	// Create system which checks projectile collisions
	m_componentSystems.projectileSystem = ECS::Instance()->createSystem<ProjectileSystem>();

	m_componentSystems.levelGeneratorSystem = ECS::Instance()->createSystem<LevelGeneratorSystem>();

	// Create systems for rendering
	m_componentSystems.beginEndFrameSystem = ECS::Instance()->createSystem<BeginEndFrameSystem>();
	m_componentSystems.boundingboxSubmitSystem = ECS::Instance()->createSystem<BoundingboxSubmitSystem>();
	m_componentSystems.metaballSubmitSystem = ECS::Instance()->createSystem<MetaballSubmitSystem>();
	m_componentSystems.modelSubmitSystem = ECS::Instance()->createSystem<ModelSubmitSystem>();
	m_componentSystems.realTimeModelSubmitSystem = ECS::Instance()->createSystem<RealTimeModelSubmitSystem>();
	m_componentSystems.renderImGuiSystem = ECS::Instance()->createSystem<RenderImGuiSystem>();

	// Create system for player input
	m_componentSystems.gameInputSystem = ECS::Instance()->createSystem<GameInputSystem>();
	m_componentSystems.gameInputSystem->initialize(&m_cam);

	// Create network send and receive systems
	m_componentSystems.networkSenderSystem = ECS::Instance()->createSystem<NetworkSenderSystem>();
	NWrapperSingleton::getInstance().setNSS(m_componentSystems.networkSenderSystem);
	m_componentSystems.networkReceiverSystem = ECS::Instance()->createSystem<NetworkReceiverSystem>();
	m_componentSystems.networkReceiverSystem->init(playerID, this, m_componentSystems.networkSenderSystem);

	// Create system for handling and updating sounds
	m_componentSystems.audioSystem = ECS::Instance()->createSystem<AudioSystem>();
}

void GameState::initConsole() {
	auto& console = Application::getInstance()->getConsole();
	console.addCommand("state <string>", [&](const std::string& param) {
		if (param == "menu") {
			requestStackPop();
			requestStackPush(States::MainMenu);
			m_poppedThisFrame = true;
			console.removeAllCommandsWithIdentifier("GameState");
			return "State change to menu requested";
		}
		else if (param == "pbr") {
			requestStackPop();
			requestStackPush(States::PBRTest);
			m_poppedThisFrame = true;
			console.removeAllCommandsWithIdentifier("GameState");
			return "State change to pbr requested";
		}
		else if (param == "perftest") {
			requestStackPop();
			requestStackPush(States::PerformanceTest);
			m_poppedThisFrame = true;
			console.removeAllCommandsWithIdentifier("GameState");
			return "State change to PerformanceTest requested";
		}
		else {
			return "Invalid state. Available states are \"menu\", \"perftest\" and \"pbr\"";
		}

	}, "GameState");
	console.addCommand("profiler", [&]() { return toggleProfiler(); }, "GameState");
	console.addCommand("EndGame", [&]() {
		NWrapperSingleton::getInstance().queueGameStateNetworkSenderEvent(
			Netcode::MessageType::MATCH_ENDED,
			nullptr
		);
		this->requestStackPop();
		this->requestStackPush(States::EndGame);
		console.removeAllCommandsWithIdentifier("GameState");

		return std::string("Match ended.");
		}, "GameState");
#ifdef _DEBUG
	console.addCommand("AddCube", [&]() {
		return createCube(m_cam.getPosition());
	}, "GameState");
	console.addCommand("tpmap", [&]() {return teleportToMap(); }, "GameState");
	console.addCommand("AddCube <int> <int> <int>", [&](std::vector<int> in) {
		if (in.size() == 3) {
			glm::vec3 pos(in[0], in[1], in[2]);
			return createCube(pos);
		}
		else {
			return std::string("Error: wrong number of inputs. Console Broken");
		}
		return std::string("wat");
	}, "GameState");
	console.addCommand("AddCube <float> <float> <float>", [&](std::vector<float> in) {
		if (in.size() == 3) {
			glm::vec3 pos(in[0], in[1], in[2]);
			return createCube(pos);
		}
		else {
			return std::string("Error: wrong number of inputs. Console Broken");
		}
		return std::string("wat");
	}, "GameState");
#endif
}

bool GameState::onEvent(Event& event) {
	EventHandler::dispatch<WindowResizeEvent>(event, SAIL_BIND_EVENT(&GameState::onResize));
	EventHandler::dispatch<NetworkSerializedPackageEvent>(event, SAIL_BIND_EVENT(&GameState::onNetworkSerializedPackageEvent));
	EventHandler::dispatch<NetworkDisconnectEvent>(event, SAIL_BIND_EVENT(&GameState::onPlayerDisconnect));
	EventHandler::dispatch<PlayerCandleDeathEvent>(event, SAIL_BIND_EVENT(&GameState::onPlayerCandleDeath));

	return true;
}

bool GameState::onResize(WindowResizeEvent& event) {
	m_cam.resize(event.getWidth(), event.getHeight());
	return true;
}

bool GameState::onNetworkSerializedPackageEvent(NetworkSerializedPackageEvent& event) {
	m_componentSystems.networkReceiverSystem->pushDataToBuffer(event.getSerializedData());
	return true;
}

bool GameState::onPlayerCandleDeath(PlayerCandleDeathEvent& event) {
	if ( !m_isSingleplayer ) {
		NWrapperSingleton::getInstance().queueGameStateNetworkSenderEvent(
			Netcode::MessageType::PLAYER_DIED,
			m_player
		);
		
		m_player->addComponent<SpectatorComponent>();
		m_player->getComponent<MovementComponent>()->constantAcceleration = glm::vec3(0.f, 0.f, 0.f);
		m_player->removeComponent<GunComponent>();
		m_player->removeAllChildren();
		// TODO: Remove all the components that can/should be removed

	} else {
		this->requestStackPop();
		this->requestStackPush(States::EndGame);
		m_poppedThisFrame = true;
	}

	// Set bot target to null when player is dead
	auto entities = m_componentSystems.aiSystem->getEntities();
	for (int i = 0; i < entities.size(); i++) {
		auto aiComp = entities[i]->getComponent<AiComponent>();
		aiComp->setTarget(nullptr);
	}

	return true;
}

bool GameState::onPlayerDisconnect(NetworkDisconnectEvent& event) {
	// This function is called from NWrapperHost::playerDisconnected()
	// Then the host sends PLAYER_DISCONNECT over the network
	// Each client recieves it in NetworkReceiverSystem::handleEvent()
	// and removes their local copy of the player there
	// Host local copy is destroyed here
	
	if (!m_isSingleplayer) {
		if (NWrapperSingleton::getInstance().isHost()) {	// Should always be true
			
			auto& receiverEntities = m_componentSystems.networkReceiverSystem->getEntities();
			for (auto& e : receiverEntities)
			{
				if (e->getComponent<NetworkSenderComponent>()->m_id >> 18 == event.getPlayerID())
				{
					NWrapperSingleton::getInstance().queueGameStateNetworkSenderEvent(
						Netcode::MessageType::PLAYER_DISCONNECT,
						e
					);

					// This will remove the entity 
					e->removeDeleteAllChildren();
					e->queueDestruction();

					// No loop break. If other entities has the same player id they should all be removed
				}
			}
		}
	}
	return true;
}

bool GameState::update(float dt, float alpha) {
	// UPDATE REAL TIME SYSTEMS
	updatePerFrameComponentSystems(dt, alpha);

	m_lights.updateBufferData();

	return true;
}

bool GameState::fixedUpdate(float dt) {
	std::wstring fpsStr = std::to_wstring(m_app->getFPS());

	m_app->getWindow()->setWindowTitle("Sail | Game Engine Demo | "
		+ Application::getPlatformName() + " | FPS: " + std::to_string(m_app->getFPS()));

	static float counter = 0.0f;
	static float size = 1.0f;
	static float change = 0.4f;

	counter += dt * 2.0f;

#ifdef _PERFORMANCE_TEST
	/* here we shoot the guns */
	for (auto e : m_performanceEntities) {
		auto pos = glm::vec3(m_player->getComponent<TransformComponent>()->getMatrix()[3]);
		auto ePos = e->getComponent<TransformComponent>()->getTranslation();
		ePos.y = ePos.y + 5.f;
		auto dir = ePos - pos;
		auto dirNorm = glm::normalize(dir);
		e->getComponent<GunComponent>()->setFiring(pos + dirNorm * 3.f, glm::vec3(0.f, -1.f, 0.f));
	}
#endif

	updatePerTickComponentSystems(dt);

	return true;
}

// Renders the state
// alpha is a the interpolation value (range [0,1]) between the last two snapshots
bool GameState::render(float dt, float alpha) {
	// Clear back buffer
	m_app->getAPI()->clear({ 0.01f, 0.01f, 0.01f, 1.0f });

	// Draw the scene. Entities with model and trans component will be rendered.
	m_componentSystems.beginEndFrameSystem->beginFrame(m_cam);
	m_componentSystems.modelSubmitSystem->submitAll(alpha);
	m_componentSystems.realTimeModelSubmitSystem->submitAll(alpha);
	m_componentSystems.metaballSubmitSystem->submitAll(alpha);
	m_componentSystems.boundingboxSubmitSystem->submitAll();
	m_componentSystems.beginEndFrameSystem->endFrameAndPresent();

	return true;
}

bool GameState::renderImgui(float dt) {
	// The ImGui window is rendered when activated on F10
	m_profiler.renderWindow();
	m_renderSettingsWindow.renderWindow();
	m_lightDebugWindow.renderWindow();
	m_playerInfoWindow.renderWindow();
	m_componentSystems.renderImGuiSystem->renderImGuiAnimationSettings();

	return false;
}

bool GameState::prepareStateChange() {
	if (m_poppedThisFrame) {
		// Do NOT reset network because we're NOT going to main menu
	}
	return true;
}

void GameState::shutDownGameState() {
	// Show mouse cursor if hidden
	Input::HideCursor(false);

	ECS::Instance()->stopAllSystems();
	ECS::Instance()->destroyAllEntities();
}

// HERE BE DRAGONS
// Make sure things are updated in the correct order or things will behave strangely
void GameState::updatePerTickComponentSystems(float dt) {
	m_currentlyReadingMask = 0;
	m_currentlyWritingMask = 0;
	m_runningSystemJobs.clear();
	m_runningSystems.clear();

	m_componentSystems.prepareUpdateSystem->update(); // HAS TO BE RUN BEFORE OTHER SYSTEMS WHICH USE TRANSFORM
	
	if (!m_isSingleplayer) {
		// Update entities with info from the network
		m_componentSystems.networkReceiverSystem->update();
		// Send out your entity info to the rest of the players
		m_componentSystems.networkSenderSystem->update();
	}
	
	m_componentSystems.movementSystem->update(dt);
	m_componentSystems.speedLimitSystem->update();
	m_componentSystems.collisionSystem->update(dt);
	m_componentSystems.movementPostCollisionSystem->update(dt);


	// This can probably be used once the respective system developers 
	//	have checked their respective systems for proper component registration
	//runSystem(dt, m_componentSystems.physicSystem); // Needs to be updated before boundingboxes etc.

	// TODO: Investigate this
	// Systems sent to runSystem() need to override the update(float dt) in BaseComponentSystem
	runSystem(dt, m_componentSystems.gunSystem); // TODO: Order?
	runSystem(dt, m_componentSystems.projectileSystem);
	runSystem(dt, m_componentSystems.animationSystem);
	runSystem(dt, m_componentSystems.aiSystem);
	runSystem(dt, m_componentSystems.candleSystem);
	runSystem(dt, m_componentSystems.updateBoundingBoxSystem);
	runSystem(dt, m_componentSystems.lifeTimeSystem);

	// Wait for all the systems to finish before starting the removal system
	for (auto& fut : m_runningSystemJobs) {
		fut.get();
	}

	// Will probably need to be called last
	m_componentSystems.entityAdderSystem->update();
	m_componentSystems.entityRemovalSystem->update();
	m_componentSystems.octreeAddRemoverSystem->update(dt);
}

void GameState::updatePerFrameComponentSystems(float dt, float alpha) {
	// TODO? move to its own thread

	NWrapperSingleton* ptr = &NWrapperSingleton::getInstance();
	NWrapperSingleton::getInstance().getNetworkWrapper()->checkForPackages();

	// Updates the camera
	m_componentSystems.gameInputSystem->update(dt, alpha);


	// There is an imgui debug toggle to override lights
	if (!m_lightDebugWindow.isManualOverrideOn()) {
		m_lights.clearPointLights();
		//check and update all lights for all entities
		m_componentSystems.lightSystem->updateLights(&m_lights);
		m_componentSystems.lightListSystem->updateLights(&m_lights);
	}

	if (m_showcaseProcGen) {
		m_cam.setPosition(glm::vec3(100.f, 100.f, 100.f));
	}
	m_componentSystems.animationSystem->updatePerFrame();
	m_componentSystems.audioSystem->update(m_cam, dt, alpha);
	m_componentSystems.octreeAddRemoverSystem->updatePerFrame(dt);
}

void GameState::runSystem(float dt, BaseComponentSystem* toRun) {
	bool started = false;
	while (!started) {
		// First check if the system can be run
		if (!(m_currentlyReadingMask & toRun->getWriteBitMask()).any() &&
			!(m_currentlyWritingMask & toRun->getReadBitMask()).any() &&
			!(m_currentlyWritingMask & toRun->getWriteBitMask()).any()) {

			m_currentlyWritingMask |= toRun->getWriteBitMask();
			m_currentlyReadingMask |= toRun->getReadBitMask();
			started = true;
			m_runningSystems.push_back(toRun);
			m_runningSystemJobs.push_back(m_app->pushJobToThreadPool([this, dt, toRun](int id) {toRun->update(dt); return toRun; }));

		} else {
			// Then loop through all futures and see if any of them are done
			for (int i = 0; i < m_runningSystemJobs.size(); i++) {
				if (m_runningSystemJobs[i].wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
					auto doneSys = m_runningSystemJobs[i].get();

					m_runningSystemJobs.erase(m_runningSystemJobs.begin() + i);
					i--;

					m_currentlyWritingMask ^= doneSys->getWriteBitMask();
					m_currentlyReadingMask ^= doneSys->getReadBitMask();

					int toRemoveIndex = -1;
					for (int j = 0; j < m_runningSystems.size(); j++) {
						// Currently just compares memory adresses (if they point to the same location they're the same object)
						if (m_runningSystems[j] == doneSys)
							toRemoveIndex = j;
					}

					m_runningSystems.erase(m_runningSystems.begin() + toRemoveIndex);

					// Since multiple systems can read from components concurrently, currently best solution I came up with
					for (auto _sys : m_runningSystems) {
						m_currentlyReadingMask |= _sys->getReadBitMask();
					}
				}
			}
		}
	}
}

const std::string GameState::teleportToMap() {
	m_player->getComponent<TransformComponent>()->setStartTranslation(glm::vec3(30.6f, 0.9f, 40.f));
	return "";
}

const std::string GameState::toggleProfiler() {
	m_profiler.toggleWindow();
	return "Toggling profiler";
}

const std::string GameState::createCube(const glm::vec3& position) {

	Model* tmpCubeModel = &m_app->getResourceManager().getModel(
		"cubeWidth1.fbx", &m_app->getResourceManager().getShaderSet<GBufferOutShader>());
	tmpCubeModel->getMesh(0)->getMaterial()->setColor(glm::vec4(0.2f, 0.8f, 0.4f, 1.0f));

	Model* tmpbbModel = &m_app->getResourceManager().getModel(
		"boundingBox.fbx", &m_app->getResourceManager().getShaderSet<GBufferWireframe>());
	tmpCubeModel->getMesh(0)->getMaterial()->setColor(glm::vec4(0.2f, 0.8f, 0.4f, 1.0f));

	auto e = ECS::Instance()->createEntity("new cube");
	e->addComponent<ModelComponent>(tmpCubeModel);

	e->addComponent<TransformComponent>(position);

	e->addComponent<BoundingBoxComponent>(tmpbbModel);

	e->addComponent<CollidableComponent>();

	return std::string("Added Cube at (" +
		std::to_string(position.x) + ":" +
		std::to_string(position.y) + ":" +
		std::to_string(position.z) + ")");
}

void GameState::createTestLevel(Shader* shader, Model* boundingBoxModel) {
	// Load models used for test level
	Model* arenaModel = &m_app->getResourceManager().getModel("arenaBasic.fbx", shader);
	arenaModel->getMesh(0)->getMaterial()->setAlbedoTexture("sponza/textures/arenaBasicTexture.tga");
	arenaModel->getMesh(0)->getMaterial()->setMetalnessScale(0.570f);
	arenaModel->getMesh(0)->getMaterial()->setRoughnessScale(0.593f);
	arenaModel->getMesh(0)->getMaterial()->setAOScale(0.023f);

	Model* barrierModel = &m_app->getResourceManager().getModel("barrierBasic.fbx", shader);
	barrierModel->getMesh(0)->getMaterial()->setAlbedoTexture("sponza/textures/barrierBasicTexture.tga");

	Model* containerModel = &m_app->getResourceManager().getModel("containerBasic.fbx", shader);
	containerModel->getMesh(0)->getMaterial()->setMetalnessScale(0.778f);
	containerModel->getMesh(0)->getMaterial()->setRoughnessScale(0.394f);
	containerModel->getMesh(0)->getMaterial()->setAOScale(0.036f);
	containerModel->getMesh(0)->getMaterial()->setAlbedoTexture("sponza/textures/containerBasicTexture.tga");

	Model* rampModel = &m_app->getResourceManager().getModel("rampBasic.fbx", shader);
	rampModel->getMesh(0)->getMaterial()->setAlbedoTexture("sponza/textures/rampBasicTexture.tga");
	rampModel->getMesh(0)->getMaterial()->setMetalnessScale(0.0f);
	rampModel->getMesh(0)->getMaterial()->setRoughnessScale(1.0f);
	rampModel->getMesh(0)->getMaterial()->setAOScale(1.0f);

	// Create entities for test level
	
	auto e = EntityFactory::CreateStaticMapObject("Arena", arenaModel, boundingBoxModel, glm::vec3(0.0f , 0.0f, 0.0f));
	e = EntityFactory::CreateStaticMapObject("Map_Barrier1", barrierModel, boundingBoxModel, glm::vec3(-16.15f * 0.3f, 0.f, 3.83f * 0.3f), glm::vec3(0.f, -0.79f, 0.f));
	e = EntityFactory::CreateStaticMapObject("Map_Barrier2", barrierModel, boundingBoxModel, glm::vec3(-4.54f * 0.3f, 0.f, 8.06f * 0.3f));
	e = EntityFactory::CreateStaticMapObject("Map_Barrier3", barrierModel, boundingBoxModel, glm::vec3(8.46f * 0.3f, 0.f, 8.06f * 0.3f));
	e = EntityFactory::CreateStaticMapObject("Map_Container1", containerModel, boundingBoxModel, glm::vec3(6.95f * 0.3f, 0.f, 25.f * 0.3f));
	e = EntityFactory::CreateStaticMapObject("Map_Container2", containerModel, boundingBoxModel, glm::vec3(-25.f * 0.3f, 0.f, 12.43f * 0.3f), glm::vec3(0.f, 1.57f, 0.f));
	e = EntityFactory::CreateStaticMapObject("Map_Container3", containerModel, boundingBoxModel, glm::vec3(-25.f * 0.3f, 2.4f, -7.73f * 0.3f), glm::vec3(0.f, 1.57f, 0.f));
	e = EntityFactory::CreateStaticMapObject("Map_Container4", containerModel, boundingBoxModel, glm::vec3(-19.67f * 0.3f, 0.f, -24.83f * 0.3f), glm::vec3(0.f, 0.79f, 0.f));
	e = EntityFactory::CreateStaticMapObject("Map_Container5", containerModel, boundingBoxModel, glm::vec3(-0.f, 0.f, -14.f * 0.3f));
	e = EntityFactory::CreateStaticMapObject("Map_Container6", containerModel, boundingBoxModel, glm::vec3(24.20f * 0.3f, 0.f, -8.f * 0.3f), glm::vec3(0.f, 1.57f, 0.f));
	e = EntityFactory::CreateStaticMapObject("Map_Container7", containerModel, boundingBoxModel, glm::vec3(24.2f * 0.3f, 2.4f, -22.8f * 0.3f), glm::vec3(0.f, 1.57f, 0.f));
	e = EntityFactory::CreateStaticMapObject("Map_Container8", containerModel, boundingBoxModel, glm::vec3(24.36f * 0.3f, 0.f, -32.41f * 0.3f));
	e = EntityFactory::CreateStaticMapObject("Map_Ramp1", rampModel, boundingBoxModel, glm::vec3(5.2f * 0.3f, 0.f, -32.25f * 0.3f), glm::vec3(0.f, 3.14f, 0.f));
	e = EntityFactory::CreateStaticMapObject("Map_Ramp2", rampModel, boundingBoxModel, glm::vec3(15.2f * 0.3f, 2.4f, -32.25f * 0.3f), glm::vec3(0.f, 3.14f, 0.f));
	e = EntityFactory::CreateStaticMapObject("Map_Ramp3", rampModel, boundingBoxModel, glm::vec3(24.f * 0.3f, 2.4f, -5.5f * 0.3f), glm::vec3(0.f, 4.71f, 0.f));
	e = EntityFactory::CreateStaticMapObject("Map_Ramp4", rampModel, boundingBoxModel, glm::vec3(24.f * 0.3f, 0.f, 9.f * 0.3f), glm::vec3(0.f, 4.71f, 0.f));
	e = EntityFactory::CreateStaticMapObject("Map_Ramp5", rampModel, boundingBoxModel, glm::vec3(-16.f * 0.3f, 0.f, 20.f * 0.3f), glm::vec3(0.f, 0.f, 0.f));
	e = EntityFactory::CreateStaticMapObject("Map_Ramp6", rampModel, boundingBoxModel, glm::vec3(-34.f * 0.3f, 0.f, 20.f * 0.3f), glm::vec3(0.f, 3.14f, 0.f));
}

void GameState::createBots(Model* boundingBoxModel, Model* characterModel, Model* projectileModel, Model* lightModel) {
	int botCount = m_app->getStateStorage().getLobbyToGameData()->botCount;

	if (botCount < 0) {
		botCount = 0;
	}

	for (size_t i = 0; i < botCount; i++) {
		glm::vec3 spawnLocation = m_componentSystems.levelGeneratorSystem->getSpawnPoint();
		if (spawnLocation.x != -1000.f) {
			auto e = EntityFactory::CreateBot(boundingBoxModel, characterModel, spawnLocation, lightModel, m_currLightIndex++, m_componentSystems.aiSystem->getNodeSystem());
		}
		else {
			Logger::Error("Bot not spawned because all spawn points are already used for this map.");
		}
	}
}

void GameState::createLevel(Shader* shader, Model* boundingBoxModel) {
	std::string tileTex = "sponza/textures/tileTexture1.tga";
	Application::getInstance()->getResourceManager().loadTexture(tileTex);

	Application::getInstance()->getResourceManager().loadTexture("pbr/Tiles/RoomWallMRAO.tga");
	Application::getInstance()->getResourceManager().loadTexture("pbr/Tiles/RoomWallNM.tga");
	Application::getInstance()->getResourceManager().loadTexture("pbr/Tiles/RoomWallAlbedo.tga");

	Application::getInstance()->getResourceManager().loadTexture("pbr/Tiles/RD_MRAo.tga");
	Application::getInstance()->getResourceManager().loadTexture("pbr/Tiles/RD_NM.tga");
	Application::getInstance()->getResourceManager().loadTexture("pbr/Tiles/RD_Albedo.tga");

	Application::getInstance()->getResourceManager().loadTexture("pbr/Tiles/CD_MRAo.tga");
	Application::getInstance()->getResourceManager().loadTexture("pbr/Tiles/CD_NM.tga");
	Application::getInstance()->getResourceManager().loadTexture("pbr/Tiles/CD_Albedo.tga");

	Application::getInstance()->getResourceManager().loadTexture("pbr/Tiles/CW_MRAo.tga");
	Application::getInstance()->getResourceManager().loadTexture("pbr/Tiles/CW_NM.tga");
	Application::getInstance()->getResourceManager().loadTexture("pbr/Tiles/CW_Albedo.tga");

	Application::getInstance()->getResourceManager().loadTexture("pbr/Tiles/F_MRAo.tga");
	Application::getInstance()->getResourceManager().loadTexture("pbr/Tiles/F_NM.tga");
	Application::getInstance()->getResourceManager().loadTexture("pbr/Tiles/F_Albedo.tga");

	Application::getInstance()->getResourceManager().loadTexture("pbr/Tiles/CF_MRAo.tga");
	Application::getInstance()->getResourceManager().loadTexture("pbr/Tiles/CF_NM.tga");
	Application::getInstance()->getResourceManager().loadTexture("pbr/Tiles/CF_Albedo.tga");

	Application::getInstance()->getResourceManager().loadTexture("pbr/Tiles/CC_MRAo.tga");
	Application::getInstance()->getResourceManager().loadTexture("pbr/Tiles/CC_NM.tga");
	Application::getInstance()->getResourceManager().loadTexture("pbr/Tiles/CC_Albedo.tga");

	Application::getInstance()->getResourceManager().loadTexture("pbr/Tiles/RC_MRAo.tga");
	Application::getInstance()->getResourceManager().loadTexture("pbr/Tiles/RC_NM.tga");
	Application::getInstance()->getResourceManager().loadTexture("pbr/Tiles/RC_Albedo.tga");

	Application::getInstance()->getResourceManager().loadTexture("pbr/Tiles/Corner_MRAo.tga");
	Application::getInstance()->getResourceManager().loadTexture("pbr/Tiles/Corner_NM.tga");
	Application::getInstance()->getResourceManager().loadTexture("pbr/Tiles/Corner_Albedo.tga");

	Application::getInstance()->getResourceManager().loadTexture("pbr/metal/metalnessRoughnessAO.tga");
	Application::getInstance()->getResourceManager().loadTexture("pbr/metal/normal.tga");
	Application::getInstance()->getResourceManager().loadTexture("pbr/metal/albedo.tga");



	//Load tileset for world
	Model* tileFlat = &m_app->getResourceManager().getModel("Tiles/tileFlat.fbx", shader);
	tileFlat->getMesh(0)->getMaterial()->setAlbedoTexture(tileTex);

	Model* roomWall = &m_app->getResourceManager().getModel("Tiles/RoomWall.fbx", shader);
	roomWall->getMesh(0)->getMaterial()->setMetalnessRoughnessAOTexture("pbr/Tiles/RoomWallMRAO.tga");
	roomWall->getMesh(0)->getMaterial()->setNormalTexture("pbr/Tiles/RoomWallNM.tga");
	roomWall->getMesh(0)->getMaterial()->setAlbedoTexture("pbr/Tiles/RoomWallAlbedo.tga");

	Model* tileDoor = &m_app->getResourceManager().getModel("Tiles/tileDoor.fbx", shader);
	tileDoor->getMesh(0)->getMaterial()->setAlbedoTexture(tileTex);

	Model* roomDoor = &m_app->getResourceManager().getModel("Tiles/RoomDoor.fbx", shader);
	roomDoor->getMesh(0)->getMaterial()->setMetalnessRoughnessAOTexture("pbr/Tiles/RD_MRAo.tga");
	roomDoor->getMesh(0)->getMaterial()->setNormalTexture("pbr/Tiles/RD_NM.tga");
	roomDoor->getMesh(0)->getMaterial()->setAlbedoTexture("pbr/Tiles/RD_Albedo.tga");


	Model* corridorDoor = &m_app->getResourceManager().getModel("Tiles/CorridorDoor.fbx", shader);
	corridorDoor->getMesh(0)->getMaterial()->setMetalnessRoughnessAOTexture("pbr/Tiles/CD_MRAo.tga");
	corridorDoor->getMesh(0)->getMaterial()->setNormalTexture("pbr/Tiles/CD_NM.tga");
	corridorDoor->getMesh(0)->getMaterial()->setAlbedoTexture("pbr/Tiles/CD_Albedo.tga");

	Model* corridorWall = &m_app->getResourceManager().getModel("Tiles/CorridorWall.fbx", shader);
	corridorWall->getMesh(0)->getMaterial()->setMetalnessRoughnessAOTexture("pbr/Tiles/CW_MRAo.tga");
	corridorWall->getMesh(0)->getMaterial()->setNormalTexture("pbr/Tiles/CW_NM.tga");
	corridorWall->getMesh(0)->getMaterial()->setAlbedoTexture("pbr/Tiles/CW_Albedo.tga");

	Model* roomCeiling = &m_app->getResourceManager().getModel("Tiles/RoomCeiling.fbx", shader);
	roomCeiling->getMesh(0)->getMaterial()->setMetalnessRoughnessAOTexture("pbr/Tiles/RC_MRAo.tga");
	roomCeiling->getMesh(0)->getMaterial()->setNormalTexture("pbr/Tiles/RC_NM.tga");
	roomCeiling->getMesh(0)->getMaterial()->setAlbedoTexture("pbr/Tiles/RC_Albedo.tga");


	Model* corridorFloor = &m_app->getResourceManager().getModel("Tiles/CorridorFloor.fbx", shader);
	corridorFloor->getMesh(0)->getMaterial()->setMetalnessRoughnessAOTexture("pbr/Tiles/CF_MRAo.tga");
	corridorFloor->getMesh(0)->getMaterial()->setNormalTexture("pbr/Tiles/CF_NM.tga");
	corridorFloor->getMesh(0)->getMaterial()->setAlbedoTexture("pbr/Tiles/CF_Albedo.tga");

	Model* roomFloor = &m_app->getResourceManager().getModel("Tiles/RoomFloor.fbx", shader);
	roomFloor->getMesh(0)->getMaterial()->setMetalnessRoughnessAOTexture("pbr/Tiles/F_MRAo.tga");
	roomFloor->getMesh(0)->getMaterial()->setNormalTexture("pbr/Tiles/F_NM.tga");
	roomFloor->getMesh(0)->getMaterial()->setAlbedoTexture("pbr/Tiles/F_Albedo.tga");

	Model* corridorCeiling = &m_app->getResourceManager().getModel("Tiles/CorridorCeiling.fbx", shader);
	corridorCeiling->getMesh(0)->getMaterial()->setMetalnessRoughnessAOTexture("pbr/Tiles/CC_MRAo.tga");
	corridorCeiling->getMesh(0)->getMaterial()->setNormalTexture("pbr/Tiles/CC_NM.tga");
	corridorCeiling->getMesh(0)->getMaterial()->setAlbedoTexture("pbr/Tiles/CC_Albedo.tga");

	Model* corridorCorner = &m_app->getResourceManager().getModel("Tiles/CorridorCorner.fbx", shader);
	corridorCorner->getMesh(0)->getMaterial()->setMetalnessRoughnessAOTexture("pbr/Tiles/Corner_MRAo.tga");
	corridorCorner->getMesh(0)->getMaterial()->setNormalTexture("pbr/Tiles/Corner_NM.tga");
	corridorCorner->getMesh(0)->getMaterial()->setAlbedoTexture("pbr/Tiles/Corner_Albedo.tga");

	Model* roomCorner = &m_app->getResourceManager().getModel("Tiles/RoomCorner.fbx", shader);
	roomCorner->getMesh(0)->getMaterial()->setMetalnessRoughnessAOTexture("pbr/Tiles/Corner_MRAo.tga");
	roomCorner->getMesh(0)->getMaterial()->setNormalTexture("pbr/Tiles/Corner_NM.tga");
	roomCorner->getMesh(0)->getMaterial()->setAlbedoTexture("pbr/Tiles/Corner_Albedo.tga");

	std::vector<Model*> tileModels;
	tileModels.resize(TileModel::NUMBOFMODELS);
	tileModels[TileModel::ROOM_FLOOR] = roomFloor;
	tileModels[TileModel::ROOM_WALL] = roomWall;
	tileModels[TileModel::ROOM_DOOR] = roomDoor;
	tileModels[TileModel::ROOM_CEILING] = roomCeiling;
	tileModels[TileModel::ROOM_CORNER] = roomCorner;

	tileModels[TileModel::CORRIDOR_FLOOR] = corridorFloor;
	tileModels[TileModel::CORRIDOR_WALL] = corridorWall;
	tileModels[TileModel::CORRIDOR_DOOR] = corridorDoor;
	tileModels[TileModel::CORRIDOR_CEILING] = corridorCeiling;
	tileModels[TileModel::CORRIDOR_CORNER] = corridorCorner;


	// Create the level generator system and put it into the datatype.
	auto map = ECS::Instance()->createEntity("Map");
	map->addComponent<MapComponent>();
	ECS::Instance()->addAllQueuedEntities();
	m_componentSystems.levelGeneratorSystem->generateMap();
	m_componentSystems.levelGeneratorSystem->createWorld(tileModels, boundingBoxModel);
}

#ifdef _PERFORMANCE_TEST
void GameState::populateScene(Model* characterModel, Model* lightModel, Model* bbModel, Model* projectileModel, Shader* shader) {
	/* 13 characters that are constantly shooting their guns */
	for (int i = 0; i < 13; i++) {
		float spawnOffsetX = -19.f + float(i) * 2.f;
		float spawnOffsetZ = 13.f + float(i) * 1.3f;
		auto e = ECS::Instance()->createEntity("Performance Test Entity " + std::to_string(i));

		std::string name = "DocGunRun4.fbx";
		Model* characterModel = &m_app->getResourceManager().getModelCopy(name, shader);
		characterModel->getMesh(0)->getMaterial()->setMetalnessRoughnessAOTexture("pbr/Character/CharacterMRAO.tga");
		characterModel->getMesh(0)->getMaterial()->setAlbedoTexture("pbr/Character/CharacterTex.tga");
		characterModel->getMesh(0)->getMaterial()->setNormalTexture("pbr/Character/CharacterNM.tga");
		characterModel->setIsAnimated(true);

		e->addComponent<ModelComponent>(characterModel);
		auto animStack = &m_app->getResourceManager().getAnimationStack(name);
		auto animComp = e->addComponent<AnimationComponent>(animStack);
		animComp->currentAnimation = animStack->getAnimation(1);
		animComp->animationTime = float(i) / animComp->currentAnimation->getMaxAnimationTime();
		e->addComponent<TransformComponent>(glm::vec3(105.543f + spawnOffsetX, 0.f, 99.5343f + spawnOffsetZ), glm::vec3(0.f, 0.f, 0.f));
		e->addComponent<BoundingBoxComponent>(bbModel)->getBoundingBox()->setHalfSize(glm::vec3(0.7f, .9f, 0.7f));
		e->addComponent<CollidableComponent>();
		e->addComponent<MovementComponent>();
		e->addComponent<SpeedLimitComponent>();
		e->addComponent<CollisionComponent>();
		e->addComponent<GunComponent>(projectileModel, bbModel);

		/* Audio */
		e->addComponent<AudioComponent>();
		Audio::SoundInfo sound{};
		sound.fileName = "../Audio/guitar.wav";
		sound.soundEffectLength = 104.0f;
		sound.volume = 1.0f;
		sound.playOnce = false;
		sound.positionalOffset = { 0.f, 1.2f, 0.f };
		sound.isPlaying = true; // Start playing the sound immediately
		e->getComponent<AudioComponent>()->defineSound(Audio::SoundType::AMBIENT, sound);

		// Add candle
		/*if (i != 12) {
			auto candleEntity = createCandleEntity("Candle Entity " + std::to_string(i), lightModel, bbModel, glm::vec3(0.f, 10.f, 0.f));
			candleEntity->getComponent<CandleComponent>()->setOwner(e->getID());
			e->addChildEntity(candleEntity);
		}*/

		/* Movement */
		e->getComponent<MovementComponent>()->constantAcceleration = glm::vec3(0.0f, -9.8f, 0.0f);
		e->getComponent<SpeedLimitComponent>()->maxSpeed = 6.f;

		m_performanceEntities.push_back(e);
	}
}
#endif
