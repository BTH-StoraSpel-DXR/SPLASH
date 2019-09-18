#include "GameState.h"
#include "imgui.h"
#include "..//Sail/src/Sail/entities/systems/physics/PhysicSystem.h"
#include "..//Sail/src/Sail/entities/ECS.h"
#include <sstream>
#include <iomanip>
#include "Network/NetworkWrapper.h"
#include "../SPLASH/src/game/events/NetworkTestEvent.h"

GameState::GameState(StateStack& stack)
: State(stack)
//, m_cam(20.f, 20.f, 0.1f, 5000.f)
, m_cam(90.f, 1280.f / 720.f, 0.1f, 5000.f)
//, m_camController(&m_cam)
, m_playerController(&m_cam, &m_scene)
, m_cc(true)
, m_profiler(true)
{
#ifdef _DEBUG
#pragma region TESTCASES
	m_cc.addCommand(std::string("Save"),			[&]() { return std::string("saved"); });
	m_cc.addCommand(std::string("Test <int>"),		[&](int in) { return std::string("test<int>"); });
	m_cc.addCommand(std::string("Test <float>"),	[&](float in) { return std::string("test<float>"); });
	m_cc.addCommand(std::string("Test <string>"),	[&](std::string in) { return std::string("test<string>"); });
	m_cc.addCommand(std::string("Test <int> <int> <int>"/*...*/), [&](std::vector<int> in) {return std::string("test<std::vector<int>"); });
	m_cc.addCommand(std::string("Test <float> <float> <float>"/*...*/), [&](std::vector<float> in) {return std::string("test<std::vector<float>"); });
#pragma endregion


	m_cc.addCommand(std::string("AddCube"), [&]() {
		auto e = ECS::Instance()->createEntity("new cube");
		e->addComponent<ModelComponent>(m_cubeModel.get());
		e->addComponent<TransformComponent>(m_cam.getPosition());

		m_scene.addEntity(e);
		return std::string("Added Cube at (" + std::to_string(m_cam.getPosition().x) + ":" + std::to_string(m_cam.getPosition().y) + ":" + std::to_string(m_cam.getPosition().z) + ")");
		});
	m_cc.addCommand(std::string("AddCube <int> <int> <int>"), [&](std::vector<int> in) {
		if (in.size() == 3) {
			glm::vec3 pos(in[0], in[1], in[2]);
			return createCube(pos);
		}
		else {
			return std::string("Error: wrong number of inputs. Console Broken");
		}
		return std::string("wat");
	});
	m_cc.addCommand(std::string("AddCube <float> <float> <float>"), [&](std::vector<float> in){
		if (in.size() == 3) {
			glm::vec3 pos(in[0], in[1], in[2]);
			return createCube(pos);
		}
		else {
			return std::string("Error: wrong number of inputs. Console Broken");
		}
		return std::string("wat");
	});
#endif
	

	/*
		Create a PhysicSystem
		If the game developer does not want to add the systems like this,
		this call could be moved inside the default constructor of ECS,
		assuming each system is included in ECS.cpp instead of here
	*/
	m_componentSystems.physicSystem = ECS::Instance()->createSystem<PhysicSystem>();

	// This was moved out from the PlayerController constructor
	// since the PhysicSystem needs to be created first
	// (or the PhysicsComponent needed to be detached and reattached
	m_playerController.getEntity()->addComponent<PhysicsComponent>();
	// Get the Application instance
	m_app = Application::getInstance();
	//m_scene = std::make_unique<Scene>(AABB(glm::vec3(-100.f, -100.f, -100.f), glm::vec3(100.f, 100.f, 100.f)));

	// Textures needs to be loaded before they can be used
	// TODO: automatically load textures when needed so the following can be removed
	Application::getInstance()->getResourceManager().loadTexture("sponza/textures/spnza_bricks_a_ddn.tga");
	Application::getInstance()->getResourceManager().loadTexture("sponza/textures/spnza_bricks_a_diff.tga");
	Application::getInstance()->getResourceManager().loadTexture("sponza/textures/spnza_bricks_a_spec.tga");
	Application::getInstance()->getResourceManager().loadTexture("sponza/textures/arenaBasicTexture.tga");
	Application::getInstance()->getResourceManager().loadTexture("sponza/textures/barrierBasicTexture.tga");
	Application::getInstance()->getResourceManager().loadTexture("sponza/textures/containerBasicTexture.tga");
	Application::getInstance()->getResourceManager().loadTexture("sponza/textures/rampBasicTexture.tga");
	//Application::getInstance()->getResourceManager().loadTexture("sponza/textures/boxOrientationTexture.tga");
	Application::getInstance()->getResourceManager().loadTexture("sponza/textures/candleBasicTexture.tga");
	Application::getInstance()->getResourceManager().loadTexture("sponza/textures/character1texture.tga");



	// Set up camera with controllers
	m_cam.setPosition(glm::vec3(1.6f, 4.7f, 7.4f));
	//m_camController.lookAt(glm::vec3(0.f));
	m_cam.lookAt(glm::vec3(0.f));
	m_playerController.getEntity()->getComponent<TransformComponent>()->setTranslation(glm::vec3(1.6f, 4.7f, 7.4f));
	
	// Add a directional light
	glm::vec3 color(0.1f, 0.1f, 0.1f);
 	glm::vec3 direction(0.4f, -0.2f, 1.0f);
	direction = glm::normalize(direction);
	//m_lights.setDirectionalLight(DirectionalLight(color, direction));
	// Add four point lights
	{
		//PointLight pl;
		//pl.setAttenuation(.0f, 0.1f, 0.02f);
		//pl.setColor(glm::vec3(1.f, 1.f, 1.f));
		//pl.setPosition(glm::vec3(3.0f, 3.1f, 3.0f));
		//pl.setIndex(0);
		//m_lights.addPointLight(pl);

		//pl.setColor(glm::vec3(1.f,1.f,1.f));
		//pl.setPosition(glm::vec3(1.0f, 3.1f, 1.0f));
		//pl.setIndex(1);
		//m_lights.addPointLight(pl);


		//pl.setColor(glm::vec3(Utils::rnd(), Utils::rnd(), Utils::rnd()));
		//pl.setPosition(glm::vec3(-4.0f, 0.1f, 4.0f));
		//m_lights.addPointLight(pl);

		//pl.setColor(glm::vec3(Utils::rnd(), Utils::rnd(), Utils::rnd()));
		//pl.setPosition(glm::vec3(4.0f, 0.1f, 4.0f));
		//m_lights.addPointLight(pl);

		//pl.setColor(glm::vec3(Utils::rnd(), Utils::rnd(), Utils::rnd()));
		//pl.setPosition(glm::vec3(4.0f, 0.1f, -4.0f));
		//m_lights.addPointLight(pl);
	}

	// Set up the scene
	//m_scene->addSkybox(L"skybox_space_512.dds"); //TODO
	m_scene.setLightSetup(&m_lights);

	// Disable culling for testing purposes
	m_app->getAPI()->setFaceCulling(GraphicsAPI::NO_CULLING);

	auto* shader = &m_app->getResourceManager().getShaderSet<MaterialShader>();

	// Create/load models
	m_cubeModel = ModelFactory::CubeModel::Create(glm::vec3(0.5f), shader);
	m_cubeModel->getMesh(0)->getMaterial()->setColor(glm::vec4(0.2f, 0.8f, 0.4f, 1.0f));
	m_planeModel = ModelFactory::PlaneModel::Create(glm::vec2(50.f), shader, glm::vec2(3.0f));
	m_planeModel->getMesh(0)->getMaterial()->setDiffuseTexture("sponza/textures/spnza_bricks_a_diff.tga");
	m_planeModel->getMesh(0)->getMaterial()->setNormalTexture("sponza/textures/spnza_bricks_a_ddn.tga");
	m_planeModel->getMesh(0)->getMaterial()->setSpecularTexture("sponza/textures/spnza_bricks_a_spec.tga");
	
	Model* fbxModel = &m_app->getResourceManager().getModel("sphere.fbx", shader);
	fbxModel->getMesh(0)->getMaterial()->setDiffuseTexture("sponza/textures/spnza_bricks_a_diff.tga");
	fbxModel->getMesh(0)->getMaterial()->setNormalTexture("sponza/textures/spnza_bricks_a_ddn.tga");
	fbxModel->getMesh(0)->getMaterial()->setSpecularTexture("sponza/textures/spnza_bricks_a_spec.tga");


	Model* arenaModel= &m_app->getResourceManager().getModel("arenaBasic.fbx", shader);
	arenaModel->getMesh(0)->getMaterial()->setDiffuseTexture("sponza/textures/arenaBasicTexture.tga");

	Model* barrierModel = &m_app->getResourceManager().getModel("barrierBasic.fbx", shader);
	barrierModel->getMesh(0)->getMaterial()->setDiffuseTexture("sponza/textures/barrierBasicTexture.tga");

	Model* containerModel= &m_app->getResourceManager().getModel("containerBasic.fbx", shader);
	containerModel->getMesh(0)->getMaterial()->setDiffuseTexture("sponza/textures/containerBasicTexture.tga");

	Model* rampModel = &m_app->getResourceManager().getModel("rampBasic.fbx", shader);
	rampModel->getMesh(0)->getMaterial()->setDiffuseTexture("sponza/textures/rampBasicTexture.tga");

	//For testing purposes only
	//Model* boxOrientation = &m_app->getResourceManager().getModel("boxOrientation.fbx", shader);
	//boxOrientation->getMesh(0)->getMaterial()->setDiffuseTexture("sponza/textures/boxOrientationTexture.tga");
	//Model* lrTest= &m_app->getResourceManager().getModel("lrTest.fbx", shader);

	Model* lightModel = &m_app->getResourceManager().getModel("candleExported.fbx", shader);
	lightModel->getMesh(0)->getMaterial()->setDiffuseTexture("sponza/textures/candleBasicTexture.tga");

	Model* characterModel = &m_app->getResourceManager().getModel("character1.fbx", shader);
	characterModel->getMesh(0)->getMaterial()->setDiffuseTexture("sponza/textures/character1texture.tga");

	// Temporary projectile model for the player's gun
	m_playerController.setProjectileModel(m_cubeModel.get());

	/*
		Creation of entitites
	*/
	{
		auto e = ECS::Instance()->createEntity("Arena");
		e->addComponent<ModelComponent>(arenaModel);
		e->addComponent<TransformComponent>(glm::vec3(0.f, 0.f, 0.f));
		e->addComponent<LightListComponent>();
		m_scene.addEntity(e);

		e = ECS::Instance()->createEntity("Barrier1");
		e->addComponent<ModelComponent>(barrierModel);
		e->addComponent<TransformComponent>(glm::vec3(-16.15f, 0.f, 3.83f), glm::vec3(0.f, -0.79f, 0.f));
		m_scene.addEntity(e);

		e = ECS::Instance()->createEntity("Barrier2");
		e->addComponent<ModelComponent>(barrierModel);
		e->addComponent<TransformComponent>(glm::vec3(-4.54f, 0.f, 8.06f));
		m_scene.addEntity(e);

		e = ECS::Instance()->createEntity("Barrier3");
		e->addComponent<ModelComponent>(barrierModel);
		e->addComponent<TransformComponent>(glm::vec3(8.46f, 0.f, 8.06f));
		m_scene.addEntity(e);

		e = ECS::Instance()->createEntity("Container1");
		e->addComponent<ModelComponent>(containerModel);
		e->addComponent<TransformComponent>(glm::vec3(6.95f, 0.f, 25.f));
		m_scene.addEntity(e);

		e = ECS::Instance()->createEntity("Container2");
		e->addComponent<ModelComponent>(containerModel);
		e->addComponent<TransformComponent>(glm::vec3(-25.f, 0.f, 12.43f), glm::vec3(0.f, 1.57f, 0.f));
		m_scene.addEntity(e);

		e = ECS::Instance()->createEntity("Container3");
		e->addComponent<ModelComponent>(containerModel);
		e->addComponent<TransformComponent>(glm::vec3(-25.f, 8.f, -7.73f), glm::vec3(0.f, 1.57f, 0.f));
		m_scene.addEntity(e);

		e = ECS::Instance()->createEntity("Container4");
		e->addComponent<ModelComponent>(containerModel);
		e->addComponent<TransformComponent>(glm::vec3(-19.67f, 0.f, -24.83f), glm::vec3(0.f, 0.79f, 0.f));
		m_scene.addEntity(e);

		e = ECS::Instance()->createEntity("Container5");
		e->addComponent<ModelComponent>(containerModel);
		e->addComponent<TransformComponent>(glm::vec3(-0.f, 0.f, -14.f));
		m_scene.addEntity(e);

		e = ECS::Instance()->createEntity("Container6");
		e->addComponent<ModelComponent>(containerModel);
		e->addComponent<TransformComponent>(glm::vec3(24.20f, 0.f, -8.f), glm::vec3(0.f, 1.57f, 0.f));
		m_scene.addEntity(e);

		e = ECS::Instance()->createEntity("Container7");
		e->addComponent<ModelComponent>(containerModel);
		e->addComponent<TransformComponent>(glm::vec3(24.2f, 8.f, -22.8f), glm::vec3(0.f, 1.57f, 0.f));
		m_scene.addEntity(e);

		e = ECS::Instance()->createEntity("Container8");
		e->addComponent<ModelComponent>(containerModel);
		e->addComponent<TransformComponent>(glm::vec3(24.36f, 0.f, -32.41f));
		m_scene.addEntity(e);

		e = ECS::Instance()->createEntity("Ramp1");
		e->addComponent<ModelComponent>(rampModel);
		e->addComponent<TransformComponent>(glm::vec3(5.2f, 0.f, -32.25f), glm::vec3(0.f, 0.f, 0.f));
		m_scene.addEntity(e);
		e = ECS::Instance()->createEntity("Ramp2");
		e->addComponent<ModelComponent>(rampModel);
		e->addComponent<TransformComponent>(glm::vec3(15.2f, 8.f, -32.25f), glm::vec3(0.f, 0.f, 0.f));
		m_scene.addEntity(e);
		e = ECS::Instance()->createEntity("Ramp3");
		e->addComponent<ModelComponent>(rampModel);
		e->addComponent<TransformComponent>(glm::vec3(24.f, 8.f, -5.5f), glm::vec3(0.f, 1.57f, 0.f));
		m_scene.addEntity(e);
		e = ECS::Instance()->createEntity("Ramp4");
		e->addComponent<ModelComponent>(rampModel);
		e->addComponent<TransformComponent>(glm::vec3(24.f, 0.f, 9.f), glm::vec3(0.f, 1.57f, 0.f));
		m_scene.addEntity(e);
		e = ECS::Instance()->createEntity("Ramp5");
		e->addComponent<ModelComponent>(rampModel);
		e->addComponent<TransformComponent>(glm::vec3(-16.f, 0.f, 20.f), glm::vec3(0.f, 3.14f, 0.f));
		m_scene.addEntity(e);
		e = ECS::Instance()->createEntity("Ramp6");
		e->addComponent<ModelComponent>(rampModel);
		e->addComponent<TransformComponent>(glm::vec3(-34.f, 0.f, 20.f), glm::vec3(0.f, 0.f, 0.f));
		m_scene.addEntity(e);

		e = ECS::Instance()->createEntity("Character");
		e->addComponent<ModelComponent>(characterModel);
		e->addComponent<TransformComponent>(glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, 0.f, 0.f));
		m_scene.addEntity(e);

		e = ECS::Instance()->createEntity("Character1");
		e->addComponent<ModelComponent>(characterModel);
		e->addComponent<TransformComponent>(glm::vec3(20.f, 0.f, 0.f), glm::vec3(0.f, 0.f, 0.f));
		m_scene.addEntity(e);
		e = ECS::Instance()->createEntity("Character2");
		e->addComponent<ModelComponent>(characterModel);
		e->addComponent<TransformComponent>(glm::vec3(0.f, 0.f, 20.f), glm::vec3(0.f, 0.f, 0.f));
		m_scene.addEntity(e);
		e = ECS::Instance()->createEntity("Character3");
		e->addComponent<ModelComponent>(characterModel);
		e->addComponent<TransformComponent>(glm::vec3(20.f, 0.f, 20.f), glm::vec3(0.f, 0.f, 0.f));
		m_scene.addEntity(e);

		//creates light with model and pointlight
		e = ECS::Instance()->createEntity("Candle1");
		e->addComponent<ModelComponent>(lightModel);
		e->addComponent<TransformComponent>(glm::vec3(3.f, 0.f, 3.f));
		PointLight pl;
		glm::vec3 lightPos = e->getComponent<TransformComponent>()->getTranslation();
		pl.setColor(glm::vec3(1.f, 1.f, 1.f));
		pl.setPosition(glm::vec3(lightPos.x, lightPos.y + 3.1f, lightPos.z));
		pl.setAttenuation(.0f, 0.1f, 0.02f);
		pl.setIndex(0);
		e->addComponent<LightComponent>(pl);
		m_scene.addEntity(e);

		e = ECS::Instance()->createEntity("Candle2");
		e->addComponent<ModelComponent>(lightModel);
		e->addComponent<TransformComponent>(glm::vec3(1.f, 0.f, 1.f));
		lightPos = e->getComponent<TransformComponent>()->getTranslation();
		pl.setColor(glm::vec3(1.f, 1.f, 1.f));
		pl.setPosition(glm::vec3(lightPos.x, lightPos.y + 3.1f, lightPos.z));
		pl.setAttenuation(.0f, 0.1f, 0.02f);
		pl.setIndex(1);
		e->addComponent<LightComponent>(pl);
		m_scene.addEntity(e);


		m_virtRAMHistory = SAIL_NEW float[100];
		m_physRAMHistory = SAIL_NEW float[100];
		// Uncomment this to enable vram budget visualization
		//m_vramBudgetHistory = SAIL_NEW float[100];
		m_vramUsageHistory = SAIL_NEW float[100];
		m_cpuHistory = SAIL_NEW float[100];
		m_frameTimesHistory = SAIL_NEW float[100];
	}
}

