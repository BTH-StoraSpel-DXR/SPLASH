#include "GameState.h"
#include "imgui.h"
#include "..//Sail/src/Sail/entities/systems/physics/PhysicSystem.h"
#include "..//Sail/src/Sail/entities/ECSManager.h"

GameState::GameState(StateStack& stack)
: State(stack)
//, m_cam(20.f, 20.f, 0.1f, 5000.f)
, m_cam(90.f, 1280.f / 720.f, 0.1f, 5000.f)
, m_camController(&m_cam)
{

	// Get the Application instance
	m_app = Application::getInstance();
	//m_scene = std::make_unique<Scene>(AABB(glm::vec3(-100.f, -100.f, -100.f), glm::vec3(100.f, 100.f, 100.f)));

	// Textures needs to be loaded before they can be used
	// TODO: automatically load textures when needed so the following can be removed
	Application::getInstance()->getResourceManager().loadTexture("sponza/textures/spnza_bricks_a_ddn.tga");
	Application::getInstance()->getResourceManager().loadTexture("sponza/textures/spnza_bricks_a_diff.tga");
	Application::getInstance()->getResourceManager().loadTexture("sponza/textures/spnza_bricks_a_spec.tga");

	// Set up camera with controllers
	m_cam.setPosition(glm::vec3(1.6f, 4.7f, 7.4f));
	m_camController.lookAt(glm::vec3(0.f));
	
	// Add a directional light
	glm::vec3 color(1.0f, 1.0f, 1.0f);
 	glm::vec3 direction(0.4f, -0.2f, 1.0f);
	direction = glm::normalize(direction);
	m_lights.setDirectionalLight(DirectionalLight(color, direction));
	// Add four point lights
	{
		PointLight pl;
		pl.setAttenuation(.0f, 0.1f, 0.02f);
		pl.setColor(glm::vec3(Utils::rnd(), Utils::rnd(), Utils::rnd()));
		pl.setPosition(glm::vec3(-4.0f, 0.1f, -4.0f));
		m_lights.addPointLight(pl);

		pl.setColor(glm::vec3(Utils::rnd(), Utils::rnd(), Utils::rnd()));
		pl.setPosition(glm::vec3(-4.0f, 0.1f, 4.0f));
		m_lights.addPointLight(pl);

		pl.setColor(glm::vec3(Utils::rnd(), Utils::rnd(), Utils::rnd()));
		pl.setPosition(glm::vec3(4.0f, 0.1f, 4.0f));
		m_lights.addPointLight(pl);

		pl.setColor(glm::vec3(Utils::rnd(), Utils::rnd(), Utils::rnd()));
		pl.setPosition(glm::vec3(4.0f, 0.1f, -4.0f));
		m_lights.addPointLight(pl);
	}

	// Set up the scene
	//m_scene->addSkybox(L"skybox_space_512.dds"); //TODO
	m_scene.setLightSetup(&m_lights);

	// Disable culling for testing purposes
	m_app->getAPI()->setFaceCulling(GraphicsAPI::NO_CULLING);

	auto* shader = &m_app->getResourceManager().getShaderSet<MaterialShader>();


	/*
		Create a PhysicSystem
		If the game developer does not want to add the systems like this,
		this call could be moved inside the default constructor of ECS,
		assuming each system is included in ECS.cpp instead of here
	*/
	ECSManager::Instance()->createSystem<PhysicSystem>();


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

	// Create entities
	auto e = ECSManager::Instance()->createEntity("Static cube");
	e->addComponent<ModelComponent>(m_cubeModel.get());
	e->addComponent<TransformComponent>(glm::vec3(-4.f, 1.f, -2.f));
	m_scene.addEntity(e);

	e = ECSManager::Instance()->createEntity("Floor");
	e->addComponent<ModelComponent>(m_planeModel.get());
	e->addComponent<TransformComponent>(glm::vec3(0.f, 0.f, 0.f));
	m_scene.addEntity(e);

	e = ECSManager::Instance()->createEntity("Clingy cube");
	e->addComponent<ModelComponent>(m_cubeModel.get());
	e->addComponent<TransformComponent>(glm::vec3(-1.2f, 1.f, -1.f), glm::vec3(0.f, 0.f, 1.07f));
	m_scene.addEntity(e);

	// Add some cubes which are connected through parenting
	m_texturedCubeEntity = ECSManager::Instance()->createEntity("Textured parent cube");
	m_texturedCubeEntity->addComponent<ModelComponent>(fbxModel);
	m_texturedCubeEntity->addComponent<TransformComponent>(glm::vec3(-1.f, 2.f, 0.f), m_texturedCubeEntity->getComponent<TransformComponent>());
	m_texturedCubeEntity->setName("MovingCube");
	m_scene.addEntity(m_texturedCubeEntity);
	e->getComponent<TransformComponent>()->setParent(m_texturedCubeEntity->getComponent<TransformComponent>());

	e = ECSManager::Instance()->createEntity("CubeRoot");
	e->addComponent<ModelComponent>(m_cubeModel.get());
	e->addComponent<TransformComponent>(glm::vec3(10.f, 0.f, 10.f));
	e->addComponent<PhysicsComponent>(glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));	// add constant rotation
	m_scene.addEntity(e);
	m_transformTestEntities.push_back(e);

	e = ECSManager::Instance()->createEntity("CubeChild");
	e->addComponent<ModelComponent>(m_cubeModel.get());
	e->addComponent<TransformComponent>(glm::vec3(1.f, 1.f, 1.f), m_transformTestEntities[0]->getComponent<TransformComponent>());
	e->addComponent<PhysicsComponent>(glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));	// add constant rotation
	m_scene.addEntity(e);
	m_transformTestEntities.push_back(e);

	e = ECSManager::Instance()->createEntity("CubeChildChild");
	e->addComponent<ModelComponent>(m_cubeModel.get());
	e->addComponent<TransformComponent>(glm::vec3(1.f, 1.f, 1.f), m_transformTestEntities[1]->getComponent<TransformComponent>());
	e->addComponent<PhysicsComponent>(glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));	// add constant rotation
	m_scene.addEntity(e);
	m_transformTestEntities.push_back(e);

	std::cout
		<< "Transform: " << TransformComponent::ID
		<< "\nText: " << TextComponent::ID
		<< "\nPhysics: " << PhysicsComponent::ID
		<< "\nModel: " << ModelComponent::ID
		<< "\nNr of components: " << ECSManager::Instance()->nrOfComponentTypes()
		<< "\nNr of transform test entities: " << m_transformTestEntities.size() << "\n";
}

