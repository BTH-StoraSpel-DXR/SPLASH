#include "GameState.h"
#include "imgui.h"
#include "Sail/entities/ECS.h"
#include "Sail/entities/components/Components.h"
#include "Sail/entities/systems/Systems.h"
#include "Sail/graphics/shader/compute/AnimationUpdateComputeShader.h"
#include "Sail/graphics/shader/compute/ParticleComputeShader.h"
#include "Sail/graphics/shader/postprocess/BlendShader.h"
#include "Sail/graphics/shader/postprocess/GaussianBlurHorizontal.h"
#include "Sail/graphics/shader/postprocess/GaussianBlurVertical.h"
#include "Sail/graphics/shader/dxr/GBufferOutShaderNoDepth.h"
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
#include "API/DX12/renderer/DX12HybridRaytracerRenderer.h"
#include "API/DX12/dxr/DXRBase.h"
#include "../Sail/src/API/Audio/AudioEngine.h"
#include "Sail/graphics/shader/postprocess/BilateralBlurHorizontal.h"
#include "Sail/graphics/shader/postprocess/BilateralBlurVertical.h"
#include "Sail/graphics/shader/dxr/ShadePassShader.h"
#include "Sail/utils/SailImGui/SailImGui.h"


constexpr int SPECTATOR_TEAM = -1;

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
	EventDispatcher::Instance().subscribe(Event::Type::NETWORK_JOINED, this);
	EventDispatcher::Instance().subscribe(Event::Type::NETWORK_UPDATE_STATE_LOAD_STATUS, this);
	EventDispatcher::Instance().subscribe(Event::Type::START_KILLCAM, this);
	EventDispatcher::Instance().subscribe(Event::Type::STOP_KILLCAM, this);

	// Reset the counter used to generate unique ComponentIDs for the network
	Netcode::resetIDCounter();

	// Get the Application instance
	m_app = Application::getInstance();
	m_isSingleplayer = NWrapperSingleton::getInstance().getPlayers().size() == 1;
	m_gameStarted = m_isSingleplayer; //Delay start of game until everyOne is ready if playing multiplayer
	m_imguiHandler = m_app->getImGuiHandler();
	NWrapperSingleton::getInstance().getNetworkWrapper()->updateStateLoadStatus(States::Game, 0); //Indicate To other players that you entered gamestate, but are not ready to start yet.
	m_waitingForPlayersWindow.setStateStatus(States::Game, 1);

	initConsole();
	m_app->setCurrentCamera(&m_cam);

	m_app->getChatWindow()->setFadeThreshold(4.0f);
	m_app->getChatWindow()->setFadeTime(0.7f);
	m_app->getChatWindow()->resetMessageTime();
	m_app->getChatWindow()->setRetainFocus(false);
	m_app->getChatWindow()->removeFocus();
	m_app->getChatWindow()->setBackgroundOpacity(0.05f);

	auto& dynamic = m_app->getSettings().gameSettingsDynamic;
	auto& settings = m_app->getSettings();
	std::vector<glm::vec3> m_teamColors;
	for (int i = 0; i < 12; i++) {
		m_teamColors.emplace_back(settings.getColor(settings.teamColorIndex(i)));
	}
	m_app->getRenderWrapper()->getCurrentRenderer()->setTeamColors(m_teamColors);

	// Update water voxel grid
	static_cast<DX12HybridRaytracerRenderer*>(m_app->getRenderWrapper()->getCurrentRenderer())->getDXRBase()->rebuildWater();

	//----Octree creation----
	//Wireframe shader
	auto* wireframeShader = &m_app->getResourceManager().getShaderSet<GBufferWireframe>();

	//Wireframe bounding box model
	Model* boundingBoxModel = &m_app->getResourceManager().getModel("boundingBox", wireframeShader);

	boundingBoxModel->getMesh(0)->getMaterial()->setColor(glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
	boundingBoxModel->getMesh(0)->getMaterial()->setAOScale(0.5);
	boundingBoxModel->getMesh(0)->getMaterial()->setMetalnessScale(0.5);
	boundingBoxModel->getMesh(0)->getMaterial()->setRoughnessScale(0.5);

	//Create octree
	m_octree = SAIL_NEW Octree(boundingBoxModel);
	m_killCamOctree = SAIL_NEW Octree(boundingBoxModel);
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

	m_ambiance = ECS::Instance()->createEntity("LabAmbiance").get();
	m_ambiance->addComponent<AudioComponent>();
	SAIL_LOG("Adding ambiance to AudioQueue");
	Application::getInstance()->addToAudioComponentQueue(m_ambiance);
	m_ambiance->addComponent<TransformComponent>(glm::vec3{ 0.0f, 0.0f, 0.0f });


	// Initialize the component systems
	initSystems(playerID);


	m_app->getRenderWrapper()->getCurrentRenderer()->setLightSetup(&m_lights);
	m_lightDebugWindow.setLightSetup(&m_lights);

	// Crosshair
	auto crosshairEntity = EntityFactory::CreateCrosshairEntity("crosshairEntity");

	// Level Creation

	createLevel(&m_app->getResourceManager().getShaderSet<GBufferOutShader>(), boundingBoxModel);
#ifndef _DEBUG
	if (NWrapperSingleton::getInstance().isHost() && m_app->getSettings().gameSettingsStatic["map"]["bots"].getSelected().value == 0.f) {
		m_componentSystems.aiSystem->initNodeSystem(m_octree);
	}
#endif
	// Player creation
	if (NWrapperSingleton::getInstance().getPlayer(NWrapperSingleton::getInstance().getMyPlayerID())->team == SPECTATOR_TEAM) {

		int id = static_cast<int>(playerID);
		glm::vec3 spawnLocation = glm::vec3(0.f);
		spawnLocation = m_componentSystems.levelSystem->getSpawnPoint(id);
		m_player = EntityFactory::CreateMySpectator(playerID, m_currLightIndex++, spawnLocation).get();

	} else {
		int id = static_cast<int>(playerID);
		glm::vec3 spawnLocation = glm::vec3(0.f);	
		spawnLocation = m_componentSystems.levelSystem->getSpawnPoint(id);
		m_player = EntityFactory::CreateMyPlayer(playerID, m_currLightIndex++, spawnLocation).get();
	}


	// Spawn other players
	for (auto p : NWrapperSingleton::getInstance().getPlayers()) {
		if (p.team != SPECTATOR_TEAM && p.id != playerID) {
			auto otherPlayer = ECS::Instance()->createEntity("Player: " + p.name);
			EntityFactory::CreateOtherPlayer(otherPlayer, p.id, m_currLightIndex++);
		}
	}


	m_componentSystems.networkReceiverSystem->setPlayer(m_player);
	m_componentSystems.networkReceiverSystem->setGameState(this);

#ifdef _PERFORMANCE_TEST
	Model* lightModel = &m_app->getResourceManager().getModel("Torch", &m_app->getResourceManager().getShaderSet<GBufferOutShader>());
	lightModel->getMesh(0)->getMaterial()->setAlbedoTexture("pbr/DDS/Torch/Torch_Albedo.dds");
	lightModel->getMesh(0)->getMaterial()->setNormalTexture("pbr/DDS/Torch/Torch_NM.dds");
	lightModel->getMesh(0)->getMaterial()->setMetalnessRoughnessAOTexture("pbr/DDS/Torch/Torch_MRAO.dds");

	populateScene(lightModel, boundingBoxModel, boundingBoxModel, &m_app->getResourceManager().getShaderSet<GBufferOutShader>());
	m_player->getComponent<TransformComponent>()->setStartTranslation(glm::vec3(54.f, 1.6f, 59.f));
#else
	#ifndef _DEBUG
	if (m_app->getSettings().gameSettingsStatic["map"]["bots"].getSelected().value == 0.f) {
		createBots();
	}
	#endif
#endif

#ifdef _DEBUG
	// Candle1 holds all lights you can place in debug...
	m_componentSystems.lightListSystem->setDebugLightListEntity("Map_Candle1");
#endif


	m_playerInfoWindow.setPlayerInfo(m_player, &m_cam);

	// Host fill its game tracker per player with player data.
	// Reset data trackers
	GameDataTracker::getInstance().init();

	// Clear all water on the level
	EventDispatcher::Instance().emit(ResetWaterEvent());


	m_inGameGui.setPlayer(m_player);
	m_inGameGui.setCrosshair(crosshairEntity.get());
	m_playerNamesinGameGui.setCamera(&m_cam);
	m_playerNamesinGameGui.setLocalPlayer(m_player);

	m_componentSystems.projectileSystem->setCrosshair(crosshairEntity.get());
	m_componentSystems.sprintingSystem->setCrosshair(crosshairEntity.get());

	// Flags for hud icons with imgui
	m_standaloneButtonflags = ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoNav |
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_AlwaysAutoResize |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoBackground;

	// Used for the killcam overlay
	m_backgroundOnlyflags = ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoNav |
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoBringToFrontOnFocus;


	//This will create all torch particles before players report done loading which will remove the freazing after waiting for players. 
	if (m_app->getSettings().applicationSettingsStatic["graphics"]["particles"].getSelected().value > 0.0f) {
		m_componentSystems.particleSystem->addQueuedEntities();
		m_componentSystems.particleSystem->update(0);
	}

	// Keep this at the bottom
	NWrapperSingleton::getInstance().getNetworkWrapper()->updateStateLoadStatus(States::Game, 1); //Indicate To other players that you are ready to start.	
}

