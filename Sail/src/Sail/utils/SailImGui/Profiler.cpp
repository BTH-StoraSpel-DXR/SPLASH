#include "pch.h"
#include "Profiler.h"
#include <Psapi.h>
#include "Sail/Application.h"

Profiler::Profiler(bool showWindow) 
	: SailImGuiWindow(showWindow)
{
	init();
}

Profiler::Profiler() 
	: SailImGuiWindow(false)
{
	init();
}

Profiler::~Profiler() {
	delete m_virtRAMHistory;
	delete m_physRAMHistory;
	delete m_vramUsageHistory;
	delete m_cpuHistory;
	delete m_frameTimesHistory;
	delete m_fixedUpdateHistory;
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

	// Allocating memory for profiler
	m_virtRAMHistory = SAIL_NEW float[100];
	m_physRAMHistory = SAIL_NEW float[100];
	m_vramUsageHistory = SAIL_NEW float[100];
	m_cpuHistory = SAIL_NEW float[100];
	m_frameTimesHistory = SAIL_NEW float[100];
	m_fixedUpdateHistory = SAIL_NEW float[100];

	for (int i = 0; i < 100; i++) {
		m_virtRAMHistory[i] = 0.f;
		m_physRAMHistory[i] = 0.f;
		m_vramUsageHistory[i] = 0.f;
		m_cpuHistory[i] = 0.f;
		m_frameTimesHistory[i] = 0.f;
		m_fixedUpdateHistory[i] = 0.f;
	}
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
	percent = static_cast<double>((sys.QuadPart - m_lastSysCPU.QuadPart) + (user.QuadPart - m_lastUserCPU.QuadPart));
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

void Profiler::renderWindow() {
	bool open = isWindowOpen();
	if (open) {
		if (ImGui::Begin("Profiler", &open)) {

			//Profiler window displaying the current usage
			showWindow(open);
			ImGui::BeginChild("Window", ImVec2(0, 0), false, 0);
			std::string header;

			header = "CPU (" + m_cpuCount + "%%)";
			ImGui::Text(header.c_str());

			header = "Frame time (" + m_ftCount + " s)";
			ImGui::Text(header.c_str());

			// "Potential" Hz in this case means what the update rate would be if the program consisted of
			// only the fixedUpdate() function running in a loop
			header = "FixedUpdate (" + m_fixedUpdateCount + " ms, " + m_potentialFixedUpdateRate + " \"potential\" Hz)";
			ImGui::Text(header.c_str());

			header = "Virtual RAM (" + m_virtCount + " MB)";
			ImGui::Text(header.c_str());

			header = "Physical RAM (" + m_physCount + " MB)";
			ImGui::Text(header.c_str());

			header = "VRAM (" + m_vramUCount + " MB)";
			ImGui::Text(header.c_str());

			ImGui::Separator();
			//Collapsing headers for graphs over time
			if (ImGui::CollapsingHeader("CPU Graph")) {
				header = "\n\n\n" + m_cpuCount + "(%)";
				ImGui::PlotLines(header.c_str(), m_cpuHistory, 100, 0, "", 0.f, 100.f, ImVec2(0, 100));
			}
			if (ImGui::CollapsingHeader("Frame Times Graph")) {
				header = "\n\n\n" + m_ftCount + "(s)";
				ImGui::PlotLines(header.c_str(), m_frameTimesHistory, 100, 0, "", 0.f, 0.015f, ImVec2(0, 100));
			}
			if (ImGui::CollapsingHeader("FixedUpdate Times Graph")) {
				header = "\n\n\n" + m_fixedUpdateCount + "(ms)";
				ImGui::PlotLines(header.c_str(), m_fixedUpdateHistory, 100, 0, "", 0.f, 0.015f, ImVec2(0, 100));
			}
			if (ImGui::CollapsingHeader("Virtual RAM Graph")) {
				header = "\n\n\n" + m_virtCount + "(MB)";
				ImGui::PlotLines(header.c_str(), m_virtRAMHistory, 100, 0, "", 0.f, 500.f, ImVec2(0, 100));

			}
			if (ImGui::CollapsingHeader("Physical RAM Graph")) {
				header = "\n\n\n" + m_physCount + "(MB)";
				ImGui::PlotLines(header.c_str(), m_physRAMHistory, 100, 0, "", 0.f, 500.f, ImVec2(0, 100));
			}
			if (ImGui::CollapsingHeader("VRAM Graph")) {
				header = "\n\n\n" + m_vramUCount + "(MB)";
				ImGui::PlotLines(header.c_str(), m_vramUsageHistory, 100, 0, "", 0.f, 500.f, ImVec2(0, 100));
			}


			ImGui::EndChild();

			float dt = Application::getInstance()->getDelta();
			float latestFixedUpdate = Application::getInstance()->getFixedUpdateDelta();

			m_profilerTimer += dt;
			//Updating graphs and current usage
			if (m_profilerTimer > 0.1f) {
				m_profilerTimer = 0.f;
				if (m_profilerCounter < 100) {

					m_virtRAMHistory[m_profilerCounter] = (float)virtMemUsage();
					m_physRAMHistory[m_profilerCounter] = (float)workSetUsage();
					m_vramUsageHistory[m_profilerCounter] = (float)vramUsage();
					m_frameTimesHistory[m_profilerCounter] = dt;
					m_fixedUpdateHistory[m_profilerCounter] = latestFixedUpdate;
					m_cpuHistory[m_profilerCounter++] = (float)processUsage();
					m_virtCount = std::to_string(virtMemUsage());
					m_physCount = std::to_string(workSetUsage());
					m_vramUCount = std::to_string(vramUsage());
					m_cpuCount = std::to_string(processUsage());
					m_ftCount = std::to_string(dt);
					m_fixedUpdateCount = std::to_string(latestFixedUpdate*1000.f);
					m_potentialFixedUpdateRate = std::to_string(static_cast<int>(1.0f / latestFixedUpdate));

				} else {
					// Copying all the history to a new array because ImGui is stupid
					float* tempFloatArr = SAIL_NEW float[100];
					std::copy(m_virtRAMHistory + 1, m_virtRAMHistory + 100, tempFloatArr);
					tempFloatArr[99] = (float)virtMemUsage();
					delete m_virtRAMHistory;
					m_virtRAMHistory = tempFloatArr;
					m_virtCount = std::to_string(virtMemUsage());

					float* tempFloatArr1 = SAIL_NEW float[100];
					std::copy(m_physRAMHistory + 1, m_physRAMHistory + 100, tempFloatArr1);
					tempFloatArr1[99] = (float)workSetUsage();
					delete m_physRAMHistory;
					m_physRAMHistory = tempFloatArr1;
					m_physCount = std::to_string(workSetUsage());

					float* tempFloatArr3 = SAIL_NEW float[100];
					std::copy(m_vramUsageHistory + 1, m_vramUsageHistory + 100, tempFloatArr3);
					tempFloatArr3[99] = (float)vramUsage();
					delete m_vramUsageHistory;
					m_vramUsageHistory = tempFloatArr3;
					m_vramUCount = std::to_string(vramUsage());

					float* tempFloatArr4 = SAIL_NEW float[100];
					std::copy(m_cpuHistory + 1, m_cpuHistory + 100, tempFloatArr4);
					tempFloatArr4[99] = (float)processUsage();
					delete m_cpuHistory;
					m_cpuHistory = tempFloatArr4;
					m_cpuCount = std::to_string(processUsage());

					float* tempFloatArr5 = SAIL_NEW float[100];
					std::copy(m_frameTimesHistory + 1, m_frameTimesHistory + 100, tempFloatArr5);
					tempFloatArr5[99] = dt;
					delete m_frameTimesHistory;
					m_frameTimesHistory = tempFloatArr5;
					m_ftCount = std::to_string(dt);

					float* tempFloatArr6 = SAIL_NEW float[100];
					std::copy(m_fixedUpdateHistory + 1, m_fixedUpdateHistory + 100, tempFloatArr6);
					tempFloatArr6[99] = latestFixedUpdate;
					delete m_fixedUpdateHistory;
					m_fixedUpdateHistory = tempFloatArr6;
					m_fixedUpdateCount = std::to_string(latestFixedUpdate*1000.f);
					m_potentialFixedUpdateRate = std::to_string(static_cast<int>(1.0f / latestFixedUpdate));
				}
			}
			ImGui::End();
		} else {
			ImGui::End();
		}
	}
}