GameState::~GameState() {
	delete m_virtRAMHistory;
	delete m_physRAMHistory;
	// Uncomment this to enable vram budget visualization
	//delete m_vramBudgetHistory;
	delete m_vramUsageHistory;
	delete m_cpuHistory;
	delete m_frameTimesHistory;
}

// Process input for the state
bool GameState::processInput(float dt) {

#ifdef _DEBUG
	// Add point light at camera pos by adding it to component
	if (Input::WasKeyJustPressed(SAIL_KEY_E)) {
		PointLight pl;
		pl.setColor(glm::vec3(Utils::rnd(), Utils::rnd(), Utils::rnd()));
		pl.setPosition(m_cam.getPosition());
		pl.setAttenuation(.0f, 0.1f, 0.02f);
		m_scene.getEntityByName("Arena")->getComponent<LightListComponent>()->m_pls.push_back(pl);
		//m_lights.addPointLight(pl);
	}

#endif

	if (Input::IsKeyPressed(SAIL_KEY_G)) {
		glm::vec3 color(1.0f, 1.0f, 1.0f);
		//m_lights.setDirectionalLight(DirectionalLight(color, m_cam.getDirection()));
		float x = ((int)(m_cam.getDirection().x * 100 + .5)) / 100.0f;
		float y = ((int)(m_cam.getDirection().y * 100 + .5)) / 100.0f;
		float z = ((int)(m_cam.getDirection().z * 100 + .5)) / 100.0f;
		std::string message = "t" + std::to_string(x) + 
			":" + std::to_string(y) + 
			":" + std::to_string(z) + ":";
		//printf(message.c_str());
		
		if (NetworkWrapper::getInstance().isHost())
		{
			m_lights.setDirectionalLight(DirectionalLight(color, m_cam.getDirection()));
			NetworkWrapper::getInstance().sendMsgAllClients(message);
		}
		else
		{
			NetworkWrapper::getInstance().sendMsg(message);
		}
	}
	if (Input::WasKeyJustPressed(SAIL_KEY_OEM_5)) {
		m_cc.toggle();
		m_profiler.toggle();
	}
	// Update the camera controller from input devices
	//m_camController.update(dt);
	m_playerController.update(dt);
	//m_physSystem.execute(dt);

	// Reload shaders
	if (Input::WasKeyJustPressed(SAIL_KEY_R)) {
		m_app->getResourceManager().reloadShader<MaterialShader>();
		Event e(Event::POTATO);
		m_app->dispatchEvent(e);
	}

	//checks if candle entity has light and if not, adds one 
	if (Input::WasKeyJustPressed(SAIL_KEY_Z)) {
		if (!m_scene.getEntityByName("Candle1")->hasComponent<LightComponent>()) {
			PointLight pl;
			glm::vec3 pos = m_scene.getEntityByName("Candle1")->getComponent<TransformComponent>()->getTranslation();
			pl.setColor(glm::vec3(1.f, 1.f, 1.f));
			pl.setPosition(glm::vec3(pos.x, pos.y + 3.1, pos.z));
			pl.setAttenuation(.0f, 0.1f, 0.02f);
			pl.setIndex(0);
			m_scene.getEntityByName("Candle1")->addComponent<LightComponent>(pl);
			//m_lights.addPointLight(pl);
		}
	}
	if (Input::WasKeyJustPressed(SAIL_KEY_V)) {
		if (!m_scene.getEntityByName("Candle2")->hasComponent<LightComponent>()) {
			PointLight pl;
			glm::vec3 pos = m_scene.getEntityByName("Candle2")->getComponent<TransformComponent>()->getTranslation();
			pl.setColor(glm::vec3(1.f, 1.f, 1.f));
			pl.setPosition(glm::vec3(pos.x, pos.y + 3.1, pos.z));
			pl.setAttenuation(.0f, 0.1f, 0.02f);
			pl.setIndex(1);
			m_scene.getEntityByName("Candle2")->addComponent<LightComponent>(pl);
			//m_lights.addPointLight(pl);
		}
	}

	//removes first added pointlight in arena
	if (Input::WasKeyJustPressed(SAIL_KEY_X)) {

		if (m_scene.getEntityByName("Arena")->getComponent<LightListComponent>()->m_pls.size() > 0) {
			m_scene.getEntityByName("Arena")->getComponent<LightListComponent>()->m_pls.erase(m_scene.getEntityByName("Arena")->getComponent<LightListComponent>()->m_pls.begin());
		}

		//m_lights.removePointLight();
	}

	//removes light from candle entities if they have it
	if (Input::WasKeyJustPressed(SAIL_KEY_M)) {
		if (m_scene.getEntityByName("Candle1")->hasComponent<LightComponent>()) {
			m_scene.getEntityByName("Candle1")->removeComponent<LightComponent> ();
		}
		//m_lights.removePLByIndex(0);
	}
	if (Input::WasKeyJustPressed(SAIL_KEY_N)) {
		if (m_scene.getEntityByName("Candle2")->hasComponent<LightComponent>()) {
			m_scene.getEntityByName("Candle2")->removeComponent<LightComponent>();
		}
		//m_lights.removePLByIndex(1);
	}
	return true;
}