GameState::~GameState() {
	Application::getInstance()->getAPI<DX12API>()->waitForGPU();

	Application::getInstance()->getConsole().removeAllCommandsWithIdentifier("GameState");
	shutDownGameState();

	Memory::SafeDelete(m_octree);
	Memory::SafeDelete(m_killCamOctree);

	m_app->getChatWindow()->setFadeThreshold(-1.0f);
	m_app->getChatWindow()->setFadeTime(-1.0f);
	m_app->getChatWindow()->setRetainFocus(true);

	EventDispatcher::Instance().unsubscribe(Event::Type::WINDOW_RESIZE, this);
	EventDispatcher::Instance().unsubscribe(Event::Type::NETWORK_SERIALIZED_DATA_RECIEVED, this);
	EventDispatcher::Instance().unsubscribe(Event::Type::NETWORK_DISCONNECT, this);
	EventDispatcher::Instance().unsubscribe(Event::Type::NETWORK_DROPPED, this);
	EventDispatcher::Instance().unsubscribe(Event::Type::NETWORK_JOINED, this);
	EventDispatcher::Instance().unsubscribe(Event::Type::NETWORK_UPDATE_STATE_LOAD_STATUS, this);

	MatchRecordSystem*& mrs = NWrapperSingleton::getInstance().recordSystem;
	if (mrs) {
		delete mrs;
		mrs = nullptr;
	}

	EventDispatcher::Instance().unsubscribe(Event::Type::START_KILLCAM, this);
	EventDispatcher::Instance().unsubscribe(Event::Type::STOP_KILLCAM, this);

	m_app->getResourceManager().clearModelCopies();
}

// Process input for the state
// NOTE: Done every frame
bool GameState::processInput(float dt) {

#ifndef DEVELOPMENT
	//Capture mouse
	if (m_app->getWindow()->isFocused()) {
		Input::HideCursor(true);
	} else {
		Input::HideCursor(false);
	}
#endif

	// Unpause Game
	if (m_componentSystems.audioSystem) {

		if (m_readyRestartAmbiance) {
			m_componentSystems.audioSystem->getAudioEngine()->pause_unpause_AllStreams(false);
			m_ambiance->getComponent<AudioComponent>()->streamSetVolume_HELPERFUNC("res/sounds/ambient/ambiance_lab.xwb", Application::getInstance()->getSettings().applicationSettingsDynamic["sound"]["global"].value);
			m_readyRestartAmbiance = false;
		}
	}

	// Pause game
	if (!InGameMenuState::IsOpen() && Input::WasKeyJustPressed(KeyBinds::SHOW_IN_GAME_MENU)) {
		if (m_componentSystems.audioSystem) {
			m_readyRestartAmbiance = true;
			ECS::Instance()->getSystem<AudioSystem>()->getAudioEngine()->pause_unpause_AllStreams(true);
			
		}
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
		} else {
			m_cam.setPosition(glm::vec3(0.f, 1.f, 0.f));
		}
	}


	if (Input::WasKeyJustPressed(KeyBinds::TOGGLE_ROOM_LIGHTS)) {
		m_componentSystems.hazardLightSystem->toggleONOFF();
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

	//Test frustum culling
	if (Input::IsKeyPressed(KeyBinds::TEST_FRUSTUMCULLING)) {
		int nrOfDraws = m_octree->frustumCulledDraw(m_cam);
		SAIL_LOG("Number of draws " + std::to_string(nrOfDraws));
	}

	// Set directional light if using forward rendering
	if (Input::IsKeyPressed(KeyBinds::SET_DIRECTIONAL_LIGHT)) {
		glm::vec3 color(1.0f, 1.0f, 1.0f);
		m_lights.setDirectionalLight(DirectionalLight(color, m_cam.getDirection()));
	}

	// Reload shaders
	if (Input::WasKeyJustPressed(KeyBinds::RELOAD_SHADER)) {
		m_app->getAPI<DX12API>()->waitForGPU();
		m_app->getResourceManager().reloadShader<ShadePassShader>();
		m_app->getResourceManager().reloadShader<BilateralBlurHorizontal>();
		m_app->getResourceManager().reloadShader<BilateralBlurVertical>();
		m_app->getResourceManager().reloadShader<BlendShader>();
		m_app->getResourceManager().reloadShader<GaussianBlurHorizontal>();
		m_app->getResourceManager().reloadShader<GaussianBlurVertical>();
		m_app->getResourceManager().reloadShader<AnimationUpdateComputeShader>();
		m_app->getResourceManager().reloadShader<ParticleComputeShader>();
		m_app->getResourceManager().reloadShader<GBufferOutShader>();
		m_app->getResourceManager().reloadShader<GBufferOutShaderNoDepth>();
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

			auto transform = m_player->getComponent<TransformComponent>();
			auto pos = glm::vec3(transform->getCurrentTransformState().m_translation);
			pos.y += 2.0f;

			for (auto e : m_player->getChildEntities()) {
				e->removeAllComponents();
				e->queueDestruction();
			}

			m_player->removeAllChildren();
			m_player->removeAllComponents();

			m_player->addComponent<TransformComponent>()->setStartTranslation(pos);
			m_player->addComponent<SpectatorComponent>();
		}
	}

#ifdef _DEBUG
	// Removes first added pointlight in arena
	if (Input::WasKeyJustPressed(KeyBinds::REMOVE_OLDEST_LIGHT)) {
		m_componentSystems.lightListSystem->removePointLightFromDebugEntity();
	}
#endif
	if (Input::WasKeyJustPressed(KeyBinds::UNLOAD_CPU_TEXTURES)) {
		Application::getInstance()->getResourceManager().printModelsToFile();
		Application::getInstance()->getResourceManager().printLoadedTexturesToFile();
		Application::getInstance()->getResourceManager().logRemainingTextures();
		Application::getInstance()->getResourceManager().unloadTextures();
	}
#endif

	return true;
}

