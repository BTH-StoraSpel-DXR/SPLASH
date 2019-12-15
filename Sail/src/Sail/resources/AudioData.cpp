#include "pch.h"
#include "AudioData.h"
#include "API/Audio/GeneralFunctions.h"

AudioData::AudioData() {

}

AudioData::AudioData(const std::string& filename, IXAudio2* xAudio2) {
	load(filename, xAudio2);
}

AudioData::~AudioData() {
	delete m_data.m_soundBuffer.pAudioData;
}

void AudioData::load(const std::string& filename, IXAudio2* xAudio2) {
	Fileloader::WAVLoader WAVLoader(filename, xAudio2, m_data);
}

XAUDIO2_BUFFER* AudioData::getSoundBuffer() {
	return &m_data.m_soundBuffer;
}

WAVEFORMATEXTENSIBLE* AudioData::getFormat() {
	return &m_data.m_formatWAV;
}

unsigned int AudioData::getByteSize() const {
	return sizeof(*this) + m_data.m_soundBuffer.AudioBytes;
}
