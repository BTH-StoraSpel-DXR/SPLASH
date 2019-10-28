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
	ULARGE_INTEGER m_lastCPU, m_lastSysCPU, m_lastUserCPU;
	int m_numProcessors;
	HANDLE m_self;

	double m_lastCPUpercent;

	// ImGui Profiler data
	float m_profilerTimer = 0.f;
	int m_profilerCounter = 0;
	float* m_virtRAMHistory;
	float* m_physRAMHistory;
	float* m_cpuHistory;
	float* m_vramUsageHistory;
	float* m_frameTimesHistory;
	float* m_fixedUpdateHistory;
	std::string m_virtCount;
	std::string m_physCount;
	std::string m_vramUCount;
	std::string m_cpuCount;
	std::string m_ftCount;
	std::string m_fixedUpdateCount;
	std::string m_potentialFixedUpdateRate;

};