#include "PBRTestState.h"
#include "imgui.h"
#include "Sail/entities/ECS.h"
#include "Sail/entities/systems/Systems.h"
#include "Sail/TimeSettings.h"

#include "Sail/events/EventDispatcher.h"

#include <sstream>
#include <iomanip>

// TEST
#include "API/DX12/resources/DX12DDSTexture.h" 

// Uncomment to use forward rendering
//#define DISABLE_RT

PBRTestState::PBRTestState(StateStack& stack)
	: State(stack)
	, m_cam(90.f, 1280.f / 720.f, 0.1f, 5000.f)
	, m_camController(&m_cam)
	, m_profiler(true) {

	EventDispatcher::Instance().subscribe(Event::Type::WINDOW_RESIZE, this);


	auto& console = Application::getInstance()->getConsole();
	console.addCommand("state <string>", [&](const std::string& param) {
		if (param == "menu") {
			requestStackPop();
			requestStackPush(States::MainMenu);
			return "State change to menu requested";
		} else {
			return "Invalid state. Available states are \"menu\"";
		}
	}, "PBRTestState");
#ifdef _DEBUG
	console.addCommand(std::string("AddCube"), [&]() {
		return createCube(m_cam.getPosition());
	}, "PBRTestState");
	console.addCommand(std::string("AddCube <int> <int> <int>"), [&](std::vector<int> in) {
		if (in.size() == 3) {
			glm::vec3 pos(in[0], in[1], in[2]);
			return createCube(pos);
		} else {
			return std::string("Error: wrong number of inputs. Console Broken");
		}
		return std::string("wat");
	}, "PBRTestState");
	console.addCommand(std::string("AddCube <float> <float> <float>"), [&](std::vector<float> in) {
		if (in.size() == 3) {
			glm::vec3 pos(in[0], in[1], in[2]);
			return createCube(pos);
		} else {
			return std::string("Error: wrong number of inputs. Console Broken");
		}
		return std::string("wat");
	}, "PBRTestState");
#endif

	m_lightDebugWindow.setLightSetup(&m_lights);

	// Create octree
	m_octree = SAIL_NEW Octree(nullptr);

	m_renderSettingsWindow.activateMaterialPicking(&m_cam, m_octree);

	// Get the Application instance
	m_app = Application::getInstance();
	
	// Create systems for rendering
	m_componentSystems.beginEndFrameSystem = ECS::Instance()->createSystem<BeginEndFrameSystem>();
	m_componentSystems.boundingboxSubmitSystem = ECS::Instance()->createSystem<BoundingboxSubmitSystem>();
	m_componentSystems.metaballSubmitSystem = ECS::Instance()->createSystem<MetaballSubmitSystem<RenderInActiveGameComponent>>();
	m_componentSystems.modelSubmitSystem = ECS::Instance()->createSystem<ModelSubmitSystem<RenderInActiveGameComponent>>();

	// Create entity adder system
	m_componentSystems.entityAdderSystem = ECS::Instance()->getEntityAdderSystem();

	// Create entity removal system
	m_componentSystems.entityRemovalSystem = ECS::Instance()->getEntityRemovalSystem();

	// Create system for updating bounding box
	m_componentSystems.updateBoundingBoxSystem = ECS::Instance()->createSystem<UpdateBoundingBoxSystem>();

	// Create system for handling octree
	m_componentSystems.octreeAddRemoverSystem = ECS::Instance()->createSystem<OctreeAddRemoverSystem<RenderInActiveGameComponent>>();
	m_componentSystems.octreeAddRemoverSystem->provideOctree(m_octree);


	// Textures needs to be loaded before they can be used
	Application::getInstance()->getResourceManager().loadTexture("pbr/pavingStones/albedo.tga");
	Application::getInstance()->getResourceManager().loadTexture("pbr/pavingStones/metalnessRoughnessAO.tga");
	Application::getInstance()->getResourceManager().loadTexture("pbr/pavingStones/normal.tga");

	Application::getInstance()->getResourceManager().loadTexture("pbr/greenTiles/albedo.tga");
	Application::getInstance()->getResourceManager().loadTexture("pbr/greenTiles/metalnessRoughnessAO.tga");
	Application::getInstance()->getResourceManager().loadTexture("pbr/greenTiles/normal.tga");

	Application::getInstance()->getResourceManager().loadTexture("pbr/metal/albedo.tga");
	Application::getInstance()->getResourceManager().loadTexture("pbr/metal/metalnessRoughnessAO.tga");
	Application::getInstance()->getResourceManager().loadTexture("pbr/metal/normal.tga");

	Application::getInstance()->getResourceManager().loadTexture("pbr/ice/albedo.tga");
	Application::getInstance()->getResourceManager().loadTexture("pbr/ice/metalnessRoughnessAO.tga");
	Application::getInstance()->getResourceManager().loadTexture("pbr/ice/normal.tga");

	Application::getInstance()->getResourceManager().loadTexture("pbr/brdfLUT.tga");


	// Add a directional light
	glm::vec3 color(0.0f, 0.0f, 0.0f);
	glm::vec3 direction(0.4f, -0.2f, 1.0f);
	direction = glm::normalize(direction);
	m_lights.setDirectionalLight(DirectionalLight(color, direction));

	PointLight pl;
	pl.setColor(glm::vec3(1.0f));
	pl.setPosition(glm::vec3(0.f, 10.f, 0.f));
	m_lights.addPointLight(pl);

	m_app->getRenderWrapper()->getCurrentRenderer()->setLightSetup(&m_lights);
	// Disable culling for testing purposes
	m_app->getAPI()->setFaceCulling(GraphicsAPI::NO_CULLING);

#ifdef DISABLE_RT
	auto* shader = &m_app->getResourceManager().getShaderSet<MaterialShader>();
	(*Application::getInstance()->getRenderWrapper()).changeRenderer(1);
	m_app->getRenderWrapper()->getCurrentRenderer()->setLightSetup(&m_lights);
#else
	auto* shader = &m_app->getResourceManager().getShaderSet<GBufferOutShader>();
#endif

	// Create/load models

	m_cubeModel = ModelFactory::CubeModel::Create(glm::vec3(0.5f), shader);
	m_cubeModel->getMesh(0)->getMaterial()->setColor(glm::vec4(0.2f, 0.8f, 0.4f, 1.0f));

	m_planeModel = ModelFactory::PlaneModel::Create(glm::vec2(20.f, 20.f), shader, glm::vec2(4.0f));
	//m_planeModel->getMesh(0)->getMaterial()->setAlbedoTexture("pbr/ice/albedo.tga");
	m_testDDSTexture = new DX12DDSTexture("res/textures/pbr/DDS/Tiles/CF_Albedo.dds");
	m_planeModel->getMesh(0)->getMaterial()->manuallySetTexture(m_testDDSTexture, 0);
	//m_planeModel->getMesh(0)->getMaterial()->setMetalnessRoughnessAOTexture("pbr/ice/metalnessRoughnessAO.tga");
	//m_planeModel->getMesh(0)->getMaterial()->setNormalTexture("pbr/ice/normal.tga");

	/*Model* cylinderModel0 = &m_app->getResourceManager().getModel("pbrCylinder", shader);
	cylinderModel0->getMesh(0)->getMaterial()->setAlbedoTexture("pbr/metal/albedo.tga");
	cylinderModel0->getMesh(0)->getMaterial()->setMetalnessRoughnessAOTexture("pbr/metal/metalnessRoughnessAO.tga");
	cylinderModel0->getMesh(0)->getMaterial()->setNormalTexture("pbr/metal/normal.tga");

	Model* cylinderModel1 = &m_app->getResourceManager().getModel("pbrCylinder_", shader);
	cylinderModel1->getMesh(0)->getMaterial()->setAlbedoTexture("pbr/pavingStones/albedo.tga");
	cylinderModel1->getMesh(0)->getMaterial()->setMetalnessRoughnessAOTexture("pbr/pavingStones/metalnessRoughnessAO.tga");
	cylinderModel1->getMesh(0)->getMaterial()->setNormalTexture("pbr/pavingStones/normal.tga");

	Model* cylinderModel2 = &m_app->getResourceManager().getModel("pbrCylinder__", shader);
	cylinderModel2->getMesh(0)->getMaterial()->setAlbedoTexture("pbr/greenTiles/albedo.tga");
	cylinderModel2->getMesh(0)->getMaterial()->setMetalnessRoughnessAOTexture("pbr/greenTiles/metalnessRoughnessAO.tga");
	cylinderModel2->getMesh(0)->getMaterial()->setNormalTexture("pbr/greenTiles/normal.tga");*/

	/*
		Creation of entities
	*/

	{	
		auto e = EntityFactory::CreateStaticMapObject("Plane", m_planeModel.get(), nullptr, glm::vec3(0.f, 0.f, 0.f));
		/*e = EntityFactory::CreateStaticMapObject("Cylinder1", cylinderModel0, nullptr, glm::vec3(0.f, 1.f, 0.f));
		e = EntityFactory::CreateStaticMapObject("Cylinder2", cylinderModel1, nullptr, glm::vec3(3.f, 1.f, 0.f));
		e = EntityFactory::CreateStaticMapObject("Cylinder3", cylinderModel2, nullptr, glm::vec3(-3.f, 1.f, 0.f));*/
	}

}

