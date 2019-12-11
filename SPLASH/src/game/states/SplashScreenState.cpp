#include "Sail/../../Sail/src/Network/NetworkModule.hpp"
#include "SplashScreenState.h"

#include "Sail.h"
#include "../libraries/imgui/imgui.h"

#include "Sail/entities/systems/render/BeginEndFrameSystem.h"
#include <string>
#include <Psapi.h>

#include "Sail/utils/GUISettings.h"
#include "Sail/resources/loaders/NotFBXLoader.h"
//#define CREATE_NOT_FBX

#include "Sail/graphics/shader/compute/AnimationUpdateComputeShader.h"
#include "Sail/graphics/shader/compute/ParticleComputeShader.h"

#define MULTI_THREADED_LOADING
//#define WAIT_FOR_MEMORY
//#define DO_NOT_LOAD_MODELS

SplashScreenState::SplashScreenState(StateStack& stack)
	: State(stack)
{
	m_input = Input::GetInstance();
	m_app = Application::getInstance();
	ResourceManager& rm = m_app->getResourceManager();

	unsigned int byteSize = rm.getTextureByteSize();

	loadTexture(rm, "splash_logo_smaller.tga");

	byteSize = rm.getTextureByteSize();

	rm.loadShaderSet<AnimationUpdateComputeShader>();
	rm.loadShaderSet<GBufferWireframe>();
	rm.setDefaultShader(&rm.getShaderSet<GBufferOutShader>());

#ifdef MULTI_THREADED_LOADING
	m_modelThread = m_app->pushJobToThreadPool([&](int id) {return loadModels(m_app); });
#else
	loadModels(m_app);
#endif

}
SplashScreenState::~SplashScreenState() {
	m_modelThread.get();
	//m_app->getResourceManager().releaseTextureUploadBuffers();
}

bool SplashScreenState::processInput(float dt) {
	return true;
}
bool SplashScreenState::update(float dt, float alpha) {
	return true;
}
bool SplashScreenState::render(float dt, float alpha) {
	return true;
}
bool SplashScreenState::renderImgui(float dt) {
	return true;
}
bool SplashScreenState::onEvent(const Event& event) {
	return true;
}

