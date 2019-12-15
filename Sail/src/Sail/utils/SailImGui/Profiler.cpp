#include "pch.h"
#include "Profiler.h"
#include <Psapi.h>
#include "Sail/Application.h"

#include "Network/NWrapperSingleton.h"
#include "Sail/entities/systems/Gameplay/ai/AiSystem.h"
#include "Sail/TimeSettings.h"

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

	for (int i = 0; i < 100; i++) {
		m_history.virtRAMHistory[i] = 0.f;
		m_history.physRAMHistory[i] = 0.f;
		m_history.vramUsageHistory[i] = 0.f;
		m_history.cpuHistory[i] = 0.f;
		m_history.frameTimesHistory[i] = 0.f;
		m_history.fixedUpdateHistory[i] = 0.f;
		m_history.averageSentPacketSizeHistory[i] = 0.f;
		
		m_history.rmModelsSizeMBHistory[i] = 0.f;
		m_history.rmAnimationsSizeMBHistory[i] = 0.f;
		m_history.rmAudioSizeMBHistory[i] = 0.f;
		m_history.rmTexturesSizeMBHistory[i] = 0.f;
		m_history.rmMiscSizeMBHistory[i] = 0.f;

#ifdef DEVELOPMENT
		m_history.ecsSizeMBHistory[i] = 0.f;
#endif
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
				ImGui::PlotLines(header.c_str(), m_history.cpuHistory, 100, 0, "", 0.f, 100.f, ImVec2(0, 100));
			}
			if (ImGui::CollapsingHeader("Frame Times Graph")) {
				header = "\n\n\n" + m_ftCount + "(s)";
				ImGui::PlotLines(header.c_str(), m_history.frameTimesHistory, 100, 0, "", 0.f, 0.015f, ImVec2(0, 100));
			}
			if (ImGui::CollapsingHeader("FixedUpdate Times Graph")) {
				header = "\n\n\n" + m_fixedUpdateCount + "(ms)";
				ImGui::PlotLines(header.c_str(), m_history.fixedUpdateHistory, 100, 0, "", 0.f, 0.015f, ImVec2(0, 100));
			}
			if (ImGui::CollapsingHeader("Virtual RAM Graph")) {
				header = "\n\n\n" + m_virtCount + "(MB)";
				ImGui::PlotLines(header.c_str(), m_history.virtRAMHistory, 100, 0, "", 0.f, 500.f, ImVec2(0, 100));
			}
			if (ImGui::CollapsingHeader("Physical RAM Graph")) {
				header = "\n\n\n" + m_physCount + "(MB)";
				ImGui::PlotLines(header.c_str(), m_history.physRAMHistory, 100, 0, "", 0.f, 500.f, ImVec2(0, 100));
			}
			if (ImGui::CollapsingHeader("VRAM Graph")) {
				header = "\n\n\n" + m_vramUCount + "(MB)";
				ImGui::PlotLines(header.c_str(), m_history.vramUsageHistory, 100, 0, "", 0.f, 500.f, ImVec2(0, 100));
			}
			if (ImGui::CollapsingHeader("Packet Size Graph")) {
				header = "\n\n\n" + m_averageSentPacketSize + "(kiloB/s)";
				ImGui::PlotLines(header.c_str(), m_history.averageSentPacketSizeHistory, 100, 0, "", 0.f, 2000.f, ImVec2(0, 100));
			}
			if (ImGui::CollapsingHeader("Resource Manager")) {
				if (ImGui::CollapsingHeader("Models Graph")) {
					header = "\n\n\n" + m_rmModelsMB + "(MB)";
					ImGui::PlotLines(header.c_str(), m_history.rmModelsSizeMBHistory, 100, 0, "", 0.f, 250.f, ImVec2(0, 100));
				}
				if (ImGui::CollapsingHeader("Animations Graph")) {
					header = "\n\n\n" + m_rmAnimationsMB + "(MB)";
					ImGui::PlotLines(header.c_str(), m_history.rmAnimationsSizeMBHistory, 100, 0, "", 0.f, 250.f, ImVec2(0, 100));
				}
				if (ImGui::CollapsingHeader("Audio Graph")) {
					header = "\n\n\n" + m_rmAudioMB + "(MB)";
					ImGui::PlotLines(header.c_str(), m_history.rmAudioSizeMBHistory, 100, 0, "", 0.f, 250.f, ImVec2(0, 100));
				}
				if (ImGui::CollapsingHeader("Textures Graph")) {
					header = "\n\n\n" + m_rmTexturesMB + "(MB)";
					ImGui::PlotLines(header.c_str(), m_history.rmTexturesSizeMBHistory, 100, 0, "", 0.f, 250.f, ImVec2(0, 100));
				}
				if (ImGui::CollapsingHeader("Misc Graph")) {
					header = "\n\n\n" + m_rmMiscMB + "(MB)";
					ImGui::PlotLines(header.c_str(), m_history.rmMiscSizeMBHistory, 100, 0, "", 0.f, 250.f, ImVec2(0, 100));
				}
			}
