#include "pch.h"
#include "MatchRecordSystem.h"
#include "Network/NWrapperSingleton.h"
#include <filesystem>
static int temp_replay_counter = 0;

MatchRecordSystem::MatchRecordSystem() {
	auto time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	auto time2 = std::ctime(&time);

	if (!std::filesystem::exists(REPLAY_TEMP_PATH)) {
		std::error_code err;
		if (!std::filesystem::create_directories(REPLAY_TEMP_PATH, err)) {
			SAIL_LOG_WARNING(err.message());
			return;
		}
	}
}

MatchRecordSystem::~MatchRecordSystem() {
	recorded.close();
}

void MatchRecordSystem::initRecording() {
	if (status) {
		recorded.close();
		replay.close();
	}
	status = 1;
	std::string path = std::string(REPLAY_TEMP_PATH) + "/LastSessionGame#" + std::to_string(temp_replay_counter++) + REPLAY_EXTENSION;
	recorded = std::ofstream(path, std::ofstream::binary);

	const std::list<Player>& players = NWrapperSingleton::getInstance().getPlayers();
	unsigned len;
	unsigned np = (unsigned)players.size();
	recorded.write((char*)&np, sizeof(np));
	for (auto p : players) {
		len = (unsigned)p.name.length() + 1;
		recorded.write((char*)&p.id, 1);
		recorded.write((char*)&p.team, 1);
		recorded.write((char*)&len, sizeof(len));
		recorded.write(p.name.c_str(), len);
	}

	std::string settings = Application::getInstance()->getSettings().serialize(Application::getInstance()->getSettings().gameSettingsStatic, Application::getInstance()->getSettings().gameSettingsDynamic);
	len = (unsigned)settings.length();
	recorded.write((char*)&len, sizeof(len));
	recorded.write((char*)&settings[0], len);
}

bool MatchRecordSystem::initReplay(std::string replayName) {	
	//replayName = std::string(REPLAY_PATH) + "/" + replayName + std::string(REPLAY_EXTENSION);
	
	if (status) {
		recorded.close();
		replay.close();
	}
	status = 2;
	replay = std::ifstream(replayName, std::ofstream::binary);

	if (!replay.is_open()) {
		return false;
	}

	unsigned np;
	unsigned strLen;
	std::string settings;

	NWrapperSingleton::getInstance().resetPlayerList();

	replay.read((char*)&np, sizeof(np));
	for (int i = 0; i < np; i++) {
		Player p;
		replay.read((char*)&p.id, 1);
		replay.read((char*)&p.team, 1);
		replay.read((char*)&strLen, sizeof(strLen));
		p.name.resize(strLen);
		replay.read(&p.name[0], strLen);

		NWrapperSingleton::getInstance().playerJoined(p);
		NWrapperSingleton::getInstance().getPlayer(p.id)->lastStateStatus.status = -1; //inform that this is not a real player
	}
	
	Player& myPlayer = NWrapperSingleton::getInstance().getMyPlayer();
	myPlayer.id = np;
	myPlayer.team = -1;
	NWrapperSingleton::getInstance().playerJoined(myPlayer);

	replay.read((char*)& strLen, sizeof(strLen));
	settings.resize(strLen);
	replay.read(&settings[0], strLen);

	Application::getInstance()->getSettings().deSerialize(settings, Application::getInstance()->getSettings().gameSettingsStatic, Application::getInstance()->getSettings().gameSettingsDynamic);
	return true;
}

bool MatchRecordSystem::endReplay() {
	if (status == 2) {
		replay.close();
	}

	return true;
}

void MatchRecordSystem::recordPackages(std::queue<std::string> data) {
	unsigned np = data.size();
	recorded.write((char*)& np, sizeof(np));

	for (size_t i = 0; i < np; i++) {
		unsigned pl = (unsigned)data.front().length();
		recorded.write((char*)& pl, sizeof(pl));
		recorded.write((char*)&data.front()[0], pl);
		data.pop();
	}
}

void MatchRecordSystem::replayPackages(std::queue<std::string>& data) {
	unsigned np;
	replay.read((char*)&np, sizeof(np));
	for (size_t i = 0; i < np; i++) {
		if (replay.eof()) {
			return;
		}
		unsigned pl;
		std::string package;
		replay.read((char*)&pl, sizeof(pl));
		package.resize(pl);
		replay.read(&package[0], pl);
		data.push(package);
	}
}

void MatchRecordSystem::CleanOldReplays() {
	temp_replay_counter = 0;
	std::error_code err;
	for (std::filesystem::path file : std::filesystem::directory_iterator(REPLAY_TEMP_PATH, err)) {
		if (file.extension() == REPLAY_EXTENSION) {
			if (!std::filesystem::remove(file, err)) {
				SAIL_LOG_WARNING("Could not remove old replay: " + file.string());
			}
		}
	}

	if (err.value() != 0) {
		SAIL_LOG_WARNING(err.message());	
	}

}