bool GameState::onEvent(Event& event) {
	Logger::Log("Received event: " + std::to_string(event.getType()));

	EventHandler::dispatch<WindowResizeEvent>(event, SAIL_BIND_EVENT(&GameState::onResize));
	EventHandler::dispatch<NetworkTestEvent>(event, SAIL_BIND_EVENT(&GameState::onNetworkTest));

	// Forward events
	m_scene.onEvent(event);

	return true;
}

bool GameState::onResize(WindowResizeEvent& event) {
	m_cam.resize(event.getWidth(), event.getHeight());
	return true;
}

bool GameState::onNetworkTest(NetworkTestEvent& event)
{
	bool yFlag = false;
	bool zFlag = false;
	float x, y, z = 0;
	std::string tmpX, tmpY, tmpZ = "";

	std::string message = event.getMessage();
	message.erase(0, 1);

	for (size_t i = 0; i < message.size(); i++)
	{
		if (message[i] == '\0' || message[i] == ':')
		{
			if (!yFlag)
			{
				yFlag = true;
			}
			else
			{
				zFlag = true;
			}

			continue;
		}

		if (!yFlag) // x
		{
			tmpX += message[i];
		}
		else if (zFlag) // z
		{
			tmpZ += message[i];
		}
		else // y
		{
			tmpY += message[i];
		}
	}
	x = std::stof(tmpX);
	y = std::stof(tmpY);
	z = std::stof(tmpZ);

	glm::vec3 color(1.0f, 1.0f, 1.0f);
	glm::vec3 tmpPos(x, y, z);
	m_lights.setDirectionalLight(DirectionalLight(color, tmpPos));

	//printf(event.getMessage().c_str());
	return false;
}