#ifdef DEVELOPMENT
			ImGui::Text(("ECS Components " + std::to_string((float)ECS::Instance()->getByteSizeComponents() / (1024.f * 1024.f)) + "(MB)").c_str());
			if (ImGui::CollapsingHeader("ECS Memory Graph")) {
				header = "\n\n\n" + m_ecsMB + "(MB)";
				ImGui::PlotLines(header.c_str(), m_history.ecsSizeMBHistory, 100, 0, "", 0.f, 250.f, ImVec2(0, 100));
			}

			if (ImGui::CollapsingHeader("Ai System")) {
				ImGui::Text(("Average path search time: " + std::to_string(ECS::Instance()->getSystem<AiSystem>()->getAveragePathSearchTime()/1000.f) + "ms").c_str());
				ImGui::Text(("Average update time: " + std::to_string(ECS::Instance()->getSystem<AiSystem>()->getAverageAiUpdateTime()/1000.f) + "ms").c_str());
			}
#endif

			ImGui::EndChild();

			float dt = Application::getInstance()->getDelta();
			float latestFixedUpdate = Application::getInstance()->getFixedUpdateDelta();


			m_profilerTimer += dt;
			constexpr float updateFrequency = 2.f;
			//Updating graphs and current usage
			if (m_profilerTimer > 1/updateFrequency) {
				m_profilerTimer = 0.f;

				size_t averagePacketSize = NWrapperSingleton::getInstance().averagePacketSizeSinceLastCheck();

				size_t rmModelsByteSize = Application::getInstance()->getResourceManager().getModelByteSize();
				size_t rmAnimationsByteSize = Application::getInstance()->getResourceManager().getAnimationsByteSize();
				size_t rmAudioByteSize = Application::getInstance()->getResourceManager().getAudioByteSize();
				size_t rmTexturesByteSize = Application::getInstance()->getResourceManager().getTextureByteSize();
				size_t rmGenericByteSize = Application::getInstance()->getResourceManager().getMiscByteSize();

#ifdef DEVELOPMENT
				const unsigned int ecsByteSize = ECS::Instance()->getByteSize();
#endif

				auto byteToMB = [] (size_t bytes) -> float {
					return (float)bytes / (1024.f * 1024.f);
				};

				if (m_profilerCounter < 100) {

					m_history.virtRAMHistory[m_profilerCounter] = (float)virtMemUsage();
					m_history.physRAMHistory[m_profilerCounter] = (float)workSetUsage();
					m_history.vramUsageHistory[m_profilerCounter] = (float)vramUsage();
					m_history.frameTimesHistory[m_profilerCounter] = dt;
					m_history.fixedUpdateHistory[m_profilerCounter] = latestFixedUpdate;
					m_history.averageSentPacketSizeHistory[m_profilerCounter] = (float)averagePacketSize;
					
					m_history.rmModelsSizeMBHistory[m_profilerCounter] = byteToMB(rmModelsByteSize);
					m_history.rmAnimationsSizeMBHistory[m_profilerCounter] = byteToMB(rmAnimationsByteSize);
					m_history.rmAudioSizeMBHistory[m_profilerCounter] = byteToMB(rmAudioByteSize);
					m_history.rmTexturesSizeMBHistory[m_profilerCounter] = byteToMB(rmTexturesByteSize);
					m_history.rmMiscSizeMBHistory[m_profilerCounter] = byteToMB(rmGenericByteSize);
					
#ifdef DEVELOPMENT
					m_history.ecsSizeMBHistory[m_profilerCounter] = byteToMB(ecsByteSize);
#endif				
					m_history.cpuHistory[m_profilerCounter++] = (float)processUsage();
					
					m_virtCount = std::to_string(virtMemUsage());
					m_physCount = std::to_string(workSetUsage());
					m_vramUCount = std::to_string(vramUsage());
					m_cpuCount = std::to_string(processUsage());
					m_ftCount = std::to_string(dt);
					m_fixedUpdateCount = std::to_string(latestFixedUpdate*1000.f);
					m_potentialFixedUpdateRate = std::to_string(static_cast<int>(1.0f / latestFixedUpdate));
					m_averageSentPacketSize = std::to_string(static_cast<size_t>((averagePacketSize* TICKRATE) / 1024.f));

					m_rmModelsMB = std::to_string(byteToMB(rmModelsByteSize));
					m_rmAnimationsMB = std::to_string(byteToMB(rmAnimationsByteSize));
					m_rmAudioMB = std::to_string(byteToMB(rmAudioByteSize));
					m_rmTexturesMB = std::to_string(byteToMB(rmTexturesByteSize));
					m_rmMiscMB = std::to_string(byteToMB(rmGenericByteSize));

#ifdef DEVELOPMENT
					m_ecsMB = std::to_string(byteToMB(ecsByteSize));
#endif

				} else {
					auto rotateArray = [](float* arr, std::string& str, float value, float stringValue) {
						float tempArr[N_HISTORY];
						std::copy(arr + 1, arr + N_HISTORY, tempArr);		// Copy all but the first to a temporary
						std::copy(tempArr, tempArr + (N_HISTORY - 1), arr);	// Copy all but the last back
						arr[N_HISTORY - 1] = value;
						str = std::to_string(stringValue);
					};

					rotateArray(m_history.virtRAMHistory, m_virtCount, (float)virtMemUsage(), (float)virtMemUsage());
					rotateArray(m_history.physRAMHistory, m_physCount, (float)workSetUsage(), (float)workSetUsage());
					rotateArray(m_history.vramUsageHistory, m_vramUCount, (float)vramUsage(), (float)vramUsage());
					rotateArray(m_history.cpuHistory, m_cpuCount, (float)processUsage(), (float)processUsage());
					rotateArray(m_history.frameTimesHistory, m_ftCount, dt, dt);

					rotateArray(m_history.fixedUpdateHistory, m_fixedUpdateCount, latestFixedUpdate, latestFixedUpdate * 1000.f);
					m_potentialFixedUpdateRate = std::to_string(static_cast<int>(1.0f / latestFixedUpdate));

					rotateArray(m_history.averageSentPacketSizeHistory, m_averageSentPacketSize, (float)averagePacketSize, (float)((averagePacketSize* TICKRATE) / 1024.f));

					rotateArray(m_history.rmModelsSizeMBHistory, m_rmModelsMB, byteToMB(rmModelsByteSize), byteToMB(rmModelsByteSize));
					rotateArray(m_history.rmAnimationsSizeMBHistory, m_rmAnimationsMB, byteToMB(rmAnimationsByteSize), byteToMB(rmAnimationsByteSize));
					rotateArray(m_history.rmAudioSizeMBHistory, m_rmAudioMB, byteToMB(rmAudioByteSize), byteToMB(rmAudioByteSize));
					rotateArray(m_history.rmTexturesSizeMBHistory, m_rmTexturesMB, byteToMB(rmTexturesByteSize), byteToMB(rmTexturesByteSize));
					rotateArray(m_history.rmMiscSizeMBHistory, m_rmMiscMB, byteToMB(rmGenericByteSize), byteToMB(rmGenericByteSize));

#ifdef DEVELOPMENT
					rotateArray(m_history.ecsSizeMBHistory, m_ecsMB, byteToMB(ecsByteSize), byteToMB(ecsByteSize));
#endif
				}
			}
			ImGui::End();
		} else {
			ImGui::End();
		}
	}
}
