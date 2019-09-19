#include "pch.h"
#include "PostProcessPipeline.h"
#include "Sail/graphics/shader/postprocess/RedTintShader.h"
#include "Sail/graphics/shader/postprocess/GaussianBlurHorizontal.h"

PostProcessPipeline::PostProcessPipeline() {

	m_dispatcher = std::unique_ptr<ComputeShaderDispatcher>(ComputeShaderDispatcher::Create());

	add<GaussianBlurHorizontal>(1.0f);
	//add<RedTintShader>(0.5f);

}

bool PostProcessPipeline::onResize(WindowResizeEvent& event) {

	return false;
}

PostProcessPipeline::~PostProcessPipeline() {
}

void PostProcessPipeline::clear() {
	m_pipelineStages.clear();
}

RenderableTexture* PostProcessPipeline::run(Texture* baseTexture, void* cmdList) {
	PostProcessInput input;
	input.inputTexture = baseTexture;
	return runInternal(input, cmdList);
}

RenderableTexture* PostProcessPipeline::run(RenderableTexture* baseTexture, void* cmdList) {
	PostProcessInput input;
	input.inputRenderableTexture = baseTexture;
	return runInternal(input, cmdList);
}

RenderableTexture* PostProcessPipeline::runInternal(PostProcessInput& input, void* cmdList) {
	Application* app = Application::getInstance();
	auto windowWidth = app->getWindow()->getWindowWidth();
	auto windowHeight = app->getWindow()->getWindowHeight();

	m_dispatcher->begin(cmdList);

	PostProcessOutput output;
	for (auto& stage : m_pipelineStages) {
		auto* settings = stage.shader->getComputeSettings();

		input.outputWidth = stage.resolutionScale * windowWidth;
		input.outputHeight = stage.resolutionScale * windowHeight;
		input.threadGroupCountX = glm::ceil(input.outputWidth * settings->threadGroupXScale);
		input.threadGroupCountY = glm::ceil(input.outputHeight * settings->threadGroupYScale);

		output = static_cast<PostProcessOutput&>(m_dispatcher->dispatch(*stage.shader, input, cmdList));
		input.inputTexture = nullptr;
		input.inputRenderableTexture = output.outputTexture;
	}
	return output.outputTexture;
}

bool PostProcessPipeline::onEvent(Event& event) {
	EventHandler::dispatch<WindowResizeEvent>(event, SAIL_BIND_EVENT(&PostProcessPipeline::onResize));

	return true;
}
