#include "pch.h"

#include "API/Audio/AudioEngine.h"

#include "API/Audio/GeneralFunctions.h"
#include "MemoryManager/MemoryManager/src/MemoryManager.h"
#include "Sail/Application.h"
#include "Sail/graphics/camera/Camera.h"
#include "Sail/graphics/geometry/Transform.h"
#include "Sail/KeyBinds.h"
#include "WaveBankReader.h"
#include "Sail/entities/components/AudioComponent.h"


#include <exception>
#include <fstream>
#include <math.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mutex>
#include <stdexcept>
#include <utility>
#include <windows.h>
#include <wrl/client.h>


#include <thread>

#include "Sail/utils/Storage/SettingStorage.h"

#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfuuid")
#pragma comment(lib, "hrtfapo.lib")

#include <xaudio2fx.h>
#pragma comment(lib,"xaudio2.lib")

AudioEngine::AudioEngine() {
	HRESULT hr;
	hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);

#pragma region ERROR_CHECKING
	try {
		if (hr != S_OK) {
			throw std::invalid_argument(nullptr);
		}
	} catch (const std::invalid_argument& e) {
		UNREFERENCED_PARAMETER(e);
		wchar_t errorMsgBuffer[256];
		wsprintfW(errorMsgBuffer, L"FUNCTION: Audio::Audio()\n\nMESSAGE: The 'CoInitialize' function failed!");
		MessageBox(NULL, errorMsgBuffer, static_cast<LPCWSTR>(L"AUDIO ERROR!"), MB_ICONERROR);
		std::exit(0);
	}
#pragma endregion

	this->initialize();
}

AudioEngine::~AudioEngine() {
	m_isRunning = false;
	for (auto& e : m_sound) {
		e.xapo.ReleaseAndGetAddressOf();
	}

	for (int i = 0; i < STREAMED_SOUNDS_COUNT; i++) {
		m_isStreaming[i] = false;
		m_isStreamPaused[i] = false;
		m_isFinished[i] = true;
		m_overlapped[i] = { 0 };
		m_streamLocks[i].store(false);
	}
}

void AudioEngine::loadSound(const std::string& filename) {
	Application::getInstance()->getResourceManager().loadAudioData(filename, m_xAudio2);
}

// TODO? One submixVoice for sound effects, one for music, etc instead of one for each sound
int AudioEngine::beginSound(const std::string& filename, Audio::EffectType effectType, float frequency, float volume) {
	if (!Application::getInstance()->getResourceManager().hasAudioData(filename)) {
		SAIL_LOG_ERROR("That audio file has NOT been loaded yet!");
		return -1;
	}

	int indexValue = fetchSoundIndex();
	m_sound[indexValue].filename = filename;
	HRESULT hr;

	// Is this the first time this function was called for this sound...
	if (m_sound[indexValue].sourceVoice == nullptr) {
		// ... create a source voice for it,
		hr = m_xAudio2->CreateSourceVoice(&m_sound[indexValue].sourceVoice, (WAVEFORMATEX*)Application::getInstance()->getResourceManager().getAudioData(filename).getFormat());
		// ... set volume,
		if (FAILED(hr)) {
			SAIL_LOG_WARNING("Failed to create a sourcevoice!");
		}
		hr = m_sound[indexValue].sourceVoice->SetVolume(volume);
		if (FAILED(hr)) {
			SAIL_LOG_WARNING("Failed to set volume to a sourcevoice!");
		}
		// ... set up hrtf.
		setUpxAPO(indexValue);

		// Create a submix specifically for this
		createXAPOsubMixVoice(&m_sound[indexValue].xAPOsubMixVoice, m_sound[indexValue].xapo);
	}
	else {
		// Reset; preparing for re-creation
		hr = m_sound[indexValue].sourceVoice->Stop();
		if (FAILED(hr)) {
			SAIL_LOG_WARNING("Failed to stop a sourcevoice!");
		}
		hr = m_sound[indexValue].sourceVoice->FlushSourceBuffers();
		if (FAILED(hr)) {
			SAIL_LOG_WARNING("Failed to flush a sourcevoice!");
		}
		hr = m_sound[indexValue].sourceVoice->Discontinuity();
		if (FAILED(hr)) {
			SAIL_LOG_WARNING("Failed to create a sourcevoice!");
		}

		// Destroy to source voice to reset settings completely
		m_sound[indexValue].sourceVoice->DestroyVoice();
		m_sound[indexValue].sourceVoice = nullptr;

		// Create the source voice
		hr = m_xAudio2->CreateSourceVoice(&m_sound[indexValue].sourceVoice, (WAVEFORMATEX*)Application::getInstance()->getResourceManager().getAudioData(filename).getFormat());
	}

	m_sound[indexValue].xAPOsubMixVoice->SetVolume(volume);

	bool useFilter = false;
	m_sound[indexValue].lowpassFiltered = false;
	if (effectType == Audio::EffectType::PROJECTILE_LOWPASS) {
		useFilter = true;
		m_sound[indexValue].lowpassFiltered = true;
	}

	//			   										  xAPO
	// [index].SourceVoice -----> [index].xAPOsubMixVoice ---> m_masterVoice
	sendVoiceTosubMixVoice(
		&m_sound[indexValue].sourceVoice,
		&m_sound[indexValue].xAPOsubMixVoice,
		useFilter
	);
	
	//					   lowpass						   xAPO
	// [index].SourceVoice ------> [index].xAPOsubMixVoice ---> m_masterVoice
	if (effectType == Audio::EffectType::PROJECTILE_LOWPASS) {
		addLowPassFilterTo(m_sound[indexValue].sourceVoice, m_sound[indexValue].xAPOsubMixVoice, frequency);
	}
	
	// Apply changes directly
	m_xAudio2->CommitChanges(XAUDIO2_COMMIT_ALL);

	return indexValue;
}