void GameState::initSystems(const unsigned char playerID) {
	m_componentSystems.teamColorSystem = ECS::Instance()->createSystem<TeamColorSystem>();
	m_componentSystems.movementSystem = ECS::Instance()->createSystem<MovementSystem<RenderInActiveGameComponent>>();

	m_componentSystems.collisionSystem = ECS::Instance()->createSystem<CollisionSystem<RenderInActiveGameComponent>>();
	m_componentSystems.collisionSystem->provideOctree(m_octree);

	m_componentSystems.movementPostCollisionSystem = ECS::Instance()->createSystem<MovementPostCollisionSystem<RenderInActiveGameComponent>>();

	m_componentSystems.speedLimitSystem = ECS::Instance()->createSystem<SpeedLimitSystem>();

	m_componentSystems.animationSystem = ECS::Instance()->createSystem<AnimationSystem<RenderInActiveGameComponent>>();
	m_componentSystems.animationChangerSystem = ECS::Instance()->createSystem<AnimationChangerSystem>();

	m_componentSystems.updateBoundingBoxSystem = ECS::Instance()->createSystem<UpdateBoundingBoxSystem>();

	m_componentSystems.octreeAddRemoverSystem = ECS::Instance()->createSystem<OctreeAddRemoverSystem<RenderInActiveGameComponent>>();
	m_componentSystems.octreeAddRemoverSystem->provideOctree(m_octree);
	m_componentSystems.octreeAddRemoverSystem->setCulling(true, &m_cam); // Enable frustum culling

	m_componentSystems.lifeTimeSystem = ECS::Instance()->createSystem<LifeTimeSystem>();
	m_componentSystems.sanitySoundSystem = ECS::Instance()->createSystem<SanitySoundSystem>();
	m_componentSystems.sanitySystem = ECS::Instance()->createSystem<SanitySystem>();

	m_componentSystems.entityAdderSystem = ECS::Instance()->getEntityAdderSystem();

	m_componentSystems.entityRemovalSystem = ECS::Instance()->getEntityRemovalSystem();

	if (NWrapperSingleton::getInstance().isHost() && m_app->getSettings().gameSettingsStatic["map"]["bots"].getSelected().value == 0.f) {
		m_componentSystems.aiSystem = ECS::Instance()->createSystem<AiSystem>();
	}

	m_componentSystems.waterCleaningSystem = ECS::Instance()->createSystem<WaterCleaningSystem>();

	m_componentSystems.lightSystem = ECS::Instance()->createSystem<LightSystem<RenderInActiveGameComponent>>();
	m_componentSystems.lightListSystem = ECS::Instance()->createSystem<LightListSystem>();
	m_componentSystems.hazardLightSystem = ECS::Instance()->createSystem<HazardLightSystem>();

	m_componentSystems.candleHealthSystem = ECS::Instance()->createSystem<CandleHealthSystem>();
	m_componentSystems.candleReignitionSystem = ECS::Instance()->createSystem<CandleReignitionSystem>();
	m_componentSystems.candlePlacementSystem = ECS::Instance()->createSystem<CandlePlacementSystem>();
	m_componentSystems.candleThrowingSystem = ECS::Instance()->createSystem<CandleThrowingSystem>();
	m_componentSystems.candleThrowingSystem->setOctree(m_octree);

	m_componentSystems.crosshairSystem = ECS::Instance()->createSystem<CrosshairSystem>();

	// Create system which prepares each new update
	m_componentSystems.prepareUpdateSystem = ECS::Instance()->createSystem<PrepareUpdateSystem>();

	// Create system which handles creation of projectiles
	m_componentSystems.gunSystem = ECS::Instance()->createSystem<GunSystem>();
	m_componentSystems.gunSystem->setOctree(m_octree);

	// Create system which checks projectile collisions
	m_componentSystems.projectileSystem = ECS::Instance()->createSystem<ProjectileSystem>();

	m_componentSystems.levelSystem = ECS::Instance()->createSystem<LevelSystem>();

	// Create systems for rendering
	m_componentSystems.beginEndFrameSystem     = ECS::Instance()->createSystem<BeginEndFrameSystem>();
	m_componentSystems.boundingboxSubmitSystem = ECS::Instance()->createSystem<BoundingboxSubmitSystem>();
	m_componentSystems.metaballSubmitSystem    = ECS::Instance()->createSystem<MetaballSubmitSystem<RenderInActiveGameComponent>>();
	m_componentSystems.modelSubmitSystem       = ECS::Instance()->createSystem<ModelSubmitSystem<RenderInActiveGameComponent>>();
	m_componentSystems.renderImGuiSystem       = ECS::Instance()->createSystem<RenderImGuiSystem>();
	m_componentSystems.guiSubmitSystem         = ECS::Instance()->createSystem<GUISubmitSystem>();

	// Create system for player input
	m_componentSystems.gameInputSystem = ECS::Instance()->createSystem<GameInputSystem>();
	m_componentSystems.gameInputSystem->initialize(&m_cam);

	m_componentSystems.spectateInputSystem = ECS::Instance()->createSystem<SpectateInputSystem>();
	m_componentSystems.spectateInputSystem->initialize(&m_cam);

	// Create network send and receive systems
	m_componentSystems.networkSenderSystem = ECS::Instance()->createSystem<NetworkSenderSystem>();

	NWrapperSingleton::getInstance().setNSS(m_componentSystems.networkSenderSystem);
	// Create Network Receiver System depending on host or client.
	if (NWrapperSingleton::getInstance().isHost() && (!NWrapperSingleton::getInstance().recordSystem || NWrapperSingleton::getInstance().recordSystem->status != 2)) {
		m_componentSystems.networkReceiverSystem = ECS::Instance()->createSystem<NetworkReceiverSystemHost>();
	} else {
		m_componentSystems.networkReceiverSystem = ECS::Instance()->createSystem<NetworkReceiverSystemClient>();
	}

	m_componentSystems.killCamReceiverSystem = ECS::Instance()->createSystem<KillCamReceiverSystem>();

	m_componentSystems.networkReceiverSystem->init(playerID, m_componentSystems.networkSenderSystem);
	m_componentSystems.networkSenderSystem->init(playerID, m_componentSystems.networkReceiverSystem, m_componentSystems.killCamReceiverSystem);

	m_componentSystems.hostSendToSpectatorSystem = ECS::Instance()->createSystem<HostSendToSpectatorSystem>();
	m_componentSystems.hostSendToSpectatorSystem->init(playerID);

	// Create system for handling and updating sounds | Disabled by default to save RAM

		// If audiosystem exists


	if (m_app->getSettings().applicationSettingsDynamic["sound"]["global"].value > 0.0f) {
		if (!ECS::Instance()->getSystem<AudioSystem>()) {
			m_app->startAudio();
		}
		m_componentSystems.audioSystem = ECS::Instance()->getSystem<AudioSystem>();
		m_ambiance->getComponent<AudioComponent>()->streamSoundRequest_HELPERFUNC("res/sounds/ambient/ambiance_lab.xwb", true, Application::getInstance()->getSettings().applicationSettingsDynamic["sound"]["global"].value, false, true);
		m_ambiance->getComponent<AudioComponent>()->streamSetVolume_HELPERFUNC("res/sounds/ambient/ambiance_lab.xwb", Application::getInstance()->getSettings().applicationSettingsDynamic["sound"]["global"].value);
	}
	/* Old code for when audiosystem isn't removed at start because of RAM reasons. */
//	m_componentSystems.audioSystem = ECS::Instance()->createSystem<AudioSystem>();

	m_componentSystems.playerSystem = ECS::Instance()->createSystem<PlayerSystem>();
	m_componentSystems.powerUpUpdateSystem = ECS::Instance()->createSystem<PowerUpUpdateSystem>();
	m_componentSystems.powerUpCollectibleSystem = ECS::Instance()->createSystem<PowerUpCollectibleSystem>();
	m_componentSystems.powerUpCollectibleSystem->init(m_componentSystems.playerSystem->getPlayers());



	//Create particle system
	m_componentSystems.particleSystem = ECS::Instance()->createSystem<ParticleSystem>();


	m_componentSystems.sprinklerSystem = ECS::Instance()->createSystem<SprinklerSystem>();
	m_componentSystems.sprinklerSystem->setOctree(m_octree);

	m_componentSystems.sprintingSystem = ECS::Instance()->createSystem<SprintingSystem>();


	// Create systems needed for the killcam
	m_componentSystems.killCamReceiverSystem->init(playerID, &m_cam);

	m_componentSystems.killCamAnimationSystem             = ECS::Instance()->createSystem<AnimationSystem<RenderInReplayComponent>>();
	m_componentSystems.killCamLightSystem                 = ECS::Instance()->createSystem<LightSystem<RenderInReplayComponent>>();
	m_componentSystems.killCamMetaballSubmitSystem        = ECS::Instance()->createSystem<MetaballSubmitSystem<RenderInReplayComponent>>();
	m_componentSystems.killCamModelSubmitSystem           = ECS::Instance()->createSystem<ModelSubmitSystem<RenderInReplayComponent>>();
	m_componentSystems.killCamMovementSystem              = ECS::Instance()->createSystem<MovementSystem<RenderInReplayComponent>>();
	m_componentSystems.killCamMovementPostCollisionSystem = ECS::Instance()->createSystem<MovementPostCollisionSystem<RenderInReplayComponent>>();

	m_componentSystems.killCamCollisionSystem = ECS::Instance()->createSystem<CollisionSystem<RenderInReplayComponent>>();
	m_componentSystems.killCamCollisionSystem->provideOctree(m_killCamOctree);

	m_componentSystems.killCamOctreeAddRemoverSystem = ECS::Instance()->createSystem<OctreeAddRemoverSystem<RenderInReplayComponent>>();
	m_componentSystems.killCamOctreeAddRemoverSystem->provideOctree(m_killCamOctree);
	m_componentSystems.killCamOctreeAddRemoverSystem->setCulling(true, &m_cam); // Enable frustum culling
}