GameState::~GameState() {
}

// Process input for the state
bool GameState::processInput(float dt) {

#ifdef _DEBUG
	// Add point light at camera pos
	if (Input::WasKeyJustPressed(SAIL_KEY_E)) {
		PointLight pl;
		pl.setColor(glm::vec3(Utils::rnd(), Utils::rnd(), Utils::rnd()));
		pl.setPosition(m_cam.getPosition());
		pl.setAttenuation(.0f, 0.1f, 0.02f);
		m_lights.addPointLight(pl);
	}

	if (Input::WasKeyJustPressed(SAIL_KEY_1)) {
		Logger::Log("Setting parent");
		m_transformTestEntities[2]->getComponent<TransformComponent>()->setParent(m_transformTestEntities[1]->getComponent<TransformComponent>());
	}
	if (Input::WasKeyJustPressed(SAIL_KEY_2)) {
		Logger::Log("Removing parent");
		m_transformTestEntities[2]->getComponent<TransformComponent>()->removeParent();
	}


	/*
		Test:
		Will add or remove the PhysicsComponent on the first entity in m_transformTestEntities
		If that entity already has the component, the first press will write a warning to the console
	*/
	if (Input::WasKeyJustPressed(SAIL_KEY_J)) {
		static bool hasPhysics = false;
		hasPhysics = !hasPhysics;

		switch (hasPhysics) {
		case true:
			m_transformTestEntities[0]->addComponent<PhysicsComponent>(glm::vec3(0, 0, 0), glm::vec3(0, 0, 0), glm::vec3(9.82f, 0, 0));
			break;
		case false:
			m_transformTestEntities[0]->removeComponent<PhysicsComponent>();
			break;
		}
	}
#endif

	if (Input::IsKeyPressed(SAIL_KEY_G)) {
		glm::vec3 color(1.0f, 1.0f, 1.0f);
		m_lights.setDirectionalLight(DirectionalLight(color, m_cam.getDirection()));
	}

	// Update the camera controller from input devices
	m_camController.update(dt);

	// Reload shaders
	if (Input::WasKeyJustPressed(SAIL_KEY_R)) {
		m_app->getResourceManager().reloadShader<MaterialShader>();
		Event e(Event::POTATO);
		m_app->dispatchEvent(e);
	}

	return true;
}

bool GameState::onEvent(Event& event) {
	Logger::Log("Received event: " + std::to_string(event.getType()));

	EventHandler::dispatch<WindowResizeEvent>(event, SAIL_BIND_EVENT(&GameState::onResize));

	// Forward events
	m_scene.onEvent(event);

	return true;
}

bool GameState::onResize(WindowResizeEvent& event) {
	m_cam.resize(event.getWidth(), event.getHeight());
	return true;
}

bool GameState::update(float dt) {

	std::wstring fpsStr = std::to_wstring(m_app->getFPS());

	m_app->getWindow()->setWindowTitle("Sail | Game Engine Demo | " + Application::getPlatformName() + " | FPS: " + std::to_string(m_app->getFPS()));

	static float counter = 0.0f;
	static float size = 1;
	static float change = 0.4f;
	
	counter += dt * 2;

	/*
		Updates all Component Systems in order
	*/
	ECSManager::Instance()->update(dt);

	if (m_texturedCubeEntity) {
		/*
			Translations, rotations and scales done here are non-constant, meaning they change between updates
			All constant transformations can be set in the PhysicsComponent and will then be updated automatically
		*/
		
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
	}

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
	return false;
}