void AudioEngine::streamSound(const std::string& filename, int streamIndex, float volume, bool isPositionalAudio, bool loop, AudioComponent* pAudioC) {
	bool expectedValue = false;
	while (!m_streamLocks[streamIndex].compare_exchange_strong(expectedValue, true));

	if (m_masterVoice == nullptr) {
		SAIL_LOG_ERROR("'IXAudio2MasterVoice' has not been correctly initialized; audio is unplayable!");
		m_streamLocks[streamIndex].store(false);
	} else {
		this->streamSoundInternal(filename, streamIndex, volume, isPositionalAudio, loop, pAudioC);
	}

	return;
}

// Note: cam is not const since cam.getUp() is not a const function
void AudioEngine::updateSoundWithCurrentPosition(int index, Camera& cam, const Transform& transform, 
	const glm::vec3& positionOffset, float alpha) {

	glm::vec3 soundPos = transform.getInterpolatedTranslation(alpha);

	// If the sound has an offset position from the entity's transform then
	// rotate the offset with the transform's rotation and add it to the position.
	if (positionOffset != glm::vec3(0, 0, 0)) {
		soundPos += glm::rotate(transform.getInterpolatedRotation(alpha), positionOffset);
	}

	glm::vec3 negativeZAxis = glm::normalize(cam.getDirection());
	glm::vec3 positiveYAxisGuess = glm::normalize(-cam.getUp());
	glm::vec3 positiveXAxis = glm::normalize(glm::cross(negativeZAxis, positiveYAxisGuess));
	glm::vec3 positiveYAxis = glm::normalize(cross(negativeZAxis, positiveXAxis));

	glm::mat4x4 rotationTransform{
		positiveXAxis.x, positiveYAxis.x, negativeZAxis.x, 0.f,
		positiveXAxis.y, positiveYAxis.y, negativeZAxis.y, 0.f,
		positiveXAxis.z, positiveYAxis.z, negativeZAxis.z, 0.f,
		0.f, 0.f, 0.f, 1.f,
	};

	// Reinterpret the sound's position in the device's coordinate system.
	glm::vec3 soundRelativeToHead = glm::vec3((rotationTransform * glm::translate(-cam.getPosition())) * glm::vec4(soundPos, 1.f));

	// Note that at (0, 0, 0) exactly, the HRTF audio will simply pass through audio. We can use a minimal offset
	// to simulate a zero distance when the hologram position vector is exactly at the device origin in order to
	// allow HRTF to continue functioning in this edge case.
	float distanceFromHologramToHead = glm::length(soundRelativeToHead);

	static const float distanceMin = 0.00001f;
	if (distanceFromHologramToHead < distanceMin) {
		soundRelativeToHead = glm::vec3(0.f, distanceMin, 0.f);
	}

	auto hrtfPosition = HrtfPosition{
		soundRelativeToHead.x,
		soundRelativeToHead.y,
		soundRelativeToHead.z
	};

	// update the source position with the new relative position
 	m_sound[index].hrtfParams->SetSourcePosition(&hrtfPosition);
}

void AudioEngine::updateStreamWithCurrentPosition(int index, Camera& cam, const Transform& transform,
	const glm::vec3& positionOffset, float alpha) {

	glm::vec3 streamPos = transform.getInterpolatedTranslation(alpha);

	// If the sound has an offset position from the entity's transform then rotate the offset with the transform's rotation and add it to the position
	//if (m_sound[index].positionOffset != glm::vec3(0, 0, 0)) {
	if (positionOffset != glm::vec3(0, 0, 0)) {
		streamPos += glm::rotate(transform.getInterpolatedRotation(alpha), positionOffset);
	}

	glm::vec3 negativeZAxis = glm::normalize(cam.getDirection());
	glm::vec3 positiveYAxisGuess = glm::normalize(-cam.getUp());
	glm::vec3 positiveXAxis = glm::normalize(glm::cross(negativeZAxis, positiveYAxisGuess));
	glm::vec3 positiveYAxis = glm::normalize(cross(negativeZAxis, positiveXAxis));

	glm::mat4x4 rotationTransform{
		positiveXAxis.x, positiveYAxis.x, negativeZAxis.x, 0.f,
		positiveXAxis.y, positiveYAxis.y, negativeZAxis.y, 0.f,
		positiveXAxis.z, positiveYAxis.z, negativeZAxis.z, 0.f,
		0.f, 0.f, 0.f, 1.f,
	};

	// Reinterpret the sound's position in the device's coordinate system.
	glm::vec3 soundRelativeToHead = glm::vec3((rotationTransform * glm::translate(-cam.getPosition())) * glm::vec4(streamPos, 1.f));

	// Note that at (0, 0, 0) exactly, the HRTF audio will simply pass through audio. We can use a minimal offset
	// to simulate a zero distance when the hologram position vector is exactly at the device origin in order to
	// allow HRTF to continue functioning in this edge case.
	float distanceFromHologramToHead = glm::length(soundRelativeToHead);

	static const float distanceMin = 0.00001f;
	if (distanceFromHologramToHead < distanceMin) {
		soundRelativeToHead = glm::vec3(0.f, distanceMin, 0.f);
	}

	auto hrtfPosition = HrtfPosition{
		soundRelativeToHead.x,
		soundRelativeToHead.y,
		soundRelativeToHead.z
	};

	// update the source position with the new relative position
	if (m_stream[index].sourceVoice != nullptr) {
		m_stream[index].hrtfParams->SetSourcePosition(&hrtfPosition);
	}
}

