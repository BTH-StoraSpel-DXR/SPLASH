#include "pch.h"
#include "PostProcessStage.h"

PostProcessStage::PostProcessStage(const std::string& filename, UINT width, UINT height)
	: ShaderPipeline(filename)
	, Width(width)
	, Height(height)
{
}

PostProcessStage::~PostProcessStage() {

}

void PostProcessStage::run(RenderableTexture& inputTexture) {
	ShaderPipeline::bind();

	// TODO: Dispatch n stuff
}

RenderableTexture& PostProcessStage::getOutput() {
	return *OutputTexture;
}

bool PostProcessStage::onResize(WindowResizeEvent& event) {

	// TODO: fix this. should be set to width*resScale instead of full width - same with height
	unsigned int width = event.getWidth();
	unsigned int height = event.getHeight();

	OutputTexture->resize(width, height);
	Width = width;
	Height = height;

	return false;
}

bool PostProcessStage::onEvent(Event & event) {
	EventHandler::dispatch<WindowResizeEvent>(event, SAIL_BIND_EVENT(&PostProcessStage::onResize));
	return true;
}
