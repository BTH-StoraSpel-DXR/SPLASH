#pragma once
#include <fstream>
#include <chrono>
#include <ctime>    

constexpr char REPLAY_PATH[] = "replays";
constexpr char REPLAY_TEMP_PATH[] = "replays/temp";
constexpr char REPLAY_EXTENSION[] = ".SPLASH";


class MatchRecordSystem {
public:
	MatchRecordSystem(); 
	~MatchRecordSystem();

	void initRecording();
	bool initReplay(std::string replayName);
	bool endReplay();

	int status = 0;

	void recordPackages(std::queue<std::string> data);
	void replayPackages(std::queue<std::string>& data);

	static void CleanOldReplays();
private:
	std::ofstream recorded;
	std::ifstream replay;
};