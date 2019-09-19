#pragma once

#include "Sail/Application.h"
#include "Sail/api/RenderableTexture.h"
#include "Sail/api/ComputeShaderDispatcher.h"

class PostProcessPipeline : public IEventListener {
public:
	class PostProcessInput : public Shader::ComputeShaderInput {
	public:
		Texture* inputTexture = nullptr;
		RenderableTexture* inputRenderableTexture = nullptr;
	};
	class PostProcessOutput : public Shader::ComputeShaderOutput {
	public:
		PostProcessOutput() {}
		RenderableTexture* outputTexture = nullptr;
	};

public:
	PostProcessPipeline();
	~PostProcessPipeline();

	template <typename T>
	int add(float resolutionScale = 1.0f) {
		Application* app = Application::getInstance();

		// Create a new stage instance with the given scale
		m_pipelineStages.emplace_back(std::make_unique<T>(), resolutionScale);

		// Return the index of the inserted stage
		return m_pipelineStages.size() - 1;
	}
	void clear();

	RenderableTexture* run(Texture* baseTexture, void* cmdList = nullptr);
	RenderableTexture* run(RenderableTexture* baseTexture, void* cmdList = nullptr);

	virtual bool onEvent(Event& event) override;

private:
	bool onResize(WindowResizeEvent& event);
	RenderableTexture* runInternal(PostProcessInput& input, void* cmdList);

private:
	struct StageData {
		StageData(std::unique_ptr<Shader> shader, float resolutionScale)
			: shader(std::move(shader))
			, resolutionScale(resolutionScale)
		{}
		std::unique_ptr<Shader> shader;
		float resolutionScale;
	};

	std::vector<StageData> m_pipelineStages;

	std::unique_ptr<ComputeShaderDispatcher> m_dispatcher;

};