#include "pch.h"
#include "Sail/api/Renderer.h"
#include "DX12ForwardRenderer.h"
#include "DX12RaytracingRenderer.h"
#include "DX12GBufferRenderer.h"
#include "DX12HybridRaytracerRenderer.h"
#include "DX12ScreenSpaceRenderer.h"

Renderer* Renderer::Create(Renderer::Type type) {
	switch (type) {
	case FORWARD:
		return SAIL_NEW DX12ForwardRenderer();
	case GBUFFER:
		return SAIL_NEW DX12GBufferRenderer();
	case HYBRID:
		return SAIL_NEW DX12HybridRaytracerRenderer();
	case SCREEN_SPACE:
		return SAIL_NEW DX12ScreenSpaceRenderer();
	default:
		Logger::Error("Tried to create a renderer of unknown or unimplemented type: " + std::to_string(type));
	}
	return nullptr;
}