void GameState::initConsole() {
	auto& console = Application::getInstance()->getConsole();
#ifdef DEVELOPMENT
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
			// Load the textures that weren't needed until now
			auto& rm = Application::getInstance()->getResourceManager();
			rm.loadTexture("pbr/metal/metalnessRoughnessAO.tga");
			rm.loadTexture("pbr/metal/normal.tga");
			rm.loadTexture("pbr/metal/albedo.tga");

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
#endif
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
	State::onEvent(event);

	switch (event.type) {
	case Event::Type::WINDOW_RESIZE:                    onResize((const WindowResizeEvent&)event); break;
	case Event::Type::NETWORK_SERIALIZED_DATA_RECIEVED: onNetworkSerializedPackageEvent((const NetworkSerializedPackageEvent&)event); break;
	case Event::Type::NETWORK_DISCONNECT:               onPlayerDisconnect((const NetworkDisconnectEvent&)event); break;
	case Event::Type::NETWORK_DROPPED:                  onPlayerDropped((const NetworkDroppedEvent&)event); break;
	case Event::Type::NETWORK_UPDATE_STATE_LOAD_STATUS: onPlayerStateStatusChanged((const NetworkUpdateStateLoadStatus&)event); break;
	case Event::Type::NETWORK_JOINED:                   onPlayerJoined((const NetworkJoinedEvent&)event); break;
	case Event::Type::START_KILLCAM:                    onStartKillCam((const StartKillCamEvent&)event); break;
	case Event::Type::STOP_KILLCAM:                     onStopKillCam((const StopKillCamEvent&)event); break;
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

bool GameState::onPlayerJoined(const NetworkJoinedEvent& event) {

	if (NWrapperSingleton::getInstance().isHost()) {
		NWrapperSingleton::getInstance().getNetworkWrapper()->setTeamOfPlayer(-1, event.player.id, false);
		NWrapperSingleton::getInstance().getNetworkWrapper()->setClientState(States::Game, event.player.id);
	}

	return true;
}

void GameState::onStartKillCam(const StartKillCamEvent& event) {
	m_isInKillCamMode = true;

	const Netcode::PlayerID killer = Netcode::getComponentOwner(event.killingProjectile);

	SettingStorage& settings = m_app->getSettings();
	NWrapperSingleton& NW = NWrapperSingleton::getInstance();

	char killerTeam = NW.getPlayer(killer)->team;
	glm::vec3 col = settings.getColor(settings.teamColorIndex(killer));
	m_killerColor = ImVec4(col.r, col.g, col.b, 1.f);
	m_killCamKillerText = NW.getPlayer(killer)->name;


	if (event.finalKillCam) {
		m_isFinalKillCam = true;
		m_killCamTitle = "ROUND WINNING SPLASH";
		m_killCamVictimText = NW.getPlayer(event.deadPlayer)->name;

		const char victimTeam = NW.getPlayer(event.deadPlayer)->team;
		col = settings.getColor(settings.teamColorIndex(event.deadPlayer));
		m_victimColor = ImVec4(col.r, col.g, col.b, 1.f);
	} else {
		m_isFinalKillCam = false;
		m_killCamTitle = "SPLASHCAM";
	}

	m_isInKillCamMode = true;
}

void GameState::onStopKillCam(const StopKillCamEvent& event) {
	m_isInKillCamMode = false;
}

void GameState::onPlayerStateStatusChanged(const NetworkUpdateStateLoadStatus& event) {

	if (NWrapperSingleton::getInstance().isHost() && m_gameStarted) {

		if (event.stateID == States::Game && event.status > 0) {
			m_componentSystems.hostSendToSpectatorSystem->sendEntityCreationPackage(event.playerID);
		}

	}
}

bool GameState::update(float dt, float alpha) {
	NWrapperSingleton* ptr = &NWrapperSingleton::getInstance();
	NWrapperSingleton::getInstance().checkForPackages();

	m_killFeedWindow.updateTiming(dt);
	waitForOtherPlayers();

	// Don't update game if game have not started. This is to sync all players to start at the same time
	if (!m_gameStarted) {
		return true;
	}

	// UPDATE REAL TIME SYSTEMS
	updatePerFrameComponentSystems(dt, alpha);

	m_lights.updateBufferData();

	return true;
}

bool GameState::fixedUpdate(float dt) {
	std::wstring fpsStr = std::to_wstring(m_app->getFPS());

#ifdef DEVELOPMENT
	m_app->getWindow()->setWindowTitle("S.P.L.A.S.H2O | Development | "
		+ Application::getPlatformName() + " | FPS: " + std::to_string(m_app->getFPS()));
#endif

	static float counter = 0.0f;
	static float size = 1.0f;
	static float change = 0.4f;

	counter += dt * 2.0f;

	//Don't update game if game have not started. This is to sync all players to start at the same time
	if (!m_gameStarted) {
		return true;
	}

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

	// This should happen before updatePerTickComponentSystems(dt) so don't move this function call
	if (m_isInKillCamMode) {
		updatePerTickKillCamComponentSystems(dt);
	}

	updatePerTickComponentSystems(dt);

	return true;
}


// Renders the state
// alpha is a the interpolation value (range [0,1]) between the last two snapshots
bool GameState::render(float dt, float alpha) {
	static float killCamAlpha = 0.f;

	// Clear back buffer
	m_app->getAPI()->clear({ 0.01f, 0.01f, 0.01f, 1.0f });

	// Draw the scene. Entities with model and trans component will be rendered.
	m_componentSystems.beginEndFrameSystem->beginFrame(m_cam);

	if (m_isInKillCamMode) {
		killCamAlpha = m_componentSystems.killCamReceiverSystem->getKillCamAlpha(alpha);

		m_componentSystems.killCamModelSubmitSystem->submitAll(killCamAlpha);
		m_componentSystems.killCamMetaballSubmitSystem->submitAll(killCamAlpha);
	} else {
		m_componentSystems.modelSubmitSystem->submitAll(alpha);
		if (Application::getInstance()->getSettings().applicationSettingsStatic["graphics"]["particles"].getSelected().value > 0.0f) {
			m_componentSystems.particleSystem->submitAll();
		}
		m_componentSystems.metaballSubmitSystem->submitAll(alpha);
		m_componentSystems.boundingboxSubmitSystem->submitAll();
	}

	m_componentSystems.guiSubmitSystem->submitAll();
	m_componentSystems.beginEndFrameSystem->endFrameAndPresent();

	return true;
}

bool GameState::renderImgui(float dt) {
	m_playerNamesinGameGui.renderWindow();
	m_inGameGui.renderWindow();
	m_killFeedWindow.renderWindow();
	if (m_wasDropped) {
		m_wasDroppedWindow.renderWindow();
	}

	if (!m_gameStarted) {
		m_waitingForPlayersWindow.renderWindow();
	}

	//m_app->getChatWindow()->renderChat(dt);

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



	if (m_isInKillCamMode) {
		const unsigned int height = m_app->getWindow()->getWindowHeight() / 6;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.f, 0.f, 0.f, 0.7f));
		if (ImGui::Begin("##KILLCAMBACKGROUNDTOP", nullptr, m_backgroundOnlyflags)) {
			ImGui::SetWindowSize(ImVec2(m_app->getWindow()->getWindowWidth(), height));
			ImGui::SetWindowPos(ImVec2(0, 0));
		}
		ImGui::End();
		ImGui::PopStyleColor(1);
		ImGui::PopStyleVar(1);


		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.f, 0.f, 0.f, 0.7f));
		if (ImGui::Begin("##KILLCAMBACKGROUNDBOTTOM", nullptr, m_backgroundOnlyflags)) {

			ImGui::SetWindowSize(ImVec2(m_app->getWindow()->getWindowWidth(), height));
			ImGui::SetWindowPos(ImVec2(0, m_app->getWindow()->getWindowHeight() - height));
		}
		ImGui::End();
		ImGui::PopStyleColor(1);
		ImGui::PopStyleVar(1);

		if (ImGui::Begin("##KILLCAMWINDOW", nullptr, m_standaloneButtonflags)) {
			//ImGui::PushFont(m_app->getImGuiHandler()->getFont("Beb70"));
			ImGui::SetWindowFontScale(m_imguiHandler->getFontScaling("BigHeader"));

			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.f));
			ImGui::Text(m_killCamTitle.c_str());
			ImGui::PopStyleColor(1);
			ImGui::SetWindowPos(ImVec2(m_app->getWindow()->getWindowWidth() * 0.5f - ImGui::GetWindowSize().x * 0.5f, height / 2 - (ImGui::GetWindowSize().y / 2)));
			//ImGui::PopFont();
		}
		ImGui::End();


		const ImVec4 textColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
		if (ImGui::Begin("##KILLCAMKILLEDBY", nullptr, m_standaloneButtonflags)) {
			ImGui::SetWindowFontScale(m_imguiHandler->getFontScaling("text"));
			//ImGui::PushFont(m_app->getImGuiHandler()->getFont("Beb30"));


			if (m_isFinalKillCam) {
				ImGui::PushStyleColor(ImGuiCol_Text, m_killerColor);
				ImGui::Text(m_killCamKillerText.c_str());
				ImGui::PopStyleColor(1);
				ImGui::SameLine();
				ImGui::PushStyleColor(ImGuiCol_Text, textColor);
				ImGui::Text("eliminated");
				ImGui::PopStyleColor(1);
				ImGui::SameLine();
				ImGui::PushStyleColor(ImGuiCol_Text, m_victimColor);
				ImGui::Text(m_killCamVictimText.c_str());
				ImGui::PopStyleColor(1);
				ImGui::SameLine();
				ImGui::PushStyleColor(ImGuiCol_Text, textColor);
				ImGui::Text("and won the round!");
				ImGui::PopStyleColor(1);
			} else {
				ImGui::PushStyleColor(ImGuiCol_Text, textColor);
				ImGui::Text("You were eliminated by");
				ImGui::PopStyleColor(1);
				ImGui::SameLine();
				ImGui::PushStyleColor(ImGuiCol_Text, m_killerColor);
				ImGui::Text(m_killCamKillerText.c_str());
				ImGui::PopStyleColor(1);
			}
			
			ImGui::SetWindowPos(ImVec2(m_app->getWindow()->getWindowWidth() * 0.5f - ImGui::GetWindowSize().x * 0.5f, m_app->getWindow()->getWindowHeight() - height / 2 - (ImGui::GetWindowSize().y / 2)));
			//ImGui::PopFont();
		}
		ImGui::End();
	}


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
void GameState::updatePerTickKillCamComponentSystems(float dt) {
	if (m_componentSystems.killCamReceiverSystem->skipUpdate()) {
		return;
	}

	m_componentSystems.killCamReceiverSystem->prepareUpdate();
	m_componentSystems.killCamLightSystem->prepareFixedUpdate();

	m_componentSystems.killCamReceiverSystem->processReplayData(dt);
	m_componentSystems.killCamMovementSystem->update(dt);
	m_componentSystems.killCamCollisionSystem->update(dt);
	m_componentSystems.killCamMovementPostCollisionSystem->update(dt);
	m_componentSystems.killCamOctreeAddRemoverSystem->update(dt);
}

