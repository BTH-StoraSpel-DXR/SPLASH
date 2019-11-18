#pragma once

// State identifiers specified below

namespace States {

	enum ID : char{
		None,
		SplashScreen,
		MainMenu,
		Lobby,
		HostLobby,
		JoinLobby,
		LoadingScreen,
		Game,
		InGameMenu,
		EndGame,
		Score,
		PBRTest,
		PerformanceTest
	};

}