PBRTestState::~PBRTestState() {
	Application::getInstance()->getConsole().removeAllCommandsWithIdentifier("PBRTestState");

	// Show mouse cursor if hidden
	Input::HideCursor(false);

	ECS::Instance()->stopAllSystems();
	ECS::Instance()->destroyAllEntities();

	delete m_octree;

	EventDispatcher::Instance().unsubscribe(Event::Type::WINDOW_RESIZE, this);
}

// Process input for the state
// NOTE: Done every frame
bool PBRTestState::processInput(float dt) {

//#ifdef _DEBUG
	// Add point light at camera pos
	if (Input::WasKeyJustPressed(KeyBinds::ADD_LIGHT)) {
		PointLight pl;
		pl.setPosition(m_cam.getPosition());
		pl.setColor(glm::vec3(Utils::rnd(), Utils::rnd(), Utils::rnd()));
		m_lights.addPointLight(pl);
	}

//#endif

	if (Input::IsKeyPressed(KeyBinds::SET_DIRECTIONAL_LIGHT)) {
		glm::vec3 color(1.0f, 1.0f, 1.0f);
		m_lights.setDirectionalLight(DirectionalLight(color, m_cam.getDirection()));
	}

	// Reload shaders
	if (Input::WasKeyJustPressed(KeyBinds::RELOAD_SHADER)) {
		m_app->getResourceManager().reloadShader<GBufferOutShader>();
	}

	m_camController.update(dt);

	return false;
}