// HERE BE DRAGONS
// Make sure things are updated in the correct order or things will behave strangely
void GameState::updatePerTickComponentSystems(float dt) {
	
	if (!m_player->getComponent<SpectatorComponent>() || m_isInKillCamMode) {
		m_playerNamesinGameGui.setMaxDistance(10);

		Octree::RayIntersectionInfo tempInfo;
		Octree* octree = m_isInKillCamMode ? m_killCamOctree : m_octree;
		octree->getRayIntersection(m_cam.getPosition(), m_cam.getDirection(), &tempInfo, m_player, 0.0, true);
		
		if (tempInfo.closestHitIndex != -1) {
			Netcode::PlayerID owner = Netcode::UNINITIALIZED_PLAYER;

			if (m_isInKillCamMode) {
				ReplayReceiverComponent* rrc = tempInfo.info[tempInfo.closestHitIndex].entity->getComponent<ReplayReceiverComponent>();
				if (rrc) {
					owner = Netcode::getComponentOwner(rrc->m_id);
				}
			} else {
				NetworkReceiverComponent* nrc = tempInfo.info[tempInfo.closestHitIndex].entity->getComponent<NetworkReceiverComponent>();
				if (nrc) {
					owner = Netcode::getComponentOwner(nrc->m_id);
				}
			}

			if (owner != Netcode::UNINITIALIZED_PLAYER) {
				CandleComponent* candleComp = tempInfo.info[tempInfo.closestHitIndex].entity->getComponent<CandleComponent>();
				Player* p = NWrapperSingleton::getInstance().getPlayer(owner);
				if (p) {
					m_playerNamesinGameGui.addPlayerToDraw(tempInfo.info[tempInfo.closestHitIndex].entity);
				}
			}
		}
	} else {
		m_playerNamesinGameGui.setMaxDistance(-1);
		for (auto p : *m_componentSystems.playerSystem->getPlayers()) {
			m_playerNamesinGameGui.addPlayerToDraw(p);
		}
	}
	
	///////////////////////////////////////
	m_currentlyReadingMask = 0;
	m_currentlyWritingMask = 0;
	m_runningSystemJobs.clear();
	m_runningSystems.clear();

	m_componentSystems.gameInputSystem->fixedUpdate(dt);
	m_componentSystems.spectateInputSystem->fixedUpdate(dt);

	m_componentSystems.prepareUpdateSystem->fixedUpdate(); // HAS TO BE RUN BEFORE OTHER SYSTEMS WHICH USE TRANSFORM
	m_componentSystems.lightSystem->prepareFixedUpdate();


	// Update entities with info from the network and from ourself
	// DON'T MOVE, should happen at the start of each tick
	m_componentSystems.networkReceiverSystem->update(dt);
	m_componentSystems.killCamReceiverSystem->update(dt); // This just increments the killcam's ringbuffer.

	m_componentSystems.movementSystem->update(dt);
	m_componentSystems.speedLimitSystem->update();
	m_componentSystems.collisionSystem->update(dt);
	m_componentSystems.movementPostCollisionSystem->update(dt);
	m_componentSystems.powerUpUpdateSystem->update(dt);
	m_componentSystems.powerUpCollectibleSystem->update(dt);
	// TODO: Investigate this
	// Systems sent to runSystem() need to override the update(float dt) in BaseComponentSystem
	if (NWrapperSingleton::getInstance().isHost() && m_app->getSettings().gameSettingsStatic["map"]["bots"].getSelected().value == 0.f) {
		runSystem(dt, m_componentSystems.aiSystem);
	}
	runSystem(dt, m_componentSystems.projectileSystem);
	runSystem(dt, m_componentSystems.animationChangerSystem);
	runSystem(dt, m_componentSystems.sprinklerSystem);
	runSystem(dt, m_componentSystems.candleThrowingSystem);
	runSystem(dt, m_componentSystems.candleHealthSystem);
	runSystem(dt, m_componentSystems.candlePlacementSystem);
	runSystem(dt, m_componentSystems.candleReignitionSystem);
	runSystem(dt, m_componentSystems.updateBoundingBoxSystem);
	runSystem(dt, m_componentSystems.gunSystem); // Run after animationSystem to make shots more in sync
	runSystem(dt, m_componentSystems.lifeTimeSystem);
	runSystem(dt, m_componentSystems.teamColorSystem);

	auto& particleSettingSelectedValue = Application::getInstance()->getSettings().applicationSettingsStatic["graphics"]["particles"].getSelected().value;
	if (particleSettingSelectedValue > 0.0f && !m_componentSystems.particleSystem->isEnabled()) {
		// Enable the particle system if previously disabled
		m_componentSystems.particleSystem->setEnabled(true);
	} else if (particleSettingSelectedValue <= 0.0f && m_componentSystems.particleSystem->isEnabled()) {
		//Disable particle system if previously enabled
		m_componentSystems.particleSystem->setEnabled(false);
	}

	runSystem(dt, m_componentSystems.particleSystem);

	runSystem(dt, m_componentSystems.sanitySystem);
	runSystem(dt, m_componentSystems.sanitySoundSystem);
	runSystem(dt, m_componentSystems.waterCleaningSystem);

	// Wait for all the systems to finish before starting the removal system
	for (auto& fut : m_runningSystemJobs) {
		fut.get();
	}
	m_componentSystems.hazardLightSystem->enableHazardLights(m_componentSystems.sprinklerSystem->getActiveRooms());

	// Send out your entity info to the rest of the players
	// DON'T MOVE, should happen at the end of each tick
	m_componentSystems.networkSenderSystem->update();
	m_componentSystems.octreeAddRemoverSystem->update(dt);
}