bool GameState::update(float dt) {

	std::wstring fpsStr = std::to_wstring(m_app->getFPS());

	m_app->getWindow()->setWindowTitle("Sail | Game Engine Demo | " + Application::getPlatformName() + " | FPS: " + std::to_string(m_app->getFPS()));

	static float counter = 0.0f;
	static float size = 1;
	static float change = 0.4f;
	
	counter += dt * 2;

	updateComponentSystems(dt);

	// Check for packages if network is connected
	if (NetworkWrapper::getInstance().isInitialized())
	{
		NetworkWrapper::getInstance().checkForPackages();
	}

	/*if (m_texturedCubeEntity) {
		//Translations, rotations and scales done here are non-constant, meaning they change between updates
		//All constant transformations can be set in the PhysicsComponent and will then be updated automatically
		
		
		// Move the cubes around
		m_texturedCubeEntity->getComponent<TransformComponent>()->setTranslation(glm::vec3(glm::sin(counter), 1.f, glm::cos(counter)));
		m_texturedCubeEntity->getComponent<TransformComponent>()->setRotations(glm::vec3(glm::sin(counter), counter, glm::cos(counter)));

		// Set translation and scale to show how parenting affects transforms
		//for (Entity::SPtr item : m_transformTestEntities) {
		for (size_t i = 1; i < m_transformTestEntities.size(); i++) {
			Entity::SPtr item = m_transformTestEntities[i];
			item->getComponent<TransformComponent>()->setScale(size);
			item->getComponent<TransformComponent>()->setTranslation(size * 3, 1.0f, size * 3);
		}
		//m_transformTestEntities[0]->getComponent<TransformComponent>()->translate(2.0f, 0.0f, 2.0f);

		size += change * dt;
		if (size > 1.2f || size < 0.7f)
			change *= -1.0f;
	}*/

	//check and update all lights for all entities
	std::vector<Entity::SPtr> entities = m_scene.getEntities();
	m_lights.clearPointLights();
	for (int i = 0; i < entities.size();i++) {
		if (entities[i]->hasComponent<LightComponent>()) {
			m_lights.addPointLight(entities[i]->getComponent<LightComponent>()->m_pointLight);
		}
		if (entities[i]->hasComponent<LightListComponent>()) {
			for (int j = 0; j < entities[i]->getComponent<LightListComponent>()->m_pls.size(); j++) {
				m_lights.addPointLight(entities[i]->getComponent<LightListComponent>()->m_pls[j]);
			}
		}
	}
	m_lights.updateBufferData();
	return true;
}

