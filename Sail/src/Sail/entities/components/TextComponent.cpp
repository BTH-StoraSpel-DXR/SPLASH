#include "pch.h"
#include "TextComponent.h"
#include "Sail/Application.h"
#include "Sail/api/shader/ShaderPipeline.h"
#include "Sail/api/GraphicsAPI.h"

TextComponent::TextComponent() {
	//m_spriteBatch = std::make_unique<DirectX::SpriteBatch>(Application::getInstance()->getAPI()->getDeviceContext());
}

TextComponent::~TextComponent() {

}

Text* TextComponent::addText(Text::Ptr text) {
	m_texts.push_back(std::move(text));
	return m_texts.back().get();
}

void TextComponent::draw() {
	auto* dxm = Application::getInstance()->getAPI<GraphicsAPI>();

	//dxm->setDepthMask(GraphicsAPI::BUFFER_DISABLED);

	// 2D text rendering
	// Beginning the spritebatch will disable depth testing
	/*if (!m_texts.empty()) {
		m_spriteBatch->Begin();
		for (Text::Ptr& text : m_texts) {
			m_font.get()->DrawString(m_spriteBatch.get(), text->getText().c_str(), text->getPosition(), text->getColor());
		}
		m_spriteBatch->End();
	}*/

	// Re-enable the depth buffer and rasterizer state after 2D rendering
	dxm->setDepthMask(GraphicsAPI::NO_MASK);
	//dxm->setFaceCulling(GraphicsAPI::BACKFACE);

	ShaderPipeline::CurrentlyBoundShader = nullptr;
}
