#pragma once

#include "Sail/api/Renderer.h"
#include <glm/glm.hpp>
#include "../DX12API.h"
#include "Sail/api/ComputeShaderDispatcher.h"

class DX12RenderableTexture;
class PostProcessPipeline;

#define MULTI_THREADED_COMMAND_RECORDING
//#define DEBUG_MULTI_THREADED_COMMAND_RECORDING

class DX12ForwardRenderer : public Renderer {
public:
	DX12ForwardRenderer();
	~DX12ForwardRenderer();

	void present(PostProcessPipeline* postProcessPipeline = nullptr, RenderableTexture* output = nullptr) override;
	virtual bool onEvent(Event& event) override;

private:
	bool onResize(WindowResizeEvent& event);

private:
	static const int MAX_RECORD_THREADS = 4;
	static const int MIN_COMMANDS_PER_THREAD = 20;

	DX12API* m_context;
	std::unique_ptr<DX12RenderableTexture> m_outputTexture;
	DX12API::Command m_command[MAX_RECORD_THREADS];

	void recordCommands(PostProcessPipeline* postProcessPipeline, const int threadID, const int frameIndex, const int start, const int nCommands, size_t oobMax, int nThreads);
};