void AudioEngine::startSpecificSound(int index, float volume) {
	if (!this->checkSoundIndex(index) || m_sound[index].sourceVoice == nullptr) {
		return;
	}

	auto hr = m_sound[index].sourceVoice->FlushSourceBuffers();

	// Submit audio data to the source voice.
	if (SUCCEEDED(hr)) {
		hr = m_sound[index].sourceVoice->SubmitSourceBuffer(
			Application::getInstance()->getResourceManager().getAudioData(m_sound[index].filename).getSoundBuffer());
	}

	if (SUCCEEDED(hr)) {
		hr = m_sound[index].sourceVoice->Start(0);
		m_sound[index].sourceVoice->SetVolume(volume);
	}
}

void AudioEngine::startDeathSound(const std::string& filename, float volume) {
	if (!Application::getInstance()->getResourceManager().hasAudioData(filename)) {
		SAIL_LOG_ERROR("That audio file has NOT been loaded yet!");
		return;
	}

	m_deathSound.filename = filename;
	HRESULT hr = S_OK;

	// Is this the first time this function was called for this sound...
	if (m_deathSound.sourceVoice == nullptr) {
		// ... create a source voice for it,
		hr = m_xAudio2->CreateSourceVoice(&m_deathSound.sourceVoice, (WAVEFORMATEX*)Application::getInstance()->getResourceManager().getAudioData(filename).getFormat());
		// ... set volume,
		hr = m_deathSound.sourceVoice->SetVolume(volume);
		// ... set up hrtf.
		hr = CreateHrtfApo(nullptr, &m_deathSound.xapo);
		hr = m_deathSound.xapo.As(&m_deathSound.hrtfParams);
		hr = m_deathSound.hrtfParams->SetEnvironment(m_deathSound.environment);
		if (FAILED(hr)) {
			SAIL_LOG_ERROR("Failed to create Hrtf Apo for death sound");
		}

		// Create a submix specifically for this
		createXAPOsubMixVoice(&m_deathSound.xAPOsubMixVoice, m_deathSound.xapo);
	}

	hr = m_deathSound.xAPOsubMixVoice->SetVolume(volume);
	if (FAILED(hr)) {
		SAIL_LOG_ERROR("Failed to set volume to death sound");
	}

	//			   										  xAPO
	// [index].SourceVoice -----> [index].xAPOsubMixVoice ---> m_masterVoice
	sendVoiceTosubMixVoice(
		&m_deathSound.sourceVoice,
		&m_deathSound.xAPOsubMixVoice,
		false
	);


	hr = m_deathSound.sourceVoice->FlushSourceBuffers();

	// Submit audio data to the source voice.
	if (SUCCEEDED(hr)) {
		hr = m_deathSound.sourceVoice->SubmitSourceBuffer(
			Application::getInstance()->getResourceManager().getAudioData(m_deathSound.filename).getSoundBuffer());
	}

	if (SUCCEEDED(hr)) {
		hr = m_deathSound.sourceVoice->Start(0);
		hr = m_deathSound.sourceVoice->SetVolume(volume);
	}

	// Apply changes directly
	m_xAudio2->CommitChanges(XAUDIO2_COMMIT_ALL);
}

void AudioEngine::updateDeathvolume(float volume) {
	m_deathSound.sourceVoice->SetVolume(volume);
}

void AudioEngine::startInsanitySound(const std::string& filename, float volume) {
	if (!Application::getInstance()->getResourceManager().hasAudioData(filename)) {
		SAIL_LOG_ERROR("That audio file has NOT been loaded yet!");
		return;
	}

	m_insanitySound.filename = filename;
	HRESULT hr = S_OK;

	// Is this the first time this function was called for this sound...
	if (m_insanitySound.sourceVoice == nullptr) {
		// ... create a source voice for it,
		hr = m_xAudio2->CreateSourceVoice(&m_insanitySound.sourceVoice, (WAVEFORMATEX*)Application::getInstance()->getResourceManager().getAudioData(filename).getFormat());
		// ... set volume,
		hr = m_insanitySound.sourceVoice->SetVolume(volume);
		// ... set up hrtf.
		hr = CreateHrtfApo(nullptr, &m_insanitySound.xapo);
		hr = m_insanitySound.xapo.As(&m_insanitySound.hrtfParams);
		hr = m_insanitySound.hrtfParams->SetEnvironment(m_insanitySound.environment);
		if (FAILED(hr)) {
			SAIL_LOG_ERROR("Failed to create Hrtf Apo for insanitydeath sound");
		}

		// Create a submix specifically for this
		createXAPOsubMixVoice(&m_insanitySound.xAPOsubMixVoice, m_insanitySound.xapo);
	}

	hr = m_insanitySound.xAPOsubMixVoice->SetVolume(volume);
	if (FAILED(hr)) {
		SAIL_LOG_ERROR("Failed to set volume to insanitydeath sound");
	}

	//			   										  xAPO
	// [index].SourceVoice -----> [index].xAPOsubMixVoice ---> m_masterVoice
	sendVoiceTosubMixVoice(
		&m_insanitySound.sourceVoice,
		&m_insanitySound.xAPOsubMixVoice,
		false
	);

	hr = m_insanitySound.sourceVoice->FlushSourceBuffers();

	// Submit audio data to the source voice.
	if (SUCCEEDED(hr)) {
		hr = m_insanitySound.sourceVoice->SubmitSourceBuffer(
			Application::getInstance()->getResourceManager().getAudioData(m_insanitySound.filename).getSoundBuffer());
	}

	if (SUCCEEDED(hr)) {
		hr = m_insanitySound.sourceVoice->Start(0);
		hr = m_insanitySound.sourceVoice->SetVolume(volume);
	}

	// Apply changes directly
	m_xAudio2->CommitChanges(XAUDIO2_COMMIT_ALL);
}

