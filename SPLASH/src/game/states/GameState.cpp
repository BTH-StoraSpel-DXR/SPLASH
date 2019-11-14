#include "GameState.h"
#include "imgui.h"
#include "Sail/entities/ECS.h"
#include "Sail/entities/components/Components.h"
#include "Sail/entities/systems/Systems.h"
#include "Sail/ai/states/AttackingState.h"
#include "Sail/graphics/shader/compute/AnimationUpdateComputeShader.h"
#include "Sail/graphics/shader/compute/ParticleComputeShader.h"
#include "Sail/graphics/shader/postprocess/BlendShader.h"
#include "Sail/graphics/shader/postprocess/GaussianBlurHorizontal.h"
#include "Sail/graphics/shader/postprocess/GaussianBlurVertical.h"
#include "Sail/TimeSettings.h"
#include "Sail/utils/GameDataTracker.h"
#include "Sail/events/EventDispatcher.h"
#include "Network/NWrapperSingleton.h"
#include "Sail/utils/GUISettings.h"
#include "Sail/graphics/geometry/factory/QuadModel.h"
#include <sstream>
#include <iomanip>
#include "InGameMenuState.h"
#include "../SPLASH/src/game/events/ResetWaterEvent.h"
#include "API/DX12/DX12API.h"

GameState::GameState(StateStack& stack)
	: State(stack)
	, m_cam(90.f, 1280.f / 720.f, 0.1f, 5000.f)
	, m_profiler(true)
	, m_showcaseProcGen(false) 
{
	EventDispatcher::Instance().subscribe(Event::Type::WINDOW_RESIZE, this);
	EventDispatcher::Instance().subscribe(Event::Type::NETWORK_SERIALIZED_DATA_RECIEVED, this);
	EventDispatcher::Instance().subscribe(Event::Type::NETWORK_DISCONNECT, this);
	EventDispatcher::Instance().subscribe(Event::Type::NETWORK_DROPPED, this);


	initConsole();

	// Get the Application instance
	m_app = Application::getInstance();
	m_isSingleplayer = NWrapperSingleton::getInstance().getPlayers().size() == 1;


	std::vector<glm::vec3> m_teamColors;
	for (int i = 0; i < 12; i++) {
		m_teamColors.push_back(TeamColorSystem::getTeamColor(i));
	}
	m_app->getRenderWrapper()->getCurrentRenderer()->setTeamColors(m_teamColors);

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
	m_cam.setDirection(glm::normalize(glm::vec3(0.48f, -0.16f, 0.85f)));
#endif

	// Initialize the component systems
	initSystems(playerID);

	// Textures needs to be loaded before they can be used
	Application::getInstance()->getResourceManager().loadTexture("pbr/Character/CharacterTex.tga");
	Application::getInstance()->getResourceManager().loadTexture("pbr/Character/CharacterMRAO.tga");
	Application::getInstance()->getResourceManager().loadTexture("pbr/Character/CharacterNM.tga");

	Application::getInstance()->getResourceManager().loadTexture("pbr/WaterGun/Watergun_Albedo.tga");
	Application::getInstance()->getResourceManager().loadTexture("pbr/WaterGun/Watergun_MRAO.tga");
	Application::getInstance()->getResourceManager().loadTexture("pbr/WaterGun/Watergun_NM.tga");

	// Font sprite map texture
	Application::getInstance()->getResourceManager().loadTexture(GUIText::fontTexture);

	// Add a directional light which is used in forward rendering
	glm::vec3 color(0.0f, 0.0f, 0.0f);
	glm::vec3 direction(0.4f, -0.2f, 1.0f);
	direction = glm::normalize(direction);
	m_lights.setDirectionalLight(DirectionalLight(color, direction));
	m_app->getRenderWrapper()->getCurrentRenderer()->setLightSetup(&m_lights);

	m_lightDebugWindow.setLightSetup(&m_lights);

	// Disable culling for testing purposes
	m_app->getAPI()->setFaceCulling(GraphicsAPI::NO_CULLING);

	auto* shader = &m_app->getResourceManager().getShaderSet<GBufferOutShader>();

	m_app->getResourceManager().setDefaultShader(shader);
	std::string playerModelName = "Doc.fbx";


	// Create/load models
	Model* cubeModel = &m_app->getResourceManager().getModel("cubeWidth1.fbx", shader);
	cubeModel->getMesh(0)->getMaterial()->setColor(glm::vec4(0.2f, 0.8f, 0.4f, 1.0f));

	Model* lightModel = &m_app->getResourceManager().getModel("Torch.fbx", shader);
	lightModel->getMesh(0)->getMaterial()->setAlbedoTexture("pbr/Torch/Torch_Albedo.tga");
	lightModel->getMesh(0)->getMaterial()->setNormalTexture("pbr/Torch/Torch_NM.tga");
	lightModel->getMesh(0)->getMaterial()->setMetalnessRoughnessAOTexture("pbr/Torch/Torch_MRAO.tga");


#ifdef DEVELOPMENT
	/* GUI testing */

	//EntityFactory::CreateScreenSpaceText("HELLO", glm::vec2(0.8f, 0.9f), glm::vec2(0.4f, 0.2f));
	/* /GUI testing */
#endif

	// Crosshair
//	EntityFactory::CreateGUIEntity("crosshairEntity", "crosshair.tga", glm::vec2(0.f, 0.f), glm::vec2(0.005f, 0.00888f));
	EntityFactory::CreateCrosshairEntity(
		"crosshairEntity",				// name
		"crosshair.tga",				// normal model
		"crosshair.tga",				// altered model (activated on hit)
		glm::vec2(0.f, 0.f),			// origin
		glm::vec2(0.005f, 0.00888f)		// size
	);

	// Level Creation

	createLevel(shader, boundingBoxModel);

	// Player creation

	int id = static_cast<int>(playerID);
	glm::vec3 spawnLocation = glm::vec3(0.f);
	for (int i = -1; i < id; i++) {

		spawnLocation = m_componentSystems.levelSystem->getSpawnPoint();
	}

	SAIL_LOG(std::to_string(spawnLocation.x));
	SAIL_LOG(std::to_string(spawnLocation.y));
	SAIL_LOG(std::to_string(spawnLocation.z));

	m_player = EntityFactory::CreateMyPlayer(playerID, m_currLightIndex++, spawnLocation).get();

	m_componentSystems.networkReceiverSystem->setPlayer(m_player);
	m_componentSystems.networkReceiverSystem->setGameState(this);

	// Bots creation
	createBots(boundingBoxModel, playerModelName, cubeModel, lightModel);

#ifdef _PERFORMANCE_TEST
	populateScene(lightModel, boundingBoxModel, boundingBoxModel, shader);
	m_player->getComponent<TransformComponent>()->setStartTranslation(glm::vec3(54.f, 1.6f, 59.f));

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

	m_ambiance = ECS::Instance()->createEntity("LabAmbiance").get();
	m_ambiance->addComponent<AudioComponent>();
	m_ambiance->addComponent<TransformComponent>(glm::vec3{ 0.0f, 0.0f, 0.0f });
	m_ambiance->getComponent<AudioComponent>()->streamSoundRequest_HELPERFUNC("res/sounds/ambient/ambiance_lab.xwb", true, 1.0f, false, true);

	m_playerInfoWindow.setPlayerInfo(m_player, &m_cam);

	// Host fill its game tracker per player with player data.
	// Reset data trackers
	GameDataTracker::getInstance().init();

	// Clear all water on the level
	EventDispatcher::Instance().emit(ResetWaterEvent());
}