// Renders the state
bool GameState::render(float dt) {

	// Clear back buffer
	m_app->getAPI()->clear({0.1f, 0.2f, 0.3f, 1.0f});

	// Draw the scene
	m_scene.draw(m_cam);

	return true;
}

bool GameState::renderImgui(float dt) {
	// The ImGui window is rendered when activated on F10
	ImGui::ShowDemoWindow();
	renderImguiConsole(dt);
	renderImguiProfiler(dt);
	return false;
}

bool GameState::renderImguiConsole(float dt) {
	

	bool open = m_cc.windowOpen();
	if (open) {
		static char buf[256] = "";
		if (ImGui::Begin("Console", &open)) {
			m_cc.windowState(open);
			std::string txt = "test";
			ImGui::BeginChild("ScrollingRegion", ImVec2(0, -30), false, ImGuiWindowFlags_HorizontalScrollbar);
			

			for (int i = 0; i < m_cc.getLog().size(); i++) {
				ImGui::TextUnformatted(m_cc.getLog()[i].c_str());
			}

			ImGui::EndChild();
			ImGui::Separator();
			bool reclaim_focus = false;
			
			m_cc.getTextField().copy(buf, m_cc.getTextField().size()+1);
			buf[m_cc.getTextField().size()] = '\0';

			//std::string* str = new std::string(m_cc.getTextField());
			std::string original = m_cc.getTextField();
			bool exec = ImGui::InputText("", buf, IM_ARRAYSIZE(buf),
				ImGuiInputTextFlags_EnterReturnsTrue);
			ImGui::SameLine();
			if (exec || ImGui::Button("Execute", ImVec2(0, 0))) {
				
				if (m_cc.execute()) {

				}
				
				
				
				reclaim_focus = true;
			}
			else {
				m_cc.setTextField(std::string(buf));
			}
			ImGui::End();
		}
		else {
		
			ImGui::End();
		}

	}






	return false;
}

