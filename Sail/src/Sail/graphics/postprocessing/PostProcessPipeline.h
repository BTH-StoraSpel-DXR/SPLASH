#pragma once

#include "Sail/Application.h"
#include "Sail/api/RenderableTexture.h"
#include "API/DX12/resources/DX12RenderableTexture.h"
#include "Sail/api/ComputeShaderDispatcher.h"

class PostProcessPipeline final : public EventReceiver {
public:
	class PostProcessInput : public Shader::ComputeShaderInput {
	public:
		Texture* inputTexture = nullptr;
		Texture* inputTextureTwo = nullptr;
		RenderableTexture* inputRenderableTexture = nullptr;
		RenderableTexture* inputRenderableTextureTwo = nullptr;
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
	void add(const std::string& name, float resolutionScale = 1.0f, float textureSizeDifference = 1.f) {
		// Create a new stage instance with the given scale
		m_stages.insert({name, StageData(std::make_unique<T>(), resolutionScale, textureSizeDifference)});
	}

	RenderableTexture* run(RenderableTexture* baseTexture, DX12RenderableTexture** gbufferTextures, void* cmdList = nullptr);

	void setBloomInput(RenderableTexture* bloomTexture);

	virtual bool onEvent(const Event& event) override;

private:
	struct StageData {
		StageData() {
			resolutionScale = 1.f;
		}
		StageData(std::unique_ptr<Shader> shader, float resolutionScale, const float textureSizeDifference)
			: shader(std::move(shader))
			, resolutionScale(resolutionScale)
			, textureSizeDifference(textureSizeDifference)
		{}
		std::unique_ptr<Shader> shader;
		float resolutionScale;
		float textureSizeDifference;
	};

	bool onResize(const WindowResizeEvent& event);
	PostProcessPipeline::PostProcessOutput* runStage(PostProcessInput& input, StageData& stage, void* cmdList);

private:
	std::map<std::string, StageData> m_stages;

	std::unique_ptr<ComputeShaderDispatcher> m_dispatcher;

	RenderableTexture* m_bloomTexture;
};