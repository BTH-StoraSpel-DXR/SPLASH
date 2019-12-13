#include "pch.h"
#include "InGameGui.h"
#include "Sail/Application.h"
#include "Sail/utils/SailImGui/CustomImGuiComponents/CustomImGuiComponents.h"
#include "Sail/utils/GameDataTracker.h"

#include "Sail/entities/components/SanityComponent.h"
#include "Sail/entities/components/SprintingComponent.h"
#include "Sail/entities/components/CrosshairComponent.h"
#include "Sail/entities/components/CandleComponent.h"
#include "Sail/entities/components/SpectatorComponent.h"
#include "Sail/entities/components/GunComponent.h"
#include "Sail/entities/components/PowerUp/PowerUpComponent.h"
#include "Sail/entities/systems/Gameplay/candles/CandleHealthSystem.h"

InGameGui::InGameGui(bool showWindow) {
}

InGameGui::~InGameGui() {
}

void InGameGui::renderWindow() {
	Application* app=Application::getInstance();
	float screenWidth = app->getWindow()->getWindowWidth();
	float screenHeight = app->getWindow()->getWindowHeight();
	float progresbarLenght = 300;
	float progresbarHeight = 40;
	float outerPadding = 50;

	bool drawCrossHair = true;
	ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse;
	flags |= ImGuiWindowFlags_NoResize;
	flags |= ImGuiWindowFlags_NoMove;
	flags |= ImGuiWindowFlags_NoNav; 
	flags |= ImGuiWindowFlags_NoBringToFrontOnFocus;
	flags |= ImGuiWindowFlags_NoTitleBar;
	flags |= ImGuiWindowFlags_AlwaysAutoResize; 
	flags |= ImGuiWindowFlags_NoSavedSettings;
	flags |= ImGuiWindowFlags_NoBackground;



	//ImGui::SetNextWindowSize(ImVec2(100,100));
	ImGui::Begin("FXAAON", nullptr, flags);
	//ImGui::SetWindowFontScale(Application::getInstance()->getImGuiHandler()->getFontScaling("text"));
	ImGui::SetWindowFontScale(2.0f);

	ImGui::Text("FXAA ON");
	ImGui::SetWindowPos(ImVec2(screenWidth / 2 + screenWidth / 4, 40));
	ImGui::End();

	//ImGui::SetNextWindowSize(ImVec2(100, 100));
	ImGui::Begin("FXAAOFF", nullptr, flags);
	ImGui::SetWindowFontScale(2.f);

	ImGui::Text("FXAA OFF");
	ImGui::SetWindowPos(ImVec2(screenWidth / 4, 40));
	ImGui::End();


	//
	//if (m_crosshairEntity && drawCrossHair && !m_player->hasComponent<SpectatorComponent>()) {
	//	if (!m_crosshairEntity->getComponent<CrosshairComponent>()->sprinting) {
	//		renderCrosshair(screenWidth, screenHeight);
	//	}
	//}

	//if (m_player && !m_player->hasComponent<SpectatorComponent>()) {
	//	ImGui::SetNextWindowPos(ImVec2(
	//		screenWidth - progresbarLenght,
	//		screenHeight - progresbarHeight * 3
	//	));

	//	ImGui::SetNextWindowSize(ImVec2(
	//		progresbarLenght,
	//		progresbarHeight - progresbarHeight * 2.2
	//	));

	//	SanityComponent* c1 = m_player->getComponent<SanityComponent>();
	//	SprintingComponent* c2 = m_player->getComponent<SprintingComponent>();
	//	CandleComponent* c3;
	//	GunComponent* c4 = m_player->getComponent<GunComponent>();
	//	PowerUpComponent* c5 = m_player->getComponent<PowerUpComponent>();
	//	ImGui::Begin("GUI", NULL, flags);
	//	ImGui::SetWindowFontScale(Application::getInstance()->getImGuiHandler()->getFontScaling("text"));

	//	for (auto e : m_player->getChildEntities()) {
	//		if (!e->isAboutToBeDestroyed() && e->hasComponent<CandleComponent>()) {
	//			c3 = e->getComponent<CandleComponent>();
	//		}
	//	}
	//	if (c1) {
	//		float val = c1->sanity / 100.f;
	//		float val_inv = 1 - val;

	//		CustomImGui::CustomProgressBar(val, ImVec2(-1, 0), "Sanity", ImVec4(1 - val_inv * 0.3, 0.6 - val_inv * 0.6, 0, 1));
	//	}

	//	if (c2) {
	//		float val;
	//		float val_inv;
	//		ImVec4 color;

	//		if (c2->exhausted) {
	//			val = (c2->downTimer / MAX_SPRINT_DOWN_TIME);
	//			val_inv = 1 - val;
	//			color = ImVec4(0.5, 0.5, 0.5, 1);
	//		}
	//		else {
	//			val = 1 - (c2->sprintTimer / c2->sprintDuration);
	//			val_inv = 1 - val;
	//			color = ImVec4(1 - val_inv * 0.3, 0.6 - val_inv * 0.6, 0, 1);
	//		}
	//		CustomImGui::CustomProgressBar(val, ImVec2(-1, 0), "Stamina", color);
	//	}
	//	if (c3) {
	//		float health = c3->health / 20;
	//		CustomImGui::CustomProgressBar(health, ImVec2(-1, 0), "Health", ImVec4(1,0,0,1));
	//	}
	//	ImGui::End();

	//	ImGui::Begin("TorchThrowButton", NULL, flags);
	//	ImGui::SetWindowFontScale(Application::getInstance()->getImGuiHandler()->getFontScaling("text"));

	//	auto* imguiHandler = app->getImGuiHandler();
	//	if (c3) {
	//		Texture& testTexture = app->getResourceManager().getTexture("Icons/TorchThrow2.tga");

	//		if (c3->isLit && c3->isCarried && c3->candleToggleTimer > 2.f) {
	//			ImGui::Image(imguiHandler->getTextureID(&testTexture), ImVec2(55, 55), ImVec2(0, 0), ImVec2(1, 1), ImVec4(1, 1, 1, 1));
	//		}
	//		else {
	//			ImGui::Image(imguiHandler->getTextureID(&testTexture), ImVec2(55, 55), ImVec2(0, 0), ImVec2(1, 1), ImVec4(0.3f, 0.3f, 0.3f, 1));
	//		}
	//		ImGui::SetWindowPos(ImVec2(screenWidth - ImGui::GetWindowSize().x - 300, screenHeight - ImGui::GetWindowSize().y - 50));
	//	}
	//	ImGui::End();
	//	if (c3 && !c3->isLit) {
	//		Texture& cantShootTexture = app->getResourceManager().getTexture("Icons/CantShootIcon1.tga");
	//		ImGui::Begin("CantShoot", nullptr, flags);
	//		ImGui::SetWindowFontScale(Application::getInstance()->getImGuiHandler()->getFontScaling("text"));

	//		ImGui::Image(imguiHandler->getTextureID(&cantShootTexture), ImVec2(64, 64));
	//		ImGui::SetWindowPos(ImVec2(
	//			screenWidth * 0.505f - 40,
	//			screenHeight * 0.55f - 40
	//		));
	//		ImGui::End();
	//		drawCrossHair = false;
	//	}

	//	int nrOfTorchesLeft = GameDataTracker::getInstance().getTorchesLeft();
	//	Texture& testTexture = app->getResourceManager().getTexture("Icons/TorchLeft.tga");
	//	ImGui::Begin("TorchesLeft", nullptr, flags);
	//	ImGui::SetWindowFontScale(Application::getInstance()->getImGuiHandler()->getFontScaling("text"));

	//	for (int i = 0; i < nrOfTorchesLeft; i++) {
	//		ImGui::Image(imguiHandler->getTextureID(&testTexture), ImVec2(55, 55));
	//		ImGui::SameLine(0.f, 0);
	//	}
	//	ImGui::SetWindowPos(ImVec2(
	//		screenWidth - ImGui::GetWindowSize().x,
	//		screenHeight - ImGui::GetWindowSize().y - 110
	//	));
	//	ImGui::End();

	//	if (c4) {
	//		float val_inv = c4->gunOverloadThreshold;
	//		float val = 1-c4->gunOverloadvalue/val_inv;
	//		ImGui::SetNextWindowSize(ImVec2(progresbarLenght, progresbarHeight - progresbarHeight * 2.2));
	//		ImGui::Begin("PowerBar", nullptr, flags);
	//		ImGui::SetWindowFontScale(Application::getInstance()->getImGuiHandler()->getFontScaling("text"));

	//		CustomImGui::CustomProgressBar(val, ImVec2(-1, 0), "", ImVec4((1-val) * 0.3, 0.6 - (1-val) * 0.6, 1, 1));
	//		ImGui::SetWindowPos(ImVec2(screenWidth / 2-progresbarLenght/2, screenHeight - 40));
	//		ImGui::End();
	//	}

	//	if (c5) {
	//		float runspeed = c5->powerUps.at(0).time;
	//		float stamina = c5->powerUps.at(1).time;
	//		float shower = c5->powerUps.at(2).time;
	//		float powerwash = c5->powerUps.at(3).time;
	//		float centerX = screenWidth * 0.505f;
	//		float centerY = screenHeight * 0.55f;

	//		ImGuiWindowFlags poFlags = ImGuiWindowFlags_NoCollapse;
	//		poFlags |= ImGuiWindowFlags_NoResize;
	//		poFlags |= ImGuiWindowFlags_NoMove;
	//		poFlags |= ImGuiWindowFlags_NoNav;
	//		poFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus;
	//		poFlags |= ImGuiWindowFlags_NoTitleBar;
	//		poFlags |= ImGuiWindowFlags_NoBackground;
	//		ImGui::Begin("PowerUps", NULL, poFlags);
	//		ImGui::SetWindowFontScale(Application::getInstance()->getImGuiHandler()->getFontScaling("text"));
	//		ImGui::SetWindowSize(ImVec2(120, 30));
	//		ImGui::SetWindowPos(ImVec2(centerX - ImGui::GetWindowSize().x * 0.5f, screenHeight-60));
	//		ImDrawList* powerUps = ImGui::GetWindowDrawList();
	//		if (runspeed > 3 || ((int)(runspeed * 10)) % 2 == 1) {
	//			powerUps->AddCircleFilled(ImVec2(centerX - 45, screenHeight - 45), 7.f, ImU32(ImColor(ImVec4(0, 1, 0, 1))));
	//		}
	//		if (stamina > 3 || ((int)(stamina* 10)) % 2 == 1) {
	//			powerUps->AddCircleFilled(ImVec2(centerX - 15, screenHeight - 45), 7.f, ImU32(ImColor(ImVec4(1, 1, 0, 1))));
	//		}
	//		if (shower > 3 || ((int)(shower * 10)) % 2 == 1) {
	//			powerUps->AddCircleFilled(ImVec2(centerX + 15, screenHeight - 45), 7.f, ImU32(ImColor(ImVec4(1, 0, 0, 1))));
	//		}
	//		if (powerwash > 3 || ((int)(powerwash * 10)) % 2 == 1) {
	//			powerUps->AddCircleFilled(ImVec2(centerX + 45, screenHeight - 45), 7.f, ImU32(ImColor(ImVec4(0, 0, 1, 1))));
	//		}
	//		ImGui::End();
	//	}
	//}
	
	//if (m_player) {
	//	//int nrOfPlayersLeft = GameDataTracker::getInstance().getPlayersLeft();
	//	int nrOfPlayersLeft = ECS::Instance()->getSystem<CandleHealthSystem>()->getNumLivingEntites();

	//	auto* imguiHandler = app->getImGuiHandler();
	//	Texture& testTexture = app->getResourceManager().getTexture("Icons/PlayersLeft.tga");
	//	ImGui::Begin("PlayersLeftIcon", nullptr, flags);
	//	ImGui::Image(imguiHandler->getTextureID(&testTexture), ImVec2(32, 32));
	//	ImGui::SetWindowPos(ImVec2(
	//		1, 8
	//	));

	//	ImGui::End();
	//	ImGui::Begin("PlayersLeftNumber", nullptr, flags);
	//	//ImGui::PushFont(imguiHandler->getFont("Beb50"));
	//	ImGui::SetWindowFontScale(Application::getInstance()->getImGuiHandler()->getFontScaling("Header1"));

	//	std::string progress = std::to_string(nrOfPlayersLeft);
	//	ImGui::Text(progress.c_str());
	//	ImGui::SetWindowPos(ImVec2(
	//		40, 1
	//	));
	//	//ImGui::PopFont();


	//	ImGui::End();
	//}
}