void AudioEngine::updateInsanityVolume(float volume) {
	m_insanitySound.sourceVoice->SetVolume(volume);
}

void AudioEngine::stopSpecificSound(int index) {
	if (this->checkSoundIndex(index) && m_sound[index].sourceVoice != nullptr) {
		m_sound[index].sourceVoice->Stop(0);
	}
}

void AudioEngine::stopSpecificStream(int index) {
	if (this->checkStreamIndex(index) && m_stream[index].sourceVoice != nullptr) {
		m_isStreaming[index] = false;
	}
}

void AudioEngine::pause_unpause_AllStreams(bool pauseTRUE_unpauseFALSE)
{
	for (int i = 0; i < STREAMED_SOUNDS_COUNT; i++) {
		this->m_isStreamPaused[i] = pauseTRUE_unpauseFALSE;
		if (this->m_stream[i].sourceVoice != nullptr) {
			if (pauseTRUE_unpauseFALSE == true) {
				this->m_stream[i].sourceVoice->Stop();
			}
			else {
				this->m_stream[i].sourceVoice->Start();
			}
		}
	}
}

void AudioEngine::stopAllStreams() {
	for (int i = 0; i < STREAMED_SOUNDS_COUNT; i++) {
		m_isStreaming[i] = false;
	}
}

void AudioEngine::stopAllSounds() {
	for (int i = 0; i < SOUND_COUNT; i++) {
		if (m_sound[i].sourceVoice != nullptr) {
			m_sound[i].sourceVoice->Stop();
			m_sound[i].sourceVoice->DestroyVoice();
			m_sound[i].sourceVoice = nullptr;
		}
	}

	for (int i = 0; i < STREAMED_SOUNDS_COUNT; i++) {
		m_isStreaming[i] = false;
	}
}

void AudioEngine::pauseAllSounds() {
	for (int i = 0; i < SOUND_COUNT; i++) {
		if (m_sound[i].sourceVoice != nullptr) {
			m_sound[i].sourceVoice->Stop();
		}
	}
}

// Note: the sourceVoice's volume might not actually be doing anything atm, use the submix volume instead
float AudioEngine::getSoundVolume(int index) {
	float returnValue = 0.0f;

	if (this->checkSoundIndex(index)) {
		m_sound[index].sourceVoice->GetVolume(&returnValue);
	} else {
		returnValue = -1.0f;
	}

	return returnValue;
}

float AudioEngine::getStreamVolume(int index) {
	float returnValue = 0.0f;

	if (this->checkStreamIndex(index)) {
		m_stream[index].sourceVoice->GetVolume(&returnValue);
	} else {
		returnValue = -1.0f;
	}

	return returnValue;
}

int AudioEngine::getSoundIndex() {
	return m_currSoundIndex;
}

int AudioEngine::getAvailableStreamIndex() {
	int returnValue = -1;

	for (int i = 0; i < STREAMED_SOUNDS_COUNT; i++) {
		if (m_isFinished[i]) {
			m_isStreaming[i] = true;
			returnValue = i;
			break;
		}
	}

	return returnValue;
}

soundStruct* AudioEngine::getSound(int index)
{
	return &m_sound[index];
}

soundStruct* AudioEngine::getStream(int index)
{
	return &m_stream[index];
}

// Note: the sourceVoice's volume might not actually be doing anything atm, use the submix volume instead
void AudioEngine::setSoundVolume(int index, float value) {
	if (this->checkSoundIndex(index)) {
		m_sound[index].sourceVoice->SetVolume(value);
		m_sound[index].xAPOsubMixVoice->SetVolume(value);
	}
}

void AudioEngine::setStreamVolume(int index, float value) {
	if (this->checkStreamIndex(index)) {
		if (m_stream[index].sourceVoice != nullptr) {
			m_stream[index].sourceVoice->SetVolume(value);
		}
	}
}

void AudioEngine::updateProjectileLowPass(float frequency, int indexToSource) {
	if (m_sound[indexToSource].lowpassFiltered == false) {
		return;
	}

	// Create new lowpass filter with updated frequency.
	XAUDIO2_FILTER_PARAMETERS lowPassFilter = createLowPassFilter(frequency);

	if (FAILED(m_sound[indexToSource].sourceVoice->SetOutputFilterParameters(
		m_sound[indexToSource].xAPOsubMixVoice,
		&lowPassFilter
	))) {
		SAIL_LOG_ERROR("Failed to update a projectile's low pass filter");
	}
}

#ifdef DEVELOPMENT
unsigned int AudioEngine::getByteSize() const {
	unsigned int size = sizeof(*this);
	size += sizeof(IXAudio2);
	size += sizeof(IXAudio2MasteringVoice);
	size += sizeof(IXAudio2SubmixVoice);
	size += sizeof(IXAPO);
	size += sizeof(IXAudio2SourceVoice);
	size += sizeof(IXAudio2SubmixVoice);
	size += sizeof(IXAPOHrtfParameters);

	return size;
}

