#pragma once

#include "SailImGuiWindow.h"

class Profiler : public SailImGuiWindow {

public:
	Profiler();
	explicit Profiler(bool showWindow);
	~Profiler();

	unsigned int workSetUsage() const;
	unsigned int virtMemUsage() const;
	double processUsage();
	unsigned int vramUsage() const;
	unsigned int vramBudget() const;

	virtual void renderWindow() override;

private:
	void init();

private:
	static constexpr int N_HISTORY = 100;

	template<int N>
	struct History {
		float virtRAMHistory[N];
		float physRAMHistory[N];
		float cpuHistory[N];
		float vramUsageHistory[N];
		float frameTimesHistory[N];
		float fixedUpdateHistory[N];
		float averageSentPacketSizeHistory[N];
			  
		float rmModelsSizeMBHistory[N];
		float rmAnimationsSizeMBHistory[N];
		float rmAudioSizeMBHistory[N];
		float rmTexturesSizeMBHistory[N];
		float rmMiscSizeMBHistory[N];

#ifdef DEVELOPMENT
		float ecsSizeMBHistory[N];
#endif
	};


	ULARGE_INTEGER m_lastCPU, m_lastSysCPU, m_lastUserCPU;
	int m_numProcessors;
	HANDLE m_self;

	double m_lastCPUpercent;

	// ImGui Profiler data
	float m_profilerTimer = 0.f;
	int m_profilerCounter = 0;

	History<N_HISTORY> m_history;

	std::string m_virtCount;
	std::string m_physCount;
	std::string m_vramUCount;
	std::string m_cpuCount;
	std::string m_ftCount;
	std::string m_fixedUpdateCount;
	std::string m_potentialFixedUpdateRate;
	std::string m_averageSentPacketSize;

	std::string m_rmModelsMB;
	std::string m_rmAnimationsMB;
	std::string m_rmAudioMB;
	std::string m_rmTexturesMB;
	std::string m_rmMiscMB;
#ifdef DEVELOPMENT
	std::string m_ecsMB;
#endif
};