void InGameGui::setPlayer(Entity* player) {
	m_player = player;
}

void InGameGui::setCrosshair(Entity* pCrosshairEntity) {
	m_crosshairEntity = pCrosshairEntity;
}

void InGameGui::renderCrosshair(float screenWidth, float screenHeight) {
	// Crosshair settings window
	CrosshairComponent* c = m_crosshairEntity->getComponent<CrosshairComponent>();

	// Crosshair
	ImVec2 crosshairSize{
		c->size,
		c->size
	};
	ImVec2 center{
		screenWidth * 0.505f,
		screenHeight * 0.55f
	};
	ImVec2 topLeft{
		center.x - crosshairSize.x * 0.5f,
		center.y - crosshairSize.y * 0.5f
	};
	ImVec2 top{
		topLeft.x + crosshairSize.x * 0.5f,
		topLeft.y
	};
	ImVec2 bot{
		center.x,
		center.y + crosshairSize.y * 0.5f
	};
	ImVec2 right{
		center.x + crosshairSize.x * 0.5f,
		center.y
	};
	ImVec2 left{
		center.x - crosshairSize.x * 0.5f,
		center.y
	};

	ImGui::SetNextWindowPos(topLeft);
	ImGui::SetNextWindowSize(crosshairSize);

	ImGuiWindowFlags crosshairFlags = ImGuiWindowFlags_NoCollapse;
	crosshairFlags |= ImGuiWindowFlags_NoResize;
	crosshairFlags |= ImGuiWindowFlags_NoMove;
	crosshairFlags |= ImGuiWindowFlags_NoNav;
	crosshairFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus;
	crosshairFlags |= ImGuiWindowFlags_NoTitleBar;
	crosshairFlags |= ImGuiWindowFlags_NoBackground;
	ImGui::Begin("Crosshair", NULL, crosshairFlags);

	const ImU32 color = ImColor(c->color);

	ImVec2 center_padded_top{
		top.x,
		center.y - c->centerPadding
	};
	ImVec2 center_padded_bot{
		top.x,
		center.y + c->centerPadding
	};
	ImVec2 center_padded_right{
		center.x + c->centerPadding,
		right.y
	};
	ImVec2 center_padded_left{
		center.x - c->centerPadding,
		left.y
	};

	ImDrawList* draw_list = ImGui::GetWindowDrawList();

	//		|
	//   
	//
	draw_list->AddLine(
		top,
		center_padded_top,
		color,
		c->thickness
	);

	//		|
	//   
	//		|
	draw_list->AddLine(
		bot,
		center_padded_bot,
		color,
		c->thickness
	);

	//		|
	//		    --
	//		|
	draw_list->AddLine(
		right,
		center_padded_right,
		color,
		c->thickness
	);

	//		|
	//	--	   --
	//		|
	draw_list->AddLine(
		left,
		center_padded_left,
		color,
		c->thickness
	);



	// Set to True/False by  CrosshairSystem
	if (c->currentlyAltered) {
		ImVec2 topRight{
			right.x,
			top.y
		};
		ImVec2 botRight{
			right.x,
			center.y + c->size * 0.5f
		};
		ImVec2 botLeft{
			left.x,
			botRight.y
		};

		ImVec2 center_padded_topLeft{
			center.x - c->centerPadding,
			center.y - c->centerPadding
		};
		ImVec2 center_padded_topRight{
			center.x + c->centerPadding,
			center.y - c->centerPadding
		};
		ImVec2 center_padded_botRight{
			center.x + c->centerPadding,
			center.y + c->centerPadding
		};
		ImVec2 center_padded_botLeft{
			center.x - c->centerPadding,
			center.y + c->centerPadding
		};

		// Set alpha-value of the color based on how long it has been altered for (F1->0)
		ImVec4 onHitColor = c->color;
		onHitColor.w = 1 - (c->passedTimeSinceAlteration / c->durationOfAlteredCrosshair);
		const ImU32 onHitcolor = ImColor(onHitColor);

		//	\
		//
		//
		// Draw an additional cross
		draw_list->AddLine(
			topLeft,
			center_padded_topLeft,
			onHitcolor,
			c->thickness
		);
		//	\	/
		//
		//
		// Draw an additional cross
		draw_list->AddLine(
			topRight,
			center_padded_topRight,
			onHitcolor,
			c->thickness
		);
		//	\	/
		//
		//		\
		// Draw an additional cross
		draw_list->AddLine(
			botRight,
			center_padded_botRight,
			onHitcolor,
			c->thickness
		);
		//	\	/
		//
		//	/   \
		// Draw an additional cross
		draw_list->AddLine(
			botLeft,
			center_padded_botLeft,
			onHitcolor,
			c->thickness
		);
	}

	ImGui::End();
}