bool SplashScreenState::loadModels(Application* app) {

	ResourceManager& rm = app->getResourceManager();

#ifdef MULTI_THREADED_LOADING
	std::future<bool> textureThread = m_app->pushJobToThreadPool([&](int id) {return loadTextures(m_app); });
#else
	loadTextures(m_app);
#endif

//#ifndef _DEBUG

	//NotFBXLoader::Save("Doc.notfbx", &rm->getModel("Doc"), &rm->getAnimationStack("Doc"));

	//Model* m = nullptr;
	//AnimationStack* a = nullptr;
	//NotFBXLoader::Load("Doc.notfbx", m, a);	

	std::string extension = "";
	ResourceManager::ImporterType type;


#ifdef CREATE_NOT_FBX
	extension = ".fbx";	
	type = ResourceManager::ImporterType::SAIL_FBXSDK;
#else
	extension = ".notfbx";
	type = ResourceManager::ImporterType::SAIL_NOT_FBXSDK;
#endif // CREATE_NOT_FBX

	loadModel(rm, "Doc" + extension, nullptr, type);
	loadModel(rm, "Torch" + extension, nullptr, type);
	loadModel(rm, "Tiles/RoomWall" + extension, nullptr, type);
	loadModel(rm, "Tiles/RoomDoor" + extension, nullptr, type);
	loadModel(rm, "Tiles/CorridorDoor" + extension, nullptr, type);
	loadModel(rm, "Tiles/CorridorWall" + extension, nullptr, type); 
	loadModel(rm, "Tiles/RoomCeiling" + extension, nullptr, type);
	loadModel(rm, "Tiles/RoomFloor" + extension, nullptr, type);
	loadModel(rm, "Tiles/CorridorCorner" + extension, nullptr, type);
	loadModel(rm, "Tiles/RoomCorner" + extension, nullptr, type); 
	loadModel(rm, "Clutter/Table" + extension, nullptr, type);
	loadModel(rm, "Clutter/Boxes" + extension, nullptr, type);
	loadModel(rm, "Clutter/MediumBox" + extension, nullptr, type);
	loadModel(rm, "Clutter/SquareBox" + extension, nullptr, type);
	loadModel(rm, "Clutter/Books1" + extension, nullptr, type);
	loadModel(rm, "Clutter/Screen" + extension, nullptr, type);
	loadModel(rm, "Clutter/Notepad" + extension, nullptr, type);
	loadModel(rm, "Clutter/Saftblandare" + extension, nullptr, type);
	loadModel(rm, "WaterPistol" + extension, nullptr, type);
	loadModel(rm, "boundingBox" + extension, &rm.getShaderSet<WireframeShader>(), type);
	loadModel(rm, "cubeWidth1" + extension, nullptr, type);
	loadModel(rm, "Clutter/Microscope" + extension, nullptr, type);
	loadModel(rm, "Clutter/CloningVats" + extension, nullptr, type);
	loadModel(rm, "Clutter/ControlStation" + extension, nullptr, type);
	loadModel(rm, "Clutter/PowerUp" + extension, nullptr, type);
	loadModel(rm, "CleaningBot" + extension, nullptr, type);



	rm.clearSceneData();
#ifdef MULTI_THREADED_LOADING
	textureThread.get();
#endif

	SAIL_LOG("Waited for textures and models " + std::to_string(m_nrOfWaits) + " times");

	return true;
}
bool SplashScreenState::loadTextures(Application* app) {
	ResourceManager& rm = app->getResourceManager();

	loadTexture(rm, "pbr/DDS/Torch/Torch_MRAO.dds");
	loadTexture(rm, "pbr/DDS/Torch/Torch_NM.dds");
	loadTexture(rm, "pbr/DDS/Torch/Torch_Albedo.dds");
	loadTexture(rm, "pbr/DDS/Tiles/RoomWallMRAO.dds");
	loadTexture(rm, "pbr/DDS/Tiles/RoomWallNM.dds");
	loadTexture(rm, "pbr/DDS/Tiles/RoomWallAlbedo.dds");
	loadTexture(rm, "pbr/DDS/Tiles/RS_MRAo.dds");
	loadTexture(rm, "pbr/DDS/Tiles/RS_NM.dds");
	loadTexture(rm, "pbr/DDS/Tiles/RS_Albedo.dds");
	loadTexture(rm, "pbr/DDS/Tiles/RD_MRAo.dds");
	loadTexture(rm, "pbr/DDS/Tiles/RD_NM.dds");
	loadTexture(rm, "pbr/DDS/Tiles/RD_Albedo.dds");
	loadTexture(rm, "pbr/DDS/Tiles/CD_MRAo.dds");
	loadTexture(rm, "pbr/DDS/Tiles/CD_NM.dds");
	loadTexture(rm, "pbr/DDS/Tiles/CD_Albedo.dds");
	loadTexture(rm, "pbr/DDS/Tiles/CW_MRAo.dds");
	loadTexture(rm, "pbr/DDS/Tiles/CW_NM.dds");
	loadTexture(rm, "pbr/DDS/Tiles/CW_Albedo.dds");
	loadTexture(rm, "pbr/DDS/Tiles/F_MRAo.dds");
	loadTexture(rm, "pbr/DDS/Tiles/F_NM.dds");
	loadTexture(rm, "pbr/DDS/Tiles/F_Albedo.dds");
	loadTexture(rm, "pbr/DDS/Tiles/CF_MRAo.dds");
	loadTexture(rm, "pbr/DDS/Tiles/CF_NM.dds");
	loadTexture(rm, "pbr/DDS/Tiles/CF_Albedo.dds");
	loadTexture(rm, "pbr/DDS/Tiles/CC_MRAo.dds");
	loadTexture(rm, "pbr/DDS/Tiles/CC_NM.dds");
	loadTexture(rm, "pbr/DDS/Tiles/CC_Albedo.dds");
	loadTexture(rm, "pbr/DDS/Tiles/RC_MRAo.dds");
	loadTexture(rm, "pbr/DDS/Tiles/RC_NM.dds");
	loadTexture(rm, "pbr/DDS/Tiles/RC_Albedo.dds");
	loadTexture(rm, "pbr/DDS/Tiles/Corner_MRAo.dds");
	loadTexture(rm, "pbr/DDS/Tiles/Corner_NM.dds");
	loadTexture(rm, "pbr/DDS/Tiles/Corner_Albedo.dds");
	loadTexture(rm, "pbr/DDS/Clutter/Saftblandare_MRAO.dds");
	loadTexture(rm, "pbr/DDS/Clutter/Saftblandare_NM.dds");
	loadTexture(rm, "pbr/DDS/Clutter/Saftblandare_Albedo.dds");
	loadTexture(rm, "pbr/DDS/Clutter/Boxes_MRAO.dds");
	loadTexture(rm, "pbr/DDS/Clutter/Boxes_NM.dds");
	loadTexture(rm, "pbr/DDS/Clutter/Boxes_Albedo.dds");
	loadTexture(rm, "pbr/DDS/Clutter/Table_MRAO.dds");
	loadTexture(rm, "pbr/DDS/Clutter/Table_NM.dds");
	loadTexture(rm, "pbr/DDS/Clutter/Table_Albedo.dds");
	loadTexture(rm, "pbr/DDS/Clutter/Book_MRAO.dds");
	loadTexture(rm, "pbr/DDS/Clutter/Book_NM.dds");
	loadTexture(rm, "pbr/DDS/Clutter/Book1_Albedo.dds");
	loadTexture(rm, "pbr/DDS/Clutter/Book2_Albedo.dds");
	loadTexture(rm, "pbr/DDS/Clutter/SquareBox_MRAO.dds");
	loadTexture(rm, "pbr/DDS/Clutter/SquareBox_NM.dds");
	loadTexture(rm, "pbr/DDS/Clutter/SquareBox_Albedo.dds");
	loadTexture(rm, "pbr/DDS/Clutter/MediumBox_MRAO.dds");
	loadTexture(rm, "pbr/DDS/Clutter/MediumBox_NM.dds");
	loadTexture(rm, "pbr/DDS/Clutter/MediumBox_Albedo.dds");
	loadTexture(rm, "pbr/DDS/Clutter/Screen_Albedo.dds");
	loadTexture(rm, "pbr/DDS/Clutter/Screen_MRAO.dds");
	loadTexture(rm, "pbr/DDS/Clutter/Screen_NM.dds");
	loadTexture(rm, "pbr/DDS/Clutter/Notepad_Albedo.dds");
	loadTexture(rm, "pbr/DDS/Clutter/Notepad_MRAO.dds");
	loadTexture(rm, "pbr/DDS/Clutter/Notepad_NM.dds");
	loadTexture(rm, "pbr/DDS/Clutter/Microscope_Albedo.dds");
	loadTexture(rm, "pbr/DDS/Clutter/Microscope_MRAO.dds");
	loadTexture(rm, "pbr/DDS/Clutter/Microscope_NM.dds");
	loadTexture(rm, "pbr/DDS/Clutter/CloningVats_Albedo.dds");
	loadTexture(rm, "pbr/DDS/Clutter/CloningVats_MRAO.dds");
	loadTexture(rm, "pbr/DDS/Clutter/CloningVats_NM.dds");
	loadTexture(rm, "pbr/DDS/Clutter/ControlStation_Albedo.dds");
	loadTexture(rm, "pbr/DDS/Clutter/ControlStation_MRAO.dds");
	loadTexture(rm, "pbr/DDS/Clutter/ControlStation_NM.dds");
	loadTexture(rm, "pbr/DDS/CleaningRobot/CleaningBot_Albedo.dds");
	loadTexture(rm, "pbr/DDS/CleaningRobot/CleaningBot_NM.dds");
	loadTexture(rm, "pbr/DDS/CleaningRobot/CleaningBot_MRAO.dds");
	loadTexture(rm, "pbr/DDS/Doc/Doc_Albedo.dds");
	loadTexture(rm, "pbr/DDS/Doc/Doc_MRAO.dds");
	loadTexture(rm, "pbr/DDS/Doc/Doc_NM.dds");
	loadTexture(rm, "pbr/DDS/WaterGun/Watergun_Albedo.dds");
	loadTexture(rm, "pbr/DDS/WaterGun/Watergun_MRAO.dds");
	loadTexture(rm, "pbr/DDS/WaterGun/Watergun_NM.dds");
	loadTexture(rm, "particles/animFire.dds");
	loadTexture(rm, "particles/animSmoke.dds");
	loadTexture(rm, "pbr/DDS/Clutter/powerUp.dds");
	loadTexture(rm, "pbr/DDS/Clutter/powerUp_MRAO.dds");
	
	loadTexture(rm, "Icons/TorchLeft.tga");
	loadTexture(rm, "Icons/TorchThrow2.tga");
	loadTexture(rm, "Icons/PlayersLeft.tga");
	loadTexture(rm, "Icons/CantShootIcon1.tga");
	loadTexture(rm, "Icons/CantShootIcon2.tga");
	
	// Load the missing texture texture
	loadTexture(rm, "missing.tga");

	return true;
}