GameState::~GameState() {
	Application::getInstance()->getConsole().removeAllCommandsWithIdentifier("GameState");
	shutDownGameState();
	delete m_octree;

	EventDispatcher::Instance().unsubscribe(Event::Type::WINDOW_RESIZE, this);
	EventDispatcher::Instance().unsubscribe(Event::Type::NETWORK_SERIALIZED_DATA_RECIEVED, this);
	EventDispatcher::Instance().unsubscribe(Event::Type::NETWORK_DISCONNECT, this);
	EventDispatcher::Instance().unsubscribe(Event::Type::NETWORK_DROPPED, this);
}

// Process input for the state
// NOTE: Done every frame
bool GameState::processInput(float dt) {

#ifndef DEVELOPMENT
	//Capture mouse
	Input::HideCursor(true);		//Shreks multiple applications on the same computer
#endif

	// Pause game
	if (!InGameMenuState::IsOpen() && Input::WasKeyJustPressed(KeyBinds::SHOW_IN_GAME_MENU)) {
		requestStackPush(States::InGameMenu);
	}

#ifdef DEVELOPMENT
#ifdef _DEBUG
	// Add point light at camera pos
	if (Input::WasKeyJustPressed(KeyBinds::ADD_LIGHT)) {
		m_componentSystems.lightListSystem->addPointLightToDebugEntity(&m_lights, &m_cam);
	}


#endif

	// Enable bright light and move camera to above procedural generated level
	if (Input::WasKeyJustPressed(KeyBinds::TOGGLE_SUN)) {
		m_lightDebugWindow.setManualOverride(!m_lightDebugWindow.isManualOverrideOn());
		m_showcaseProcGen = m_lightDebugWindow.isManualOverrideOn();
		if (m_showcaseProcGen) {
			m_lights.getPLs()[0].setPosition(glm::vec3(100.f, 20.f, 100.f));
			m_lights.getPLs()[0].setAttenuation(0.2f, 0.f, 0.f);
		} else {
			m_cam.setPosition(glm::vec3(0.f, 1.f, 0.f));
		}
	}


	if (Input::WasKeyJustPressed(KeyBinds::TOGGLE_ROOM_LIGHTS)) {
		m_componentSystems.spotLightSystem->toggleONOFF();
	}


	// Show boudning boxes
	if (Input::WasKeyJustPressed(KeyBinds::TOGGLE_BOUNDINGBOXES)) {
		m_componentSystems.boundingboxSubmitSystem->toggleHitboxes();
	}

	//Test ray intersection
	if (Input::IsKeyPressed(KeyBinds::TEST_RAYINTERSECTION)) {
		Octree::RayIntersectionInfo tempInfo;
		m_octree->getRayIntersection(m_cam.getPosition(), m_cam.getDirection(), &tempInfo);
		if (tempInfo.closestHitIndex != -1) {
			SAIL_LOG("Ray intersection with " + tempInfo.info[tempInfo.closestHitIndex].entity->getName() + ", " + std::to_string(tempInfo.closestHit) + " meters away");
		}
	}

	if (Input::WasKeyJustPressed(KeyBinds::SPRAY)) {
		Octree::RayIntersectionInfo tempInfo;
		m_octree->getRayIntersection(m_cam.getPosition(), m_cam.getDirection(), &tempInfo);
		if (tempInfo.closestHit >= 0.0f) {
			// size (the size you want) = 0.3
			// halfSize = (1 / 0.3) * 0.5 = 1.667
			m_app->getRenderWrapper()->getCurrentRenderer()->submitDecal(m_cam.getPosition() + m_cam.getDirection() * tempInfo.closestHit, glm::identity<glm::mat4>(), glm::vec3(1.667f));
		}
	}

	//Test frustum culling
	if (Input::IsKeyPressed(KeyBinds::TEST_FRUSTUMCULLING)) {
		int nrOfDraws = m_octree->frustumCulledDraw(m_cam);
		SAIL_LOG("Number of draws " + std::to_string(nrOfDraws));
	}

	// TODO: Move this to a system
	// Toggle ai following the player
	if (Input::WasKeyJustPressed(KeyBinds::TOGGLE_AI_FOLLOWING)) {
		auto entities = m_componentSystems.aiSystem->getEntities();
		for (int i = 0; i < entities.size(); i++) {
			auto aiComp = entities[i]->getComponent<AiComponent>();
			if (aiComp->entityTarget == nullptr) {

				// Find the candle child entity of player
				Entity* candle = nullptr;
				std::vector<Entity*> children = m_player->getChildEntities();
				for (auto& child : children) {
					if (child->hasComponent<CandleComponent>()) {
						candle = child;
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
	if (Input::IsKeyPressed(KeyBinds::SET_DIRECTIONAL_LIGHT)) {
		glm::vec3 color(1.0f, 1.0f, 1.0f);
		m_lights.setDirectionalLight(DirectionalLight(color, m_cam.getDirection()));
	}

	// Reload shaders
	if (Input::WasKeyJustPressed(KeyBinds::RELOAD_SHADER)) {
		m_app->getAPI<DX12API>()->waitForGPU();
		m_app->getResourceManager().reloadShader<BlendShader>();
		m_app->getResourceManager().reloadShader<GaussianBlurHorizontal>();
		m_app->getResourceManager().reloadShader<GaussianBlurVertical>();
		m_app->getResourceManager().reloadShader<AnimationUpdateComputeShader>();
		m_app->getResourceManager().reloadShader<ParticleComputeShader>();
		m_app->getResourceManager().reloadShader<GBufferOutShader>();
		m_app->getResourceManager().reloadShader<GuiShader>();
	}

	if (Input::WasKeyJustPressed(KeyBinds::TOGGLE_SPHERE)) {
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

	if (Input::WasKeyJustPressed(KeyBinds::SPECTATOR_DEBUG)) {
		// Get position and rotation to look at middle of the map from above
		{
			auto parTrans = m_player->getComponent<TransformComponent>();
			auto pos = glm::vec3(parTrans->getMatrixWithUpdate()[3]);
			pos.y = 20.f;
			parTrans->setTranslation(pos);
			

			auto& mapSettings = Application::getInstance()->getSettings().gameSettingsDynamic["map"];

			auto middleOfLevel = glm::vec3(mapSettings["tileSize"].value * mapSettings["sizeX"].value / 2.f, 0.f, mapSettings["tileSize"].value * mapSettings["sizeY"].value / 2.f);
			auto dir = glm::normalize(middleOfLevel - pos);
			auto rots = Utils::getRotations(dir);
			parTrans->setRotations(glm::vec3(0.f, -rots.y, rots.x));
			m_player->addComponent<SpectatorComponent>();
			m_player->getComponent<MovementComponent>()->constantAcceleration = glm::vec3(0.f);
			m_player->getComponent<MovementComponent>()->velocity = glm::vec3(0.f);
		}
	}

#ifdef _DEBUG
	// Removes first added pointlight in arena
	if (Input::WasKeyJustPressed(KeyBinds::REMOVE_OLDEST_LIGHT)) {
		m_componentSystems.lightListSystem->removePointLightFromDebugEntity();
	}
#endif
#endif

	return true;
}

void GameState::initSystems(const unsigned char playerID) {
	m_componentSystems.teamColorSystem = ECS::Instance()->createSystem<TeamColorSystem>();
	m_componentSystems.movementSystem = ECS::Instance()->createSystem<MovementSystem>();
	
	m_componentSystems.collisionSystem = ECS::Instance()->createSystem<CollisionSystem>();
	m_componentSystems.collisionSystem->provideOctree(m_octree);

	m_componentSystems.movementPostCollisionSystem = ECS::Instance()->createSystem<MovementPostCollisionSystem>();

	m_componentSystems.speedLimitSystem = ECS::Instance()->createSystem<SpeedLimitSystem>();

	m_componentSystems.animationSystem = ECS::Instance()->createSystem<AnimationSystem>();
	m_componentSystems.animationChangerSystem = ECS::Instance()->createSystem<AnimationChangerSystem>();

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
	m_componentSystems.spotLightSystem = ECS::Instance()->createSystem<SpotLightSystem>();


	m_componentSystems.candleHealthSystem = ECS::Instance()->createSystem<CandleHealthSystem>();
	m_componentSystems.candleReignitionSystem = ECS::Instance()->createSystem<CandleReignitionSystem>();
	m_componentSystems.candlePlacementSystem = ECS::Instance()->createSystem<CandlePlacementSystem>();
	m_componentSystems.candlePlacementSystem->setOctree(m_octree);

	// Create system which prepares each new update
	m_componentSystems.prepareUpdateSystem = ECS::Instance()->createSystem<PrepareUpdateSystem>();

	// Create system which handles creation of projectiles
	m_componentSystems.gunSystem = ECS::Instance()->createSystem<GunSystem>();

	// Create system which checks projectile collisions
	m_componentSystems.projectileSystem = ECS::Instance()->createSystem<ProjectileSystem>();

	m_componentSystems.levelSystem = ECS::Instance()->createSystem<LevelSystem>();

	// Create systems for rendering
	m_componentSystems.beginEndFrameSystem = ECS::Instance()->createSystem<BeginEndFrameSystem>();
	m_componentSystems.boundingboxSubmitSystem = ECS::Instance()->createSystem<BoundingboxSubmitSystem>();
	m_componentSystems.metaballSubmitSystem = ECS::Instance()->createSystem<MetaballSubmitSystem>();
	m_componentSystems.modelSubmitSystem = ECS::Instance()->createSystem<ModelSubmitSystem>();
	m_componentSystems.renderImGuiSystem = ECS::Instance()->createSystem<RenderImGuiSystem>();
	m_componentSystems.guiSubmitSystem = ECS::Instance()->createSystem<GUISubmitSystem>();

	// Create system for player input
	m_componentSystems.gameInputSystem = ECS::Instance()->createSystem<GameInputSystem>();
	m_componentSystems.gameInputSystem->initialize(&m_cam);

	// Create network send and receive systems
	m_componentSystems.networkSenderSystem = ECS::Instance()->createSystem<NetworkSenderSystem>();

	NWrapperSingleton::getInstance().setNSS(m_componentSystems.networkSenderSystem);
	// Create Network Reciever System depending on host or client.
	if (NWrapperSingleton::getInstance().isHost()) {
		m_componentSystems.networkReceiverSystem = ECS::Instance()->createSystem<NetworkReceiverSystemHost>();
	} else {
		m_componentSystems.networkReceiverSystem = ECS::Instance()->createSystem<NetworkReceiverSystemClient>();
	}
	m_componentSystems.killCamReceiverSystem = ECS::Instance()->createSystem<KillCamReceiverSystem>();

	m_componentSystems.killCamReceiverSystem->init(playerID, m_componentSystems.networkSenderSystem);
	m_componentSystems.networkReceiverSystem->init(playerID, m_componentSystems.networkSenderSystem);
	m_componentSystems.networkSenderSystem->init(playerID, m_componentSystems.networkReceiverSystem, m_componentSystems.killCamReceiverSystem);



	// Create system for handling and updating sounds
	m_componentSystems.audioSystem = ECS::Instance()->createSystem<AudioSystem>();

	m_componentSystems.playerSystem = ECS::Instance()->createSystem<PlayerSystem>();

	//Create particle system
	m_componentSystems.particleSystem = ECS::Instance()->createSystem<ParticleSystem>();

	m_componentSystems.sprinklerSystem = ECS::Instance()->createSystem<SprinklerSystem>();
	m_componentSystems.sprinklerSystem->setOctree(m_octree);

	m_componentSystems.sprintingSystem = ECS::Instance()->createSystem<SprintingSystem>();
}

void GameState::initConsole() {
	auto& console = Application::getInstance()->getConsole();
	console.addCommand("state <string>", [&](const std::string& param) {
		bool stateChanged = false;
		std::string returnMsg = "Invalid state. Available states are \"menu\" and \"pbr\"";
		if (param == "menu") {
			requestStackPop();
			requestStackPush(States::MainMenu);
			stateChanged = true;
			returnMsg = "State change to menu requested";
		}
		else if (param == "pbr") {
			requestStackPop();
			requestStackPush(States::PBRTest);
			stateChanged = true;
			returnMsg = "State change to pbr requested";
		}

		if (stateChanged) {
			// Reset the network
			// Needs to be done to allow new games to be started
			NWrapperSingleton::getInstance().resetNetwork();
			NWrapperSingleton::getInstance().resetWrapper();
		}
		return returnMsg.c_str();

	}, "GameState");
	console.addCommand("profiler", [&]() { return toggleProfiler(); }, "GameState");
	console.addCommand("EndGame", [&]() {
		NWrapperSingleton::getInstance().queueGameStateNetworkSenderEvent(
			Netcode::MessageType::MATCH_ENDED,
			nullptr
		);

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

bool GameState::onEvent(const Event& event) {
	switch (event.type) {
	case Event::Type::WINDOW_RESIZE:					onResize((const WindowResizeEvent&)event); break;
	case Event::Type::NETWORK_SERIALIZED_DATA_RECIEVED:	onNetworkSerializedPackageEvent((const NetworkSerializedPackageEvent&)event); break;
	case Event::Type::NETWORK_DISCONNECT:				onPlayerDisconnect((const NetworkDisconnectEvent&)event); break;
	case Event::Type::NETWORK_DROPPED:					onPlayerDropped((const NetworkDroppedEvent&)event); break;
	default: break;
	}

	return true;
}
	
bool GameState::onResize(const WindowResizeEvent& event) {
	m_cam.resize(event.width, event.height);
	return true;
}

bool GameState::onNetworkSerializedPackageEvent(const NetworkSerializedPackageEvent& event) {
	m_componentSystems.networkReceiverSystem->handleIncomingData(event.serializedData);
	m_componentSystems.killCamReceiverSystem->handleIncomingData(event.serializedData);
	return true;
}

bool GameState::onPlayerDisconnect(const NetworkDisconnectEvent& event) {
	if (m_isSingleplayer) {
		return true;
	}

	GameDataTracker::getInstance().logMessage(event.player.name + " Left The Game!");
	logSomeoneDisconnected(event.player.id);

	return true;
}

bool GameState::onPlayerDropped(const NetworkDroppedEvent& event) {
	// I was dropped!
	// Saddest of bois.

	SAIL_LOG_WARNING("CONNECTION TO HOST HAS BEEN LOST");
	m_wasDropped = true;	// Activates a renderImgui window

	return false;
}

bool GameState::update(float dt, float alpha) {
	// UPDATE REAL TIME SYSTEMS
	updatePerFrameComponentSystems(dt, alpha);

	m_killFeedWindow.updateTiming(dt);
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
		auto pos = glm::vec3(m_player->getComponent<TransformComponent>()->getMatrixWithUpdate()[3]);
		auto ePos = e->getComponent<TransformComponent>()->getTranslation();
		ePos.y = ePos.y + 5.f;
		auto dir = ePos - pos;
		auto dirNorm = glm::normalize(dir);
		e->getComponent<GunComponent>()->setFiring(pos + dirNorm * 3.f, glm::vec3(0.f, -1.f, 0.f));
	}
#endif

	
	updatePerTickComponentSystems(dt);

	if (m_isInKillCamMode) {
		updateKillCamComponentSystems(dt);
	}

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
	m_componentSystems.particleSystem->submitAll();
	m_componentSystems.metaballSubmitSystem->submitAll(alpha);
	m_componentSystems.boundingboxSubmitSystem->submitAll();
	m_componentSystems.guiSubmitSystem->submitAll();
	m_componentSystems.beginEndFrameSystem->endFrameAndPresent();

	return true;
}

bool GameState::renderImgui(float dt) {
	m_killFeedWindow.renderWindow();
	if (m_wasDropped) {
		m_wasDroppedWindow.renderWindow();
	}

	//// KEEP UNTILL FINISHED WITH HANDPOSITIONS
	//static glm::vec3 lPos(0.563f, 1.059f, 0.110f);
	//static glm::vec3 rPos(-0.555f, 1.052f, 0.107f);
	//static glm::vec3 lRot(1.178f, -0.462f, 0.600f);
	//static glm::vec3 rRot(1.280f, 0.283f, -0.179f);
	//if (ImGui::Begin("HandLocation")) {
	//	ImGui::SliderFloat("##lposx", &lPos.x, -1.5f, 1.5f);
	//	ImGui::SliderFloat("##lposy", &lPos.y, -1.5f, 1.5f);
	//	ImGui::SliderFloat("##lposz", &lPos.z, -1.5f, 1.5f);
	//	ImGui::Spacing();
	//	ImGui::SliderFloat("##rPosx", &rPos.x, -1.5f, 1.5f);
	//	ImGui::SliderFloat("##rPosy", &rPos.y, -1.5f, 1.5f);
	//	ImGui::SliderFloat("##rPosz", &rPos.z, -1.5f, 1.5f);
	//	ImGui::Spacing();
	//	ImGui::Spacing();
	//	ImGui::SliderFloat("##lRotx", &lRot.x, -3.14f, 3.14f);
	//	ImGui::SliderFloat("##lRoty", &lRot.y, -3.14f, 3.14f);
	//	ImGui::SliderFloat("##lRotz", &lRot.z, -3.14f, 3.14f);
	//	ImGui::Spacing();
	//	ImGui::SliderFloat("##rRotx", &rRot.x, -3.14f, 3.14f);
	//	ImGui::SliderFloat("##rRoty", &rRot.y, -3.14f, 3.14f);
	//	ImGui::SliderFloat("##rRotz", &rRot.z, -3.14f, 3.14f);
	//}
	//ImGui::End();
	//ECS::Instance()->getSystem<AnimationSystem>()->updateHands(lPos, rPos, lRot, rRot);


	return false;
}

bool GameState::renderImguiDebug(float dt) {
	// The ImGui windows are rendered when activated on F10
	m_profiler.renderWindow();
	m_renderSettingsWindow.renderWindow();
	m_lightDebugWindow.renderWindow();
	m_playerInfoWindow.renderWindow();
	m_networkInfoImGuiWindow.renderWindow();
	
	m_ecsSystemInfoImGuiWindow.renderWindow();

	return false;
}

void GameState::shutDownGameState() {
	// Show mouse cursor if hidden
	Input::HideCursor(false);
	ECS::Instance()->stopAllSystems();
	ECS::Instance()->destroyAllEntities();
}


// TODO: Add more systems here that only deal with replay entities/components
void GameState::updateKillCamComponentSystems(float dt) {
	
	// TODO: Prepare transform update for interpolation etc.

	m_componentSystems.killCamReceiverSystem->processReplayData(dt);
	

	// TODO: run relevant systems in parallel
	//runSystem(dt, m_componentSystems.killCamReceiverSystem);


	// Wait for all systems to finish executing
	for (auto& fut : m_runningSystemJobs) {
		fut.get();
	}
}

// HERE BE DRAGONS
// Make sure things are updated in the correct order or things will behave strangely
void GameState::updatePerTickComponentSystems(float dt) {
	m_currentlyReadingMask = 0;
	m_currentlyWritingMask = 0;
	m_runningSystemJobs.clear();
	m_runningSystems.clear();

	m_componentSystems.gameInputSystem->fixedUpdate(dt);

	m_componentSystems.prepareUpdateSystem->update(); // HAS TO BE RUN BEFORE OTHER SYSTEMS WHICH USE TRANSFORM
	
	// Update entities with info from the network and from ourself
	// DON'T MOVE, should happen at the start of each tick
	m_componentSystems.networkReceiverSystem->update(dt);
	m_componentSystems.killCamReceiverSystem->update(dt); // This just increments the killcam's ringbuffer.

	m_componentSystems.movementSystem->update(dt);
	m_componentSystems.speedLimitSystem->update();
	m_componentSystems.collisionSystem->update(dt);
	m_componentSystems.movementPostCollisionSystem->update(dt);

	// TODO: Investigate this
	// Systems sent to runSystem() need to override the update(float dt) in BaseComponentSystem
	runSystem(dt, m_componentSystems.projectileSystem);
	runSystem(dt, m_componentSystems.animationChangerSystem);
	runSystem(dt, m_componentSystems.animationSystem);
	runSystem(dt, m_componentSystems.aiSystem);
	runSystem(dt, m_componentSystems.sprinklerSystem);
	runSystem(dt, m_componentSystems.candleHealthSystem);
	runSystem(dt, m_componentSystems.candlePlacementSystem);
	runSystem(dt, m_componentSystems.candleReignitionSystem);
	runSystem(dt, m_componentSystems.updateBoundingBoxSystem);
	runSystem(dt, m_componentSystems.gunSystem); // Run after animationSystem to make shots more in sync
	runSystem(dt, m_componentSystems.lifeTimeSystem);
	runSystem(dt, m_componentSystems.teamColorSystem);
	runSystem(dt, m_componentSystems.particleSystem);

	// Wait for all the systems to finish before starting the removal system
	for (auto& fut : m_runningSystemJobs) {
		fut.get();
	}
	m_componentSystems.spotLightSystem->enableHazardLights(m_componentSystems.sprinklerSystem->getActiveRooms());

	// Send out your entity info to the rest of the players
	// DON'T MOVE, should happen at the end of each tick
	m_componentSystems.networkSenderSystem->update();
	m_componentSystems.octreeAddRemoverSystem->update(dt);
}

void GameState::updatePerFrameComponentSystems(float dt, float alpha) {
	// TODO? move to its own thread

	NWrapperSingleton* ptr = &NWrapperSingleton::getInstance();
	NWrapperSingleton::getInstance().checkForPackages();

	m_componentSystems.sprintingSystem->update(dt, alpha);
	// Updates keyboard/mouse input and the camera
	m_componentSystems.gameInputSystem->update(dt, alpha);

	// There is an imgui debug toggle to override lights
	if (!m_lightDebugWindow.isManualOverrideOn()) {
		m_lights.clearPointLights();
		//check and update all lights for all entities
		m_componentSystems.lightSystem->updateLights(&m_lights);
		m_componentSystems.lightListSystem->updateLights(&m_lights);
		m_componentSystems.spotLightSystem->updateLights(&m_lights, alpha, dt);
	}

	if (m_showcaseProcGen) {
		m_cam.setPosition(glm::vec3(100.f, 100.f, 100.f));
	}
	m_componentSystems.animationSystem->updatePerFrame();
	m_componentSystems.audioSystem->update(m_cam, dt, alpha);
	m_componentSystems.octreeAddRemoverSystem->updatePerFrame(dt);

	// Will probably need to be called last
	m_componentSystems.entityAdderSystem->update();
	m_componentSystems.entityRemovalSystem->update();
}

#define DISABLE_RUNSYSTEM_MT
void GameState::runSystem(float dt, BaseComponentSystem* toRun) {
#ifdef DISABLE_RUNSYSTEM_MT
	toRun->update(dt);
#else
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
						// Currently just compares memory addresses (if they point to the same location they're the same object)
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
#endif
}

const std::string GameState::teleportToMap() {
	m_player->getComponent<TransformComponent>()->setStartTranslation(glm::vec3(30.6f, 0.9f, 40.f));
	return "";
}

const std::string GameState::toggleProfiler() {
	m_profiler.toggleWindow();
	return "Toggling profiler";
}

void GameState::logSomeoneDisconnected(unsigned char id) {
	// Construct log message
	std::string logMessage = "'";
	logMessage += NWrapperSingleton::getInstance().getPlayer(id)->name;
	logMessage += "' has disconnected from the game.";

	// Log it
	SAIL_LOG(logMessage);
}

const std::string GameState::createCube(const glm::vec3& position) {

	Model* tmpCubeModel = &m_app->getResourceManager().getModel(
		"cubeWidth1.fbx", &m_app->getResourceManager().getShaderSet<GBufferOutShader>());
	tmpCubeModel->getMesh(0)->getMaterial()->setColor(glm::vec4(0.2f, 0.8f, 0.4f, 1.0f));

	Model* tmpbbModel = &m_app->getResourceManager().getModel(
		"boundingBox.fbx", &m_app->getResourceManager().getShaderSet<GBufferWireframe>());
	tmpCubeModel->getMesh(0)->getMaterial()->setColor(glm::vec4(0.2f, 0.8f, 0.4f, 1.0f));

	auto e = ECS::Instance()->createEntity("new cube");
	//e->addComponent<ModelComponent>(tmpCubeModel);

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

void GameState::createBots(Model* boundingBoxModel, const std::string& characterModel, Model* projectileModel, Model* lightModel) {
	int botCount = m_app->getStateStorage().getLobbyToGameData()->botCount;

	if (botCount < 0) {
		botCount = 0;
	}

	for (size_t i = 0; i < botCount; i++) {
		glm::vec3 spawnLocation = m_componentSystems.levelSystem->getSpawnPoint();
		if (spawnLocation.x != -1000.f) {
			auto e = EntityFactory::CreateBot(boundingBoxModel, &m_app->getResourceManager().getModelCopy(characterModel), spawnLocation, lightModel, m_currLightIndex++, m_componentSystems.aiSystem->getNodeSystem());
		}
		else {
			SAIL_LOG_ERROR("Bot not spawned because all spawn points are already used for this map.");
		}
	}
}

void GameState::createLevel(Shader* shader, Model* boundingBoxModel) {
	std::string tileTex = "sponza/textures/tileTexture1.tga";
	std::vector<Model*> tileModels;
	std::vector<Model*> clutterModels;
	//Load textures for level
	{
		ResourceManager& manager = Application::getInstance()->getResourceManager();
		manager.loadTexture(tileTex);
	}

	//Load tileset for world
	{
		Model* roomWall = &m_app->getResourceManager().getModel("Tiles/RoomWall.fbx", shader);
		roomWall->getMesh(0)->getMaterial()->setMetalnessRoughnessAOTexture("pbr/Tiles/RoomWallMRAO.tga");
		roomWall->getMesh(0)->getMaterial()->setNormalTexture("pbr/Tiles/RoomWallNM.tga");
		roomWall->getMesh(0)->getMaterial()->setAlbedoTexture("pbr/Tiles/RoomWallAlbedo.tga");

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

		Model* cTable = &m_app->getResourceManager().getModel("Clutter/Table.fbx", shader);
		cTable->getMesh(0)->getMaterial()->setMetalnessRoughnessAOTexture("pbr/Clutter/Table_MRAO.tga");
		cTable->getMesh(0)->getMaterial()->setNormalTexture("pbr/Clutter/Table_NM.tga");
		cTable->getMesh(0)->getMaterial()->setAlbedoTexture("pbr/Clutter/Table_Albedo.tga");

		Model* cBoxes = &m_app->getResourceManager().getModel("Clutter/Boxes.fbx", shader);
		cBoxes->getMesh(0)->getMaterial()->setMetalnessRoughnessAOTexture("pbr/Clutter/Boxes_MRAO.tga");
		cBoxes->getMesh(0)->getMaterial()->setNormalTexture("pbr/Clutter/Boxes_NM.tga");
		cBoxes->getMesh(0)->getMaterial()->setAlbedoTexture("pbr/Clutter/Boxes_Albedo.tga");

		Model* cMediumBox = &m_app->getResourceManager().getModel("Clutter/MediumBox.fbx", shader);
		cMediumBox->getMesh(0)->getMaterial()->setMetalnessRoughnessAOTexture("pbr/Clutter/MediumBox_MRAO.tga");
		cMediumBox->getMesh(0)->getMaterial()->setNormalTexture("pbr/Clutter/MediumBox_NM.tga");
		cMediumBox->getMesh(0)->getMaterial()->setAlbedoTexture("pbr/Clutter/MediumBox_Albedo.tga");

		Model* cSquareBox = &m_app->getResourceManager().getModel("Clutter/SquareBox.fbx", shader);
		cSquareBox->getMesh(0)->getMaterial()->setMetalnessRoughnessAOTexture("pbr/Clutter/SquareBox_MRAO.tga");
		cSquareBox->getMesh(0)->getMaterial()->setNormalTexture("pbr/Clutter/SquareBox_NM.tga");
		cSquareBox->getMesh(0)->getMaterial()->setAlbedoTexture("pbr/Clutter/SquareBox_Albedo.tga");

		Model* cBooks1 = &m_app->getResourceManager().getModel("Clutter/Books1.fbx", shader);
		cBooks1->getMesh(0)->getMaterial()->setMetalnessRoughnessAOTexture("pbr/Clutter/Book_MRAO.tga");
		cBooks1->getMesh(0)->getMaterial()->setNormalTexture("pbr/Clutter/Book_NM.tga");
		cBooks1->getMesh(0)->getMaterial()->setAlbedoTexture("pbr/Clutter/Book1_Albedo.tga");

		Model* cBooks2 = &m_app->getResourceManager().getModel("Clutter/Books2.fbx", shader);
		cBooks2->getMesh(0)->getMaterial()->setMetalnessRoughnessAOTexture("pbr/Clutter/Book_MRAO.tga");
		cBooks2->getMesh(0)->getMaterial()->setNormalTexture("pbr/Clutter/Book_NM.tga");
		cBooks2->getMesh(0)->getMaterial()->setAlbedoTexture("pbr/Clutter/Book2_Albedo.tga");

		Model* cScreen = &m_app->getResourceManager().getModel("Clutter/Screen.fbx", shader);
		cScreen->getMesh(0)->getMaterial()->setMetalnessRoughnessAOTexture("pbr/Clutter/Screen_MRAO.tga");
		cScreen->getMesh(0)->getMaterial()->setNormalTexture("pbr/Clutter/Screen_NM.tga");
		cScreen->getMesh(0)->getMaterial()->setAlbedoTexture("pbr/Clutter/Screen_Albedo.tga");

		Model* cNotepad = &m_app->getResourceManager().getModel("Clutter/Notepad.fbx", shader);
		cNotepad->getMesh(0)->getMaterial()->setMetalnessRoughnessAOTexture("pbr/Clutter/Notepad_MRAO.tga");
		cNotepad->getMesh(0)->getMaterial()->setNormalTexture("pbr/Clutter/Notepad_NM.tga");
		cNotepad->getMesh(0)->getMaterial()->setAlbedoTexture("pbr/Clutter/Notepad_Albedo.tga");

		Model* cMicroscope= &m_app->getResourceManager().getModel("Clutter/Microscope.fbx", shader);
		cMicroscope->getMesh(0)->getMaterial()->setMetalnessRoughnessAOTexture("pbr/Clutter/Microscope_MRAO.tga");
		cMicroscope->getMesh(0)->getMaterial()->setNormalTexture("pbr/Clutter/Microscope_NM.tga");
		cMicroscope->getMesh(0)->getMaterial()->setAlbedoTexture("pbr/Clutter/Microscope_Albedo.tga");


		Model* saftblandare = &m_app->getResourceManager().getModel("Clutter/Saftblandare.fbx", shader);
		saftblandare->getMesh(0)->getMaterial()->setMetalnessRoughnessAOTexture("pbr/Clutter/Saftblandare_MRAO.tga");
		saftblandare->getMesh(0)->getMaterial()->setNormalTexture("pbr/Clutter/Saftblandare_NM.tga");
		saftblandare->getMesh(0)->getMaterial()->setAlbedoTexture("pbr/Clutter/Saftblandare_Albedo.tga");

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

		clutterModels.resize(ClutterModel::NUMBOFCLUTTER);
		clutterModels[ClutterModel::SAFTBLANDARE] = saftblandare;
		clutterModels[ClutterModel::TABLE] = cTable;
		clutterModels[ClutterModel::BOXES] = cBoxes;
		clutterModels[ClutterModel::MEDIUMBOX] = cMediumBox;
		clutterModels[ClutterModel::BOOKS1] = cBooks1;
		clutterModels[ClutterModel::BOOKS2] = cBooks2;
		clutterModels[ClutterModel::SQUAREBOX] = cSquareBox;
		clutterModels[ClutterModel::SCREEN] = cScreen;
		clutterModels[ClutterModel::NOTEPAD] = cNotepad;
		clutterModels[ClutterModel::MICROSCOPE] = cMicroscope;

	}

	// Create the level generator system and put it into the datatype.
	SettingStorage& settings = m_app->getSettings();

	m_componentSystems.levelSystem->seed = settings.gameSettingsDynamic["map"]["seed"].value;
	m_componentSystems.levelSystem->clutterModifier = settings.gameSettingsDynamic["map"]["clutter"].value * 100;
	m_componentSystems.levelSystem->xsize = settings.gameSettingsDynamic["map"]["sizeX"].value;
	m_componentSystems.levelSystem->ysize = settings.gameSettingsDynamic["map"]["sizeY"].value;

	m_componentSystems.levelSystem->generateMap();
	m_componentSystems.levelSystem->createWorld(tileModels, boundingBoxModel);
	m_componentSystems.levelSystem->addClutterModel(clutterModels, boundingBoxModel);
	m_componentSystems.gameInputSystem->m_mapPointer = m_componentSystems.levelSystem;
}

#ifdef _PERFORMANCE_TEST
void GameState::populateScene(Model* lightModel, Model* bbModel, Model* projectileModel, Shader* shader) {
	/* 13 characters that are constantly shooting their guns */
	for (int i = 0; i < 13; i++) {
		SAIL_LOG("Adding performance test player.");
		float spawnOffsetX = 43.f + float(i) * 2.f;
		float spawnOffsetZ = 52.f + float(i) * 1.3f;

		auto e = ECS::Instance()->createEntity("Performance Test Entity " + std::to_string(i));

		EntityFactory::CreatePerformancePlayer(e, i, glm::vec3(spawnOffsetX, -0.9f, spawnOffsetZ));

		m_performanceEntities.push_back(e);
	}
}
#endif
