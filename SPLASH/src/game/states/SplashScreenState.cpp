
#include "Sail/../../Sail/src/Network/NetworkModule.hpp"
#include "SplashScreenState.h"

#include "Sail.h"
#include "../libraries/imgui/imgui.h"

#include "Sail/entities/systems/render/BeginEndFrameSystem.h"
#include <string>



SplashScreenState::SplashScreenState(StateStack& stack)
	: State(stack)
{
	m_input = Input::GetInstance();
	m_app = Application::getInstance();

	m_app->getResourceManager().loadTexture("splash_logo_smaller.tga");
	m_modelThread = m_app->pushJobToThreadPool([&](int id) {return loadModels(m_app); });
}

SplashScreenState::~SplashScreenState() {
	m_modelThread.get();
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
	ResourceManager* rm = &app->getResourceManager();
	rm->setDefaultShader(&rm->getShaderSet<GBufferOutShader>());
	std::future<bool> textureThread = m_app->pushJobToThreadPool([&](int id) {return loadTextures(m_app); });

//#ifndef _DEBUG
	rm->loadModel("Doc.fbx");
	rm->loadModel("Torch.fbx");
	rm->loadModel("candleExported.fbx");
	rm->loadModel("Tiles/RoomWall.fbx");
	rm->loadModel("Tiles/RoomDoor.fbx");
	rm->loadModel("Tiles/CorridorDoor.fbx");
	rm->loadModel("Tiles/CorridorWall.fbx"); 
	rm->loadModel("Tiles/RoomCeiling.fbx");
	rm->loadModel("Tiles/RoomFloor.fbx");
	rm->loadModel("Tiles/CorridorCorner.fbx");
	rm->loadModel("Tiles/RoomCorner.fbx"); 
	rm->loadModel("Clutter/Table.fbx");
	rm->loadModel("Clutter/Boxes.fbx");
	rm->loadModel("Clutter/MediumBox.fbx");
	rm->loadModel("Clutter/SquareBox.fbx");
	rm->loadModel("Clutter/Books1.fbx");
	rm->loadModel("Clutter/Screen.fbx");
	rm->loadModel("Clutter/Notepad.fbx");
	rm->loadModel("Clutter/Saftblandare.fbx");
	rm->loadModel("WaterPistol.fbx");
	rm->loadModel("boundingBox.fbx", &rm->getShaderSet<WireframeShader>());
	rm->loadModel("cubeWidth1.fbx");
	rm->loadModel("Clutter/Microscope.fbx");
	rm->loadModel("Clutter/CloningVats.fbx");
	rm->loadModel("Clutter/ControlStation.fbx");
	rm->loadModel("Clutter/PowerUp.fbx");
	rm->loadModel("CleaningBot.fbx");


	//LEAVE THIS FOR A MULTITHREADED FUTURE
//#else
//
//	std::vector <std::future<bool>> modelThreads;
//	
//	modelThreads.push_back(m_app->pushJobToThreadPool([&](int id) {return rm->loadModel("Doc.fbx"); }));
//	modelThreads.push_back(m_app->pushJobToThreadPool([&](int id) {return rm->loadModel("candleExported.fbx"); }));
//	modelThreads.push_back(m_app->pushJobToThreadPool([&](int id) {return rm->loadModel("Tiles/tileFlat.fbx"); }));
//	modelThreads.push_back(m_app->pushJobToThreadPool([&](int id) {return rm->loadModel("Tiles/RoomWall.fbx"); }));
//	modelThreads.push_back(m_app->pushJobToThreadPool([&](int id) {return rm->loadModel("Tiles/tileDoor.fbx"); }));
//	modelThreads.push_back(m_app->pushJobToThreadPool([&](int id) {return rm->loadModel("Tiles/RoomDoor.fbx"); }));
//	modelThreads.push_back(m_app->pushJobToThreadPool([&](int id) {return rm->loadModel("Tiles/CorridorDoor.fbx"); }));
//	modelThreads.push_back(m_app->pushJobToThreadPool([&](int id) {return rm->loadModel("Tiles/CorridorWall.fbx"); }));
//	modelThreads.push_back(m_app->pushJobToThreadPool([&](int id) {return rm->loadModel("Tiles/RoomCeiling.fbx"); }));
//	modelThreads.push_back(m_app->pushJobToThreadPool([&](int id) {return rm->loadModel("Tiles/CorridorFloor.fbx"); }));
//	modelThreads.push_back(m_app->pushJobToThreadPool([&](int id) {return rm->loadModel("Tiles/RoomFloor.fbx"); }));
//	modelThreads.push_back(m_app->pushJobToThreadPool([&](int id) {return rm->loadModel("Tiles/CorridorCeiling.fbx"); }));
//	modelThreads.push_back(m_app->pushJobToThreadPool([&](int id) {return rm->loadModel("Tiles/CorridorCorner.fbx"); }));
//	modelThreads.push_back(m_app->pushJobToThreadPool([&](int id) {return rm->loadModel("Tiles/RoomCorner.fbx"); }));
//	modelThreads.push_back(m_app->pushJobToThreadPool([&](int id) {return rm->loadModel("Clutter/SmallObject.fbx"); }));
//	modelThreads.push_back(m_app->pushJobToThreadPool([&](int id) {return rm->loadModel("Clutter/MediumObject.fbx"); }));
//	modelThreads.push_back(m_app->pushJobToThreadPool([&](int id) {return rm->loadModel("Clutter/LargeObject.fbx"); }));
//
//
//
//	for (auto& modelThread : modelThreads) {
//		modelThread.get();
//	}
//#endif


	textureThread.get();
	rm->clearSceneData();
	return true;
}