void AudioEngine::logDebugData() {
	using namespace std;
	XAUDIO2_PERFORMANCE_DATA data;
	m_xAudio2->GetPerformanceData(&data);
	string log = "";
	string activeLog = "";
	string perFrameLog = "";
	activeLog += string("ActiveMatrixMixCount: ")			+= to_string(data.ActiveMatrixMixCount)		  += string(" ");
	activeLog += string("ActiveResamplerCount: ")			+= to_string(data.ActiveResamplerCount)		  += string(" ");
	activeLog += string("ActiveSourceVoiceCount: ")		+= to_string(data.ActiveSourceVoiceCount)	  += string(" ");
	activeLog += string("ActiveSubmixVoiceCount: ")		+= to_string(data.ActiveSubmixVoiceCount)	  += string(" ");
	activeLog += string("ActiveXmaSourceVoices: ")		+= to_string(data.ActiveXmaSourceVoices)	  += string(" ");
	activeLog += string("ActiveXmaStreams: ")				+= to_string(data.ActiveXmaStreams)			  += string(" ");
	activeLog += string("TotalSourceVoiceCount: ") += to_string(data.TotalSourceVoiceCount) += string(" ");
	perFrameLog += string("AudioCyclesSinceLastQuery: ")	+= to_string(data.AudioCyclesSinceLastQuery)  += string(" ");
	perFrameLog += string("CurrentLatencyInSamples: ")		+= to_string(data.CurrentLatencyInSamples)	  += string(" ");    
	perFrameLog += string("MaximumCyclesPerQuantum: ")		+= to_string(data.MaximumCyclesPerQuantum)	  += string(" ");
	perFrameLog += string("MinimumCyclesPerQuantum: ") += to_string(data.MinimumCyclesPerQuantum) += string(" ");
	perFrameLog += string("TotalCyclesSinceLastQueryv: ") += to_string(data.TotalCyclesSinceLastQuery) += string(" ");
	log += string("MemoryUsageInBytes: ")			+= to_string(data.MemoryUsageInBytes)		  += string(" ");
	log += string("GlitchesSinceEngineStarted: ") += to_string(data.GlitchesSinceEngineStarted)	  += string(" ");
		
	SAIL_LOG(activeLog.c_str());
	cout << "\n";
	SAIL_LOG(log.c_str());
	cout << "\n";
	SAIL_LOG(perFrameLog.c_str());
	cout << " ------------------------ NEW AUDIO FRAME --------------------------------- \n";
}
#endif

void AudioEngine::initialize() {
	// Init soundObjects
	for (int i = 0; i < SOUND_COUNT; i++) {
		m_sound[i].sourceVoice = nullptr;
	}
	// Init streamObjects
	for (int i = 0; i < STREAMED_SOUNDS_COUNT; i++) {
		m_stream[i].sourceVoice = nullptr;
		m_isStreaming[i] = false;
		m_isStreamPaused[i] = false;
		m_isFinished[i] = true;
		m_overlapped[i] = { 0 };
		m_streamLocks[i].store(false);
	}

	if (FAILED(this->initXAudio2())) {
		SAIL_LOG_ERROR("Failed to init XAudio2!");
		m_initFailed = true;
	}
}

void AudioEngine::activateDebugLayer() {
	XAUDIO2_DEBUG_CONFIGURATION debugConfig = {};
	debugConfig.TraceMask = XAUDIO2_LOG_WARNINGS;		// Also enables LOG_ERRORS
	debugConfig.TraceMask |= XAUDIO2_LOG_DETAIL;		// Also enables LOG_INFO
	debugConfig.TraceMask |= XAUDIO2_LOG_FUNC_CALLS;	// Also enables LOG_API_CALLS
	debugConfig.TraceMask |= XAUDIO2_LOG_TIMING;
	debugConfig.TraceMask |= XAUDIO2_LOG_LOCKS;
	debugConfig.TraceMask |= XAUDIO2_LOG_MEMORY;
	debugConfig.TraceMask |= XAUDIO2_LOG_STREAMING;
	debugConfig.BreakMask = XAUDIO2_LOG_ERRORS;
	m_xAudio2->SetDebugConfiguration(&debugConfig, nullptr);

#if (_WIN32_WINNT >= WIN_32_WINNT_WIN10)
	SAIL_LOG("AUDIOENGINE: Xaudio 2.9 Debugging enabled | Windows 10");
#elif (_WIN32_WINNT >= WIN_32_WINNT_WIN8)
	SAIL_LOG("AUDIOENGINE: Xaudio 2.8 debugging enabled | Windows 8");
#else
	SAIL_LOG("AUDIOENGINE: Xaudio 2.7 debugging enabled");
#endif
}

HRESULT AudioEngine::initXAudio2() {
	UINT32 flags = XAUDIO2_1024_QUANTUM;

#ifdef DEVELOPMENT
	//flags |= XAUDIO2_DEBUG_ENGINE;
#endif
	
	HRESULT hr = XAudio2Create(&m_xAudio2, flags);

	// HRTF APO expects mono audio data at 48kHz and produces stereo output at 48kHz
	// so we create a stereo mastering voice with specific rendering sample rate of 48kHz.
	// Mastering voice will be automatically destroyed when XAudio2 instance is destroyed.
	if (SUCCEEDED(hr)) {
		hr = m_xAudio2->CreateMasteringVoice(&m_masterVoice, 2, 48000);

		if (FAILED(hr)) {
			assert(false);
		}
	}
	else {
		SAIL_LOG_ERROR("Failed to create Xaudio2");
	}

#if DEVELOPMENT
	// Activate debug layer if we're in development
	//activateDebugLayer();
#endif

	return hr;
}

int AudioEngine::fetchSoundIndex() {
	m_currSoundIndex++;
	m_currSoundIndex %= SOUND_COUNT;

	return m_currSoundIndex;
}

void AudioEngine::sendVoiceTo(IXAudio2SourceVoice* *source, IXAudio2Voice* *destination, bool useFilter) {
	XAUDIO2_VOICE_SENDS sends = {};
	XAUDIO2_SEND_DESCRIPTOR sendDesc = {};
	if (useFilter) {
		sendDesc.Flags = XAUDIO2_SEND_USEFILTER;
	}
	sendDesc.pOutputVoice = *destination;
	sends.SendCount = 1;
	sends.pSends = &sendDesc;
	if (FAILED((*source)->SetOutputVoices(&sends))) {
		SAIL_LOG_ERROR("Failed to connect a source voice to xAPO SubmixVoice");
	}
}

