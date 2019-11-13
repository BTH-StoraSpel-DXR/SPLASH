
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

	m_modelThread = m_app->pushJobToThreadPool([&](int id) {return loadModels(m_app); });
}

SplashScreenState::~SplashScreenState() {
	m_modelThread.get();
}


bool SplashScreenState::processInput(float dt) {
	return false;
}

bool SplashScreenState::update(float dt, float alpha) {
	return false;
}

bool SplashScreenState::render(float dt, float alpha) {
	return false;
}

bool SplashScreenState::renderImgui(float dt) {
	return false;
}

bool SplashScreenState::onEvent(Event& event) {
	return false;
}


bool SplashScreenState::loadModels(Application* app) {
	ResourceManager* rm = &app->getResourceManager();
	rm->setDefaultShader(&rm->getShaderSet<GBufferOutShader>());
	std::future<bool> textureThread = m_app->pushJobToThreadPool([&](int id) {return loadTextures(m_app); });

//#ifndef _DEBUG
	rm->loadModel("Doc.fbx");
	rm->loadModel("Torch.fbx");
	rm->loadModel("candleExported.fbx");
	rm->loadModel("Tiles/tileFlat.fbx");
	rm->loadModel("Tiles/RoomWall.fbx");
	rm->loadModel("Tiles/tileDoor.fbx"); 
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
	return true;
}

bool SplashScreenState::loadTextures(Application* app) {
	ResourceManager* rm = &app->getResourceManager();

	rm->loadTexture("pbr/Torch/Torch_MRAO.tga");
	rm->loadTexture("pbr/Torch/Torch_NM.tga");
	rm->loadTexture("pbr/Torch/Torch_Albedo.tga");

	rm->loadTexture("pbr/Tiles/RoomWallMRAO.tga");
	rm->loadTexture("pbr/Tiles/RoomWallNM.tga");
	rm->loadTexture("pbr/Tiles/RoomWallAlbedo.tga");

	rm->loadTexture("pbr/Tiles/RS_MRAo.tga");
	rm->loadTexture("pbr/Tiles/RS_NM.tga");
	rm->loadTexture("pbr/Tiles/RS_Albedo.tga");

	rm->loadTexture("pbr/Tiles/RD_MRAo.tga");
	rm->loadTexture("pbr/Tiles/RD_NM.tga");
	rm->loadTexture("pbr/Tiles/RD_Albedo.tga");

	rm->loadTexture("pbr/Tiles/CD_MRAo.tga");
	rm->loadTexture("pbr/Tiles/CD_NM.tga");
	rm->loadTexture("pbr/Tiles/CD_Albedo.tga");

	rm->loadTexture("pbr/Tiles/CW_MRAo.tga");
	rm->loadTexture("pbr/Tiles/CW_NM.tga");
	rm->loadTexture("pbr/Tiles/CW_Albedo.tga");

	rm->loadTexture("pbr/Tiles/F_MRAo.tga");
	rm->loadTexture("pbr/Tiles/F_NM.tga");
	rm->loadTexture("pbr/Tiles/F_Albedo.tga");

	rm->loadTexture("pbr/Tiles/CF_MRAo.tga");
	rm->loadTexture("pbr/Tiles/CF_NM.tga");
	rm->loadTexture("pbr/Tiles/CF_Albedo.tga");

	rm->loadTexture("pbr/Tiles/CC_MRAo.tga");
	rm->loadTexture("pbr/Tiles/CC_NM.tga");
	rm->loadTexture("pbr/Tiles/CC_Albedo.tga");

	rm->loadTexture("pbr/Tiles/RC_MRAo.tga");
	rm->loadTexture("pbr/Tiles/RC_NM.tga");
	rm->loadTexture("pbr/Tiles/RC_Albedo.tga");

	rm->loadTexture("pbr/Tiles/Corner_MRAo.tga");
	rm->loadTexture("pbr/Tiles/Corner_NM.tga");
	rm->loadTexture("pbr/Tiles/Corner_Albedo.tga");

	rm->loadTexture("pbr/metal/metalnessRoughnessAO.tga");
	rm->loadTexture("pbr/metal/normal.tga");
	rm->loadTexture("pbr/metal/albedo.tga");

	rm->loadTexture("pbr/Clutter/Saftblandare_MRAO.tga");
	rm->loadTexture("pbr/Clutter/Saftblandare_NM.tga");
	rm->loadTexture("pbr/Clutter/Saftblandare_Albedo.tga");

	rm->loadTexture("pbr/Clutter/Boxes_MRAO.tga");
	rm->loadTexture("pbr/Clutter/Boxes_NM.tga");
	rm->loadTexture("pbr/Clutter/Boxes_Albedo.tga");

	rm->loadTexture("pbr/Clutter/Table_MRAO.tga");
	rm->loadTexture("pbr/Clutter/Table_NM.tga");
	rm->loadTexture("pbr/Clutter/Table_Albedo.tga");

	rm->loadTexture("pbr/Clutter/Book_MRAO.tga");
	rm->loadTexture("pbr/Clutter/Book_NM.tga");
	rm->loadTexture("pbr/Clutter/Book1_Albedo.tga");
	rm->loadTexture("pbr/Clutter/Book2_Albedo.tga");

	rm->loadTexture("pbr/Clutter/SquareBox_MRAO.tga");
	rm->loadTexture("pbr/Clutter/SquareBox_NM.tga");
	rm->loadTexture("pbr/Clutter/SquareBox_Albedo.tga");

	rm->loadTexture("pbr/Clutter/MediumBox_MRAO.tga");
	rm->loadTexture("pbr/Clutter/MediumBox_NM.tga");
	rm->loadTexture("pbr/Clutter/MediumBox_Albedo.tga");

	rm->loadTexture("pbr/Clutter/Screen_Albedo.tga");
	rm->loadTexture("pbr/Clutter/Screen_MRAO.tga");
	rm->loadTexture("pbr/Clutter/Screen_NM.tga");

	rm->loadTexture("pbr/Clutter/Notepad_Albedo.tga");
	rm->loadTexture("pbr/Clutter/Notepad_MRAO.tga");
	rm->loadTexture("pbr/Clutter/Notepad_NM.tga");

	rm->loadTexture("pbr/Clutter/Microscope_Albedo.tga");
	rm->loadTexture("pbr/Clutter/Microscope_MRAO.tga");
	rm->loadTexture("pbr/Clutter/Microscope_NM.tga");

	return false;
}