bool GameState::renderImguiProfiler(float dt) {


	bool open = m_profiler.windowOpen();
	if (open) {
		if (ImGui::Begin("Profiler", &open)) {
			m_profiler.windowState(open);
			ImGui::BeginChild("Window", ImVec2(0, 0), false, 0);
			std::string header;

			header = "CPU (" + m_cpuCount + "%%)";
			ImGui::Text(header.c_str());

			header = "Frame time (" + m_ftCount + " seconds)";
			ImGui::Text(header.c_str());

			header = "Virtual RAM (" + m_virtCount + " MB)";
			ImGui::Text(header.c_str());

			header = "Physical RAM (" + m_physCount + " MB)";
			ImGui::Text(header.c_str());

			header = "VRAM (" + m_vramUCount + " MB)";
			ImGui::Text(header.c_str());

			// Uncomment this to enable vram budget visualization

			/*header = "VRAM Available (" + m_vramBCount + " MB)";
			ImGui::Text(header.c_str());*/


			ImGui::Separator();
			if (ImGui::CollapsingHeader("CPU Graph")) {
				header = "\n\n\n" + m_cpuCount + "(%)";
				ImGui::PlotLines(header.c_str(), m_cpuHistory, 100, 0, "", 0.f, 100.f, ImVec2(0, 100));
			}
			if (ImGui::CollapsingHeader("Frame Times Graph")) {
				header = "\n\n\n" + m_ftCount + "(s)";
				ImGui::PlotLines(header.c_str(), m_frameTimesHistory, 100, 0, "", 0.f, 0.01f, ImVec2(0, 100));
			}
			if (ImGui::CollapsingHeader("Virtual RAM Graph")) {
				header = "\n\n\n" + m_virtCount + "(MB)";
				ImGui::PlotLines(header.c_str(), m_virtRAMHistory, 100, 0, "", 0.f, 500.f, ImVec2(0, 100));

			}
			if (ImGui::CollapsingHeader("Physical RAM Graph")) {
				header = "\n\n\n" + m_physCount + "(MB)";
				ImGui::PlotLines(header.c_str(), m_physRAMHistory, 100, 0, "", 0.f, 500.f, ImVec2(0, 100));
			}
			if (ImGui::CollapsingHeader("VRAM Graph")) {
				header = "\n\n\n" + m_vramUCount + "(MB)";
				ImGui::PlotLines(header.c_str(), m_vramUsageHistory, 100, 0, "", 0.f, 500.f, ImVec2(0, 100));
			}

			// Uncomment this to enable vram budget visualization

			/*if (ImGui::CollapsingHeader("VRAM Budget Graph")) {
				header = "\n\n\n" + m_vramBCount + "(MB)";
				ImGui::PlotLines(header.c_str(), m_vramBudgetHistory, 100, 0, "", 0.f, 6000.f, ImVec2(0, 100));
			}*/



			ImGui::EndChild();

			m_profilerTimer += dt;
			if (m_profilerTimer > 0.2f) {
				m_profilerTimer = 0.f;
				if (m_profilerCounter <= 100) {

					// Uncomment this to enable vram budget visualization

					//m_vramBudgetHistory[m_profilerCounter] = m_profiler.vramBudget();
					//m_vramBCount = "\n\n\n" + std::to_string(m_profiler.vramBudget());

					m_virtRAMHistory[m_profilerCounter] = m_profiler.virtMemUsage();
					m_physRAMHistory[m_profilerCounter] = m_profiler.workSetUsage();
					m_vramUsageHistory[m_profilerCounter] = m_profiler.vramUsage();
					m_frameTimesHistory[m_profilerCounter] = dt;
					m_cpuHistory[m_profilerCounter++] = m_profiler.processUsage();
					m_virtCount = std::to_string(m_profiler.virtMemUsage());
					m_physCount = std::to_string(m_profiler.workSetUsage());
					m_vramUCount = std::to_string(m_profiler.vramUsage());
					m_cpuCount = std::to_string(m_profiler.processUsage());
					m_ftCount = std::to_string(dt);

				}
				else {
					float* tempFloatArr = SAIL_NEW float[100];
					std::copy(m_virtRAMHistory + 1, m_virtRAMHistory + 100, tempFloatArr);
					tempFloatArr[99] = m_profiler.virtMemUsage();
					delete m_virtRAMHistory;
					m_virtRAMHistory = tempFloatArr;
					m_virtCount = std::to_string(m_profiler.virtMemUsage());

					float* tempFloatArr1 = SAIL_NEW float[100];
					std::copy(m_physRAMHistory + 1, m_physRAMHistory + 100, tempFloatArr1);
					tempFloatArr1[99] = m_profiler.workSetUsage();
					delete m_physRAMHistory;
					m_physRAMHistory = tempFloatArr1;
					m_physCount = std::to_string(m_profiler.workSetUsage());

					// Uncomment this to enable vram budget visualization

					/*float* tempFloatArr2 = SAIL_NEW float[100];
					std::copy(m_vramBudgetHistory + 1, m_vramBudgetHistory + 101, tempFloatArr2);
					tempFloatArr2[99] = m_profiler.vramBudget();
					delete m_vramBudgetHistory;
					m_vramBudgetHistory = tempFloatArr2;
					m_vramBCount = std::to_string(m_profiler.vramBudget());*/

					float* tempFloatArr3 = SAIL_NEW float[100];
					std::copy(m_vramUsageHistory + 1, m_vramUsageHistory + 100, tempFloatArr3);
					tempFloatArr3[99] = m_profiler.vramUsage();
					delete m_vramUsageHistory;
					m_vramUsageHistory = tempFloatArr3;
					m_vramUCount = std::to_string(m_profiler.vramUsage());

					float* tempFloatArr4 = SAIL_NEW float[100];
					std::copy(m_cpuHistory + 1, m_cpuHistory + 100, tempFloatArr4);
					tempFloatArr4[99] = m_profiler.processUsage();
					delete m_cpuHistory;
					m_cpuHistory = tempFloatArr4;
					m_cpuCount = std::to_string(m_profiler.processUsage());

					float* tempFloatArr5 = SAIL_NEW float[100];
					std::copy(m_frameTimesHistory + 1, m_frameTimesHistory + 100, tempFloatArr5);
					tempFloatArr5[99] = dt;
					delete m_frameTimesHistory;
					m_frameTimesHistory = tempFloatArr5;
					m_ftCount = std::to_string(dt);
				}
			}
			ImGui::End();
		}
		else {

			ImGui::End();
		}

	}






	return false;
}

void GameState::updateComponentSystems(float dt) {
	m_componentSystems.physicSystem->update(dt);
}

const std::string GameState::createCube(const glm::vec3& position) {
	auto e = ECS::Instance()->createEntity("new cube");
	e->addComponent<ModelComponent>(m_cubeModel.get());
	e->addComponent<TransformComponent>(position);
	m_scene.addEntity(e);
	return std::string("Added Cube at (" +
		std::to_string(position.x) + ":" +
		std::to_string(position.y) + ":" +
		std::to_string(position.z) + ")");
}