void AudioEngine::sendVoiceTosubMixVoice(IXAudio2SourceVoice** source, IXAudio2SubmixVoice** destination, bool useFilter) {
	XAUDIO2_VOICE_SENDS sends = {};
	XAUDIO2_SEND_DESCRIPTOR sendDesc = {};
	if (useFilter) {
		sendDesc.Flags = XAUDIO2_SEND_USEFILTER;
	}
	sendDesc.pOutputVoice = *destination;
	sends.SendCount = 1;
	sends.pSends = &sendDesc;
	if (FAILED((*source)->SetOutputVoices(&sends))) {
		SAIL_LOG_ERROR("Failed to connect a source voice to xAPO SubmixVoice");
	}
}

void AudioEngine::addLowPassFilterTo(IXAudio2SourceVoice* source, IXAudio2Voice* destination, float frequency) {
	HRESULT hr;
	XAUDIO2_FILTER_PARAMETERS lowPassFilter = createLowPassFilter(frequency);
	if (FAILED(hr = source->SetOutputFilterParameters(
		destination,
		&lowPassFilter
	))) {
		SAIL_LOG_ERROR("Failed to apply a low-pass filter!");
	}
}

void AudioEngine::setUpxAPO(int indexValue) {
	HRESULT hr = CreateHrtfApo(nullptr, &m_sound[indexValue].xapo);
	hr = m_sound[indexValue].xapo.As(&m_sound[indexValue].hrtfParams);
	hr = m_sound[indexValue].hrtfParams->SetEnvironment(m_sound[indexValue].environment);
	if (FAILED(hr)) {
		SAIL_LOG_ERROR("Failed to create Hrtf Apo");
	}
}

XAUDIO2_EFFECT_DESCRIPTOR AudioEngine::createXAPPOEffect(Microsoft::WRL::ComPtr<IXAPO> xapo) {
	XAUDIO2_EFFECT_DESCRIPTOR fxDesc{};
	fxDesc.InitialState = TRUE;
	fxDesc.OutputChannels = 2;          // Stereo output
	fxDesc.pEffect = xapo.Get();        // HRTF xAPO set as the effect.

	return fxDesc;
}

XAUDIO2_FILTER_PARAMETERS AudioEngine::createLowPassFilter(float cutoffFrequence) {
	float sumFrequency = 2.0f * sinf((X3DAUDIO_PI * cutoffFrequence) / /*Samplerate*/48000.0f);
	if (sumFrequency > XAUDIO2_MAX_FILTER_FREQUENCY) {
		sumFrequency = XAUDIO2_MAX_FILTER_FREQUENCY;
	}
	
	XAUDIO2_FILTER_PARAMETERS filterParameters = { 
		LowPassFilter,
		sumFrequency,
		XAUDIO2_MAX_FILTER_ONEOVERQ
	}; // see XAudio2CutoffFrequencyToRadians() in XAudio2.h for more information on the formula used here

	return filterParameters;
}

void AudioEngine::createXAPOsubMixVoice(IXAudio2SubmixVoice* *source, Microsoft::WRL::ComPtr<IXAPO> xapo) {
	if (*source != nullptr) {
		SAIL_LOG_ERROR("Attempted to create a xAPOsubMixVoice which already existed");
		return;
	}

	XAUDIO2_EFFECT_DESCRIPTOR xAPOeffectDesc{};
	XAUDIO2_EFFECT_CHAIN xAPOeffectChain{};
	XAUDIO2_VOICE_SENDS xAPOsends = {};
	XAUDIO2_SEND_DESCRIPTOR xAPOsendDesc = {};
	// Set xAPO Effect
	xAPOeffectDesc = createXAPPOEffect(xapo);
	// Attach effect to chain
	xAPOeffectChain.EffectCount = 1;
	xAPOeffectChain.pEffectDescriptors = &xAPOeffectDesc;
	// Set destination as MasterVoice
	xAPOsendDesc.pOutputVoice = m_masterVoice;
	xAPOsends.SendCount = 1;
	xAPOsends.pSends = &xAPOsendDesc;

	// HRTF APO expects mono 48kHz input, so we configure the submix voice for that format.
	HRESULT hr = m_xAudio2->CreateSubmixVoice(source, 1, 48000, 0, 0, &xAPOsends, &xAPOeffectChain);
	if (FAILED(hr)) {
		SAIL_LOG_ERROR("Failed to create xAPO submix voice");
	}
}