bool SplashScreenState::loadTextures(Application* app) {
	ResourceManager* rm = &app->getResourceManager();

	rm->loadTexture("pbr/DDS/Torch/Torch_MRAO.dds");
	rm->loadTexture("pbr/DDS/Torch/Torch_NM.dds");
	rm->loadTexture("pbr/DDS/Torch/Torch_Albedo.dds");

	rm->loadTexture("pbr/DDS/Tiles/RoomWallMRAO.dds");
	rm->loadTexture("pbr/DDS/Tiles/RoomWallNM.dds");
	rm->loadTexture("pbr/DDS/Tiles/RoomWallAlbedo.dds");

	rm->loadTexture("pbr/DDS/Tiles/RS_MRAo.dds");
	rm->loadTexture("pbr/DDS/Tiles/RS_NM.dds");
	rm->loadTexture("pbr/DDS/Tiles/RS_Albedo.dds");

	rm->loadTexture("pbr/DDS/Tiles/RD_MRAo.dds");
	rm->loadTexture("pbr/DDS/Tiles/RD_NM.dds");
	rm->loadTexture("pbr/DDS/Tiles/RD_Albedo.dds");

	rm->loadTexture("pbr/DDS/Tiles/CD_MRAo.dds");
	rm->loadTexture("pbr/DDS/Tiles/CD_NM.dds");
	rm->loadTexture("pbr/DDS/Tiles/CD_Albedo.dds");

	rm->loadTexture("pbr/DDS/Tiles/CW_MRAo.dds");
	rm->loadTexture("pbr/DDS/Tiles/CW_NM.dds");
	rm->loadTexture("pbr/DDS/Tiles/CW_Albedo.dds");

	rm->loadTexture("pbr/DDS/Tiles/F_MRAo.dds");
	rm->loadTexture("pbr/DDS/Tiles/F_NM.dds");
	rm->loadTexture("pbr/DDS/Tiles/F_Albedo.dds");

	rm->loadTexture("pbr/DDS/Tiles/CF_MRAo.dds");
	rm->loadTexture("pbr/DDS/Tiles/CF_NM.dds");
	rm->loadTexture("pbr/DDS/Tiles/CF_Albedo.dds");

	rm->loadTexture("pbr/DDS/Tiles/CC_MRAo.dds");
	rm->loadTexture("pbr/DDS/Tiles/CC_NM.dds");
	rm->loadTexture("pbr/DDS/Tiles/CC_Albedo.dds");

	rm->loadTexture("pbr/DDS/Tiles/RC_MRAo.dds");
	rm->loadTexture("pbr/DDS/Tiles/RC_NM.dds");
	rm->loadTexture("pbr/DDS/Tiles/RC_Albedo.dds");

	rm->loadTexture("pbr/DDS/Tiles/Corner_MRAo.dds");
	rm->loadTexture("pbr/DDS/Tiles/Corner_NM.dds");
	rm->loadTexture("pbr/DDS/Tiles/Corner_Albedo.dds");

	rm->loadTexture("pbr/metal/metalnessRoughnessAO.tga");
	rm->loadTexture("pbr/metal/normal.tga");
	rm->loadTexture("pbr/metal/albedo.tga");

	rm->loadTexture("pbr/DDS/Clutter/Saftblandare_MRAO.dds");
	rm->loadTexture("pbr/DDS/Clutter/Saftblandare_NM.dds");
	rm->loadTexture("pbr/DDS/Clutter/Saftblandare_Albedo.dds");

	rm->loadTexture("pbr/DDS/Clutter/Boxes_MRAO.dds");
	rm->loadTexture("pbr/DDS/Clutter/Boxes_NM.dds");
	rm->loadTexture("pbr/DDS/Clutter/Boxes_Albedo.dds");

	rm->loadTexture("pbr/DDS/Clutter/Table_MRAO.dds");
	rm->loadTexture("pbr/DDS/Clutter/Table_NM.dds");
	rm->loadTexture("pbr/DDS/Clutter/Table_Albedo.dds");

	rm->loadTexture("pbr/DDS/Clutter/Book_MRAO.dds");
	rm->loadTexture("pbr/DDS/Clutter/Book_NM.dds");
	rm->loadTexture("pbr/DDS/Clutter/Book1_Albedo.dds");
	rm->loadTexture("pbr/DDS/Clutter/Book2_Albedo.dds");

	rm->loadTexture("pbr/DDS/Clutter/SquareBox_MRAO.dds");
	rm->loadTexture("pbr/DDS/Clutter/SquareBox_NM.dds");
	rm->loadTexture("pbr/DDS/Clutter/SquareBox_Albedo.dds");

	rm->loadTexture("pbr/DDS/Clutter/MediumBox_MRAO.dds");
	rm->loadTexture("pbr/DDS/Clutter/MediumBox_NM.dds");
	rm->loadTexture("pbr/DDS/Clutter/MediumBox_Albedo.dds");

	rm->loadTexture("pbr/DDS/Clutter/Screen_Albedo.dds");
	rm->loadTexture("pbr/DDS/Clutter/Screen_MRAO.dds");
	rm->loadTexture("pbr/DDS/Clutter/Screen_NM.dds");

	rm->loadTexture("pbr/DDS/Clutter/Notepad_Albedo.dds");
	rm->loadTexture("pbr/DDS/Clutter/Notepad_MRAO.dds");
	rm->loadTexture("pbr/DDS/Clutter/Notepad_NM.dds");

	rm->loadTexture("pbr/DDS/Clutter/Microscope_Albedo.dds");
	rm->loadTexture("pbr/DDS/Clutter/Microscope_MRAO.dds");
	rm->loadTexture("pbr/DDS/Clutter/Microscope_NM.dds");
	
	rm->loadTexture("pbr/DDS/Clutter/CloningVats_Albedo.dds");
	rm->loadTexture("pbr/DDS/Clutter/CloningVats_MRAO.dds");
	rm->loadTexture("pbr/DDS/Clutter/CloningVats_NM.dds");

	rm->loadTexture("pbr/DDS/Clutter/ControlStation_Albedo.dds");
	rm->loadTexture("pbr/DDS/Clutter/ControlStation_MRAO.dds");
	rm->loadTexture("pbr/DDS/Clutter/ControlStation_NM.dds");

	rm->loadTexture("pbr/DDS/Clutter/powerUp.dds");
	rm->loadTexture("pbr/DDS/Clutter/powerUp_MRAO.dds");

	rm->loadTexture("Icons/TorchLeft.tga");
	rm->loadTexture("Icons/TorchThrow2.tga");
	rm->loadTexture("Icons/PlayersLeft.tga");
	rm->loadTexture("Icons/CantShootIcon1.tga");
	rm->loadTexture("Icons/CantShootIcon2.tga");

	rm->loadTexture("pbr/DDS/CleaningRobot/CleaningBot_Albedo.dds");
	rm->loadTexture("pbr/DDS/CleaningRobot/CleaningBot_NM.dds");
	rm->loadTexture("pbr/DDS/CleaningRobot/CleaningBot_MRAO.dds");

	rm->loadTexture("pbr/DDS/Doc/Doc_Albedo.dds");
	rm->loadTexture("pbr/DDS/Doc/Doc_MRAO.dds");
	rm->loadTexture("pbr/DDS/Doc/Doc_NM.dds");

	rm->loadTexture("pbr/DDS/WaterGun/Watergun_Albedo.dds");
	rm->loadTexture("pbr/DDS/WaterGun/Watergun_MRAO.dds");
	rm->loadTexture("pbr/DDS/WaterGun/Watergun_NM.dds");

	return true;
}