void SplashScreenState::loadModel(ResourceManager& rm, std::string filename, Shader* shader, const ResourceManager::ImporterType type) {
#ifdef WAIT_FOR_MEMORY
	constexpr size_t UPPER_LIMIT_MB = 230;
	waitUntilMemoryIsBelow(MB_to_Byte(UPPER_LIMIT_MB));
#endif
	rm.loadModel(filename, shader, type);
}

void SplashScreenState::loadTexture(ResourceManager& rm, const char* filename) {
#ifdef WAIT_FOR_MEMORY
	constexpr size_t UPPER_LIMIT_MB = 230;
	waitUntilMemoryIsBelow(MB_to_Byte(UPPER_LIMIT_MB));
#endif
	rm.loadTexture(filename);
}

void SplashScreenState::waitUntilMemoryIsBelow(size_t size) {
	PROCESS_MEMORY_COUNTERS pmc;
	int waitCount = 0;

	while (true) {
		GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc));
		if (pmc.WorkingSetSize < size) {
			break;
		}
		else {
			{
				std::unique_lock<std::mutex> lock(m_waitMutex);
				m_nrOfWaits++;
			}

			// Prevents infinite waiting on a single resource
			if (waitCount++ > 1'000'000) {
				waitCount = 0;
				break;
			}
			continue;
		}
	}
}

size_t SplashScreenState::MB_to_Byte(size_t sizeMB) {
	return sizeMB * 1024 * 1024;
}