void AudioEngine::streamSoundInternal(const std::string& filename, int myIndex, float volume, bool isPositionalAudio, bool loop, AudioComponent* pAudioC) {

	if (!m_isRunning) {
		return;
	}

#pragma region VARIABLES_LIST
	// Set to 'officially streaming'
	m_isStreaming[myIndex] = true;
	m_isFinished[myIndex] = false;
	// Release the 'streamLock' AFTER we've set 'm_isStreaming[myIndex] = true'
	m_streamLocks[myIndex].store(false);

	WCHAR wavebank[MAX_PATH];
	DirectX::WaveBankReader wbr;
	StreamingVoiceContext voiceContext;
	HRESULT hr;
	char formatBuff[64];
	WAVEFORMATEX* wfx = nullptr;
	DirectX::WaveBankReader::Metadata metadata;
	std::unique_ptr<uint8_t[]> buffers[MAX_BUFFER_COUNT];
	float currentVolume = 0.0f;
	int totalChunks = 0;
	int currentChunk = 0;
#pragma endregion

	hr = FindMediaFileCch(wavebank, MAX_PATH, stringToWString(filename).c_str());
	if (hr != S_OK) {
		SAIL_LOG_ERROR("Failed to find the specified '.xwb' file!");
		return;
	}

	hr = wbr.Open(wavebank);
	if (hr != S_OK) {
		SAIL_LOG_ERROR("Failed to open wavebank file!");
		return;
	}

	if (!wbr.IsStreamingBank()) {
		SAIL_LOG_ERROR("Tried to stream a non-streamable '.xwb' file! Contact Oliver if you've gotten this message!");
		return;
	}

	while (m_isStreaming[myIndex]) {
		// For every piece of audio within the '.xwb' file (preferably only be 1)
		for (DWORD i = 0; i < wbr.Count(); i++) {

			// Get info we need to play this wave (need space fo PCM, ADPCM, and xWMA formats)
			wfx = reinterpret_cast<WAVEFORMATEX*>(&formatBuff);
			hr = wbr.GetFormat(i, wfx, 64);
			if (hr != S_OK) {
				SAIL_LOG_ERROR("Failed to get wave format for '.xwb' file!");
			}

			hr = wbr.GetMetadata(i, metadata);
			if (hr != S_OK) {
				SAIL_LOG_ERROR("Failed to get meta data for '.xwb' file!");
			}

			Microsoft::WRL::ComPtr<IXAPO> xapo;

			// Passing in nullptr as the first arg for HrtfApoInit initializes the APO with defaults of
			// omnidirectional sound with natural distance decay behavior.
			// CreateHrtfApo will fail with E_NOTIMPL on unsupported platforms.
			if (isPositionalAudio) {
				HRESULT hr = CreateHrtfApo(nullptr, &xapo);

				if (SUCCEEDED(hr)) {
					hr = xapo.As(&m_stream[myIndex].hrtfParams);
				}

				// Set the default environment.
				if (SUCCEEDED(hr)) {
					hr = m_stream[myIndex].hrtfParams->SetEnvironment(m_stream[myIndex].environment);
				}
			}

			hr = m_xAudio2->CreateSourceVoice(&m_stream[myIndex].sourceVoice, wfx, 0, 1.0f, &voiceContext);
			if (hr != S_OK) {
				SAIL_LOG_ERROR("Failed to create source voice!");
			}

			m_stream[myIndex].sourceVoice->SetVolume(0);

			// THIS IS THE OTHER VERSION FOR ADPC
					// ... for ADPC-WAV compressed file-type
					//hr = xAudio->CreateSourceVoice(&pSourceVoice, (WAVEFORMATEX*)& adpcwf);

			if (FAILED(hr)) {
				SAIL_LOG_ERROR("Failed to create the actual 'SourceVoice' for a sound file!");
			}

			// Route the source voice to the submix voice.
			// The complete graph pipeline looks like this -
			// Source Voice -> Submix Voice (HRTF xAPO) -> Mastering Voice
			if (isPositionalAudio && SUCCEEDED(hr)) {
				XAUDIO2_VOICE_SENDS sends = {};
				XAUDIO2_SEND_DESCRIPTOR sendDesc = {};
				sendDesc.pOutputVoice = m_stream[myIndex].xAPOsubMixVoice;
				sends.SendCount = 1;
				sends.pSends = &sendDesc;
				hr = m_stream[myIndex].sourceVoice->SetOutputVoices(&sends);
			}

			// Create the 'overlapped' structure as well as buffers to handle async I/O
#if (_WIN32_WINNT >= _WIN32_WINNT_VISTA)
			m_overlapped[myIndex].hEvent = CreateEventEx(nullptr, nullptr, CREATE_EVENT_MANUAL_RESET, EVENT_MODIFY_STATE | SYNCHRONIZE);
#else
			m_overlapped.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
#endif

			if ((STREAMING_BUFFER_SIZE % wfx->nBlockAlign) != 0) {

				// non-PCM data will fail here. ADPCM requires a more complicated streaming mechanism to deal with submission in audio frames that do
				// not necessarily align to the 2K async boundary.
				m_isStreaming[myIndex] = false;
				break;
			}

			for (size_t j = 0; j < MAX_BUFFER_COUNT; ++j) {
				buffers[j].reset(SAIL_NEW uint8_t[STREAMING_BUFFER_SIZE]);
			}

			DWORD currentDiskReadBuffer = 0;
			DWORD currentPosition = 0;

			HANDLE async = wbr.GetAsyncHandle();

			// Reading from the file (when time-since-last-read has passed threshold)
			while ((currentPosition < metadata.lengthBytes) && m_isStreaming[myIndex]) {
				while (m_isStreamPaused[myIndex]); // Wait while paused

				DWORD cbValid = std::min(STREAMING_BUFFER_SIZE, static_cast<int>(metadata.lengthBytes - static_cast<UINT32>(currentPosition)));
				m_overlapped[myIndex].Offset = metadata.offsetBytes + currentPosition;

				bool wait = false;
				if (!ReadFile(async, buffers[currentDiskReadBuffer].get(), STREAMING_BUFFER_SIZE, nullptr, &m_overlapped[myIndex])) {
					DWORD error = GetLastError();
					if (error != ERROR_IO_PENDING) {
						m_isStreaming[myIndex] = false;
						break;
					}
					wait = true;
				}

				currentPosition += cbValid;

				// Sleep while waiting for currently-playing chunk to finish
				if (wait) {
					WaitForSingleObject(m_overlapped[myIndex].hEvent, INFINITE);
				}

				DWORD cb;
#if (_WIN32_WINNT >= 0x0602 /*_WIN32_WINNT_WIN8*/)
				BOOL result = GetOverlappedResultEx(async, &m_overlapped[myIndex], &cb, 0, FALSE);
#else
				BOOL result = GetOverlappedResult(async, &ovlCurrentRequest, &cb, FALSE);
#endif

				if (!result) {
					m_isStreaming[myIndex] = false;
					break;
				}

				// Now that the event has been signaled, we know we have audio available.
				// We'd like to keep no more than MAX_BUFFER_COUNT - 1 buffers on the queue,
				// so that one buffer is always free for disk I/O.
				XAUDIO2_VOICE_STATE state;
				for (;;) {
					m_stream[myIndex].sourceVoice->GetState(&state);
					if (state.BuffersQueued < MAX_BUFFER_COUNT - 1) {
						break;
					}

					m_stream[myIndex].sourceVoice->Start();
					if (currentVolume < volume) {
						currentVolume += (0.1f * volume);
						m_stream[myIndex].sourceVoice->SetVolume(currentVolume);
					}

					currentChunk++;
					WaitForSingleObject(voiceContext.hBufferEndEvent, INFINITE);
				}

				// Submit buffer full of audio and get another read request going.
				XAUDIO2_BUFFER buf = { 0 };
				buf.AudioBytes = cbValid;
				buf.pAudioData = buffers[currentDiskReadBuffer].get();
				if (currentPosition >= metadata.lengthBytes) {
					buf.Flags = XAUDIO2_END_OF_STREAM;
				}

				m_stream[myIndex].sourceVoice->SubmitSourceBuffer(&buf);

				currentDiskReadBuffer++;
				currentDiskReadBuffer %= MAX_BUFFER_COUNT;
			}
		}

		if (!loop) {
			m_isStreaming[myIndex] = false;
		}

		currentChunk = 0;
		currentVolume = 0;
		// There is a rare case where multi-threading means this sourceVoice MAY have been
		// cleaned by another part of the system (AKA: Keep this nullptr check to avoid rare crash)
		if (m_stream[myIndex].sourceVoice != nullptr) {
			m_stream[myIndex].sourceVoice->Stop();
		}
	}
	
	// Clean up
	if (m_stream[myIndex].sourceVoice != nullptr) {
		m_stream[myIndex].sourceVoice->Stop();
		m_stream[myIndex].sourceVoice->DestroyVoice();
		m_stream[myIndex].sourceVoice = nullptr;
		CloseHandle(m_overlapped[myIndex].hEvent);
	}

	m_isFinished[myIndex] = true;
}

