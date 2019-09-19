#pragma once

#include "Sail/events/Events.h"
#include "Sail/graphics/shader/Shader.h"

class ComputeShaderDispatcher : public IEventListener {
public:
	static ComputeShaderDispatcher* Create();
	ComputeShaderDispatcher() {}
	virtual ~ComputeShaderDispatcher() {}

	virtual void begin(void* cmdList = nullptr) = 0;
	virtual Shader::ComputeShaderOutput& dispatch(Shader& computeShader, Shader::ComputeShaderInput& input, void* cmdList = nullptr) = 0;

	virtual bool onEvent(Event& event) override { return true; };

};