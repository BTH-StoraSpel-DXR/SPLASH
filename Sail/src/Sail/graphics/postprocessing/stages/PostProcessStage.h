#pragma once

#include "Sail/api/RenderableTexture.h"
#include "Sail/api/shader/ShaderPipeline.h"

class Model;

class PostProcessStage : public ShaderPipeline, public IEventListener {
public:
	PostProcessStage(const std::string& filename, UINT width, UINT height);
	virtual ~PostProcessStage();

	virtual void run(RenderableTexture& inputTexture);
	RenderableTexture& getOutput();

	virtual bool onEvent(Event& event) override;

protected:
	virtual bool onResize(WindowResizeEvent& event);

protected:
	RenderableTexture* OutputTexture;
	UINT Width, Height;

};