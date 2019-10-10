#pragma once

#include "..//BaseComponentSystem.h"

class AudioComponent;
class AudioEngine;
class Camera;

class AudioSystem final : public BaseComponentSystem {
public:
	AudioSystem();
	~AudioSystem();

	AudioEngine* getAudioEngine();

	void initialize();
	void update(float dt) override {} // Use the below function instead
	void update(Camera& cam, float dt, float alpha);
	void stop() override;

	bool hasUpdated = false;

private:
	AudioEngine* m_audioEngine;
	int m_currStreamIndex = 0;
};