void GameState::updatePerFrameComponentSystems(float dt, float alpha) {
	const float killCamAlpha = m_componentSystems.killCamReceiverSystem->getKillCamAlpha(alpha);
	const float killCamDelta = m_componentSystems.killCamReceiverSystem->getKillCamDelta(dt);

	// TODO? move to its own thread

	m_cam.newFrame(); // Has to run before the camera update
	m_componentSystems.prepareUpdateSystem->update(); // HAS TO BE RUN BEFORE OTHER SYSTEMS WHICH USE TRANSFORM

	m_componentSystems.sprintingSystem->update(dt, alpha);

	m_componentSystems.gameInputSystem->processMouseInput(dt);
	if (m_isInKillCamMode) {
		m_componentSystems.killCamAnimationSystem->update(killCamDelta);
		m_componentSystems.killCamAnimationSystem->updatePerFrame();
	} else {
		m_componentSystems.animationSystem->update(dt);
		m_componentSystems.animationSystem->updatePerFrame();
	}
	// Updates keyboard/mouse input and the camera
	m_componentSystems.gameInputSystem->updateCameraPosition(alpha);
	m_componentSystems.spectateInputSystem->update(dt, alpha);


	// There is an imgui debug toggle to override lights
	if (!m_lightDebugWindow.isManualOverrideOn()) {
		m_lights.clearPointLights();
		//check and update all lights for all entities
		if (m_isInKillCamMode) {
			m_componentSystems.killCamLightSystem->updateLights(&m_lights, killCamAlpha);
		} else {
			m_componentSystems.lightSystem->updateLights(&m_lights, alpha);
		}
		m_componentSystems.lightListSystem->updateLights(&m_lights);
		m_componentSystems.hazardLightSystem->updateLights(&m_lights, alpha, dt);
	}

	m_componentSystems.crosshairSystem->update(dt);

	if (m_showcaseProcGen) {
		m_cam.setPosition(glm::vec3(100.f, 100.f, 100.f));
	}
	
	if (m_componentSystems.audioSystem) {
		m_componentSystems.audioSystem->update(m_cam, dt, alpha);
	}
	else if(!m_componentSystems.audioSystem && m_app->getSettings().applicationSettingsDynamic["sound"]["global"].value > 0.0f) {
		if (!ECS::Instance()->getSystem<AudioSystem>()) {
			m_app->startAudio();
		}
		m_componentSystems.audioSystem = ECS::Instance()->getSystem<AudioSystem>();
		m_readyRestartAmbiance = true;
		m_ambiance->getComponent<AudioComponent>()->streamSoundRequest_HELPERFUNC("res/sounds/ambient/ambiance_lab.xwb", true, Application::getInstance()->getSettings().applicationSettingsDynamic["sound"]["global"].value, false, true);

		m_ambiance->getComponent<AudioComponent>()->streamSetVolume_HELPERFUNC("res/sounds/ambient/ambiance_lab.xwb", Application::getInstance()->getSettings().applicationSettingsDynamic["sound"]["global"].value);
	}
	m_componentSystems.octreeAddRemoverSystem->updatePerFrame(dt);

	if (m_isInKillCamMode) {
		m_componentSystems.killCamReceiverSystem->updatePerFrame(dt, killCamAlpha);
	}

	// Will probably need to be called last
	m_playerNamesinGameGui.update(dt);
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

void GameState::waitForOtherPlayers() {
	MatchRecordSystem* mrs = NWrapperSingleton::getInstance().recordSystem;
	if (NWrapperSingleton::getInstance().isHost() || (mrs && mrs->status == 2)) {
		if (!m_gameStarted) {
			bool allReady = true;

			//See if anyone is not ready
			//TODO: maybe dont count in spectators here.
			//TODO: what will happen if new players joins in gamestate but before gamestart?
			for (auto p : NWrapperSingleton::getInstance().getPlayers()) {
				if ((p.lastStateStatus.status != 1 || p.lastStateStatus.state != States::Game) && p.lastStateStatus.status != -1) {
					allReady = false;
					break;
				}
			}

			//If all players are ready, host can give approval to start game and set m_gameStarted = true
			if (allReady) {
				m_gameStarted = true;
				NWrapperSingleton::getInstance().getNetworkWrapper()->updateStateLoadStatus(States::Game, 2);
			}
		}
	} else {
		if (!m_gameStarted) {
			//If Host have given approval to start game, set m_gameStarted = true
			auto p = NWrapperSingleton::getInstance().getPlayer(0);
			if (p->lastStateStatus.status != -1) {
				if (p->lastStateStatus.status == 2 && p->lastStateStatus.state == States::Game) {
					m_gameStarted = true;
				}
			} else {
				//If in replay match mode, just look for anyone informing that the game should start. since player 0 is not the "host" from the clients perspective
				for (auto p : NWrapperSingleton::getInstance().getPlayers()) {
					if (p.lastStateStatus.state == States::Game && p.lastStateStatus.status == 2) {
						m_gameStarted = true;
						break;
					}
				}
			}
		}
	}
}

const std::string GameState::createCube(const glm::vec3& position) {
	Model* tmpCubeModel = &m_app->getResourceManager().getModel(
		"cubeWidth1", &m_app->getResourceManager().getShaderSet<GBufferOutShader>());
	tmpCubeModel->getMesh(0)->getMaterial()->setColor(glm::vec4(0.2f, 0.8f, 0.4f, 1.0f));

	Model* tmpbbModel = &m_app->getResourceManager().getModel(
		"boundingBox", &m_app->getResourceManager().getShaderSet<GBufferWireframe>());
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

void GameState::createBots() {
	auto botCount = static_cast<int>(m_app->getSettings().gameSettingsDynamic["bots"]["count"].value);

	for (size_t i = 0; i < botCount; i++) {
		glm::vec3 spawnLocation = m_componentSystems.levelSystem->getBotSpawnPoint(i);
		if (spawnLocation.x != -1000.f) {
			auto compID = Netcode::generateUniqueBotID();
			if (NWrapperSingleton::getInstance().isHost()) {
				EntityFactory::CreateCleaningBotHost(spawnLocation, m_componentSystems.aiSystem->getNodeSystem(), compID);
			} else {
				EntityFactory::CreateCleaningBot(spawnLocation, compID);
			}
		}
		else {
			SAIL_LOG_ERROR("Bot not spawned because all spawn points are already used for this map.");
		}
	}
}

void GameState::createLevel(Shader* shader, Model* boundingBoxModel) {
	std::vector<Model*> tileModels;
	std::vector<Model*> clutterModels;

	//Load tileset for world
	{
		Model* roomWall = &m_app->getResourceManager().getModel("Tiles/RoomWall", shader);
		roomWall->getMesh(0)->getMaterial()->setMetalnessRoughnessAOTexture("pbr/DDS/Tiles/RoomWallMRAO.dds");
		roomWall->getMesh(0)->getMaterial()->setNormalTexture("pbr/DDS/Tiles/RoomWallNM.dds");
		roomWall->getMesh(0)->getMaterial()->setAlbedoTexture("pbr/DDS/Tiles/RoomWallAlbedo.dds");

		Model* roomDoor = &m_app->getResourceManager().getModel("Tiles/RoomDoor", shader);
		roomDoor->getMesh(0)->getMaterial()->setMetalnessRoughnessAOTexture("pbr/DDS/Tiles/RD_MRAo.dds");
		roomDoor->getMesh(0)->getMaterial()->setNormalTexture("pbr/DDS/Tiles/RD_NM.dds");
		roomDoor->getMesh(0)->getMaterial()->setAlbedoTexture("pbr/DDS/Tiles/RD_Albedo.dds");

		Model* corridorDoor = &m_app->getResourceManager().getModel("Tiles/CorridorDoor", shader);
		corridorDoor->getMesh(0)->getMaterial()->setMetalnessRoughnessAOTexture("pbr/DDS/Tiles/CD_MRAo.dds");
		corridorDoor->getMesh(0)->getMaterial()->setNormalTexture("pbr/DDS/Tiles/CD_NM.dds");
		corridorDoor->getMesh(0)->getMaterial()->setAlbedoTexture("pbr/DDS/Tiles/CD_Albedo.dds");

		Model* corridorWall = &m_app->getResourceManager().getModel("Tiles/CorridorWall", shader);
		corridorWall->getMesh(0)->getMaterial()->setMetalnessRoughnessAOTexture("pbr/DDS/Tiles/CW_MRAo.dds");
		corridorWall->getMesh(0)->getMaterial()->setNormalTexture("pbr/DDS/Tiles/CW_NM.dds");
		corridorWall->getMesh(0)->getMaterial()->setAlbedoTexture("pbr/DDS/Tiles/CW_Albedo.dds");

		Model* roomCeiling = &m_app->getResourceManager().getModel("Tiles/RoomCeiling", shader);
		roomCeiling->getMesh(0)->getMaterial()->setMetalnessRoughnessAOTexture("pbr/DDS/Tiles/RC_MRAo.dds");
		roomCeiling->getMesh(0)->getMaterial()->setNormalTexture("pbr/DDS/Tiles/RC_NM.dds");
		roomCeiling->getMesh(0)->getMaterial()->setAlbedoTexture("pbr/DDS/Tiles/RC_Albedo.dds");

		Model* roomServer = &m_app->getResourceManager().getModelCopy("Tiles/RoomWall", shader);
		roomServer->getMesh(0)->getMaterial()->setMetalnessRoughnessAOTexture("pbr/DDS/Tiles/RS_MRAo.dds");
		roomServer->getMesh(0)->getMaterial()->setNormalTexture("pbr/DDS/Tiles/RS_NM.dds");
		roomServer->getMesh(0)->getMaterial()->setAlbedoTexture("pbr/DDS/Tiles/RS_Albedo.dds");

		Model* corridorFloor = &m_app->getResourceManager().getModel("Tiles/RoomFloor", shader);
		corridorFloor->getMesh(0)->getMaterial()->setMetalnessRoughnessAOTexture("pbr/DDS/Tiles/CF_MRAo.dds");
		corridorFloor->getMesh(0)->getMaterial()->setNormalTexture("pbr/DDS/Tiles/CF_NM.dds");
		corridorFloor->getMesh(0)->getMaterial()->setAlbedoTexture("pbr/DDS/Tiles/CF_Albedo.dds");

		Model* roomFloor = &m_app->getResourceManager().getModelCopy("Tiles/RoomFloor", shader);
		roomFloor->getMesh(0)->getMaterial()->setMetalnessRoughnessAOTexture("pbr/DDS/Tiles/F_MRAo.dds");
		roomFloor->getMesh(0)->getMaterial()->setNormalTexture("pbr/DDS/Tiles/F_NM.dds");
		roomFloor->getMesh(0)->getMaterial()->setAlbedoTexture("pbr/DDS/Tiles/F_Albedo.dds");

		Model* corridorCeiling = &m_app->getResourceManager().getModelCopy("Tiles/RoomCeiling", shader);
		corridorCeiling->getMesh(0)->getMaterial()->setMetalnessRoughnessAOTexture("pbr/DDS/Tiles/CC_MRAo.dds");
		corridorCeiling->getMesh(0)->getMaterial()->setNormalTexture("pbr/DDS/Tiles/CC_NM.dds");
		corridorCeiling->getMesh(0)->getMaterial()->setAlbedoTexture("pbr/DDS/Tiles/CC_Albedo.dds");

		Model* corridorCorner = &m_app->getResourceManager().getModel("Tiles/CorridorCorner", shader);
		corridorCorner->getMesh(0)->getMaterial()->setMetalnessRoughnessAOTexture("pbr/DDS/Tiles/Corner_MRAo.dds");
		corridorCorner->getMesh(0)->getMaterial()->setNormalTexture("pbr/DDS/Tiles/Corner_NM.dds");
		corridorCorner->getMesh(0)->getMaterial()->setAlbedoTexture("pbr/DDS/Tiles/Corner_Albedo.dds");

		Model* roomCorner = &m_app->getResourceManager().getModel("Tiles/RoomCorner", shader);
		roomCorner->getMesh(0)->getMaterial()->setMetalnessRoughnessAOTexture("pbr/DDS/Tiles/Corner_MRAo.dds");
		roomCorner->getMesh(0)->getMaterial()->setNormalTexture("pbr/DDS/Tiles/Corner_NM.dds");
		roomCorner->getMesh(0)->getMaterial()->setAlbedoTexture("pbr/DDS/Tiles/Corner_Albedo.dds");

		Model* cTable = &m_app->getResourceManager().getModel("Clutter/Table", shader);
		cTable->getMesh(0)->getMaterial()->setMetalnessRoughnessAOTexture("pbr/DDS/Clutter/Table_MRAO.dds");
		cTable->getMesh(0)->getMaterial()->setNormalTexture("pbr/DDS/Clutter/Table_NM.dds");
		cTable->getMesh(0)->getMaterial()->setAlbedoTexture("pbr/DDS/Clutter/Table_Albedo.dds");

		Model* cBoxes = &m_app->getResourceManager().getModel("Clutter/Boxes", shader);
		cBoxes->getMesh(0)->getMaterial()->setMetalnessRoughnessAOTexture("pbr/DDS/Clutter/Boxes_MRAO.dds");
		cBoxes->getMesh(0)->getMaterial()->setNormalTexture("pbr/DDS/Clutter/Boxes_NM.dds");
		cBoxes->getMesh(0)->getMaterial()->setAlbedoTexture("pbr/DDS/Clutter/Boxes_Albedo.dds");

		Model* cMediumBox = &m_app->getResourceManager().getModel("Clutter/MediumBox", shader);
		cMediumBox->getMesh(0)->getMaterial()->setMetalnessRoughnessAOTexture("pbr/DDS/Clutter/MediumBox_MRAO.dds");
		cMediumBox->getMesh(0)->getMaterial()->setNormalTexture("pbr/DDS/Clutter/MediumBox_NM.dds");
		cMediumBox->getMesh(0)->getMaterial()->setAlbedoTexture("pbr/DDS/Clutter/MediumBox_Albedo.dds");

		Model* cSquareBox = &m_app->getResourceManager().getModel("Clutter/SquareBox", shader);
		cSquareBox->getMesh(0)->getMaterial()->setMetalnessRoughnessAOTexture("pbr/DDS/Clutter/SquareBox_MRAO.dds");
		cSquareBox->getMesh(0)->getMaterial()->setNormalTexture("pbr/DDS/Clutter/SquareBox_NM.dds");
		cSquareBox->getMesh(0)->getMaterial()->setAlbedoTexture("pbr/DDS/Clutter/SquareBox_Albedo.dds");

		Model* cBooks1 = &m_app->getResourceManager().getModel("Clutter/Books1", shader);
		cBooks1->getMesh(0)->getMaterial()->setMetalnessRoughnessAOTexture("pbr/DDS/Clutter/Book_MRAO.dds");
		cBooks1->getMesh(0)->getMaterial()->setNormalTexture("pbr/DDS/Clutter/Book_NM.dds");
		cBooks1->getMesh(0)->getMaterial()->setAlbedoTexture("pbr/DDS/Clutter/Book1_Albedo.dds");

		Model* cBooks2 = &m_app->getResourceManager().getModelCopy("Clutter/Books1", shader);
		cBooks2->getMesh(0)->getMaterial()->setMetalnessRoughnessAOTexture("pbr/DDS/Clutter/Book_MRAO.dds");
		cBooks2->getMesh(0)->getMaterial()->setNormalTexture("pbr/DDS/Clutter/Book_NM.dds");
		cBooks2->getMesh(0)->getMaterial()->setAlbedoTexture("pbr/DDS/Clutter/Book2_Albedo.dds");

		Model* cScreen = &m_app->getResourceManager().getModel("Clutter/Screen", shader);
		cScreen->getMesh(0)->getMaterial()->setMetalnessRoughnessAOTexture("pbr/DDS/Clutter/Screen_MRAO.dds");
		cScreen->getMesh(0)->getMaterial()->setNormalTexture("pbr/DDS/Clutter/Screen_NM.dds");
		cScreen->getMesh(0)->getMaterial()->setAlbedoTexture("pbr/DDS/Clutter/Screen_Albedo.dds");

		Model* cNotepad = &m_app->getResourceManager().getModel("Clutter/Notepad", shader);
		cNotepad->getMesh(0)->getMaterial()->setMetalnessRoughnessAOTexture("pbr/DDS/Clutter/Notepad_MRAO.dds");
		cNotepad->getMesh(0)->getMaterial()->setNormalTexture("pbr/DDS/Clutter/Notepad_NM.dds");
		cNotepad->getMesh(0)->getMaterial()->setAlbedoTexture("pbr/DDS/Clutter/Notepad_Albedo.dds");

		Model* cMicroscope= &m_app->getResourceManager().getModel("Clutter/Microscope", shader);
		cMicroscope->getMesh(0)->getMaterial()->setMetalnessRoughnessAOTexture("pbr/DDS/Clutter/Microscope_MRAO.dds");
		cMicroscope->getMesh(0)->getMaterial()->setNormalTexture("pbr/DDS/Clutter/Microscope_NM.dds");
		cMicroscope->getMesh(0)->getMaterial()->setAlbedoTexture("pbr/DDS/Clutter/Microscope_Albedo.dds");

		Model* saftblandare = &m_app->getResourceManager().getModel("Clutter/Saftblandare", shader);
		saftblandare->getMesh(0)->getMaterial()->setMetalnessRoughnessAOTexture("pbr/DDS/Clutter/Saftblandare_MRAO.dds");
		saftblandare->getMesh(0)->getMaterial()->setNormalTexture("pbr/DDS/Clutter/Saftblandare_NM.dds");
		saftblandare->getMesh(0)->getMaterial()->setAlbedoTexture("pbr/DDS/Clutter/Saftblandare_Albedo.dds");

		Model* cloningVats = &m_app->getResourceManager().getModel("Clutter/CloningVats", shader);
		cloningVats->getMesh(0)->getMaterial()->setMetalnessRoughnessAOTexture("pbr/DDS/Clutter/CloningVats_MRAO.dds");
		cloningVats->getMesh(0)->getMaterial()->setNormalTexture("pbr/DDS/Clutter/CloningVats_NM.dds");
		cloningVats->getMesh(0)->getMaterial()->setAlbedoTexture("pbr/DDS/Clutter/CloningVats_Albedo.dds");

		Model* controlStation = &m_app->getResourceManager().getModel("Clutter/ControlStation", shader);
		controlStation->getMesh(0)->getMaterial()->setMetalnessRoughnessAOTexture("pbr/DDS/Clutter/ControlStation_MRAO.dds");
		controlStation->getMesh(0)->getMaterial()->setNormalTexture("pbr/DDS/Clutter/ControlStation_NM.dds");
		controlStation->getMesh(0)->getMaterial()->setAlbedoTexture("pbr/DDS/Clutter/ControlStation_Albedo.dds");


		tileModels.resize(TileModel::NUMBOFMODELS);
		tileModels[TileModel::ROOM_FLOOR] = roomFloor;
		tileModels[TileModel::ROOM_WALL] = roomWall;
		tileModels[TileModel::ROOM_DOOR] = roomDoor;
		tileModels[TileModel::ROOM_CEILING] = roomCeiling;
		tileModels[TileModel::ROOM_CORNER] = roomCorner;
		tileModels[TileModel::ROOM_SERVER] = roomServer;


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
		clutterModels[ClutterModel::CLONINGVATS] = cloningVats;
		clutterModels[ClutterModel::CONTROLSTATION] = controlStation;
	}

	// Create the level generator system and put it into the datatype.
	SettingStorage& settings = m_app->getSettings();
	m_componentSystems.levelSystem->destroyWorld();

	m_componentSystems.levelSystem->seed = settings.gameSettingsDynamic["map"]["seed"].value;
	m_componentSystems.levelSystem->clutterModifier = settings.gameSettingsDynamic["map"]["clutter"].value * 100;
	m_componentSystems.levelSystem->xsize = settings.gameSettingsDynamic["map"]["sizeX"].value;
	m_componentSystems.levelSystem->ysize = settings.gameSettingsDynamic["map"]["sizeY"].value;

	m_componentSystems.levelSystem->generateMap();
	m_componentSystems.levelSystem->createWorld(tileModels, boundingBoxModel);
	m_componentSystems.levelSystem->addClutterModel(clutterModels, boundingBoxModel);
	m_componentSystems.gameInputSystem->m_mapPointer = m_componentSystems.levelSystem;

	// SPAWN POWERUPS IF ENABLED
	if (settings.gameSettingsStatic["map"]["Powerup"].getSelected().value == 0.0f) {


		m_componentSystems.powerUpCollectibleSystem->setSpawnPoints(m_componentSystems.levelSystem->powerUpSpawnPoints);
		m_componentSystems.powerUpCollectibleSystem->
			setDuration(settings.gameSettingsDynamic["powerup"]["duration"].value);
		m_componentSystems.powerUpCollectibleSystem->setRespawnTime(settings.gameSettingsDynamic["powerup"]["respawnTime"].value);
		if (NWrapperSingleton::getInstance().isHost()) {
			m_componentSystems.powerUpCollectibleSystem->spawnPowerUps(settings.gameSettingsDynamic["powerup"]["count"].value);
		}
	}




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