bool PBRTestState::onEvent(const Event& event) {
	switch (event.type) {
	case Event::Type::WINDOW_RESIZE: onResize((const WindowResizeEvent&)event); break;
	default: break;
	}

	return true;
}

bool PBRTestState::onResize(const WindowResizeEvent& event) {
	m_cam.resize(event.width, event.height);
	return true;
}

bool PBRTestState::update(float dt, float alpha) {

	m_lights.updateBufferData();
	return false;
}

bool PBRTestState::fixedUpdate(float dt) {

	m_componentSystems.updateBoundingBoxSystem->update(dt);
	m_componentSystems.octreeAddRemoverSystem->update(dt);

	std::wstring fpsStr = std::to_wstring(m_app->getFPS());

#ifdef DEVELOPMENT
	m_app->getWindow()->setWindowTitle("S.P.L.A.S.H.2.O | Development | "
									   + Application::getPlatformName() + " | FPS: " + std::to_string(m_app->getFPS()));
#endif

	static float counter = 0.0f;
	static float size = 1.0f;
	static float change = 0.4f;

	counter += dt * 2.0f;

	// Will probably need to be called last
	m_componentSystems.entityAdderSystem->update();
	m_componentSystems.entityRemovalSystem->update();

	return false;
}

// Renders the state
// alpha is a the interpolation value (range [0,1]) between the last two snapshots
bool PBRTestState::render(float dt, float alpha) {
	// Clear back buffer
	m_app->getAPI()->clear({ 0.01f, 0.01f, 0.01f, 1.0f });

	// Draw the scene. Entities with model and trans component will be rendered.
	m_componentSystems.beginEndFrameSystem->beginFrame(m_cam);

	m_componentSystems.modelSubmitSystem->submitAll(alpha, dt);
	m_componentSystems.metaballSubmitSystem->submitAll(alpha);
	m_componentSystems.boundingboxSubmitSystem->submitAll();

	m_componentSystems.beginEndFrameSystem->endFrameAndPresent();

	return false;
}

bool PBRTestState::renderImgui(float dt) {
	// The ImGui window is rendered when activated on F10
	ImGui::ShowDemoWindow();
	m_profiler.renderWindow();
	m_renderSettingsWindow.renderWindow();
	m_lightDebugWindow.renderWindow();

	return false;
}

const std::string PBRTestState::createCube(const glm::vec3& position) {
	auto e = ECS::Instance()->createEntity("new cube");
	e->addComponent<ModelComponent>(m_cubeModel.get());
	e->addComponent<TransformComponent>(position);
	return std::string("Added Cube at (" +
		std::to_string(position.x) + ":" +
		std::to_string(position.y) + ":" +
		std::to_string(position.z) + ")");
}