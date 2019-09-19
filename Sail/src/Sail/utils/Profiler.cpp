#include "pch.h"
#include "Profiler.h"
#include <Psapi.h>
#include "Sail/Application.h"
#include "Sail/api/GraphicsAPI.h"

Profiler::Profiler(const bool windowState) : SailImGuiWindow(windowState) {
	init();
}

Profiler::Profiler() : SailImGuiWindow(false) {
	init();
}

Profiler::~Profiler() {

}

void Profiler::init() {
	SYSTEM_INFO sysInfo;
	FILETIME ftime, fsys, fuser;

	GetSystemInfo(&sysInfo);
	m_numProcessors = sysInfo.dwNumberOfProcessors;

	GetSystemTimeAsFileTime(&ftime);
	memcpy(&m_lastCPU, &ftime, sizeof(FILETIME));

	m_self = GetCurrentProcess();
	GetProcessTimes(m_self, &ftime, &ftime, &fsys, &fuser);
	memcpy(&m_lastSysCPU, &fsys, sizeof(FILETIME));
	memcpy(&m_lastUserCPU, &fuser, sizeof(FILETIME));
}

unsigned int Profiler::workSetUsage() const{
	PROCESS_MEMORY_COUNTERS_EX pmc;
	GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc));
	SIZE_T physMemUsed = pmc.WorkingSetSize;
	
	return (int)(physMemUsed / (1024 * 1024));
}

unsigned int Profiler::virtMemUsage() const {
	PROCESS_MEMORY_COUNTERS_EX pmc;
	GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)& pmc, sizeof(pmc));
	SIZE_T virMemUsed = pmc.PrivateUsage;

	return (int)(virMemUsed / (1024 * 1024));
}

double Profiler::processUsage(){
	FILETIME ftime;
	FILETIME fsys;
	FILETIME fuser;
	ULARGE_INTEGER now;
	ULARGE_INTEGER sys;
	ULARGE_INTEGER user;
	double percent;

	GetSystemTimeAsFileTime(&ftime);
	memcpy(&now, &ftime, sizeof(FILETIME));

	GetProcessTimes(m_self, &ftime, &ftime, &fsys, &fuser);
	memcpy(&sys, &fsys, sizeof(FILETIME));
	memcpy(&user, &fuser, sizeof(FILETIME));
	percent = (sys.QuadPart - m_lastSysCPU.QuadPart) + (user.QuadPart - m_lastUserCPU.QuadPart);
	percent /= (now.QuadPart - m_lastCPU.QuadPart);
	percent /= m_numProcessors;
	m_lastCPU = now;
	m_lastSysCPU = sys;
	m_lastUserCPU = user;

	if (percent > 0) {
		m_lastCPUpercent = percent;
	}

	return m_lastCPUpercent * 100;
}

unsigned int Profiler::vramUsage() const {
	return Application::getInstance()->getAPI()->getMemoryUsage();
}

unsigned int Profiler::vramBudget() const {
	return Application::getInstance()->getAPI()->getMemoryBudget();
}