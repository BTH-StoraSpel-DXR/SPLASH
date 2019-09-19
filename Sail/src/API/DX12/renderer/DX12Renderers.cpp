#include "pch.h"
#include "Sail/api/Renderer.h"
#include "DX12ForwardRenderer.h"
#include "DX12RaytracingRenderer.h"

Renderer* Renderer::Create(Renderer::Type type) {
	switch (type) {
	case FORWARD:
		return SAIL_NEW DX12ForwardRenderer();
	case RAYTRACED:
		return SAIL_NEW DX12RaytracingRenderer();
	default:
		Logger::Error("Tried to create a renderer of unknown or unimplemented type: " + std::to_string(type));
	}
	return nullptr;
}