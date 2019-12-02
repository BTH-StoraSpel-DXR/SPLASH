#include "pch.h"
#include "PostProcessPipeline.h"
#include "Sail/graphics/shader/postprocess/RedTintShader.h"
#include "Sail/graphics/shader/postprocess/GaussianBlurHorizontal.h"
#include "Sail/graphics/shader/postprocess/GaussianBlurVertical.h"
#include "Sail/graphics/shader/postprocess/BlendShader.h"
#include "Sail/graphics/shader/postprocess/FXAAShader.h"
#include "Sail/graphics/shader/postprocess/TonemapShader.h"
#include "Sail/events/EventDispatcher.h"

PostProcessPipeline::PostProcessPipeline() 
	: m_bloomTexture(nullptr)
{
	m_dispatcher = std::unique_ptr<ComputeShaderDispatcher>(ComputeShaderDispatcher::Create());

	add<FXAAShader>("FXAA", 1.0f);
	add<GaussianBlurVertical>("BloomBlur1V", 1.0f);
	add<GaussianBlurHorizontal>("BloomBlur1H", 1.0f);
	add<GaussianBlurVertical>("BloomBlur2V", 0.75f, 1.f / 0.75f);
	add<GaussianBlurHorizontal>("BloomBlur2H", 0.75f);
	add<GaussianBlurVertical>("BloomBlur3V", 0.5f, 1.5f);
	add<GaussianBlurHorizontal>("BloomBlur3H", 0.5f);
	add<BlendShader>("BloomBlend", 1.0f, 1.f / 2.f);
	add<TonemapShader>("Tonemapper", 1.0f);

	EventDispatcher::Instance().subscribe(Event::Type::WINDOW_RESIZE, this);
}

bool PostProcessPipeline::onResize(const WindowResizeEvent& event) {

	return false;
}

PostProcessPipeline::~PostProcessPipeline() {
	EventDispatcher::Instance().unsubscribe(Event::Type::WINDOW_RESIZE, this);
}

RenderableTexture* PostProcessPipeline::run(RenderableTexture* baseTexture, void* cmdList) {
	auto& settings = Application::getInstance()->getSettings();

	// Read from game settings
	bool enableFXAA = settings.applicationSettingsStatic["graphics"]["fxaa"].getSelected().value > 0.f;
	float bloomAmount = settings.applicationSettingsStatic["graphics"]["bloom"].getSelected().value;

	PostProcessInput input;

	// Stage one - fxaa if enabled
	RenderableTexture* stageOneOutput = baseTexture;
	if (enableFXAA) {
		// FXAA
		input.inputRenderableTexture = baseTexture;
		auto* output = runStage(input, m_stages["FXAA"], cmdList);
		stageOneOutput = output->outputTexture;
	}

	// Stage two - bloom if enabled
	RenderableTexture* stageTwoOutput = stageOneOutput;
	if (bloomAmount > 0.f) {
		input.inputRenderableTexture = m_bloomTexture;
		// Blur pass one
		auto* output = runStage(input, m_stages["BloomBlur1V"], cmdList);
		input.inputRenderableTexture = output->outputTexture;
		output = runStage(input, m_stages["BloomBlur1H"], cmdList);
		// Blur pass two
		input.inputRenderableTexture = output->outputTexture;
		output = runStage(input, m_stages["BloomBlur2V"], cmdList);
		input.inputRenderableTexture = output->outputTexture;
		output = runStage(input, m_stages["BloomBlur2H"], cmdList);
		// Blur pass three
		input.inputRenderableTexture = output->outputTexture;
		output = runStage(input, m_stages["BloomBlur3V"], cmdList);
		input.inputRenderableTexture = output->outputTexture;
		auto* bloomOutput = runStage(input, m_stages["BloomBlur3H"], cmdList);

		input.inputRenderableTexture = stageOneOutput;
		input.inputRenderableTextureTwo = bloomOutput->outputTexture;

		auto& blendStage = m_stages["BloomBlend"];
		// Set blend amount in shader
		blendStage.shader->getPipeline()->trySetCBufferVar("blendFactor", &bloomAmount, sizeof(float));
		output = runStage(input, blendStage, cmdList);
		// Set final stage output
		stageTwoOutput = output->outputTexture;
	}

	// Last stage - tonemapping, always runs
	input.inputRenderableTexture = stageTwoOutput;
	auto& tonemapStage = m_stages["Tonemapper"];
	float gamma = Application::getInstance()->getSettings().applicationSettingsDynamic["graphics"]["gamma"].value;
	// Set gamma in shader
	tonemapStage.shader->getPipeline()->trySetCBufferVar("gamma", &gamma, sizeof(float));
	auto* tonemapOutput = runStage(input, tonemapStage, cmdList);

	return tonemapOutput->outputTexture;
}

void PostProcessPipeline::setBloomInput(RenderableTexture* bloomTexture) {
	m_bloomTexture = bloomTexture;
}

PostProcessPipeline::PostProcessOutput* PostProcessPipeline::runStage(PostProcessInput& input, StageData& stage, void* cmdList) {
	Application* app = Application::getInstance();
	auto windowWidth = app->getWindow()->getWindowWidth();
	auto windowHeight = app->getWindow()->getWindowHeight();

	m_dispatcher->begin(cmdList);

	PostProcessOutput* output;
	auto* settings = stage.shader->getComputeSettings();

	input.outputWidth = (unsigned int)(stage.resolutionScale * windowWidth);
	input.outputHeight = (unsigned int)(stage.resolutionScale * windowHeight);
	input.threadGroupCountX = (unsigned int)glm::ceil(input.outputWidth * settings->threadGroupXScale);
	input.threadGroupCountY = (unsigned int)glm::ceil(input.outputHeight * settings->threadGroupYScale);

	stage.shader->getPipeline()->setCBufferVar("textureSizeDifference", &stage.textureSizeDifference, sizeof(float));
	glm::u32vec2 textureSize = glm::u32vec2(input.outputWidth, input.outputHeight);
	stage.shader->getPipeline()->trySetCBufferVar("textureSize", &textureSize, sizeof(glm::u32vec2));
	
	output = static_cast<PostProcessOutput*>(&m_dispatcher->dispatch(*stage.shader, input, cmdList));
	return output;
}

bool PostProcessPipeline::onEvent(const Event& event) {
	switch (event.type) {
	case Event::Type::WINDOW_RESIZE: onResize((const WindowResizeEvent&)event); break;
	default: break;
	}

	return true;
}