//--------------------------------------------------------------------------------------
// Helper function to try to find the location of a media file
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT AudioEngine::FindMediaFileCch(WCHAR* strDestPath, int cchDest, LPCWSTR strFilename)
{
	bool bFound = false;

	if (!strFilename || strFilename[0] == 0 || !strDestPath || cchDest < 10) {
		return E_INVALIDARG;
	}

	// Get the exe name, and exe path
	WCHAR strExePath[MAX_PATH] = { 0 };
	WCHAR strExeName[MAX_PATH] = { 0 };
	WCHAR* strLastSlash = nullptr;
	GetModuleFileName(nullptr, strExePath, MAX_PATH);
	strExePath[MAX_PATH - 1] = 0;
	strLastSlash = wcsrchr(strExePath, TEXT('\\'));
	if (strLastSlash) {
		wcscpy_s(strExeName, MAX_PATH, &strLastSlash[1]);

		// Chop the exe name from the exe path
		*strLastSlash = 0;

		// Chop the .exe from the exe name
		strLastSlash = wcsrchr(strExeName, TEXT('.'));
		if (strLastSlash) {
			*strLastSlash = 0;
		}
	}

	wcscpy_s(strDestPath, cchDest, strFilename);
	if (GetFileAttributes(strDestPath) != 0xFFFFFFFF) {
		return S_OK;
	}

	// Search all parent directories starting at .\ and using strFilename as the leaf name
	WCHAR strLeafName[MAX_PATH] = { 0 };
	wcscpy_s(strLeafName, MAX_PATH, strFilename);

	WCHAR strFullPath[MAX_PATH] = { 0 };
	WCHAR strFullFileName[MAX_PATH] = { 0 };
	WCHAR strSearch[MAX_PATH] = { 0 };
	WCHAR* strFilePart = nullptr;

	GetFullPathName(L".", MAX_PATH, strFullPath, &strFilePart);
	if (!strFilePart) {
		return E_FAIL;
	}

	while (strFilePart && *strFilePart != '\0') {
		swprintf_s(strFullFileName, MAX_PATH, L"%s\\%s", strFullPath, strLeafName);
		if (GetFileAttributes(strFullFileName) != 0xFFFFFFFF) {
			wcscpy_s(strDestPath, cchDest, strFullFileName);
			bFound = true;
			break;
		}

		swprintf_s(strFullFileName, MAX_PATH, L"%s\\%s\\%s", strFullPath, strExeName, strLeafName);
		if (GetFileAttributes(strFullFileName) != 0xFFFFFFFF) {
			wcscpy_s(strDestPath, cchDest, strFullFileName);
			bFound = true;
			break;
		}

		swprintf_s(strSearch, MAX_PATH, L"%s\\..", strFullPath);
		GetFullPathName(strSearch, MAX_PATH, strFullPath, &strFilePart);
	}
	if (bFound) {
		return S_OK;
	}

	// On failure, return the file as the path but also return an error code
	wcscpy_s(strDestPath, cchDest, strFilename);

	return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
}

bool AudioEngine::checkSoundIndex(int index) {
	if (index < 0 || index > SOUND_COUNT) {
		SAIL_LOG_ERROR("Tried to STOP a sound from playing with an INVALID INDEX!");
		return false;
	} else {
		return true;
	}
}

bool AudioEngine::checkStreamIndex(int index) {
	if (index < 0 || index > STREAMED_SOUNDS_COUNT) {
		SAIL_LOG_ERROR("Tried to STOP a sound from being streamed with an INVALID INDEX!");
		return false;
	} else {
		return true;
	}
}
